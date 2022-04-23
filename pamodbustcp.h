#ifndef PAMODBUSTCP_H
#define PAMODBUSTCP_H
#include "modbusbase.h"


class PAmodbusTcp:public ModbusBase
{
public:
    #define MBAP_SIZE 7
    PAmodbusTcp();
    bool analypPotocol(const QByteArray &in,QByteArray &out,const QByteArray &m_reqCmd);
    bool createCmd(QByteArray &outcmd,const qint8 adr,const qint8 fun,const quint16 regAdr,const qint16 regLen) Q_DECL_OVERRIDE;
private:
 //   QByteArray m_reqCmd;
};

#endif // PAMODBUSTCP_H
