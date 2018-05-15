#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "stdio.h"
#include <QDebug>
#include <QMutex>

extern "C"{
#include "libavformat/avformat.h"
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
   ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initUi()
{

}

void eaLogCallback(void*context,int level,const char* fmt,va_list lv){
    const char*tag="EllaPlayer";
    switch(level){
    case AV_LOG_DEBUG:
         qDebug()<<tag;
         printf(fmt,lv);
         break;
    case AV_LOG_VERBOSE:
        qDebug()<<tag;
        printf(fmt,lv);
        break;
    case AV_LOG_INFO:
        qDebug()<<tag;
        printf(fmt,lv);
        break;
    case AV_LOG_WARNING:
        qWarning()<<tag;
        printf(fmt,lv);
        break;
    case AV_LOG_ERROR:
        qCritical()<<tag;
        printf(fmt,lv);
        break;
    }
}

/**
 * @brief MainWindow::eaCallback
 * @param mutex
 * @param op
 * @return
 */
int eaCallback(void **mutex, AVLockOp op)
{
    switch (op) {
    case AV_LOCK_CREATE:
        *mutex=new QMutex;
        if(!*mutex){
           av_log(NULL,AV_LOG_FATAL,"create QMutex()");
           return 1;
        }
        break;
    case AV_LOCK_OBTAIN:
        static_cast<QMutex*>(*mutex)->lock();
    case AV_LOCK_RELEASE:
        static_cast<QMutex*>(*mutex)->unlock();
    case AV_LOCK_DESTROY:
        delete static_cast<QMutex*>(*mutex);
        break;
    default:
        break;
    }
    return 0;
}

void MainWindow::initFfmpeg()
{
   av_log_set_flags(AV_LOG_SKIP_REPEATED);
   av_log_set_callback(eaLogCallback);
#if CONFIG_AVDEVICE
   avdevice_register_all();
#endif
#if CONFIG_AVFILTER
   avfilter_register_all();
#endif

   av_register_all();
   avformat_network_init();
   av_lockmgr_register(eaCallback);
}
