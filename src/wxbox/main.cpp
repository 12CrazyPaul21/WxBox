#include <QApplication>
#include "mainwindow.h"

void ExitHandler()
{
    google::protobuf::ShutdownProtobufLibrary();
}

int main(int argc, char* argv[])
{
    Q_INIT_RESOURCE(wxbox);
    QApplication app(argc, argv);
    MainWindow   window;
    window.show();
    atexit(ExitHandler);
    return app.exec();
}
