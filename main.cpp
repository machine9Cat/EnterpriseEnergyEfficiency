
#include <QCoreApplication>
#include "mainapp.h"
#include <iostream>



//ttt可以
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    MainApp mainapp;
    int ret = app.exec();
    if (ret == RESTART_CODE) {
       QProcess::startDetached(qApp->applicationFilePath(), QStringList());
       return 0;
    }
    return ret;
}
