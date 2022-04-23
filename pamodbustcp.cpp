#include "pamodbustcp.h"

PAmodbusTcp::PAmodbusTcp()
{

}

bool PAmodbusTcp::analypPotocol(const QByteArray &in, QByteArray &out, const QByteArray &m_reqCmd)
{
    if(in.size()<MBAP_SIZE)
    {
        m_errCode = eErrLen;
        m_errMsg = "modbusTcp potocolAnalyp read too less.";
        return false;
    }
    if((in[2]!=0)||(in[3]!=0))
    {
        m_errCode = eErrModbusTcp;
        m_errMsg = "modbusTcp potocolAnalyp no a modbus.";
        return false;
    }
    int pduLen = in[4];
    pduLen = (pduLen<<8) + in[5];

    if(pduLen+MBAP_SIZE-1 !=in.size())
    {
        m_errCode = eErrLen;
        m_errMsg = "modbusTcp potocolAnalyp len err.";
        return false;
    }
    return analyBase(in.mid(6),m_reqCmd.mid(6),out);
}

bool PAmodbusTcp::createCmd(QByteArray &outcmd, const qint8 adr, const qint8 fun, const quint16 regAdr, const qint16 regLen)
{
    outcmd.clear();
    outcmd.append('\0');//标识符
    outcmd.append('\0');//标识符
    outcmd.append('\0');//Modbus标识符
    outcmd.append('\0');//Modbus标识符
    outcmd.append('\0');//长度 。。
    outcmd.append(6);//长度 。。
    creatCmdBase(outcmd,adr,fun,regAdr,regLen);
    //计算出需要请求的数据长度。
   // m_reqNeedLen = regLen*2 +5 ;//地址1+命令1+数据字节1+CRC2=5
    return true;
}

