#include "chbase.h"

ChBase::ChBase(QObject *parent):
    QObject(parent)
{

}
eErrCode ChBase::getLastErr(QString &errmsg) {
    errmsg = m_errMsg;
    return m_errCode;
}
