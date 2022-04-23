#include "swapx.h"

quint16 Swapx::c(quint16 x,quint32 t)
{
    if(t==1)
        return (swap8Short(x));
    else
        return x;
}
quint32 Swapx::c(quint32 x,quint32 t)
{
    switch(t)
    {
    case 1:
        return swap8Long(x);
    case 2:
        return swap16Long(x);
    case 3:
        return swap8t16Long(x);
    default:
        return x;
    }
}
quint64 Swapx::c(quint64 x, quint32 t)
{
    switch(t)
    {
    case 1:
        return swap8Llong(x);
    case 2:
        return swap16Llong(x);
    case 3:
        return swap8t16Llong(x);
    case 4:
        return swap32Llong(x);
    case 5:
        return swap8t32Llong(x);
    case 6:
        return swap16t32Llong(x);
    case 7:
        return swap8t16t32Llong(x);
    default:
        return x;
    }
}

void  Swapx::c(const QByteArray &x,QByteArray &out, quint32 t)
{
    if(t>0)
    {
        out.clear();
        int size=x.size();
        while(size)
        {
            size--;
            out.append(x.at(size));
        }
    }
    else
        out = x;
}


quint16 Swapx::swap8Short(quint16 x)
{
    return ((x>>8)|(x<<8));
}

quint32 Swapx::swap8Long(quint32 x)
{
    return (quint32)(swap8Short(x>>16))<<16 | (quint32)swap8Short(x);
}
quint32 Swapx::swap16Long(quint32 x)
{
   return((x>>16)|(x<<16) );
}
quint32 Swapx::swap8t16Long(quint32 x)
{
    return swap16Long(swap8Long(x));
}

quint64  Swapx::swap8Llong(quint64 x)
{

    return  (quint64)(swap8Long(x>>32))<<32 | swap8Long(x);
}

quint64  Swapx::swap16Llong(quint64 x)
{
    return (quint64)(swap16Long(x>>32))<<32 | swap16Long(x);
}

quint64  Swapx::swap32Llong(quint64 x)
{
    return((x>>32)|(x<<32));
}
quint64  Swapx::swap8t16Llong(quint64 x)
{
   return swap16Llong(swap8Llong(x));
}
quint64  Swapx::swap8t32Llong(quint64 x)
{
   return swap32Llong(swap8Llong(x));
}
quint64  Swapx::swap16t32Llong(quint64 x)
{
   return swap32Llong(swap16Llong(x));
}
quint64  Swapx::swap8t16t32Llong(quint64 x)
{
   return swap32Llong(swap8t16Llong(x));
}
