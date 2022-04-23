#include "analybase.h"

AnalyBase::AnalyBase()
{

}

///
/// \brief Node::analyData 根据解析模版进行数据解析，分段
/// \param inData
/// \param outData
///
bool AnalyBase::analyData(const QByteArray &inData, QStringList &outData, const sAnaly &analyPara) {
    QString outVal;             //输出值
    int index = 0;                //截取位置
    QByteArray bytex;           //截取到的字段
    outData.clear();
    foreach(sTemplate tpl, analyPara.m_listTemplate) {
        bytex = inData.mid(index, tpl.dataLen); //根据类型和字节顺序来处理。
                                                //调用转换函数，输出字符串。
        if (bytex.size() != tpl.dataLen) {
            errCode = eErrAnalyData;
            errMsg = "byte len err";
            return false;
        }

        if (cnvToStr(bytex, outVal, tpl.dataType,analyPara) != true) {
            errCode = eErrAnalyData;
            errMsg = "convert string err";
            return false;
        }
        outData.append(outVal);   //加入输出list
        index += tpl.dataLen;
    }
    return true; //返回正确
}

eErrCode AnalyBase::getLastErr(QString &errmsg)
{
    errmsg = errMsg;
    return errCode;
}

///
/// \brief Node::cnvToStr 转换byte 到 字符串
/// \param in   输入
/// \param out  输出字符
/// \param datatype 数据类型
/// \return
///
bool AnalyBase::cnvToStr(const QByteArray &in, QString &out, eMbType datatype,const sAnaly &analyPara) {
    bool ok = true;
    int size = in.size();
    if ((size <= 8) && (datatype <= eUllong)) {  //标准数据类型
        quint64 val = 0;
        for (int i = 0; i < size; i++) {
            val <<= 8;
            val |= (quint8)in.at(i);
        }
        switch (datatype) {
        case eChar:
            out = QString("%1").arg((qint8)val);
            break;
        case eUchar:
            out = QString("%1").arg((quint8)val);
            break;
        case eShort:
            val = swap.c((quint16)val, analyPara.swap2Set); //字节交换处理
            out = QString("%1").arg((qint16)val);
            break;
        case eUshort:
            val = swap.c((quint16)val, analyPara.swap2Set); //字节交换处理
            out = QString("%1").arg((quint16)val);
            break;
        case eLong:
            val = swap.c((quint32)val, analyPara.swap4Set); //字节交换处理
            out = QString("%1").arg((qint32)val);
            break;
        case eUlong:
            val = swap.c((quint32)val, analyPara.swap4Set); //字节交换处理
            out = QString("%1").arg((quint32)val);
            break;
        case eFloat:
        {
            val = swap.c((quint32)val, analyPara.swap4Set); //字节交换处理
            float f;
            memcpy(&f,&val,4);
            out = QString("%1").arg(f);//*(float *)&val
        }
            break;
        case eDouble:{
            val = swap.c((quint64)val, analyPara.swap8Set); //字节交换处理
            double d;
            memcpy(&d,&val,8);
            out = QString("%1").arg(d);
        }
            break;
        case eLlong:
            val = swap.c((quint64)val, analyPara.swap8Set); //字节交换处理
            out = QString("%1").arg((qint64)val);
            break;
        case eUllong:
            val = swap.c((quint64)val, analyPara.swap8Set); //字节交换处理
            out = QString("%1").arg((quint64)val);
            break;
        default:
            ok = false;
        }
    } else if ((datatype > eUllong) && (datatype < eZbcdP0)) { //非标数据类型
        QByteArray nin; //数据反转。swap
        swap.c(in, nin, analyPara.swapASet);
        switch (datatype) {
        case eBcd:
            ok = bcd2String(nin, out);
        case eZbcd:
            ok = bcdz2String(nin, out);
        case eAsc:
            out.fromStdString(nin.toStdString());
            break;
        default:
            ok = false;
        }
    } else if ((datatype >= eZbcdP0) && (datatype <= eZbcdP8)) { //吴符号
        QByteArray nin; //数据反转。swap
        swap.c(in, nin, analyPara.swapASet);
        ok = bcdz2String(nin, out);
        if (ok) { //加上小数点和符合
            ok = strAddPoint(out, datatype - eZbcdP0);
            if (ok) strRemoveUnusedZero(out);
        }
    } else if ((datatype >= eZbcdP0N) && (datatype <= eZbcdP8N)) {
        QByteArray nin; //数据反转。swap
        swap.c(in, nin, analyPara.swapASet);
        ok = bcdz2StringSignl(nin, out);
        if (ok) { //加上小数点和符合
            ok = strAddPoint(out, datatype - eZbcdP0N);
            if (ok) strRemoveUnusedZero(out);
        }
    } else if ((datatype >= eZbcdP0U) && (datatype <= eZbcdP8U)) { //带单位的数据
        QByteArray nin; //数据反转。swap
        swap.c(in, nin, analyPara.swapASet);
        quint8 unit = nin.at(0);
        nin.remove(0,1);//去掉单位字节
        //保存单位，
        ok = bcdz2String(nin, out);
        if (ok) { //加上小数点和符合
            ok = strAddPoint(out, datatype - eZbcdP0U);
            if (ok) {
                strRemoveUnusedZero(out);
                ok = unitUnify(unit, out); //单位统一处理。
            }
        }
    } else ok = false;
    return ok;
}



