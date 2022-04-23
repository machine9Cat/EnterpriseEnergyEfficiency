#include "zrcalc.h"
#include "..\qslog\QsLog.h"
///
/// \brief Zrcalc::checkCTemplate
/// \param tpl
/// \return
///检测模版是否有效
Zrcalc::Zrcalc(ChannelThread *pdev) :
    NodeWork(pdev) {

}

Zrcalc::~Zrcalc() {
    //防止内存泄漏
    foreach(sProcess * proc, m_CalcProcess) {
        delete proc;
    }
    foreach(sPnPara * pnpara, m_CalcVar) {
        delete pnpara;
    }
}

void Zrcalc::processOneThing() {
    bool nextdul = true;
    QString outstr;

    QLOG_TRACE()<<QThread::currentThreadId();
    QLOG_INFO()<< "[" << paraCh->chId<< paraDev->id<< m_id<<"]"<<"Acq...";
    if (nextdul && (acqData() != true)) {
        nextdul = false;
    }
    if (nextdul && (calc(outstr) != true)) {
        nextdul = false;
    }
    //保存
    if(nextdul){
        QLOG_TRACE()<<"CnvData"<<outstr;
        if (insertData(QStringList(outstr)) != true) {
            nextdul = false;
        }
    }

    //完成处理，出差处理
    if (nextdul == false) {
        QLOG_INFO() << "[" << paraCh->chId<< paraDev->id<< m_id<<"]"
            << "ErrC:" << getNodeErr() << "ErrM:" << getNodeErrMsg();
    }
}

bool Zrcalc::queryVaule(sPnPara *pnpara) {
    QSqlQuery *query = chThread->db->GetSqlQuery();
    if (query == NULL) {
        QLOG_ERROR() << "OPen Db err";
        return false;
    }
    QString sql = QString("SELECT * FROM CJ_ACQPOINTDATA WHERE CHID=%1 AND DEVICEID =%2 AND POINTID=%3 ORDER BY ID ASC")
        .arg(pnpara->cid).arg(pnpara->did).arg(pnpara->nid);
    query->clear();
    if (query->exec(sql) != true) {
        QLOG_ERROR() << "sql Error";
        chThread->db->ReConnect();
        return false;
    }
    //得到第一条数据。
    if (query->first()) {
        pnpara->value = query->value("VALUE1").toString();
        pnpara->time_s = query->value("ACQDateTime").toDateTime().toTime_t();
        return true;
    }
    return false;
}
///
/// \brief Zrcalc::acqData
/// \param respData
/// \return
///采集数据，采集到节点存储中，并且判断是否合法
bool Zrcalc::acqData() {
    if(m_CalcVar.size()==0)
    {
        errCode = eErrAppBug;
        errMsg = "zrcalc node acqData fun app Bug.";
        QLOG_DEBUG()<<"m_CalcVar size is 0. Is Bug.";
        return false;
    }
    //将各个节点数据采集回来。使用节点数据的最后一条
    foreach(sPnPara * pnpara, m_CalcVar) {
        if (queryVaule(pnpara) != true) {
            errCode = eErrCalcNode;
            errMsg = "acq node data not Complete.";
            return false;
        }
        else{
            QLOG_TRACE()<<"AcqData:"<< "[" <<pnpara->cid<<pnpara->did
                       <<pnpara->nid<<"]"<< pnpara->value;
        }
    }
    //数据有效型判断，比如 根据时间差来做判断，
    if (m_syncType == eSyncMatch) {
        QList<quint32> timList;
        foreach(sPnPara * pnpara, m_CalcVar) {
            timList.append(pnpara->time_s);
        }
        qSort(timList.begin(), timList.end());
        if ((timList.last() - timList.first()) > m_trgMatchTime) {
            errCode = eErrCalcNode;
            errMsg = "acq node data sync time over.";
            return false;
        }
    }

    return true;
}

