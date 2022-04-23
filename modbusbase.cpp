#include "modbusbase.h"
#include "..\qslog\QsLog.h"


ModbusBase::ModbusBase() {

}

bool ModbusBase::creatCmdBase(QByteArray &outcmd, const qint8 adr, const qint8 fun, const quint16 regAdr, const qint16 regLen) {
    outcmd.append(adr);
    outcmd.append(fun);
    outcmd.append(regAdr >> 8);
    outcmd.append(regAdr);
    outcmd.append(regLen >> 8);
    outcmd.append(regLen);
    return true;
}

bool ModbusBase::analyBase(const QByteArray &in, const QByteArray &cmd, QByteArray &out) {
    if (cmd.length() < 6) {
        QLOG_DEBUG() << "ModBus cmd err.";
        return false;
    }
    if (in.length() < 2) {
        m_errCode = eErrLen;
        m_errMsg = "modbusTcp data len err.";
        return false;
    }
    //地址
    if (in.at(0) != cmd.at(0)) {
        m_errCode = eErrAdr;
        m_errMsg = "modbusTcp potocolAnalyp addr err.";
        return false;
    }
    //功能码相同
    if (in.at(1) != cmd.at(1)) { //判断 是否==0x8x情况，可以用来做错误标注
        if ((in.at(1) & 0x7F) == cmd.at(1)) {
            m_errCode = eErrPotocolSelf;
            m_errMsg = QString("modbusTcp potocolAnalyp mater return err. ModBusErrCode:0x%1")
                .arg((quint8)in.at(2),2,16,QLatin1Char('0'));
        } else {
            m_errCode = eErrFunCode;
            m_errMsg = "modbusTcp potocolAnalyp fun code err.";
        }
        return false;   //功能码不符
    }
    int len = in.at(2);
    if (len != cmd.at(5) * 2) {
        m_errCode = eErrLen;
        m_errMsg = "modbusTcp potocolAnalyp len err.";
        return false;
    }
    //
    out = in.mid(3, len);
    return true;
}

///
/// \brief Node::setTemplate 设置解析模版
/// \param in
///
bool ModbusBase::createDataTemplate(QList<sTemplate> &m_listTemplate, const QString &in) {
    QStringList templist; //模版转换。
    templist = in.split(",");
    if (templist.size() < 2) {
        m_errCode = eErrTemplate;
        m_errMsg = "template format err 1";
        return false;
    }
    //根据模版协议头处理，这里首先去掉协议头。因为现在只有一种协议
    templist.removeFirst();
    bool ok;
    int npos;
    sTemplate tpl;
    foreach(QString mb, templist) {
        npos = mb.indexOf("+");
        tpl.dataLen = mb.left(npos).toInt(&ok);
        if ((npos < 1) || (ok != true)) {
            m_errCode = eErrTemplate;
            m_errMsg = "template format err 2";
            return false;
        }
        tpl.dataType = (eMbType)mb.right(npos).toInt(&ok);
        if (ok != true) {
            m_errCode = eErrTemplate;
            m_errMsg = "template format err 3";
            return false;
        }
        m_listTemplate.append(tpl);
    }
    return true;
}



