#include <QApplication>
#include <QTranslator>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include "mainwindow.h"
#include "config.h"

int main(int argc, char* argv[])
{
    Q_INIT_RESOURCE(wxbox);
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
