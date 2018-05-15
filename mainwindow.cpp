#include "mainwindow.h"
#include "ui_mainwindow.h"

extern "C"{
#include "libavformat/avformat.h"
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initFfmpeg();
    avformat_network_init();
}

MainWindow::~MainWindow()
{
    delete ui;
}
