#include "chzrtcp.h"
#include "..\qslog\QsLog.h"
ChZrTcp::ChZrTcp(QObject *parent) : ChBase(parent) {
    socket = new QTcpSocket(this);
    socket->socketOption(QAbstractSocket::LowDelayOption); //低延迟打开
    m_activeTime = new QTimer(this);
    m_activeTime->setSingleShot(true);
    connect(m_activeTime, SIGNAL(timeout()), this, SLOT(disConnectSocket()));
    m_asyncCon = connect(socket, SIGNAL(readyRead()), this, SLOT(asyncReceive()));
}

ChZrTcp::~ChZrTcp() {
    delete m_activeTime;
    delete socket;
}

bool ChZrTcp::acqData(QByteArray &reqCmd, QByteArray &respData, int timeOut) {
    disconnect(m_asyncCon); //断开异步接收处理
    if (socket->state() != QAbstractSocket::ConnectedState) { //是否建立了连接
        socket->connectToHost(m_tcppara.remoteIP, m_tcppara.ipPort);
        if (socket->waitForConnected(timeOut) != true) {   //连接失败
            m_errCode = eErrTcpConnect;
            m_errMsg  = "socket connect Host fail.";
            return false;
        }
        if (socket->waitForReadyRead(2000) == true) asyncReceive();
        //发送启动串口
        m_zrTcpHead.resize(ZR_TCP_HEAD_SIZE);
        int len = ZR_TCP_HEAD_SIZE;
        m_zrTcpHead[0] = len >> 24;
        m_zrTcpHead[1] = len >> 16;
        m_zrTcpHead[2] = len >> 8;
        m_zrTcpHead[3] = len;
        socket->write(m_zrTcpHead);
        socket->waitForBytesWritten(timeOut);
    }
    bool okflg = true;
    //已经建立了连接
    m_activeTime->start(m_tcppara.activeTimeVaule); //处理active 定时器。
                                                    //进行采集
    int len = reqCmd.length();
    len += ZR_TCP_HEAD_SIZE;
    m_zrTcpHead.resize(ZR_TCP_HEAD_SIZE);
    m_zrTcpHead[0] = len >> 24;
    m_zrTcpHead[1] = len >> 16;
    m_zrTcpHead[2] = len >> 8;
    m_zrTcpHead[3] = len;
    m_zrTcpHead += reqCmd;
    socket->write(m_zrTcpHead);
    QLOG_TRACE() << "ZRTCP REQ:" << m_zrTcpHead.toHex();
    if (socket->waitForBytesWritten(timeOut) != true) {
        socket->disconnectFromHost();
        m_errCode = eErrTcpWrite;
        m_errMsg  = "Socket Write TimeOut.";
        okflg = false;
    } else {
        QElapsedTimer timer;
        timer.start();
        int readTime = timeOut;
        while (1) {
            if (socket->waitForReadyRead(readTime) != true) { //读超时
                m_errCode = eErrTcpRead;
                m_errMsg  = "Socket Read TimeOut.";
                okflg = false;
                break;
            }
            respData = socket->readAll();
            if (respData.size() < ZR_TCP_HEAD_SIZE) {
                m_errCode = eErrTcpRlen;
                m_errMsg  = "Socket Read too less.";
                okflg = false;
                break;
            }
            //判断头
            int len;
            len = respData.at('\0');
            len <<= 8;
            len += (quint8)respData.at(1);
            len <<= 8;
            len += (quint8)respData.at(2);
            len <<= 8;
            len += (quint8)respData.at(3);
            if (len != respData.size()) { //1长度检测
                m_errCode = eErrTcpRlen;
                m_errMsg  = "Socket Read len err.";
                okflg = false;
                break;
            }
            quint32 cmd;
            cmd = respData.at(4);
            cmd <<= 8;
            cmd += (quint8)respData.at(5);
            cmd <<= 8;
            cmd += (quint8)respData.at(6);
            cmd <<= 8;
            cmd += (quint8)respData.at(7);
            //数据还是握手，1握手，3数据
            if (cmd == 1) { //握手,直接应答握手数据。
                socket->write(m_zrTcpAck);
                socket->waitForBytesWritten(timeOut);
                //检查超时时间还剩多少。
                readTime = timeOut - timer.elapsed();
                if (readTime < 0) {
                    m_errCode = eErrReadTo;
                    m_errMsg  = "Socket Read TimeOut.";
                    okflg = false;
                    break;
                }
            } else if (cmd == 3) { //数据
                respData = respData.mid(ZR_TCP_HEAD_SIZE);
                break;
            } else {
                m_errCode = eErrZrTcpCmd;
                m_errMsg  = "ZrTcpCmd field err.";
                okflg = false;
                break;
            }
        }
        if (okflg) {
            if (respData.size() == 0) {
                m_errCode = eErrTcpRlen;
                m_errMsg  = "Socket Read too less.";
                okflg = false;
            }
        }
    }
    m_asyncCon = connect(socket, SIGNAL(readyRead()), this, SLOT(asyncReceive()));
    return okflg;
}

