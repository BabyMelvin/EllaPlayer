#-------------------------------------------------
#
# Project created by QtCreator 2018-05-15T16:48:11
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PlayerQt
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    eapacketqueue.cpp \
    eaaudio.cpp \
    eavideo.cpp \
    eaclock.cpp

INCLUDEPATH += $$PWD/ffmpeg/include

QMAKE_CXXFLAGS += -std=c++11

LIBS    += $$PWD/ffmpeg/lib/avcodec.lib \
            $$PWD/ffmpeg/lib/avdevice.lib \
            $$PWD/ffmpeg/lib/avfilter.lib \
            $$PWD/ffmpeg/lib/avformat.lib \
            $$PWD/ffmpeg/lib/avutil.lib \
            $$PWD/ffmpeg/lib/postproc.lib \
            $$PWD/ffmpeg/lib/swresample.lib \
            $$PWD/ffmpeg/lib/swscale.lib

HEADERS  += mainwindow.h \
    eapacketqueue.h \
    eaaudio.h \
    eavideo.h \
    eaclock.h

FORMS    += mainwindow.ui
