#include "potocolbase.h"

PotocolBase::PotocolBase()
{

}

eErrCode PotocolBase::getLastErr(QString &errmsg)
{
    errmsg = m_errMsg;
    return m_errCode;
}
