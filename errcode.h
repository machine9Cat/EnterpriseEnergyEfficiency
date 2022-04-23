#ifndef ERRCODE
#define ERRCODE
enum eErrCode{
    eErrNone=0,
    //通道级别
    eErrOpen,
    eErrSys,    //串口系统报错
    eErrReadTo,
    eSER_NOERR,eSER_TIMEOUT,eSER_SERSYS,
    eErrTcpConnect,
    eErrTcpWrite,
    eErrTcpRead,
    eErrTcpRlen,
    eErrZrTcpCmd,
    //协议解析级别
    eErrLen=30,  //长度错误
    eErrAdr,
    eErrPotocolSelf,//协议本身应答的错误码，
    eErrFunCode,
    eErrCrc,
    eErrModbusTcp,
    eErrAck,    //应答
    eErrTag,
    eErrMaterType,//表类型
    eErrTemplate,   //模版错误
    //数据解析级别
    eErrAnalyData=60,
    //节点级别
    eErrPotocol=100,        //1协议错误
    eErrPotocolAnalyp,  //2协议解析
    eErrTmplate,        //3数据模版错误
    eErrDb,             //4数据库错误
    eErrInsertData,     //6数据插入
    eErrSerial,         //7串口错误
    eErrSerSys,         //8串口系统错误
    eErrSerReadTo,      //9串口读超时
    eErrCalcNode,       //10节点计算
    eErrCalcNodeTrg,    //11节点数据触发
    eErrTcp,            //12TCP错误
    eErrAppBug,         //13app BUG
    eErrNodePara        //14节点参数错误（）
};
#endif // ERRCODE

