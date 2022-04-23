#include "junqtdatabase.h"

DatabaseSetings::DatabaseSetings(const DatabaseModeEnum &databastMode, const QString &databaseType, const QString &connectionName) {
    m_databaseMode = databastMode;
    m_databaseType = databaseType;
    m_connectionName = connectionName;
}
DatabaseSetings::DatabaseSetings(const QString &databaseType, const QString &connectionName, const QString &nameModeName) :
    DatabaseSetings(DatabaseNameMode, databaseType, connectionName) {
    m_nameModeName = nameModeName;
}

DatabaseSetings::DatabaseSetings(const QString &databaseType, const QString &connectionName, const QString &hostModeHostName, const int &hostModeHostPort, const QString &hostModeDatabaseName, const QString &hostModeUserName, const QString &hostModePassword) :
    DatabaseSetings(DatabaseHostMode, databaseType, connectionName) {
    m_hostModeHostName = hostModeHostName;
    m_hostModeHostPort = hostModeHostPort;
    m_hostModeDatabaseName = hostModeDatabaseName;
    m_hostModeUserName = hostModeUserName;
    m_hostModePassword = hostModePassword;
}
//-----------------------
junQtDatabase::junQtDatabase(const DatabaseSetings &databaseSetings) :
    m_databaseSetings(databaseSetings) {
    if (m_database) this->removeDateBase();
    m_database = new QSqlDatabase(QSqlDatabase::addDatabase(databaseSetings.databaseType(), databaseSetings.connectionName()));
    switch (m_databaseSetings.databaseMode()) {
    case DatabaseNameMode:
        m_database->setDatabaseName(m_databaseSetings.nameModeName());
        break;
    case DatabaseHostMode:
        m_database->setHostName(m_databaseSetings.hostModeHostName());
        m_database->setPort(m_databaseSetings.hostModeHostPort());
        m_database->setUserName(m_databaseSetings.hostModeUserName());
        m_database->setPassword(m_databaseSetings.hostModePassword());
        m_database->setDatabaseName(m_databaseSetings.hostModeDatabaseName());
        break;
    default:
        qDebug() << "database init Err." << __LINE__;
        this->removeDateBase();
        return;
    }
    //创建一个定时器，用于处理超时。
    dbToTimer = new QTimer(this);
    connect(dbToTimer, SIGNAL(timeout()), this, SLOT(dbDisConnect()));
    dbToTimer->setSingleShot(true);
}

junQtDatabase::~junQtDatabase() {
    removeDateBase();
}

void junQtDatabase::removeDateBase() {
    if (m_database) {
        if (query) {
            delete query;
            query = NULL;
        }
        delete m_database;
        m_database = NULL;
        QSqlDatabase::removeDatabase(m_databaseSetings.connectionName());
        if (dbToTimer) delete dbToTimer;
        dbToTimer = NULL;
    }
}
///
/// \brief junQtDatabase::GetSqlQuery
/// \return
///得到query 对象，如果返回NULL 表示数据库连接失败
QSqlQuery* junQtDatabase::GetSqlQuery() {
    if (m_database->isOpen()) {
        dbToTimer->start(timeOutVaule);
        return query;
    }
    if (m_database->open() != true) {
        return NULL;
    }
    query = new QSqlQuery(*m_database);
    dbToTimer->start(timeOutVaule);
    return query;
}

///
/// \brief junQtDatabase::SqlReConnect
///数据库重新连接
void junQtDatabase::ReConnect() {
    dbDisConnect();
    GetSqlQuery();
}
///
/// \brief junQtDatabase::dbTimeOut
///超时后处理
void junQtDatabase::dbDisConnect() {
    if (query) {
        delete query;
        query = NULL;
    }
    if (m_database->isOpen()) m_database->close();
}

