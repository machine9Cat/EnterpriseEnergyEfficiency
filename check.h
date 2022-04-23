#ifndef CHECK_H
#define CHECK_H

#include "QtCore"

class Check
{
public:
    Check();
    bool crc16(const char *dat,int len);
    bool crc16(const QByteArray &dat);
    void crc16Append(char *dat,int len);
    void crc16Append(QByteArray &dat);

    bool sum8(const QByteArray &dat);
    void sum8Append(QByteArray &dat);
};

#endif // CHECK_H
