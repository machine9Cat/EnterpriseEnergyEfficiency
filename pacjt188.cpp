#include "pacjt188.h"
#include "check.h"

PAcjt188::PAcjt188()
{

}

bool PAcjt188::analypPotocol(const QByteArray &in, QByteArray &out,const QByteArray &m_reqCmd)
{
    int  size = in.size();
    int pos=0;
    for(;pos<size;pos++)
    {//去掉前导码
       // if((unsigned char)in.at(pos)!= 0xfe)
       //     break;
        if((unsigned char)in.at(pos)== 0x68)
            break;
    }
    if(pos==size)
    {
        m_errCode = eErrLen;
        m_errMsg = "start code err! :";
        m_errMsg+=in.toHex();
        return false;       //数据长度错误
    }
    if(size-pos < MIN_FARME_LEN+1)             //12 是不包括数据域的最少字节数
    {
        m_errCode = eErrLen;
        m_errMsg = "read byte too less! :";
        m_errMsg+=in.toHex();
        return false;       //数据长度错误
    }

    if(in.at(size-1)!=0x16)
    {
        m_errCode = eErrPotocolAnalyp;
        m_errMsg = "end code err! :";
        m_errMsg+=in.toHex();
        return false;       //起始结束码错误
    }

    QByteArray nin = in.mid(pos,size-pos-1);

    //sum
    Check ck;
    if(ck.sum8(nin)!=true)
    {
        m_errCode = eErrCrc;
        m_errMsg = "chack Sum err! :";
        m_errMsg+=in.toHex();
        return false;       //校验错误
    }
    // 地址码检测
    if(m_reqCmd.mid(PREAMBLE_LEN+ADR_FIELD_OFFSET,ADR_LEN) != nin.mid(ADR_FIELD_OFFSET,ADR_LEN))
    {
        m_errCode = eErrAdr;
        m_errMsg = "addr err! :";
        m_errMsg+=in.toHex();
        return false;
    }
    //应答类型检测
    quint8 ccode = nin.at(CTRL_FIELD_OFFSET);
    if(ccode ==0x81) //0x91
    {//数据长度检测
        quint8 len = nin.at(LEN_FIELD_OFFSET);
        if((len<TAG_LEN+1)&&(len+MIN_FARME_LEN != nin.size())) //68tta1a2a3a4a5a6a7xxllt1t0_len_ck
        {
            m_errCode = eErrPotocolAnalyp;
            m_errMsg = "data section len err:";
            m_errMsg+=in.toHex();
            return false;   //有效数据长度错误
        }
        //输出有效数据 这块要根据 表类型来处理。
        for(int i=0;i<m_dataLen;i++)
        {
            qint8 x = nin.at(DATA_OFFSET+m_dataOffset+i);
            out.append(x);
        }
    }
    else //if(ccode==0xD1)
    {//应答错误//其他不支持的控制码。
        m_errCode = eErrPotocolAnalyp;
        m_errMsg = "ACK code err or unsupport! :";
        m_errMsg+=in.toHex();
        return false;
    }
    return true;
}

bool PAcjt188::createCmd(QByteArray &outcmd, const QString &adrStr, const qint32 dataTag, const qint8 materType)
{
    bool ok;
    qint64 adr = adrStr.toLongLong(&ok,16);
    if(ok!=true)
    {
        m_errCode = eErrAdr;
        m_errMsg = "CJT188 addr err";
        return false;
    }
    if((m_materType>=20)&&(m_materType<=29))
    {
        if(dataTag>eStaSt)
        {
            m_errCode = eErrMaterType;
            m_errMsg = "CJT188 MaterType err";
            return false;
        }
    }
    else
    {
        if(dataTag>eStaSt)
        {
            m_errCode = eErrTag;
            m_errMsg = "CJT188 Tag err";
            return false;
        }
    }
   outcmd.clear();
   outcmd.append(0x68);

   outcmd.append(m_materType);

   outcmd.append(adr);
   outcmd.append(adr>>8);
   outcmd.append(adr>>16);
   outcmd.append(adr>>24);
   outcmd.append(adr>>32);
   outcmd.append(adr>>40);
   outcmd.append(adr>>48);

   outcmd.append(0x01);   //读数据
   outcmd.append(0x03);   //l=2
   outcmd.append(0x90);
   outcmd.append(0x1f);
   outcmd.append(0x01);  //SER ??

   Check ck;
   ck.sum8Append(outcmd);//CS
   outcmd.append(0x16);
   for(int i=0;i<PREAMBLE_LEN;i++)
    outcmd.push_front(0xFE);   //加入前导符
   //m_reqNeedLen = -1;   //-1表示读取长度字节不指定

   m_materType = materType;
   m_readDtType = dataTag;

   return true;
}

