#ifndef PACJT188_H
#define PACJT188_H
#include "potocolbase.h"
#include "analytype.h"

class PAcjt188 :public PotocolBase
{
public:
#define PREAMBLE_LEN 2  //发送前导码长度
#define ADR_LEN    7
#define TAG_LEN    2
#define SER_LEN    1
#define START_FIELD_OFFSET 0
#define TYPE_FIELD_OFFSET   START_FIELD_OFFSET+1
#define ADR_FIELD_OFFSET    TYPE_FIELD_OFFSET+1
#define CTRL_FIELD_OFFSET   ADR_FIELD_OFFSET+ADR_LEN
#define LEN_FIELD_OFFSET    CTRL_FIELD_OFFSET+1
#define DATA_FIELD_OFFSET   LEN_FIELD_OFFSET+1
#define DATA_OFFSET         DATA_FIELD_OFFSET+TAG_LEN+SER_LEN

#define MIN_FARME_LEN  DATA_FIELD_OFFSET+2 //最小不含有数据域和结束符的长度。

    enum eFlowMaterRead{//流量表读标签
        eTotalFlow,eDayFlow,eNowTime,eStaSt
    };
    enum eHotMaterRead{//流量表读标签
        eDayHot,eNowHot,eHotPower,eFlow,eTotalFlowHot,eInTmp,eOutTmp,eTotalTime,eNowTimeHot,eStaStHot
    };
    quint8 m_materType;//表类型
    quint8 m_readDtType;//读取数据类型
    quint8 m_dataOffset;//截取位置 offset
    quint8 m_dataLen;   //截取数据长度字节L
public:
    PAcjt188();
    bool analypPotocol(const QByteArray &in, QByteArray &out, const QByteArray &m_reqCmd) Q_DECL_OVERRIDE;
    bool createCmd(QByteArray &outcmd, const QString &adrStr, const qint32 dataTag, const qint8 materType);

    bool createDataTemplate(QList<sTemplate> &m_listTemplate, const qint32 readType);

};

#endif // PACJT188_H
