#ifndef POTOCOLBASE_H
#define POTOCOLBASE_H

#include <QtCore>
#include "errcode.h"

class PotocolBase
{
public:
    PotocolBase();
    virtual bool analypPotocol(const QByteArray &in, QByteArray &out,const QByteArray &reqcmd)=0;

    bool analyBase(const QByteArray &in, const QByteArray &cmd, QByteArray &out);
    eErrCode getLastErr(QString &errmsg);
protected:
    eErrCode m_errCode=eErrNone;
    QString m_errMsg;
};

#endif // POTOCOLBASE_H