void ChZrTcp::setTcpPara(const QString &ip, const int port, const quint32 activeTimeVaule, quint8 virtualSerNum) {
    m_tcppara.remoteIP = ip;
    m_tcppara.ipPort = port;
    m_tcppara.activeTimeVaule = activeTimeVaule;
    m_tcppara.virtualSerNum = virtualSerNum + 0x64;
    //生成数据帧
    m_zrTcpHead.clear();
    m_zrTcpHead.append('\0'); //长度4byte
    m_zrTcpHead.append('\0');
    m_zrTcpHead.append('\0');
    m_zrTcpHead.append('\0');
    m_zrTcpHead.append('\0'); //指令4byte
    m_zrTcpHead.append('\0');
    m_zrTcpHead.append('\0');
    m_zrTcpHead.append(0x03); //数据指令
    m_zrTcpHead.append('\0'); //本地ID
    m_zrTcpHead.append('\0');
    m_zrTcpHead.append('\0');
    m_zrTcpHead.append(m_tcppara.virtualSerNum);
    m_zrTcpHead.append('\0'); //远程ID
    m_zrTcpHead.append('\0');
    m_zrTcpHead.append('\0');
    m_zrTcpHead.append(0x14);

    m_zrTcpAck.clear();
    m_zrTcpAck.append('\0'); //长度4byte
    m_zrTcpAck.append('\0');
    m_zrTcpAck.append('\0');
    m_zrTcpAck.append(0x10);
    m_zrTcpAck.append('\0'); //指令4byte
    m_zrTcpAck.append('\0');
    m_zrTcpAck.append('\0');
    m_zrTcpAck.append(0x01); //握手指令
    m_zrTcpAck.append('\0'); //本地ID
    m_zrTcpAck.append('\0');
    m_zrTcpAck.append('\0');
    m_zrTcpAck.append(m_tcppara.virtualSerNum);
    m_zrTcpAck.append('\0'); //远程ID
    m_zrTcpAck.append('\0');
    m_zrTcpAck.append('\0');
    m_zrTcpAck.append(0x14);

}

void ChZrTcp::disConnectSocket() {
    socket->disconnectFromHost();
}

void ChZrTcp::asyncReceive() {
    QByteArray readata;
    readata = socket->readAll();
    if (readata.size() == ZR_TCP_HEAD_SIZE) {
        quint32 cmd;
        cmd = readata.at(4);
        cmd <<= 8;
        cmd += (quint8)readata.at(5);
        cmd <<= 8;
        cmd += (quint8)readata.at(6);
        cmd <<= 8;
        cmd += (quint8)readata.at(7);
        if (cmd == 0x01) {
            socket->write(m_zrTcpAck);
            socket->waitForBytesWritten();
            QLOG_TRACE() << "ack Shake hands.";
        }
    }
}

