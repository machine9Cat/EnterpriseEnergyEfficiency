#include "mainapp.h"
#include <QDebug>
#include "channelthread.h"
#include <iostream>
#include "..\qslog\QsLog.h"
#include "..\qslog\QsLogDest.h"

void logFunction(const QString &message, QsLogging::Level level) {
    std::cout <<qPrintable(message)<< std::endl;

}

MainApp::MainApp(QObject *parent) : QObject(parent) {
    restoreConfig();    //自动恢复配置
    QSettings *setings = new QSettings("config.ini", QSettings::IniFormat); // 当前目录的INI文件
    setings->beginGroup("LOG");
    int maxFileSize = setings->value("MaxFileSize", 10240).toInt();
    int maxFileCount = setings->value("MaxFileCount", 100).toInt();
    QString logLevel = setings->value("LogLevel", "Trace").toString();
    setings->endGroup();

    QsLogging::Level level;
    if (logLevel.contains(QRegExp("\\bTrace|trace|TRACE\\b"))) level = QsLogging::TraceLevel;
    else if (logLevel.contains(QRegExp("\\bDebug|debug|DEBUG\\b"))) level = QsLogging::DebugLevel;
    else if (logLevel.contains(QRegExp("\\bInfo|info|INFO\\b"))) level = QsLogging::InfoLevel;
    else if (logLevel.contains(QRegExp("\\bWarn|warn|WARN\\b"))) level = QsLogging::WarnLevel;
    else if (logLevel.contains(QRegExp("\\bError|error|ERROR\\b"))) level = QsLogging::ErrorLevel;
    else if (logLevel.contains(QRegExp("\\bFatal|fatal|FATAL\\b"))) level = QsLogging::FatalLevel;
    else level = QsLogging::OffLevel;

    using namespace QsLogging;
    // 1. init the logging mechanism
    Logger &logger = Logger::instance();
    logger.setLoggingLevel(level);

    QDir(qApp->applicationDirPath()).mkdir("log");    //创建一个log文件夹
    const QString sLogPath(QDir(qApp->applicationDirPath()).filePath("log/log.txt"));

    // 2. 输出到
    DestinationPtr fileDestination(DestinationFactory::MakeFileDestination(
                                       sLogPath, EnableLogRotation, MaxSizeBytes(maxFileSize), MaxOldLogCount(maxFileCount)));                 //输出到文件                                                                                                                                         // DestinationPtr debugDestination(DestinationFactory::MakeDebugOutputDestination());          //输出到调试窗口
    DestinationPtr functorDestination(DestinationFactory::MakeFunctorDestination(&logFunction)); //输出到函数中
                                                                                                 //还可以输出到object 中
    logger.addDestination(fileDestination);
    // logger.addDestination(debugDestination);
    logger.addDestination(functorDestination);
    // 3. start logging
    QLOG_INFO() << "Program started";
    QLOG_INFO() <<"VER:"<<APP_VER;//app 版本
    QLOG_INFO() << "Qt" << QT_VERSION_STR << "running on" << qVersion();

    setings->beginGroup("DB");
    QString driver = setings->value("Driver", "QOCI").toString();
    QString hostname = setings->value("HostName", "10.0.3.172").toString();
    int hostPort = setings->value("HostPort", 1521).toInt();
    QString name = setings->value("DbName", "Energy").toString();
    QString userName = setings->value("UserName", "root").toString();
    QString password = setings->value("UserPassword", "root").toString();
    setings->endGroup();
    setings->beginGroup("SYS");
    m_chackDbInvTime = setings->value("ChackDbInvTime", 5000).toInt();
    setings->endGroup();
    db = new junQtDatabase({driver, "mdb", hostname, hostPort, name, userName, password });
    loadChannelConfig(0);
    //
    QTimer *chackUpdateTimer = new QTimer(this);
    connect(chackUpdateTimer, SIGNAL(timeout()), this, SLOT(chackUpdate()));
    chackUpdateTimer->start(m_chackDbInvTime);

    //建立一个tcp 通信，用于和 监控程序通信。利用信号曹方式，进行处理。
    setings->beginGroup("MONITOR");
    monitorHostName = setings->value("IP", "127.0.0.1").toString();
    monitorHostPort = setings->value("PORT", 9999).toInt();
    monitorTimeOutSec = setings->value("TIMEOUT", 30).toInt();
    monitorAppPath = setings->value("PATH", "D:/htzr/CJmonitor/CJmonitor.exe").toString();
    setings->endGroup();
    delete setings;

    socket = new QTcpSocket;
    socket->connectToHost(monitorHostName, monitorHostPort);

    //接收到I，M fine
    connect(socket, SIGNAL(readyRead()), this, SLOT(readSocket()));
    //
    monitorTimer = new QTimer(this);
    connect(monitorTimer, SIGNAL(timeout()), this, SLOT(monitorTimeOutDul()));
    monitorTimer->start(10000); //10s 发送一次心跳
    monitorCount = 0;
}

