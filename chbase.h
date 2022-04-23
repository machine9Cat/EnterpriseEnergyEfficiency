#ifndef CHBASE_H
#define CHBASE_H
#include "QtCore"
#include "errcode.h"

class ChBase:public QObject
{
    Q_OBJECT
public:
    ChBase(QObject *parent);
    virtual bool acqData(QByteArray &reqCmd, QByteArray &respData, int timeOut)=0;  //采集数据（重构）
public:
    eErrCode getLastErr(QString &errmsg);

protected:
    eErrCode m_errCode=eErrNone;
    QString m_errMsg;

};

#endif // CHBASE_H
