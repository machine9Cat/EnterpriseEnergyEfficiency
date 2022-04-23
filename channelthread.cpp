#include "channelthread.h"
#include "zrcalc.h"
#include "..\qslog\QsLog.h"

#include "chserial.h"
#include "chtcp.h"
#include "chzrtcp.h"

#include "modbusbase.h"
#include "pamodbusrtu.h"
#include "pamodbustcp.h"
#include "pacjt188.h"
#include "padl645.h"
#include "nodework.h"

ChannelThread::ChannelThread(const sChannelPara &channelSetings) {
    m_chPara = channelSetings;
}

ChannelThread::~ChannelThread() {

}

///
/// \brief ChannelThread::readyQuit
///退出前准备
void ChannelThread::readyQuit() {
    QLOG_TRACE() << "app ready quit";
    foreach(sDevcie * dev, m_devMap) {
        foreach(sNode * snode, dev->listNode) {
            NodeWork *node = (NodeWork *)snode->pt;
            delete node;
            dev->listNode.removeOne(snode);
            delete snode;
        }
        if (dev->listNode.size() == 0) {
            m_devMap.remove(m_devMap.key(dev));
            delete dev;          //删除这个指针
        }
    }
    //关闭硬件通道
    if (chbase) {
        delete  chbase;
        chbase = NULL;
    }
    if (ptobase) {
        delete ptobase;
        ptobase = NULL;
    }
    if (analybase) {
        delete analybase;
        analybase = NULL;
    }
    //数据库断开
    if (db != NULL) delete db;
    QLOG_INFO() << "threadID:" << QThread::currentThreadId() << "stop...";
    //thread 关闭
    this->thread()->quit();
    moveToThread(m_fatherThread);
}

void ChannelThread::readyWork() {
    QLOG_INFO() << "Thread:" << QThread::currentThreadId() << "working...";
    work();
}
bool ChannelThread::initNodeCpuLocal() {
    QSqlQuery *query;
    Zrcalc *node;
    int type, time;
    foreach(sDevcie * dev, m_devMap) {
        query = queryNode(m_chPara.chId, dev->id, m_chPara.tNId);
        if (query == NULL) return false;
        while (query->next()) { //解析节点,根据协议来配置
            node  = new Zrcalc(this);
            //1
            //2
            //3初始化参数
            type = query->value("RWrite").toInt();
            node->setAcqType(type);                         //设置采集
            time = query->value("DataTag").toString().toInt();
            type = query->value("CJTMeterType").toInt();    //设置同步
            node->setSyncType(type, time);

            //2.初始化节点参数
            NodeWork::sNpara npara; //
            node->m_id = query->value("ID").toInt();
            npara.processCode = query->value("ProcessCode").toString();
            npara.acqInvTime = query->value("AcqInvTime").toInt();
            npara.dbTable = query->value("dbTable").toString();
            node->setNpara(&npara);

            //3解析计算模版创建数据模版
            QString mb = query->value("PraseTmp").toString();

            node->checkCTemplateAndDul(mb);
            //node->createDataTemplate();
            //4 节点加入设备
            sNode *snode = new sNode;
            snode->pt = node;
            QString name = query->value("Name").toString();
            snode->name =  dev->name + "(" + name + ")";
            dev->listNode.append(snode);
            //5
        }
    }
    return true;
}