MainApp::~MainApp() {
    if (mDatabase) {
        if (mDatabase->isOpen()) mDatabase->close();
        mDatabase->removeDatabase("mydb");
        delete mDatabase;
    }
    delete  monitorTimer;
    delete  socket;
    delete chackUpdateTimer;
}
///
/// \brief MainApp::loadChannelConfig
///载入通道 参数，并且实例话线程
void MainApp::loadChannelConfig(bool isTest) {
    if (db == NULL) {
        QLOG_FATAL() << "db pointer NULL";
        return;
    }
    QString sql;
    if (isTest) sql = QString("SELECT * FROM CJ_CHPARACFG where id = %1").arg(m_tCId);
    else sql =  "SELECT * FROM CJ_CHPARACFG";
    //查询配置表，进行通道配置
    QSqlQuery *query = db->GetSqlQuery();
    if (query == NULL) {
        QLOG_ERROR() << "OPen Db err";
        db->ReConnect();
        return;
    }
    if (query->exec(sql) != true) {QLOG_ERROR() << "sql Error";}
    ChannelThread::sChannelPara chSet;
    while (query->next()) {
        chSet.chId = query->value(0).toInt();
        chSet.procType = (ProcTypeEnum)query->value("ProcType").toInt();
        chSet.chPhy = (PhyChEnum)query->value("PhyCh").toInt(); //协议类型
        chSet.bps = query->value("BaudRate").toInt();
        chSet.dataBits = query->value("DataBits").toInt();
        chSet.parityBits = query->value("Parity").toInt();
        chSet.stopBits = query->value("StopBits").toInt();
        chSet.portName = query->value("PortName").toString();
        chSet.ipPort = query->value("IPPort").toInt();
        chSet.remoteIP = query->value("RemoteIP").toString();
        chSet.activeTimeVaule = 600000;
        //= query->value("SCANTIME").toInt();
        chSet.isTest = isTest;
        chSet.tDId = m_tDId;
        chSet.tNId = m_tNid;
        // QThread newThread;
        sThread *thread = new sThread;
        thread->p = new QThread;
        thread->obj = new ChannelThread(chSet);
        thread->obj->setFatherThread(this->thread());
        thread->obj->moveToThread(thread->p);
        thread->sta =  MainApp::TsReady;
        threadList.append(thread);
    }
    query->clear();
    //启动线程
    QLOG_INFO() << threadList.size() << "thread ready...";
    foreach(sThread * thread, threadList) {
        connect(this, SIGNAL(notifyThreadQuit()), thread->obj, SLOT(readyQuit()));
        connect(this, SIGNAL(notifyThreadWork()), thread->obj, SLOT(readyWork()));

        //如果是测试通道的话，建立一个测试返回连接。
        if (isTest) {
            connect(thread->obj, SIGNAL(testFinish(int, QString)), this, SLOT(testFinishDul(int, QString)));
        }
        //    connect(thread->p, &QThread::finished, thread->obj, &QObject::deleteLater);
        thread->p->start(); //&WorkerThread::finished
        thread->sta = MainApp::TsRun;
    }
    emit notifyThreadWork();
}
///
/// \brief MainApp::exitChannel
/// \return
///通道退
bool MainApp::exitChannel() {
    //通知 通道线程退
    emit notifyThreadQuit();
    //检测所有线程是否退出
    int n = 30; //30s
    do {
        QThread::msleep(1000); //延时
        foreach(sThread * thread, threadList) {
            if (thread->p->isFinished()) {
                delete thread->p;   //删除其所在的进程。
                delete thread->obj; //在当前进行析构obj
                threadList.removeOne(thread);
            }
        }
        n--;
    }while (n & threadList.size());
    //
    if (n == 0) {
        QLOG_WARN() << "all thread quit fail.";
        return false;
    } else return true;
}
///
/// \brief MainApp::restoreConfig 恢复配置
///
void MainApp::restoreConfig() {
    QSettings *setings = new QSettings("config.ini", QSettings::IniFormat); // 当前目录的INI文件
    bool restore = setings->value("RESTORE", true).toBool();
    if (restore == true) {
        setings->setValue("RESTORE", false);
        setings->beginGroup("DB");
        setings->setValue("Driver", "QOCI");
        setings->setValue("HostName", "10.0.3.172");
        setings->setValue("HostPort", 1521);
        setings->setValue("DbName", "Energy");
        setings->setValue("UserName", "root");
        setings->setValue("UserPassword", "root");
        setings->endGroup();
        setings->beginGroup("SYS");
        setings->setValue("ChackDbInvTime", 5000);
        setings->endGroup();

        setings->beginGroup("MONITOR");
        setings->setValue("IP", "127.0.0.1");
        setings->setValue("PORT", 9999);
        setings->setValue("TIMEOUT", 30);
        setings->setValue("PATH", "D:\\htzr\\CJmonitor\\CJmonitor.exe");
        setings->endGroup();

        setings->beginGroup("LOG");
        setings->setValue("MaxFileSize", 10240);
        setings->setValue("MaxFileCount", 100);
        setings->setValue("LogLevel", "Trace");
        setings->endGroup();

        setings->sync();
    }
    delete setings;
}
///
/// \brief MainApp::chackUpdate
///检测表更新
void MainApp::chackUpdate() {
    if (db == NULL) {
        QLOG_FATAL() << "db pointer NULL";
        return;
    }
    //查询更新表内容。开是否有配置更
    QSqlQuery *query = db->GetSqlQuery();
    if (query == NULL) {
        QLOG_ERROR() << "OPen Db err";
        return;
    }
    query->clear();
    if (query->exec("SELECT * FROM CJ_ACQStateMonitor") != true) {
        db->ReConnect();
        QLOG_ERROR() << "Error SELECT * FROM CJ_ACQStateMonitor";
        return;
    }
    bool fReStart = false;
    bool fUpdate = false;
    int fCmdTest = 0;
    if (query->first()) {
        QDateTime dt = query->value("DateTime").toDateTime();
        if (dt != m_backupUpdateTime) {
            m_backupUpdateTime = dt;
            fReStart = query->value("F_restart").toBool();
            fUpdate = query->value("F_update").toBool();
            fCmdTest = query->value("F_CMDTEST").toInt();
            if (fCmdTest==1) {
                m_tCId = query->value("CHID").toInt();
                m_tDId = query->value("DEVICEID").toInt();
                m_tNid = query->value("POINTID").toInt();
            }
        }
    }
    query->clear();
    if (fReStart || fUpdate) {
        if (true != query->exec("UPDATE CJ_ACQStateMonitor SET F_restart = 0,F_update = 0")) {
            QLOG_DEBUG() << "query UPDATE CJ_ACQStateMonitor Table err!";
        }
    }
    //
    if (fCmdTest==1) {
        if (this->exitChannel()) //停止现在运行中的线程
            loadChannelConfig(true); //执行重新初始化通道,带测试
        else { //应答
            testFinishDul(255, "exit channel err!,ACQ App Err.");
            QLOG_FATAL() << "Err:update table exit channel err!Now restart app...";
            qApp->exit(RESTART_CODE); //强制退出，准备重启软件
        }
    } else if (fUpdate) { //有更新，
        if (this->exitChannel()) //停止现在运行中的线程
            loadChannelConfig(); //执行重新初始化通道
        else {
            QLOG_FATAL() << "Err:update table exit channel err!Now restart app...";
            qApp->exit(RESTART_CODE); //强制退出，准备重启软件
        }
    } else if (fReStart) {
        this->exitChannel();
        qApp->exit(RESTART_CODE);
    }
}
///
/// \brief MainApp::testFinishDul
/// \param result
/// \param reutStr
///测试完成处理。
void MainApp::testFinishDul(int result, QString reutStr) {
    //写入测试表数据
    QSqlQuery *query = db->GetSqlQuery();
    if (query == NULL) {
        QLOG_ERROR() << "OPen Db err" << __LINE__;
        return;
    }
    query->clear();
    QDateTime local(QDateTime::currentDateTime());
    QString sql;
    sql = QString("DELETE CJ_CMDTESTRESULT WHERE  CHID=%1 AND DEVICEID=%2 AND POINTID=%3").arg(m_tCId).arg(m_tDId).arg(m_tNid);
    if (query->exec(sql) != true) {
        qDebug() << query->lastError().text();
        db->ReConnect();
        goto exit;
    }
    if (result != 0) { //有错误
        reutStr.insert(0,"失败! ");
        sql = QString("INSERT INTO CJ_CMDTESTRESULT (CHID,DEVICEID,POINTID,TESTPASS,ACQDATETIME,FAILREASON)");
        sql += QString("VALUES(%1,%2,%3,%4,TO_DATE('%5','YYYY-MM-DD HH24:MI:SS'),'%6')")
            .arg(m_tCId).arg(m_tDId).arg(m_tNid).arg(result)
                .arg(local.toString("yyyy-MM-dd HH:mm:ss")).arg(reutStr);
    } else { //正确，reutStr 保存着 结果。
        QStringList list =  reutStr.split(";"); //测试通过 reutStr保存着用能工序代码和数值。用分号分开
        sql = QString("INSERT INTO CJ_CMDTESTRESULT (CHID,DEVICEID,POINTID,TESTPASS,PROCESSCODE,ACQDATETIME,VALUE1,FAILREASON)");
        sql += QString("VALUES(%1,%2,%3,%4,'%5',TO_DATE ('%6','YYYY-MM-DD HH24:MI:SS'),'%7','%8')")
            .arg(m_tCId).arg(m_tDId).arg(m_tNid).arg(result).arg(list.at(0))
                .arg(local.toString("yyyy-MM-dd HH:mm:ss")).arg(list.at(1)).arg("通过!");
    }

    if (query->exec(sql) != true) {
        QLOG_ERROR() << "query INSERT CJ_CMDTESTRESULT Table err!" << __LINE__;
        if (query->lastError().type() == QSqlError::ConnectionError) db->ReConnect();
        goto exit;
    }
    //修改状态表
    sql = "UPDATE CJ_ACQStateMonitor SET F_CMDTEST = 2";
    query->clear();
    if (true != query->exec(sql)) {
        db->ReConnect();
        QLOG_ERROR() << "query UPDATE CJ_ACQStateMonitor Table err!" << __LINE__;
    }

exit:
    if (reLoadAllChannel() != true) { //停止现在运行中的线程
        QLOG_ERROR() << "Err:update table exit channel err!Now restart app...";
        qApp->exit(RESTART_CODE); //强制退出，准备重启软件
    }
}

