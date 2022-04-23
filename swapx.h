#ifndef SWAPX_H
#define SWAPX_H

#include <QtCore>
///
/// \brief The Swapx class 用与字节顺序转换。支持 双 四 八 字节类型变量
/// x 对应字节序
/// 0 不转换1122334455667788
/// 1 swap8             单字节变换 例     2211443366558877
/// 2 swap16            双字节变换 例     3344112277885566
/// 3 swap8t16          单字节变换后双字节变换 4433221188776655
/// 4 swap32            四字节变换 例     5566778811223344
/// 5 swap8t32          单、四字节变换     6655887722114433
/// 6 swap16t32         双、四字节变换     7788556633441122
/// 7 swap8t16t32       单、双、四字节变换  8877665544332211

class Swapx{
public:
    quint16 c(quint16 x,quint32 t);
    quint32 c(quint32 x, quint32 t);
    quint64 c(quint64 x,quint32 t);
    void  c(const QByteArray &x,QByteArray &out, quint32 t);
protected:

private:
    quint16 swap8Short(quint16 x);
    quint32 swap8Long(quint32 x);
    quint32 swap16Long(quint32 x);
    quint32 swap8t16Long(quint32 x);
    quint64  swap8Llong(quint64 x);
    quint64  swap16Llong(quint64 x);
    quint64  swap32Llong(quint64 x);
    quint64  swap8t16Llong(quint64 x);
    quint64  swap8t32Llong(quint64 x);
    quint64  swap16t32Llong(quint64 x);
    quint64  swap8t16t32Llong(quint64 x);
};

#endif // SWAPX_H
