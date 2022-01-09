#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    Q_INIT_RESOURCE(wxbox);
    QApplication app(argc, argv);
    MainWindow   window;
    window.show();
    int retval = app.exec();
    google::protobuf::ShutdownProtobufLibrary();
    return retval;
}
