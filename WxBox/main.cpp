#include "mainwindow.h"
#include <QApplication>
#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    spdlog::info("wxbox is running!!!!!!");
    return a.exec();
}
