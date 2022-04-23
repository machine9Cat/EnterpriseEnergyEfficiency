#-------------------------------------------------
#
# Project created by QtCreator 2015-08-06T14:40:58
#
#-------------------------------------------------
include(..\qslog\QsLog.pri)

QT       += core sql serialport network
QT       -= gui

TARGET = eeeCs
CONFIG   += console c++11
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    channelthread.cpp \
    mainapp.cpp \
    junqtdatabase.cpp \
    swapx.cpp \
    common.cpp \
    check.cpp \
    zrcalc.cpp \
    chtcp.cpp \
    modbusbase.cpp \
    pamodbusrtu.cpp \
    pamodbustcp.cpp \
    chserial.cpp \
    nodework.cpp \
    chbase.cpp \
    potocolbase.cpp \
    analybase.cpp \
    padl645.cpp \
    pacjt188.cpp \
    chzrtcp.cpp

HEADERS += \
    channelthread.h \
    mainapp.h \
    junqtdatabase.h \
    swapx.h \
    common.h \
    check.h \
    dl645_07_map.h \
    zrcalc.h \
    version.h \
    chtcp.h \
    modbusbase.h \
    pamodbusrtu.h \
    pamodbustcp.h \
    chserial.h \
    nodework.h \
    chbase.h \
    potocolbase.h \
    analybase.h \
    padl645.h \
    pacjt188.h \
    analytype.h \
    errcode.h \
    chzrtcp.h

CONFIG(release, debug|release):DEFINES += QT_NO_WARNING_OUTPUT\
                                            QT_NO_DEBUG_OUTPUT
win32 {
# On Windows, uncomment the following lines to build a plugin that can
# be used also in Internet Explorer, through ActiveX.
#  CONFIG += qaxserver
#  LIBS += -lurlmon
RC_FILE		= eee.rc

}

DISTFILES += \
    หตร๗.txt