bool ChannelThread::initNode(ChBase *chbase, PotocolBase *ptobase, AnalyBase *analybase) {
    if (chbase == NULL || ptobase == NULL || analybase == NULL) {
        QLOG_DEBUG() << "init Node fun para err.";
        return false;
    }
    QSqlQuery *query;
    QString errmsg;
    eErrCode errcode;
    foreach(sDevcie * dev, m_devMap) {
        query = queryNode(m_chPara.chId, dev->id, m_chPara.tNId);
        if (query == NULL) return false;
        while (query->next()) { //解析节点,根据协议来配置
            NodeWork *node = new NodeWork(this);        //创建一个节点
            node->bindBase(chbase, ptobase, analybase); //绑定base
                                                        //0.初始化节点统一参数
            NodeWork::sNpara npara; //
            node->m_id = query->value("ID").toInt();
            npara.processCode = query->value("ProcessCode").toString();
            npara.acqInvTime = query->value("AcqInvTime").toInt();
            npara.dbTable = query->value("dbTable").toString();
            node->setNpara(&npara);

            switch (m_chPara.procType) {
            case PT_ModBusRTU:
            case PT_ModBusTCP:
                {
                    bool okflg;
                    quint8 fun = query->value("FunCode").toInt();
                    quint8 adr = dev->addr.toInt();
                    quint16 regadr = query->value("DataAddr").toString().toInt(&okflg,16);
                    quint16 reglen = query->value("MBRegsNum").toInt();
                    //1.生成采集命令
                    ModbusBase *pto =  (ModbusBase *)ptobase;
                    pto->createCmd(node->m_reqCmd, adr, fun, regadr, reglen);
                    //3.模版初始化
                    QString mb = query->value("PraseTmp").toString();
                    pto->createDataTemplate(node->analyPara.m_listTemplate, mb);
                }
                break;
            case PT_CJT188:
                {
                    bool okflg;
                    int tag = query->value("DataTag").toString().toInt(&okflg,16);
                    int type = query->value("CJTMeterType").toInt();
                    //1.生成采集命令
                    PAcjt188 *pto =  (PAcjt188 *)ptobase;
                    pto->createCmd(node->m_reqCmd, dev->addr, tag, type);
                    //3创建数据模版
                    pto->createDataTemplate(node->analyPara.m_listTemplate, tag);
                    dev->swapASet = 1; //字节不倒叙，CJT188 需要
                }
                break;

            case PT_DL645_97:
            case PT_DL645_07:
                {
                bool okflg;
                    int tag = query->value("DataTag").toString().toInt(&okflg,16);
                    //1.生成采集命令
                    PAdl645 *pto =  (PAdl645 *)ptobase;
                    pto->createCmd(node->m_reqCmd, dev->addr, tag);
                    //3创建数据模版
                    pto->createDataTemplate(node->analyPara.m_listTemplate, tag);
                    //type
                    if (m_chPara.procType == PT_DL645_07) pto->setDl645Type(PAdl645::eDL645_07);
                    else pto->setDl645Type(PAdl645::eDL645_97);
                    dev->swapASet = 1; //字节倒叙，645特有
                }
                break;
            case PT_ZRCLC:
                {
//                node  = new Zrcalc(this);
//                type = query->value("RWrite").toInt();
//                node->setAcqType(type);                         //设置采集
//                time = query->value("DataTag").toInt();
//                type = query->value("CJTMeterType").toInt();    //设置同步
//                node->setSyncType(type, time);
//                //3解析计算模版创建数据模版
//                QString mb = query->value("PraseTmp").toString();

//                node->checkCTemplateAndDul(mb);
//                node->createDataTemplate();
                }
                break;
            default:
                QLOG_DEBUG() << "err.12311";
                return false;
            }
            //协议解析参数
            node->analyPara.swap2Set = dev->swap2Set;
            node->analyPara.swap4Set = dev->swap4Set;
            node->analyPara.swap8Set = dev->swap8Set;
            node->analyPara.swapASet = dev->swapASet;

            //错误码统一到节点
            errcode = chbase->getLastErr(errmsg);
            if (errcode == eErrNone) {
                errcode = ptobase->getLastErr(errmsg);
                if (errcode == eErrNone) {
                    errcode = analybase->getLastErr(errmsg);
                    if (errcode != eErrNone) node->setErr(errcode, errmsg);
                } else node->setErr(errcode, errmsg);
            } else node->setErr(errcode, errmsg);

            //4 节点加入设备
            sNode *snode = new sNode;
            snode->pt = node;
            QString name = query->value("Name").toString();
            snode->name =  dev->name + "(" + name + ")";
            dev->listNode.append(snode);
        }
    }
    return true;
}

