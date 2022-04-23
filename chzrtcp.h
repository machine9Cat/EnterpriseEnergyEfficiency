#ifndef CHZRTCP_H
#define CHZRTCP_H
#include "QtCore"
#include <QtNetwork>
#include "chbase.h"

class ChZrTcp : public ChBase
{
    Q_OBJECT
#define ZR_TCP_HEAD_SIZE 16
    struct sChTcpPara{
        QString remoteIP;       //IP
        int ipPort;             //port
        quint32 activeTimeVaule;//存活时间
        quint8 virtualSerNum;   //虚拟串口号
    };
public:
    ChZrTcp(QObject *parent);
    ~ChZrTcp();
    bool acqData(QByteArray &reqCmd,QByteArray &respData,int timeOut) Q_DECL_OVERRIDE;
    void setTcpPara(const QString &ip,const int port,const quint32 activeTimeVaule,quint8 virtualSerNum);
private slots:
    void disConnectSocket();
    void asyncReceive();    //接收到异步数据处理
private:
    QTimer *m_activeTime;    //存活定时器器
    QTcpSocket *socket;
    sChTcpPara m_tcppara;
    QByteArray m_zrTcpHead;
    QByteArray m_zrTcpAck;
    QMetaObject::Connection m_asyncCon;
};

#endif // CHZRTCP_H