///
/// \brief Node::unitUnify
/// \param unit 单位
/// \param str  数值
/// \return
///单位统一。
bool AnalyBase::unitUnify(quint8 unit, QString str) {
    bool ok;
    double df;
    //str to double float
    df = str.toDouble(&ok);
    if (!ok) return false;
    switch (unit) {
    case DEF_WH:
        df = df / 1000;
        break;
    case DEF_KWH:
        break;
    case DEF_MWH:
        df = df * 1000;
        break;
    case DEF_100MWH:
        df = df * 100000;
        break;
    case DEF_J:
        df = df / 1000000000;
        break;
    case DEF_KJ:
        df = df / 1000000;
        break;
    case DEF_MJ:
        df = df / 1000;
        break;
    case DEF_GJ:
        break;
    case DEF_100GJ:
        df = df * 100;
        break;
    case DEF_W:
        df = df / 1000;
        break;
    case DEF_KW:
        break;
    case DEF_MW:
        df = df * 1000;
        break;
    case DEF_L:
        df = df / 1000;
        break;
    case DEF_M3:
        break;
    case DEF_LPH:
        df = df / 1000;
        break;
    case DEF_M3PH:
        break;
    default:
        return false;   //不支持的单位
    }
    //
    str = QString("%1").arg(df);
    return true;
}

bool AnalyBase::bcd2String(const QByteArray &in, QString &out) {
    quint8 val;
    out.clear();
    for (int i = 0; i < in.size(); i++) {
        val = in.at(i);
        if (val > 9) return false;
        out.append(val + 0x30);
    }
    return true;
}
///
/// \brief Node::bcdz2String
/// \param in
/// \param out
/// \return
///
bool AnalyBase::bcdz2String(const QByteArray &in, QString &out) {
    quint8 valL;
    out.clear();
    foreach(quint8 valH, in) {
        valL = valH & 0x0F;
        valH = (valH >> 4) & 0x0f;
        if ((valH > 9) || (valL > 9)) return false;
        out.append(valH + 0x30);
        out.append(valL + 0x30);
    }
    return true;
}

bool AnalyBase::bcdz2StringSignl(const QByteArray &in, QString &out) {
    if (in.size() < 1) return false;
    out.clear();
    quint8 valH, valL = in.at(0);
    if (valL & 0x80) out.append('-');

    valH = (valL >> 4) & 0x07;
    valL = valL & 0x0F;
    if ((valH > 9) || (valL > 9)) return false;
    out.append(valH + 0x30);
    out.append(valL + 0x30);
    for (int i = 1; i < in.size() - 1; i++) {
        valL = in.at(i);
        valH = (valL >> 4) & 0x07;
        valL = valL & 0x0F;
        if ((valH > 9) || (valL > 9)) return false;
        out.append(valH + 0x30);
        out.append(valL + 0x30);
    }
    return true;
}
///
/// \brief Node::strAddPoint
/// \param in
/// \param pos 保留小数点位数，在数据往前pos位置插入小数点
/// \return  如果数据位数少于预留的小数点位置，返回 false
///
bool AnalyBase::strAddPoint(QString &in, int pos) {
    if (pos == 0) return false;
    int size = in.size();
    if (size == 0) return false;

    if (in.at(0) == '-') {
        if (size - 1 < pos) return false;
        if (size - 1 == pos) {
            in.insert(1, '0');
            size += 1;
        }
    } else {
        if (size < pos) return false;
        if (size == pos) { //小数点和数据长度相等时，前面先补一个零
            in.insert(0, '0');
            size += 1;
        }
    }
    if (size < pos) return false;
    in = in.insert(size - pos, '.');
    return true;
}

///
/// \brief Node::removeUnusedZero
/// \param in
///删除数据中多余的0例如 00.33->0.33 -00.33 -0.33
bool AnalyBase::strRemoveUnusedZero(QString &in) {
    if (in.size() < 2) return false;

    int pos = in.indexOf('.');
    if (pos < 0) { //无小数点
        pos = in.size();
    }

    int s;
    if (in.at(0) == '-') {
        if (pos < 2) return false;
        pos -= 2;
        s = 1;
    } else {
        if (pos < 1) return false;
        pos -= 1;
        s = 0;
    }
    int n = 0;
    for (int i = s; i < pos; i++) {
        if (in.at(i) == '0') n++;
        else break;
    }
    if (n) in = in.remove(s, n);
    return true;
}

