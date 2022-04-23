#ifndef ANALYBASE_H
#define ANALYBASE_H

#include <QtCore>
#include "swapx.h"
#include "analytype.h"
#include "errcode.h"

#define DEF_WH 0x02
#define DEF_KWH 0x05    //标准到这
#define DEF_MWH 0x08
#define DEF_100MWH 0x0A

#define DEF_J   0x01
#define DEF_KJ  0x0B
#define DEF_MJ  0x0E
#define DEF_GJ      0x11    //标准到这
#define DEF_100GJ   0x13

#define DEF_W   0x14
#define DEF_KW  0x1A
#define DEF_MW  0x17
#define DEF_L   0x29    //升
#define DEF_M3  0x2C    //立方米 标准到这
#define DEF_LPH     0x32    //升每小时
#define DEF_M3PH    0x35    //立方米每小时 标准到这

class AnalyBase
{
public:
    Swapx swap;
public:
    AnalyBase();
    virtual bool analyData(const QByteArray &inData, QStringList &outData,const sAnaly &analyPara);//数据解析
    bool setTemplate(const QString &in);
    eErrCode getLastErr(QString &errmsg);
private:
    eErrCode errCode=eErrNone;
    QString errMsg;
private:
    bool cnvToStr(const QByteArray &in, QString &out, eMbType datatype, const sAnaly &analyPara);
    bool unitUnify(quint8 unit, QString str);
    bool bcd2String(const QByteArray &in, QString &out);
    bool bcdz2String(const QByteArray &in, QString &out);
    bool bcdz2StringSignl(const QByteArray &in, QString &out);
    bool strAddPoint(QString &in, int pos);
    bool strRemoveUnusedZero(QString &in);
};



#endif // ANALYBASE_H
