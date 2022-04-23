#ifndef ANALYTYPE_H
#define ANALYTYPE_H
#include <QtCore>
enum eMbType{
    eNone,eChar,eUchar,eShort,eUshort,eLong,eUlong,eFloat,eDouble,
    eLlong,eUllong,eBcd,eZbcd,eAsc,
    eZbcdP0= 20,eZbcdP1,eZbcdP2,eZbcdP3,eZbcdP4,eZbcdP5,eZbcdP6,eZbcdP7,eZbcdP8,            //eZbcdPxN 表示小数点位置，没有符合位
    eZbcdP0N= 30,eZbcdP1N,eZbcdP2N,eZbcdP3N,eZbcdP4N,eZbcdP5N,eZbcdP6N,eZbcdP7N,eZbcdP8N,   //eZbcdPxN 表示小数点位置，和最高bit位为符合位
    eZbcdP0U= 40,eZbcdP1U,eZbcdP2U,eZbcdP3U,eZbcdP4U,eZbcdP5U,eZbcdP6U,eZbcdP7U,eZbcdP8U,   //eZbcdPxU 表示小数点位置，带单位字节
    eT0=50,eT0N,eT1,eT2,eT3,eT4,eT5,eT6,eT7,eT8,eT9,
    eT10,eT11,eT12,eT13,eT14,eT15,eT16,eT17,eT18,eT19,
    eT20,eT21
};

struct sTemplate{
    qint8 dataLen;
    eMbType dataType;
};

struct sAnaly{//数据分析参数
    quint8 swapASet;           //所有字节倒叙用于配置一些 非标准类型数据是否倒叙
    quint8 swap2Set;           //2 字节序列配置
    quint8 swap4Set;           //2 字节序列配置
    quint8 swap8Set;           //8 字节序列配置
    QList<sTemplate> m_listTemplate;    //数据解析，解析模版
};

#endif // ANALYTYPE_H