bool ChannelThread::workCheckAndStart() {
    int err = eErrNodePara;
    QString strerr = "node CH para err.";

    foreach(sDevcie * dev, m_devMap) {
        foreach(sNode * snode, dev->listNode) {
            NodeWork *node = (NodeWork *)snode->pt;
            if (node->getNodeErr() != eErrNone) {
                err = node->getNodeErr();
                strerr = node->getNodeErrMsg();
                QLOG_INFO() << "CId:" << m_chPara.chId << "DId:" << dev->id << "NId:" << node->m_id << node->getNodeErrMsg();
                delete node;
                dev->listNode.removeOne(snode);
                delete snode;
                continue;
            }
            if (node->setCpara() != true) { //设置 通道参数
                err = node->getNodeErr();
                strerr = node->getNodeErrMsg();
                QLOG_INFO() << "CId:" << m_chPara.chId << "DId:" << dev->id << "NId:" << node->m_id << "channl para is wrong!this node remove.";
                delete node;
                dev->listNode.removeOne(snode);
                delete snode;
                continue;
            }
            if (node->setDpara(dev->id) != true) { //设置设备参数。 //bug.dev->id
                err = node->getNodeErr();
                strerr = node->getNodeErrMsg();
                QLOG_INFO() << "CId:" << m_chPara.chId << "DId:" << dev->id << "NId:" << node->m_id << "device para is wrong!this node remove.";
                delete node;
                dev->listNode.removeOne(snode);
                delete snode;
                continue;
            }
            if (node->setDbSection() != true) { //设置数据表 段出错
                err = node->getNodeErr();
                strerr = node->getNodeErrMsg();
                QLOG_INFO() << "CId:" << m_chPara.chId << "DId:" << dev->id << "NId:" << node->m_id << "acq database table section is wrong!this node remove.";
                delete node;
                dev->listNode.removeOne(snode);
                delete snode;
                continue;
            }
//            if (node->checkTemplate() != true) { //模版检测失败
//                err = node->getNodeErr();
//                strerr = node->getNodeErrMsg();
//                QLOG_INFO() << "CId:" << m_chPara.chId << "DId:" << dev->id << "NId:" << node->m_id << "tmplate and table not match!this node remove.";
//                delete node;
//                dev->listNode.removeOne(snode);
//                delete snode;
//                continue;
//            }
            node->startUp();    //节点启动
            QLOG_INFO() << "CId:" << m_chPara.chId << "DId:" << dev->id << "NId:" << node->m_id << "node startUp.";
        }
        if (dev->listNode.size() == 0) {
            QLOG_INFO() << "CId:" << m_chPara.chId << "DId:" << dev->id << "node is empty,this device removed";
            m_devMap.remove(m_devMap.key(dev));
            delete dev;          //删除这个指针
        }
    }
    if (m_devMap.size() == 0) {   //删除通道所打开的物理设备
        if (chbase) {
            delete chbase;
            chbase = NULL;
        }
        if (ptobase) {
            delete ptobase;
            ptobase = NULL;
        }
        if (analybase) {
            delete analybase;
            analybase = NULL;
        }
        //在测试中，如果没有任何节点说明，这个节点参数有错误，没有能启动这个节点。这个时候要发送测试失败和对应原因.
        if (m_chPara.isTest) {
            emit testFinish(err, strerr);    //通知测试完成
        }
    }
    return true;
}

bool ChannelThread::openDb() {
    QSettings *setings = new QSettings("config.ini", QSettings::IniFormat); // 当前目录的INI文件
    setings->beginGroup("DB");
    QString driver = setings->value("Driver", "QOCI").toString();
    QString hostname = setings->value("HostName", "10.0.3.172").toString();
    int hostPort = setings->value("HostPort", 1521).toInt();
    QString name = setings->value("DbName", "Energy").toString();
    QString userName = setings->value("UserName", "root").toString();
    QString password = setings->value("UserPassword", "root").toString();
    setings->endGroup();
    delete setings;
    QString dbname = QString("mdb%1").arg(m_chPara.chId);
    db = new junQtDatabase({driver, dbname, hostname, hostPort, name, userName, password });
    return true;
}

