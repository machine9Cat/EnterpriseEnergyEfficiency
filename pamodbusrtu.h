#ifndef PAMODBUSRTU_H
#define PAMODBUSRTU_H
#include "modbusbase.h"
#include "potocolbase.h"
class PAmodbusRtu :public ModbusBase
{
public:
    PAmodbusRtu();
    bool analypPotocol(const QByteArray &in,QByteArray &out,const QByteArray &m_reqCmd) Q_DECL_OVERRIDE;
    bool createCmd(QByteArray &outcmd,const qint8 adr,const qint8 fun,const quint16 regAdr,const qint16 regLen) Q_DECL_OVERRIDE;

};

#endif // PAMODBUSRTU_H
