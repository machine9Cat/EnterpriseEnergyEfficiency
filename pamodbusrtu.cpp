#include "pamodbusrtu.h"
#include "check.h"
PAmodbusRtu::PAmodbusRtu()
{

}

bool PAmodbusRtu::analypPotocol(const QByteArray &in, QByteArray &out,const QByteArray &m_reqCmd)
{
    //crc 验证数据
    Check ck;
    if(ck.crc16(in)!=true)
    {
        m_errCode = eErrCrc;
        m_errMsg = "modbusTcp potocolAnalyp CRC err.";
        return false;
    }
    return analyBase(in,m_reqCmd,out);
}

bool PAmodbusRtu::createCmd(QByteArray &outcmd, const qint8 adr, const qint8 fun, const quint16 regAdr, const qint16 regLen)
{
    outcmd.clear();
    creatCmdBase(outcmd,adr,fun,regAdr,regLen);
    //add crc
    Check ck;
    ck.crc16Append(outcmd);
    return true;
}