bool MainApp::reLoadAllChannel() {
    if (this->exitChannel()) { //停止现在运行中的线程
        loadChannelConfig(); //执行重新初始化通道
        return true;
    } else return false;
}

void MainApp::readSocket() {
    QByteArray in;
    in = socket->read(128);
    QByteArray cmp;
    cmp = "I'm Ok!";
    if (in == cmp) monitorCount = 0;
}

//10s
void MainApp::monitorTimeOutDul() {
    const char buf[] = { "how are you!" };
    //在这里处理监控程序
    QAbstractSocket::SocketState state;
    state = socket->state();
    if (state == QAbstractSocket::ConnectedState) {
        socket->write(buf);
    } else if (state == QAbstractSocket::UnconnectedState) {
        socket->connectToHost(monitorHostName, monitorHostPort);
    }
    monitorCount += 10;
    if (monitorCount > monitorTimeOutSec) {
        monitorCount = 0;
        QLOG_INFO() << "restart monitor App.";
        int index = monitorAppPath.lastIndexOf(QRegExp("/"));
        if(index<0)
        {
            QLOG_WARN()<<"MonitorApp Path err.";
            qDebug()<<"11";
        }
        else
        {
            index +=1;
            QString KillStr = "taskkill /f /im ";
            KillStr+=monitorAppPath.mid(index);
            QProcess::execute(KillStr);//kill 外部同类程序
            //监控超时。重启监控程序。
            QProcess::startDetached(monitorAppPath, QStringList(),monitorAppPath.left(index));
        }
    }
}

