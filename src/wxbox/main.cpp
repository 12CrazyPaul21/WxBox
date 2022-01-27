#include <mainwindow.h>

static void ExitHandler()
{
    google::protobuf::ShutdownProtobufLibrary();
    spdlog::drop_all();
    AppConfig::singleton().submit();
}

static void ExceptionExtHandler()
{
    spdlog::error("WxBox is crash");
    ExitHandler();
}

int main(int argc, char* argv[])
{
    wb_process::AppSingleton singleton("____WxBox_App_Singleton_Mutex____", true);

    // load application config
    AppConfig& config = AppConfig::singleton();
    config.load(wb_file::JoinPath(wb_file::GetProcessRootPath(), AppConfig::APP_CONFIG_NAME));

    // init logger
    AppConfig::RegisterLogger();

    // register exit handler
    atexit(ExitHandler);

    // register unhandled exception dumper
    wb_coredump::RegisterUnhandledExceptionAutoDumper(config.coredump_prefix(), config.coredump_path(), config.crashdumper(), config.i18n_path(), true);
    wb_coredump::RegisterExceptionExitCallback(ExceptionExtHandler);
    wb_coredump::ChangeDumperLanguage(config.language());

    Q_INIT_RESOURCE(wxbox);
    QApplication app(argc, argv);
    MainWindow   window;

    if (!window.checkSystemVersionSupported()) {
        return 0;
    }

    window.show();
    return app.exec();
}