//type: 10-19 水表：
// 0 累计流量
// 1 结算日累计流量
// 2 当前时间
// 3 状态ST
//  20-29
// 0结算日热量
// 1当前热量
// 2热功率
// 3流量
// 4累积流量
// 5供水温度
// 6回水温度
// 7累积工作时间
// 8实时时间
// 9状态ST


bool PAcjt188::createDataTemplate(QList<sTemplate> &m_listTemplate,const qint32 readType)
{
    sTemplate tpl;
    if((m_materType>=20)&&(m_materType<=29))
    {
        switch(readType)
        {
        case eDayHot:
            m_dataOffset = 0;
            tpl.dataLen =5;
            tpl.dataType=eZbcdP2U; //带单位字节的数据xxxxxx.xxdd
            break;
        case eNowHot:
            m_dataOffset = 5;
            tpl.dataLen =5;
            tpl.dataType=eZbcdP2U; //带单位字节的数据xxxxxx.xxdd
            break;
        case eHotPower:
            m_dataOffset = 10;
            tpl.dataLen =5;
            tpl.dataType=eZbcdP2U; //带单位字节的数据xxxxxx.xxdd
            break;
        case eTotalFlowHot:
            m_dataOffset = 15;
            tpl.dataLen =5;
            tpl.dataType=eZbcdP2U; //带单位字节的数据xxxxxx.xxdd
            break;
        case eFlow:
            m_dataOffset = 20;
            tpl.dataLen =5;
            tpl.dataType=eZbcdP4U; //带单位字节的数据xxxxxx.xxdd
            break;
        case eInTmp:
            m_dataOffset = 25;
            tpl.dataLen =3;
            tpl.dataType=eZbcdP2; //无单位字节的数据xxxx.xx
            break;
        case eOutTmp:
            m_dataOffset = 28;
            tpl.dataLen =3;
            tpl.dataType=eZbcdP2; //无单位字节的数据xxxx.xx
            break;
        case eTotalTime:
            m_dataOffset = 31;
            tpl.dataLen =3;
            tpl.dataType=eZbcd; //
            break;
        case eNowTimeHot:
            m_dataOffset = 34;
            tpl.dataLen =7;
            tpl.dataType=eZbcd;
            break;
        case eStaStHot:
            m_dataOffset = 41;
            tpl.dataLen =2;
            tpl.dataType=eUshort;
            break;
        default:
            m_errCode = eErrTag;
            m_errMsg = "CJT188 data tag not support.";
            return false;
        }
    }
    else
    {
        switch(readType)
        {
        case eTotalFlow:
            m_dataOffset = 0;
            tpl.dataLen =5;
            tpl.dataType=eZbcdP2U; //带单位字节的数据xxxxxx.xxdd
            break;
        case eDayFlow:
            m_dataOffset = 5;
            tpl.dataLen =5;
            tpl.dataType=eZbcdP2U; //带单位字节的数据xxxxxx.xxdd
            break;
        case eNowTime:
            m_dataOffset = 10;
            tpl.dataLen =7;
            tpl.dataType=eZbcd;
            break;
        case eStaSt:
            m_dataOffset = 17;
            tpl.dataLen =2;
            tpl.dataType=eUshort;
            break;
        default:
            m_errCode = eErrTag;
            m_errMsg = "CJT188 data tag not support.";
            return false;
        }
    }
    m_dataLen = tpl.dataLen;
    m_listTemplate.append(tpl);
    return true;
}

