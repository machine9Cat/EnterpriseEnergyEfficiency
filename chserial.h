#ifndef CHSERIAL_H
#define CHSERIAL_H
#include <QtSerialPort/QSerialPort>
#include "chbase.h"
class ChSerial : public ChBase
{
public:
    ChSerial(QObject *parent);
    ~ChSerial();
public:
#define DEF_SER_MAX_READ_SIZE 1024      //串口一次可以读最大长度字节数

    struct sSerialPara{
        QString portName ;
        int bps ;
        QSerialPort::DataBits dataBits ;
        QSerialPort::StopBits stopBits ;
        QSerialPort::Parity parity ;
    };
    bool acqData(QByteArray &reqCmd,QByteArray &respData,int timeOut) Q_DECL_OVERRIDE;
    void setReConnect(bool recon);    //配置 重连接标记,true 每次采集完成后断开连接。false 不断开
    void setSerialpara(const QString &name, int bps, quint8 datab, quint8 stopb, quint8 parityb);

private:
    bool sendReceive(const QByteArray &send,QByteArray &rece,int recLen,int timeOutMs);

private:
    sSerialPara m_serialPara;
    bool m_reConnect = false;
    QSerialPort *serial;
};

#endif // CHSERIAL_H
