#ifndef ZRCALC_H
#define ZRCALC_H

#include "nodework.h"

class Zrcalc :public NodeWork
{
public:
    Zrcalc(ChannelThread *pdev);
    ~Zrcalc();
    enum eDateAcqType{//数据采集方式：时间间隔，节点触发
        eTimeInv,eNodeTrg
    };
    enum eDataSyncType{//数据同步方式： -强制-匹配
        eSyncForce,eSyncMatch
    };

    struct sPnPara{
        int cid;
        int did;
        int nid;
        bool isTrg;
        quint8 sta;
        QString value;
        quint32 time_s;
    };
    struct sProcess{
        QString math;
        QString value;
    };

public:
    bool acqData();
    bool calc(QString &outstr);//数据计算
    bool initClcm(QString &tpl);        //初始化计算方法。
    //初始化
    bool checkCTemplateAndDul(const QString &tpl);
    void startUp();
    void createDataTemplate();
    bool setSyncType(int type, quint32 matchTim=-1);
    bool setAcqType(int type);
private slots:
    void processOneThing();
private:
    QMap<QString, sPnPara *> m_CalcVar;       //计算变量
    QMap<QString, sProcess *> m_CalcProcess;   //计算过程
    eDateAcqType m_acqType;     //采集方式
    eDataSyncType m_syncType;   //同步方式
    quint32 m_trgMatchTime;   //分钟
    //
    QString m_clacMath;             //计算公式
    bool setMathToCalcProcess(const QString &inMath);
    bool checkMath(const QString &math);
    QList<QString> getMathVar(const QString &math);
    bool calcWithMath(const QString &imath, QString &math);
    bool calcf(const QString & v1, const QString & v2, QString &out, const char sign);
    bool queryVaule(sPnPara *pnpara);
    bool verToValue(const QString &v1, double &dv1);
};

#endif // ZRCALC_H
