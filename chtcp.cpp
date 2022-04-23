#include "chtcp.h"

ChTcp::ChTcp(QObject *parent):ChBase(parent)
{
    socket = new QTcpSocket(this);
    m_activeTime = new QTimer(this);
    m_activeTime->setSingleShot(true);
    connect(m_activeTime,SIGNAL(timeout()),this,SLOT(disConnectSocket()));
}

ChTcp::~ChTcp()
{
    delete m_activeTime;
    delete socket;
}

bool ChTcp::acqData(QByteArray &reqCmd, QByteArray &respData, int timeOut)
{
    if(socket->state()!=QAbstractSocket::ConnectedState)
    {//是否建立了连接
        socket->connectToHost(m_tcppara.remoteIP,m_tcppara.ipPort);
        if(socket->waitForConnected(timeOut)!=true)
        {   //连接失败
            m_errCode = eErrTcpConnect;
            m_errMsg  = "socket connect Host fail.";
            return false;
        }
    }
    //已经建立了连接
    m_activeTime->start(m_tcppara.activeTimeVaule);//处理active 定时器。
    //进行采集
    socket->write(reqCmd);
    if(socket->waitForBytesWritten(timeOut)!=true)
    {
        socket->disconnectFromHost();
        m_errCode = eErrTcpWrite;
        m_errMsg  = "Socket Write TimeOut.";
        return false;
    }
    if (socket->waitForReadyRead(timeOut)!=true)
    {//读超时
        m_errCode = eErrTcpRead;
        m_errMsg  = "Socket Read TimeOut.";
        return false;
    }
    respData = socket->readAll();

    if(respData.size()==0)
    {
        m_errCode = eErrTcpRlen;
        m_errMsg  = "Socket Read too less.";
        return false;
    }
//    if((respData.at(0)!= m_reqCmd[0])||(respData.at(1)!= m_reqCmd[1]))
//    {
//        errCode = eErrTcp;
//        errMsg  = "Socket Read signl err.";
//        return false;
//    }
    return true;
}

void ChTcp::setTcpPara(const QString &ip, const int port, const quint32 activeTimeVaule)
{
    m_tcppara.remoteIP = ip;
    m_tcppara.ipPort = port;
    m_tcppara.activeTimeVaule = activeTimeVaule;
}

void ChTcp::disConnectSocket() {
    socket->disconnectFromHost();
}
