#ifndef MAINAPP_H
#define MAINAPP_H

#include <QtNetwork>
#include <QtSql>
//#include "seting.h"
#include "junqtdatabase.h"
#include "channelthread.h"

#include "version.h"
#define APP_MJVER "02.00"       //主版本号。
#define APP_VER (APP_MJVER "." SOURCE_CODE_VERSION)//主版本号.SVN 版本号

#define RESTART_CODE 773

class MainApp : public QObject
{
    Q_OBJECT
public:
    explicit MainApp(QObject *parent = 0);
    ~MainApp(void);
signals:
    notifyThreadQuit();
    notifyThreadWork();
public slots:
    void chackUpdate();
    void testFinishDul(int result,QString reutStr);
public:
    junQtDatabase *db = NULL;
    enum eThreadSta{
        TsReady,TsRun,TsCanQuit
    };
    struct sThread{
        eThreadSta sta= TsReady;
        QThread *p;
        ChannelThread *obj;
    };


private:
    QSqlDatabase *mDatabase=NULL;
    QDateTime m_backupUpdateTime;
    QList<sThread *> threadList;
    int m_chackDbInvTime;   //检测更新间隔时间
    int m_tCId;
    int m_tDId;
    int m_tNid;
    QTcpSocket *socket;
    QString monitorHostName;
    int monitorHostPort;
    int monitorTimeOutSec;
    int monitorCount;
    QString monitorAppPath;
    QTimer *monitorTimer;
    QTimer *chackUpdateTimer;
private:
    void loadChannelConfig(bool isTest=0);
    bool exitChannel();
    void restoreConfig();
    bool reLoadAllChannel();

private slots:
    void readSocket();
    void monitorTimeOutDul();
};

#endif // MAINAPP_H