///
/// \brief Zrcalc::calc
/// \param out
/// \return
///计算
bool Zrcalc::calc(QString &outstr) {
    QString math;
    if(m_CalcProcess.size()==0)
    {
        errCode = eErrAppBug;
        errMsg = "zrcalc node calc fun app Bug.";
        QLOG_DEBUG()<<"m_CalcProcess size is 0.Is Bug.";
        return false;
    }
    //根据计算 过程，来完成所有计算功能
    foreach(sProcess * proc, m_CalcProcess) {
        math = proc->math;
        //根据单体公式计算。
        if (calcWithMath(math, proc->value) != true) return false;
    }
    //完成所有过程计算，输出值。
    outstr = m_CalcProcess.last()->value;

    return true;
}

///
/// \brief Zrcalc::calcWithMath
/// \param math
/// \param out
/// \return
///根据公式计算
bool Zrcalc::calcWithMath(const QString &imath, QString &out) {

    QString lvar, rvar;
    int pos, pl, pr;
    QString math = imath;
    //先乘除x/
    while (1) {
        pos = math.indexOf(QRegExp("[*/]"));
        if (pos < 0) break;  //直到完成*/运算
        pl = math.lastIndexOf(QRegExp("[+\\-*/]"), pos - 1);
        if (pl < 0) pl = 0;
        else pl += 1;
        pr = math.indexOf("[+\\-*/]", pos + 1);
        if (pr > 0) pr -= 1;
        else pr = math.size() - 1;
        //取出左变量 右面变量
        lvar = math.mid(pl, pos - pl);
        rvar = math.mid(pos + 1, pr - pos);
        //根据变量名取出 变量对应的值或者是常量
        if (calcf(lvar, rvar, out, math.at(pos).toLatin1()) != true) return false;
        //进行位置替换
        math.replace(pl, pr - pl, out);
    }
    //合并符号++-
    QRegExp re("[+\\-]{2,}");
    while (1) {
        pos = re.indexIn(math);
        if (pos < 0) break;
        lvar = re.cap(1); //先得到加减内容，然后合并。
        if (lvar.count('-') % 2) math.replace(pos, lvar.size(), '-'); //替换
        else math.replace(pos, lvar.size(), '+');
    }
    //加减法则计算+-
    while (1) {
        pos = math.indexOf(QRegExp("[+\\-]"));
        if (pos < 0) break;  //直到完成+-运算
        pr = math.indexOf("[+\\-]", pos + 1);
        if (pr > 0) pr -= 1;
        else pr = math.size() - 1;
        //取出左变量 右面变量
        lvar = math.mid(0, pos);
        rvar = math.mid(pos + 1, pr - pos);
        //根据变量名取出 变量对应的值或者是常量
        if (calcf(lvar, rvar, out, math.at(pos).toLatin1()) != true) return false;
        //进行位置替换
        math.replace(0, pr + 1, out);
    }
    //only var eg. P1
    if (math.indexOf(QRegExp("P\\d*|G\\d*")) >= 0) {
        return (calcf(math, "0", out, 'n'));
    }
    return true;
}

bool Zrcalc::verToValue(const QString &v1, double &dv1) {
    bool ok;
    if (v1.size() == 0) return false;
    if (v1.at(0) == 'P') {
        if (m_CalcVar.contains(v1)) dv1 =  m_CalcVar[v1]->value.toDouble(&ok);
        else ok = false;
    } else if (v1.at(0) == 'G') {
        if (m_CalcProcess.contains(v1)) dv1 =  m_CalcProcess[v1]->value.toDouble(&ok);
        else ok = false;
    } else {
        dv1 = v1.toDouble(&ok);
    }
    return ok;
}

///
/// \brief Zrcalc::calcf
/// \param v1
/// \param v2
/// \param out
/// \param sign
/// \return
///查找计算
bool Zrcalc::calcf(const QString &v1, const QString &v2, QString &out, const char sign) {
    //根据字符串变量得到数值。
    double dv1, dv2, dout;
    if (verToValue(v1, dv1) != true) return false;
    if (verToValue(v2, dv2) != true) return false;
    switch (sign) {
    case '+':
        dout = dv1 + dv2;
        break;
    case '-':
        dout = dv1 - dv2;
        break;
    case '*':
        dout = dv1 * dv2;
        break;
    case '/':
        if (dv2 == 0) return false;
        dout = dv1 / dv2;
        break;
    case 'n':
        dout = dv1;
        break;
    default :
        return false;
    }
    out = QString("%1").arg(dout);
    return true;
}

