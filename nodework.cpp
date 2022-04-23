#include "nodework.h"
#include "..\qslog\QsLog.h"
NodeWork::NodeWork(ChannelThread *pthread):
    chThread(pthread)
{
    pTimerAcqInv = new QTimer(this);
    pTimerReAcqInv = new QTimer(this);
    connect(pTimerAcqInv, SIGNAL(timeout()), this, SLOT(processOneThing()));
    connect(pTimerReAcqInv, SIGNAL(timeout()), this, SLOT(processOneThing()));
}

NodeWork::~NodeWork()
{
    delete pTimerAcqInv;
    delete pTimerReAcqInv;
}

///
/// \brief Node::startUp 节点启动
///
void NodeWork::startUp() {
    if (paraCh->isTest) {
        processOneThing();
        //得到测试结果。发送测试通知。
        emit chThread->testFinish(errCode, errMsg);
    } else {
        pTimerAcqInv->start(paraPt.acqInvTime * 1000); //定时器间隔
        int id = pTimerAcqInv->timerId();
        if (pTimerAcqInv->isActive() != true) QLOG_INFO() << id << "id time is  not run";
        else QLOG_INFO() << id << "id time is runing";
    }
}

void NodeWork::processOneThing(){
    bool nextdul = true;

    QByteArray respData;
    QByteArray trueData;
    QStringList listData;

    eErrCode errcode;
    QString errMsg;
    QLOG_TRACE()<<QThread::currentThreadId();
    QLOG_INFO()<< "[" << paraCh->chId<< paraDev->id<< m_id<<"]"<<"Acq...";
    //采集
    if (chBase->acqData(m_reqCmd,respData,paraDev->toCmdExec) != true) {
        nextdul = false;
        errcode = chBase->getLastErr(errMsg);
        this->setErr(errcode,errMsg);
        doReAcq(true); //重发机制处理,可以利用其返回值进行重采集仍然失败处理
    } else doReAcq(false);
    //协议解析
    if(nextdul)
    {
        QLOG_TRACE()<<"AcqData:"<<respData.toHex();
        if(potocolBase->analypPotocol(respData, trueData,m_reqCmd) != true)//协议解析
        {
            nextdul = false;
            errcode = potocolBase->getLastErr(errMsg);
            this->setErr(errcode,errMsg);
        }
    }
    //数据解析
    if(nextdul)
    {
        QLOG_TRACE()<<"PclData:"<<trueData.toHex();
        if(analyBase->analyData(trueData, listData,analyPara) != true)//数据解析
        {
            errcode = analyBase->getLastErr(errMsg);
            this->setErr(errcode,errMsg);
            nextdul = false;
        }
    }
    //数据存储
    if(nextdul)
    {
        QLOG_TRACE()<<"CnvData"<<listData;
        if(insertData(listData) != true)//数据插入数据库
            nextdul = false;
    }

    //完成处理，出差处理
    if (nextdul == false) {
        QLOG_WARN() << "[" << paraCh->chId<< paraDev->id<< m_id<<"]"
                    << "ErrC:"<< getNodeErr()<<"ErrM:"<< getNodeErrMsg();
    }
}

///
/// \brief Node::insertData 插入数据到数据库
/// \param in
/// \return
///
bool NodeWork::insertData(const QStringList &in) {

    QSqlQuery *query = chThread->db->GetSqlQuery();
    if (query == NULL) {
        QLOG_ERROR() << "OPen Db err" << __LINE__;

        errCode = eErrDb;
        errMsg = "open db err.";
        return false;
    }

    QString sec;
    int section = in.size();    //得到数据段个数
    if (m_tabFieldsCount < section + 7) { //数据字段个数不正确
        errCode = eErrInsertData;
        errMsg = "data section err.";
        return false;
    }
    //生成插入语句
    QDateTime local(QDateTime::currentDateTime());
    sec = QString("INSERT INTO CJ_ACQPOINTDATA (ChID,DeviceID,PointID,ProcessCode,ACQDateTime,DataState");

    for (int i = 0; i < section; i++) {
        sec += QString(",Value%1").arg(i + 1);
    }
    sec += ")";
    sec += QString(" VALUES(%1,%2,%3,'%4',TO_DATE ('%5','YYYY-MM-DD HH24:MI:SS'),%6").arg(paraCh->chId).arg(paraDev->id)
        .arg(m_id).arg(paraPt.processCode).arg(local.toString("yyyy-MM-dd HH:mm:ss")).arg(0);
    for (int i = 0; i < section; i++) {
        sec += ",";
        sec += QString("'%1'").arg(in.at(i));
    }
    sec += ")";
    QLOG_TRACE()<<sec;
    if (query->exec(sec) != true) {
        QLOG_ERROR() << query->lastError().text();
        errCode = eErrDb;
        errMsg = query->lastError().text();
        chThread->db->ReConnect();
        return false;
    }
    if (paraCh->isTest) {
        errCode = eErrNone;
        errMsg = paraPt.processCode;
        errMsg += ";";
        errMsg += in.at(0);
    }
    return true;
}

