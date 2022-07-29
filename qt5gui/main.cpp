#include "mainwindow.h"
#include <QApplication>
#include <QProcess>
#include <QDir>
#include <QDebug>
#include <QThread>

int main(int argc, char *argv[])
{
    int result = 0;
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    QProcess *video_streamer_process = new QProcess();
    //video_streamer_process->setWorkingDirectory(QDir::homePath());
    video_streamer_process->start("xterm", QStringList() << "+hold" << "-e" << "../nesfr_vr_video_streamer/build/video_stream");

    QProcess *ctrl_process = new QProcess();
    ctrl_process->start("xterm", QStringList() << "+hold" << "-e" << "../nesfr_vr_ctrl/build/rs2_vr_ctrl");

    qDebug() << QDir::homePath();
    result &= a.exec();

    qDebug()<< video_streamer_process->readAllStandardOutput();
    qDebug()<< ctrl_process->readAllStandardOutput();
    video_streamer_process->terminate();
    ctrl_process->terminate();
    return result;
}
