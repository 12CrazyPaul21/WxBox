#include <QApplication>
#include "mainwindow.h"

void ExitHandler()
{
    google::protobuf::ShutdownProtobufLibrary();
}

int main(int argc, char* argv[])
{
    wb_process::AppSingleton singleton("____WxBox_App_Singleton_Mutex____", true);

    Q_INIT_RESOURCE(wxbox);
    QApplication app(argc, argv);
    MainWindow   window;
    window.show();
    atexit(ExitHandler);
    return app.exec();
}