///
/// \brief Node::doReAcq
/// \param runAble
/// \return true：执行完成，false：规定重传次数执行完成，说明重新采集仍然没有有效
///处理超时，
bool NodeWork::doReAcq(bool runAble) {
    if (runAble) {
        if (paraDev->toReTryTimes == 0) //是否需要重传。
            return false;
        if (pTimerReAcqInv->isActive()) {
            if (--m_reAcqCount == 0) {
                pTimerReAcqInv->stop();
                return false;
            }
        } else {
            m_reAcqCount = paraDev->toReTryTimes;
            pTimerReAcqInv->start(paraDev->toReTryInv); //超时重试间隔
        }
    } else {
        if (pTimerReAcqInv->isActive()) pTimerReAcqInv->stop();
    }
    return true;
}


eErrCode NodeWork::getNodeErr() {
    return errCode;
}

QString& NodeWork::getNodeErrMsg() {
    return errMsg;
}

bool NodeWork::bindBase(ChBase *chbase, PotocolBase *potocolbase, AnalyBase *analybase)
{
    if (chbase==NULL && potocolbase==NULL && analybase==NULL)
        return false;
    chBase = chbase;
    potocolBase = potocolbase;
    analyBase = analybase;
    return true;
}


bool NodeWork::setNpara(NodeWork::sNpara *pNpara) { //Node::
    paraPt = *pNpara;
    //chack para ???
    if (paraPt.acqInvTime == 0) paraPt.acqInvTime = 1;
    return true;
}

void NodeWork::setErr(eErrCode errcode, QString &errmsg)
{
    errCode = errcode;
    errMsg = errmsg;
}

bool NodeWork::setDpara(qint32 id) {
    if (chThread->m_devMap.contains(id) != true) return false;
    paraDev = chThread->m_devMap.value(id);
    //校验参数 ？？？
    return true;
}

bool NodeWork::setCpara() {
    paraCh = &chThread->m_chPara;
    //校验参数 ？？？
    return true;
}
///
/// \brief Node::setDbSection 设置数据表 段
/// \return
///
bool NodeWork::setDbSection() {
    QSqlQuery *query = chThread->db->GetSqlQuery();
    if (query == NULL) {
        QLOG_ERROR() << "OPen Db err" << __LINE__;
        errCode = eErrDb;
        errMsg = "open db err.";
        return false;
    }
    QString sec;
    query->clear();
    QString tabname = paraPt.dbTable;
    //得到表尺寸。
    sec = QString("select count(*) from user_tab_cols where table_name=upper('%1')")
        .arg(tabname); //
    query->exec(sec);
    if (query->first()) {
        bool ok;
        m_tabFieldsCount = query->value(0).toInt(&ok);
        if (ok != true) {
            errCode = eErrDb;
            errMsg = "query table get section err";
            return false;
        } else return true;
    } else {
        errCode = eErrDb;
        errMsg = "query table get section err";
        chThread->db->ReConnect();
        return false;
    }
}

///
/// \brief Node::dulTemplate 模板处理
/// \param in
/// \return
///
bool NodeWork::checkTemplate()
{    //验证模板是否正确，对模板进行解析。
    int section = analyPara.m_listTemplate.size();    //数据段个数
    if (section == 0) {
        errCode = eErrTmplate;
        errMsg = "Tmplate check section err";
        return false;
    }
    //段是否正确。
    if (m_tabFieldsCount !=  ACQ_DBTABLE_FIXED_SECTION + section) {   //-1+7
        errCode = eErrTmplate;
        errMsg = "Tmplate check section err";
        return false;
    }
    return true;
}
