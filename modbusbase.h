#ifndef MODBUSBASE_H
#define MODBUSBASE_H

#include "potocolbase.h"
#include "analytype.h"
class ModbusBase :public PotocolBase
{
public:
    ModbusBase();
    virtual bool createCmd(QByteArray &outcmd, const qint8 adr, const qint8 fun, const quint16 regAdr, const qint16 regLen)=0;
    bool creatCmdBase(QByteArray &outcmd, const qint8 adr,const qint8 fun, const quint16 regAdr, const qint16 regLen);
    bool analyBase(const QByteArray &in, const QByteArray &cmd, QByteArray &out);
//    int getLastErr(QString &errmsg);
//private:
//protected:
//    int m_errCode;
    //    QString m_errMsg;
    bool createDataTemplate(QList<sTemplate> &m_listTemplate, const QString &in);
};

#endif // MODBUSBASE_H