bool Zrcalc::checkCTemplateAndDul(const QString &tpl) {
    QStringList strl = tpl.split(";",QString::SkipEmptyParts);
    if (strl.size() < 2)
    {
        errCode = eErrTmplate;
        errMsg = "zrcalc node Template Err.";
        return false;   //至少要有一个计算节点和一个公式
    }
    //先判断公式是否合法
    QString math = strl.last();
    math.remove(QChar(' '));        //去掉空格
    if (checkMath(math) != true)
    {
        errCode = eErrTmplate;
        errMsg = "zrcalc node Template Err.";
        return false;               //公式不合法
    }
    strl.removeLast();
    sPnPara *pnpara;
    bool ok = true;
    //去掉 公式 在判断节点是否都正确
    foreach(QString str, strl) {
        QStringList strp = str.split(",");
        if (strp.size() != 4) {
            ok = false;
            break;
        }
        pnpara = new sPnPara;
        pnpara->cid = QString(strp.at(1)).toInt(&ok);
        if (!ok) break;
        pnpara->did = QString(strp.at(2)).toInt(&ok);
        if (!ok) break;
        pnpara->nid = QString(strp.at(3)).toInt(&ok);
        if (!ok) break;
        m_CalcVar.insert(strp.at(0), pnpara);
    }
    if (!ok)
    {
        errCode = eErrTmplate;
        errMsg = "zrcalc node Template Err.";
        return false;
    }
    QList<QString> list = getMathVar(math); //取出公式变量。
                                            //检测公式中变量是否都在模板中
    foreach(QString var, list) {
        if (m_CalcVar.contains(var) != true)
        {
            errCode = eErrTmplate;
            errMsg = "zrcalc node Template Err.";
            return false; //
        }
    }
    //将公式设置到 计算过成中
    setMathToCalcProcess(math);
    return true;
}

void Zrcalc::startUp() {
    if (paraCh->isTest) {
        processOneThing();
        //得到测试结果。发送测试通知。
        emit chThread->testFinish(errCode, errMsg);
    } else {
        if (m_acqType == eTimeInv) {
            pTimerAcqInv->start(paraPt.acqInvTime * 1000); //定时器间隔
            int id = pTimerAcqInv->timerId();
            if (pTimerAcqInv->isActive() != true) QLOG_INFO() << id << "id time is not run";
            else QLOG_INFO() << id << "id time is runing";
        }
    }
}

///
/// \brief Zrcalc::setTrgSource
/// \param type
/// \param matchTim
///设置采集类型
bool Zrcalc::setAcqType(int type) {
    if ((type == eTimeInv) || (type == eNodeTrg)) {
        m_acqType = (eDateAcqType)type;
    } else {
        errCode = eErrCalcNodeTrg;
        errMsg = "Calc node AcqType err.";
        return false;
    }
    return true;
}
///
/// \brief Zrcalc::setSyncType
/// \param type
/// \param matchTim
/// \return
///设置同步方式
bool Zrcalc::setSyncType(int type, quint32 matchTim) {
    if ((type == eSyncForce)) {
        m_syncType = (eDataSyncType)type;
    } else if (type == eSyncMatch) {
        m_syncType = (eDataSyncType)type;
        if (matchTim < 1) matchTim = 1;
        m_trgMatchTime = matchTim * 60;

    } else {
        errCode = eErrCalcNodeTrg;
        errMsg = "Calc node SyncType err.";
        return false;
    }

    return true;
}



//0,包含符号 0-9 ()+-*/ 这些字符，其他都错误
//1.左右括号成对出现
//2.左括号前不能是常量和变量，运算符号后不能出现右面括号
//3.符号不能单独出现在最左边和最右边
//4.÷后面不能为0的常量。
//5.变量只能是P0——P99 P前不能有数字。
//6.常量最大8位 包含正负号和小数点
//7.一个常数中小数点出现2次以上。或者出现在前面和后面

