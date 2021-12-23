#include "mainwindow.h"
#include <QApplication>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include "config.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    YAML::Load("[1, 2, 3]");
    spdlog::info("wxbox is running!!!!!!");
    return a.exec();
}
