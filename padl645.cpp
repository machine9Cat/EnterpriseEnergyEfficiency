#include "padl645.h"
#include "check.h"

PAdl645::PAdl645()
{

}

bool PAdl645::analypPotocol(const QByteArray &in, QByteArray &out,const QByteArray &m_reqCmd)
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

    if(size-pos < 12)             //12 是不包括数据域的最少字节数
    {
        m_errCode = eErrLen;
        m_errMsg = "read byte less 12! :";
        m_errMsg+=in.toHex();
        return false;       //数据长度错误
    }

    if((in.at(pos+7)!=0x68)||(in.at(size-1)!=0x16))
    {
        m_errCode = eErrPotocolAnalyp;
        m_errMsg = "start or end code err! :";
        m_errMsg+=in.toHex();
        return false;       //起始结束码错误
    }
    QByteArray nin = in.mid(pos,size-pos-1);

    //sum
    Check ck;
    if(ck.sum8(nin)!=true)
    {
        m_errCode = eErrPotocolAnalyp;
        m_errMsg = "chack Sum err! :";
        m_errMsg+=in.toHex();
        return false;       //校验错误
    }
    // 地址码检测
    if(m_reqCmd.mid(5,6) != nin.mid(1,6))
    {
        m_errCode = eErrPotocolAnalyp;
        m_errMsg = "addr err! :";
        m_errMsg+=in.toHex();
        return false;
    }
    //应答类型检测
    quint8 ccode = nin.at(8);
    quint8 ackCode,tagLen;
    if(m_dl645Type==eDL645_97)
    {
         ackCode= 0x81;
         tagLen = 2;
    }
    else
    {
        ackCode = 0x91;
        tagLen = 4;
    }
    if(ccode ==ackCode) //0x91
    {//数据长度检测
        quint8 len = nin.at(9);
        if((len<tagLen)&&(len+11 != nin.size())) //68a1a2a3a4a5a668xxllt1t0_len_ck
        {
            m_errCode = eErrLen;
            m_errMsg = "data section len err:";
            m_errMsg+=in.toHex();
            return false;   //有效数据长度错误
        }
        //输出有效数据
        for(int i=0;i<len-tagLen;i++)
        {
            qint8 x = nin.at(14+i);
            out.append(x-0x33);
        }
    }
    else //if(ccode==0xD1)
    {//应答错误//其他不支持的控制码。
        m_errCode = eErrAck;
        m_errMsg = "ACK code err or unsupport! :";
        m_errMsg+=in.toHex();
        return false;
    }
    return true;
}

bool PAdl645::createCmd(QByteArray &outcmd, const QString &adrStr, const qint32 dataTag)
{
    bool ok;
    quint64 adr = adrStr.toULongLong(&ok,16);
    if(ok!=true)
    {
        m_errCode = eErrAdr;
        m_errMsg = "DL645 addr err";
        return false;
    }
   outcmd.clear();
   outcmd.append(0x68);

   outcmd.append(adr);
   outcmd.append(adr>>8);
   outcmd.append(adr>>16);
   outcmd.append(adr>>24);
   outcmd.append(adr>>32);
   outcmd.append(adr>>40);

   outcmd.append(0x68);
   if(m_dl645Type==eDL645_97)
   {
       outcmd.append(0x01);   //zhu 11
       outcmd.append(0x02);   //l=2
       outcmd.append((dataTag&0xff)+0x33);
       outcmd.append(((dataTag>>8)&0xff)+0x33);
   }
   else
   {
       outcmd.append(0x11);   //zhu
       outcmd.append(0x04);   //l=4
       outcmd.append((dataTag&0xff)+0x33);
       outcmd.append(((dataTag>>8)&0xff)+0x33);
       outcmd.append(((dataTag>>16)&0xff)+0x33);
       outcmd.append(((dataTag>>24)&0xff)+0x33);
   }

   Check ck;
   ck.sum8Append(outcmd);//CS
   outcmd.append(0x16);
   outcmd.push_front(0xFE);   //加入4个前导符
   outcmd.push_front(0xFE);
   outcmd.push_front(0xFE);
   outcmd.push_front(0xFE);

  // m_reqNeedLen = -1;   //-1表示读取长度字节不指定

   return true;
}

bool PAdl645::createDataTemplate(QList<sTemplate> &m_listTemplate, const qint32 dataTag)
{
    quint8 tag3 ;
    quint8 tag2 ;
    quint8 tag1 ;
    quint8 tag0 ;
    int dl645TagMax;
    const sDl645Tag *pTag;
    if(m_dl645Type==eDL645_97)
    {
        tag3 = (dataTag>>12)&0x0F;
        tag2 = (dataTag>>8)&0x0f;

        tag1 = (dataTag>>4)&0x0f;
        tag0 = dataTag&0x0f;
        dl645TagMax =DL64597MAXTAG;
        pTag = dl64597tag;
    }
    else
    {
        tag3 = dataTag>>24;
        tag2 = dataTag>>16;
        tag1 = dataTag>>8;
        tag0 = dataTag;
        dl645TagMax =DL64507MAXTAG;
        pTag = dl64507tag;
    }
    sTemplate tpl;
    int i,section;

    sDl645Tag stag;
    for(i=0;i<dl645TagMax;i++)
    {
       stag = pTag[i];    //查找命令码
       if(tag3!=stag.t3)
           continue;
       if((tag2<stag.t2[0])||(tag2>stag.t2[1]))
           continue;
       if((tag1<stag.t1[0])||(tag1>stag.t1[1]))
           continue;
       if((tag0<stag.t0[0])||(tag0>stag.t0[1]))
           continue;
       tpl.dataLen = stag.dlen;
       tpl.dataType = (eMbType)stag.dtype;
       section = stag.sec;
       break;
    }
    if(i==DL64507MAXTAG)
    {
        m_errCode = eErrTag;
        m_errMsg = "DL645 data tag not support.";
        return false;   //数据标识不支持
    }
    while(section)
    {
        m_listTemplate.append(tpl);
        section--;
    }
    return true;
}

