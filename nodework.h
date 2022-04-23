#ifndef NODEWORK_H
#define NODEWORK_H

#include <QtCore>
#include "chbase.h"
#include "potocolbase.h"

#include "swapx.h"
#include "junqtdatabase.h"
#include "channelthread.h"
#include "analybase.h"
#include "errcode.h"

class NodeWork : public QObject
{
    Q_OBJECT
public:
    #define ACQ_DBTABLE_FIXED_SECTION 7     //采集数据表中的固定字段
    struct sNpara
    {
        qint32  id;
        qint32  acqInvTime;
        QString processCode;
        QString dbTable;
    } ;

    qint32 m_id;                //节点ID
    QByteArray m_reqCmd;        //节点请求命令
    int m_reqNeedLen;           //节点请求需要数据长度
    sAnaly analyPara;           //数据分析需求参数


    ChannelThread *chThread;                //所在进程
    ChannelThread::sChannelPara *paraCh;    //通道参数
    ChannelThread::sDevcie *paraDev;        //设备参数
    sNpara paraPt;                          //节点参数

public:
    NodeWork(ChannelThread *pthread);
    ~NodeWork();
    bool setDbSection();
    bool setCpara();
    bool setDpara(qint32 id);
    bool setNpara(NodeWork::sNpara *pNpara);
    void setErr(eErrCode errcode, QString &errmsg);
    eErrCode getNodeErr();
    QString &getNodeErrMsg();
    bool bindBase(ChBase *chbase, PotocolBase *potocolbase, AnalyBase *analybase);
    void startUp();
    bool checkTemplate();
private slots:
    virtual void processOneThing();
private:
    ChBase *chBase;             //通道基类
    PotocolBase *potocolBase;   //协议基类
    AnalyBase *analyBase;       //数据分析基类

    int m_tabFieldsCount;
    int m_reAcqCount;
private:
    bool doReAcq(bool runAble);

protected:
    QTimer *pTimerAcqInv;       //采样间隔定时器
    QTimer *pTimerReAcqInv;     //重发采样间隔定时器

    eErrCode errCode=eErrNone;
    QString errMsg;
protected:
    bool insertData(const QStringList &in);
};

#endif // NODEWORK_H
