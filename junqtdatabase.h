#ifndef JUNQTDATABASE_H
#define JUNQTDATABASE_H

#include <QObject>
//#include "seting.h"
#include <QtSql>

#define PropertyDeclare(Type, Name, setName, ...)                   \
    private:                                                        \
    Type m_ ## Name __VA_ARGS__;                                    \
    public:                                                         \
    inline const Type &Name(void) const { return m_ ## Name; }      \
    inline void setName(const Type &Name) { m_ ## Name = Name; }    \
    private:

enum DatabaseModeEnum{ DatabaseNameMode, DatabaseHostMode };
enum QueryMode { QueryAutoMode, QueryMultiMode, QuerySingleMode };

class DatabaseSetings
{
private:
    PropertyDeclare(DatabaseModeEnum, databaseMode, setDatabaseMode)
    PropertyDeclare(QString, databaseType, setDatabaseType)
    PropertyDeclare(QString, connectionName, setConnectionName)

    // File mode
    PropertyDeclare(QString, nameModeName, setNameModeName)

    // Host mode
    PropertyDeclare(QString, hostModeHostName, setHostModeHostName)
    PropertyDeclare(int, hostModeHostPort, setHostModeHostPort)
    PropertyDeclare(QString, hostModeDatabaseName, setHostModeDatabaseName)
    PropertyDeclare(QString, hostModeUserName, setHostModeUserName)
    PropertyDeclare(QString, hostModePassword, setHostModePassword)

private:
    DatabaseSetings(const DatabaseModeEnum &databastMode, const QString &databaseType, const QString &connectionName);
public:
    DatabaseSetings(const QString &databaseType, const QString &connectionName, const QString &nameModeName);
    DatabaseSetings(const QString &databaseType, const QString &connectionName, const QString &hostModeHostName, const int &hostModeHostPort,const QString &hostModeDatabaseName, const QString &hostModeUserName, const QString &hostModePassword);
};

class junQtDatabase :public QObject
{
    Q_OBJECT
public:
    explicit junQtDatabase(const DatabaseSetings &databaseSetings);
    ~junQtDatabase();
    QSqlQuery *GetSqlQuery();
    void ReConnect();
private:
    DatabaseSetings m_databaseSetings;
    QSqlDatabase *m_database = NULL;
    QSqlQuery *query = NULL;
    QTimer *dbToTimer;
    quint32 timeOutVaule=30000;
    void removeDateBase();
private slots:
    void dbDisConnect();
};

#endif // JUNQTDATABASE_H