bool Zrcalc::checkMath(const QString &math) {
    //0,只包含符号 0-9 ()+-*/ 这些字符，其他都错误
    if (math.contains(QRegExp("[^+\\-*/()\\d.P]"))) return false;
    //1.左右括号成对出现
    if (math.count("(") != math.count(")")) return false;

    //2左括号前不能是常量和变量,只能是运算符和左（除了左括弧在最左边） eg. 12(
    if (math.contains(QRegExp("[^+\\-*/(]\\("))) //左括号前除了+-*/(不能是其他字符
        return false;

    //3 有括弧后只能有 +-*/) eg. )12
    if (math.contains(QRegExp("\\)[^+\\-*/)]"))) //左括号前除了+-*/(不能是其他字符
        return false;

    //4右括弧前不能出现运算符号  eg. +)
    if (math.contains(QRegExp("[+\\-*/.]\\)"))) return false;

    //6.常量最大8位 包含小数点
    if (math.contains(QRegExp("[\\d.]{8,}"))) return false;

    //7.一个常数中小数点出现2次以上。或者出现在前面和后面
    QRegExp re("([\\d.]{1,})");
    int pos = 0;
    while (1) {
        pos = re.indexIn(math, pos);
        if (pos < 0) break;
        pos += re.matchedLength();
        QString pstr = re.cap(1);
        //检测pstr 小数点是否合法
        if (math.count(".") > 1) return false;
        if (pstr.at(0) == '.') return false;
        if (pstr.at(pstr.size() - 1) == '.') return false;
    }
    //8 变量只能是P0——P99
    if (math.contains(QRegExp("P\\d{3,}"))) //最多2个
        return false;
    if (math.contains(QRegExp("P[^\\d]"))) //至少一个数字
        return false;
    //P 前面不能有数字 和.
    if (math.contains(QRegExp("\\dP"))) //至少一个数字
        return false;
    //不能出现连续符号
    if (math.contains(QRegExp("[+\\-*/]{2,}"))) return false;
    //最后一个字符不能是符号
    if (math.lastIndexOf(QRegExp("[+\\-*/.]")) == math.size() - 1) return false;
    return true;
}
///
/// \brief Zrcalc::getMathVar
/// \param math
/// \return
///得到公式变量
QList<QString>  Zrcalc::getMathVar(const QString &math) {
    QList<QString> list;
    QRegExp sep("(P\\d+)");
    int pos = 0;
    while ((pos = sep.indexIn(math, pos)) > -1) {
        pos += sep.matchedLength();
        list.append(sep.cap(1));
    }
    return list;
}

///
/// \brief Zrcalc::setMathToCalcProcess
/// \param math
/// \return
///设置 公式到计算处理过程
bool Zrcalc::setMathToCalcProcess(const QString &inMath) {
    //1.找到最右侧,左括弧，
    QString math = inMath;
    QString gkey;
    sProcess *proc;
    int lpos, rpos, pn = 0;
    while (1) { //将括弧内容去掉了
        lpos = math.lastIndexOf('(');
        if (lpos < 0) break;
        rpos = math.indexOf(')', lpos);
        if (rpos < 0) break;
        if (lpos + 1 == rpos) break; //括弧内没内容
                                     //取下括弧内容，插入到计算过程
        proc = new sProcess;
        proc->math = math.mid(lpos + 1, rpos - lpos - 1);
        proc->value = "0";
        gkey = QString("G%1").arg(pn++);

        m_CalcProcess.insert(gkey, proc);
        //将这个括弧内容替换成 gkey
        math.replace(lpos, rpos - lpos + 1, gkey);
    }
    //最后不带括弧的算式保存到过程最后。
    proc = new sProcess;
    proc->math = math;
    proc->value = "0";
    gkey = QString("G%1").arg(pn);
    m_CalcProcess.insert(gkey, proc);
    return true;
}
