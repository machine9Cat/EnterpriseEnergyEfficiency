#ifndef CHTCP_H
#define CHTCP_H

#include "QtCore"
//#include <QtCore/qobject.h>
#include <QtNetwork>
#include "chbase.h"

class ChTcp : public ChBase
{
    Q_OBJECT
    struct sChTcpPara{
        QString remoteIP;       //IP
        int ipPort;             //port
        quint32 activeTimeVaule;//存活时间
    };

public:
    ChTcp(QObject *parent);

    ~ChTcp();
    bool acqData(QByteArray &reqCmd,QByteArray &respData,int timeOut) Q_DECL_OVERRIDE;
    void setTcpPara(const QString &ip,const int port,const quint32 activeTimeVaule);
private slots:
    void disConnectSocket();
private:
    QTimer *m_activeTime;    //存活定时器器
    QTcpSocket *socket;
    sChTcpPara m_tcppara;

};

#endif // CHTCP_H