bool ChannelThread::addDevice() {
    if (db == NULL) {
        QLOG_FATAL() << "dp point Null";
        return false;
    }

    QSqlQuery *query = db->GetSqlQuery();
    if (query == NULL) {
        QLOG_ERROR() << "OPen Db err";
        return false;
    }
    QString sql;
    if (m_chPara.isTest) sql = QString("SELECT * FROM CJ_CHDEVICECFG WHERE CHID=%1 AND ID=%2").arg(m_chPara.chId).arg(m_chPara.tDId);
    else sql = QString("SELECT * FROM CJ_CHDEVICECFG WHERE CHID = %1").arg(m_chPara.chId);
    if (query->exec(sql) != true) {
        QLOG_ERROR() << "sql Error:";
        db->ReConnect();
        return false;
    }

    while (query->next()) {
        sDevcie *dev = new sDevcie;
        dev->id = query->value("ID").toInt();
        dev->name = query->value("Name").toString();
        dev->addr = query->value("Addr").toString();
        dev->swap2Set = query->value("MB16Fmt").toInt();
        dev->swap4Set = query->value("MB32Fmt").toInt();
        dev->swap8Set = query->value("MB64Fmt").toInt();
        dev->toCmdExec = query->value("ToCmdExec").toInt();
        dev->toReTryInv = query->value("ToReTryInv").toInt();
        dev->toReTryTimes = query->value("ToReTryTimes").toInt();
        dev->frameMinTime = query->value("FrameMinTime").toInt();
        dev->listNode.clear();
        m_devMap.insert(dev->id, dev);
    }
    return true;
}

QSqlQuery* ChannelThread::queryNode(int cid, int did, int nid) {
    QString sql;
    QSqlQuery *query = db->GetSqlQuery();
    if (query == NULL) {
        QLOG_ERROR() << "OPen Db err";
        return NULL;
    }
    if (m_chPara.isTest) sql = QString("SELECT * FROM CJ_CHPOINTCFG WHERE DEVICEID =%1 AND CHID=%2 AND ID=%3")
            .arg(did).arg(cid).arg(nid);
    else sql = QString("SELECT * FROM CJ_CHPOINTCFG WHERE DEVICEID = %1 AND CHID = %2")
            .arg(did).arg(cid);
    query->clear();
    if (query->exec(sql) != true) {
        QLOG_ERROR() << "sql query Error";
        db->ReConnect();
        return NULL;
    }
    return query;
}

bool ChannelThread::work() {
    if (openDb() != true) return false; //数据库连接错误，延时后在处理
                                        //0载入当前通道下所有设备
    if (addDevice() != true) return false;
    //1根据物理通道类型，实例化物理通道

    switch (m_chPara.chPhy) {
    case PHY_Serial:
        {
            ChSerial *chserial = new ChSerial(this);
            chserial->setSerialpara(m_chPara.portName, m_chPara.bps, m_chPara.dataBits, m_chPara.stopBits, m_chPara.parityBits);
            chserial->setReConnect(false);
            chbase = (ChBase *)chserial;
        }
        break;
    case Phy_NetTcp:
        {
            ChTcp *chtcp = new ChTcp(this);
            chtcp->setTcpPara(m_chPara.remoteIP, m_chPara.ipPort, m_chPara.activeTimeVaule);
            chbase = (ChBase *)chtcp;
        }
        break;
    case Phy_LocalCpu:
        initNodeCpuLocal();
        break;
    case Phy_RemoteCpu:
        break;
    case Phy_ZrTcp:
        {
            ChZrTcp *chzrtcp = new ChZrTcp(this);
            chzrtcp->setTcpPara(m_chPara.remoteIP, m_chPara.ipPort, m_chPara.activeTimeVaule, m_chPara.bps); //bps 中保存虚拟串口号
            chbase = (ChBase *)chzrtcp;
        }
        break;
    default: //无协议处理
        QLOG_WARN() << "通道参数中物理通道字段错误。";
        return false;
    }
    //2设备列举，实例化协议解析
    switch (m_chPara.procType) {
    case PT_ModBusRTU:
        ptobase = (PotocolBase *)new PAmodbusRtu();
        break;
    case PT_CJT188:
        ptobase = (PotocolBase *)new PAcjt188();
        break;
    case PT_DL645_97:
    case PT_DL645_07:
        ptobase = (PotocolBase *)new PAdl645();
        break;
    case PT_ModBusTCP:
        ptobase = (PotocolBase *)new PAmodbusTcp();
        break;
    case PT_ZRCLC:
        //initNodeCpuLocal();
        break;
    default:
        QLOG_WARN() << "通道参数中协议字段错误。";
        return false;
    }
    //3实例化数据解析
    analybase = new AnalyBase();

    //4传入节点创建，进行一类节点实例化和初始化。
    initNode(chbase, ptobase, analybase);

    //5 检测和启动
    workCheckAndStart();

    return true;
}


