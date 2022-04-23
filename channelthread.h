#ifndef CHANNELTHREAD_H
#define CHANNELTHREAD_H

#include <qthread.h>

#include <QtSql>
#include "junqtdatabase.h"

#include "chbase.h"
#include "potocolbase.h"
#include "analytype.h"
#include "analybase.h"


struct sDevice {
    int id;                 //设备ID
    QString name;           //
    int adr;                //设备通信地址
    int dataFm;             //数据格式
    int ToReTryTimes;       //超时重发次数
    int ToReTryInv;         //超时重发间隔
    int FrameMinTime;       //帧通信最小间隔
};
struct sNode {
    void *pt;               //节点指针
    QString name;           //节点名
};



enum PhyChEnum {PHY_Serial,
                Phy_NetTcp,
                Phy_LocalCpu,
                Phy_RemoteCpu,
                Phy_ZrTcp};

enum ProcTypeEnum {PT_ModBusRTU,
                   PT_ModBusTCP,
                   PT_DL645_97,
                   PT_DL645_07,
                   PT_CJT188,
                   PT_ZRCLC};


class ChannelThread : public QObject { //QThread
    Q_OBJECT

public:
    struct sChannelPara{ //通道参数
        bool isTest;
        int tDId;   //测试的Did
        int tNId;   //测试的nID
        int chId;
        PhyChEnum       chPhy;
        ProcTypeEnum    procType;
        int ipPort;
        QString remoteIP;
        QString portName;
        int bps;
        quint8 dataBits;
        quint8 stopBits;
        quint8 parityBits;
        quint32 activeTimeVaule;
    };
    struct sDevcie { //设备参数
        int id;
        QString name;
        QString addr;
        quint8 swapASet;           //所有字节倒叙用于配置一些 非标准类型数据是否倒叙
        quint8 swap2Set;           //2 字节序列配置
        quint8 swap4Set;           //2 字节序列配置
        quint8 swap8Set;           //8 字节序列配置
        quint16 toCmdExec;
        quint16 toReTryInv;
        quint16 toReTryTimes;
        quint16 frameMinTime;
        QList<sNode *> listNode;
    };
public:
    ChannelThread(const sChannelPara &channelSetings);
    ~ChannelThread();

    void setFatherThread(QThread *father) { m_fatherThread = father;}

    QThread *m_fatherThread = NULL;
    junQtDatabase *db = NULL;
    QMap<qint32, sDevcie *> m_devMap;
    sChannelPara m_chPara;  //

signals:
    void chCanQuit();
    void testFinish(int result, QString rsutStr);

public slots:
    void readyQuit();
    void readyWork();
private:
    ChBase *chbase = NULL;         //通道基础
    PotocolBase *ptobase = NULL;
    AnalyBase *analybase = NULL;
private:
    bool work();
    bool workCheckAndStart();
    bool openDb();
    bool addDevice();
    QSqlQuery* queryNode(int cid, int did, int nid = 0);
    bool initNode(ChBase *chbase, PotocolBase *ptobase, AnalyBase *analybase);
    bool initNodeCpuLocal();
protected:
    // void run() Q_DECL_OVERRIDE;
};

#endif // CHANNELTHREAD_H
