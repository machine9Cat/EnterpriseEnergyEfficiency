#include "chserial.h"
#include "..\qslog\QsLog.h"
#include "qthread.h"
ChSerial::ChSerial(QObject *parent) :
    ChBase(parent) {
    serial = new QSerialPort;
}
ChSerial::~ChSerial() {
    if (serial->isOpen())
        serial->close();
    delete  serial;
}

bool ChSerial::acqData(QByteArray &reqCmd, QByteArray &respData, int timeOut) {
    if (m_reConnect) { //需要每次都重新打开。
        serial->close();
    }
    if (serial->isOpen() != true) {
        serial->setPortName(m_serialPara.portName);
        serial->setBaudRate(m_serialPara.bps);
        serial->setDataBits(m_serialPara.dataBits);
        serial->setStopBits(m_serialPara.stopBits);
        serial->setParity(m_serialPara.parity);
        if (serial->open(QIODevice::ReadWrite) != true) {
            m_errCode = eErrOpen;
            m_errMsg = QString("open %1 fail:%2").arg(m_serialPara.portName).arg(serial->error());
            QLOG_WARN() << m_errMsg;
            return false;
        }
    }
    //发送采集命令。重试次数、重试间隔、超时时间
    bool berr = sendReceive(reqCmd, respData, -1, timeOut);
    if (berr == false) {
        if (m_errCode == eErrSys) { //串口出现系统错误，关闭串口，下次重新打开串口
            serial->close();
        }
        QLOG_WARN() << m_serialPara.portName << "_ERR_T:" << m_errCode << "_ERR_M:" << m_errMsg;
    }
    return berr;
}

void ChSerial::setReConnect(bool recon)
{
    m_reConnect = recon;
}

void ChSerial::setSerialpara(const QString &name, int bps, quint8 datab, quint8 stopb, quint8 parityb) {
    m_serialPara.portName = name;
    m_serialPara.bps = bps;
    if (datab == 5) m_serialPara.dataBits = QSerialPort::Data5;
    else if (datab == 6) m_serialPara.dataBits = QSerialPort::Data6;
    else if (datab == 7) m_serialPara.dataBits = QSerialPort::Data7;
    else m_serialPara.dataBits = QSerialPort::Data8;
    if (stopb == 2) m_serialPara.stopBits = QSerialPort::TwoStop;
    else if (stopb == 3) m_serialPara.stopBits = QSerialPort::OneAndHalfStop;
    else m_serialPara.stopBits = QSerialPort::OneStop;

    if (parityb == 1)  //
        m_serialPara.parity = QSerialPort::OddParity;
    else if (parityb == 2) m_serialPara.parity = QSerialPort::EvenParity;
    else if (parityb == 3) m_serialPara.parity = QSerialPort::SpaceParity;
    else if (parityb == 4) m_serialPara.parity = QSerialPort::MarkParity;
    else m_serialPara.parity = QSerialPort::NoParity;

}

bool ChSerial::sendReceive(const QByteArray &send, QByteArray &rece, int recLen, int timeOutMs) {
    bool okflg = false;
    serial->clearError();
    serial->clear(); //清空收发缓存
    int size = serial->write(send);
    if (size != send.size()) {
        m_errCode = eErrSys;
        m_errMsg = QString("serial sys err:%1").arg(serial->error());
        return okflg;
    }
    QThread::msleep(50);
    if (serial->waitForReadyRead(timeOutMs)) {
        if (recLen < 0) {
            rece = serial->readAll();
            while (serial->waitForReadyRead(50)) {
                rece += serial->readAll();
                if (rece.size() > DEF_SER_MAX_READ_SIZE) break;
            }
            if (rece.size() == 0) {
                m_errCode = eErrReadTo;
                m_errMsg = "read miss bytes";
            } else okflg = true;
        } else {
            rece = serial->read(recLen);
            int len = rece.length();
            while ((len < recLen) && (serial->waitForReadyRead(50))) {
                rece += serial->read(recLen - len);
                len = rece.length();
            }
            if (len == recLen) okflg = true;
            else {
                m_errCode = eErrReadTo;
                m_errMsg = "read miss bytes";
            }
        }
    } else {
        if (serial->error() != QSerialPort::TimeoutError) {
            m_errCode = eErrSys;
            m_errMsg = QString("sys ser err:%1").arg(serial->error());
        } else {
            m_errCode = eErrReadTo;
            m_errMsg = "read timeout";
        }
    }

    return okflg;
}
