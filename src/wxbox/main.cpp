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
    wb_process::AppSingleton singleton("WxBox_App", true);

    // load application config
    AppConfig& config = AppConfig::singleton();
    config.load(wb_file::JoinPath(wb_file::GetProcessRootPath(), AppConfig::APP_CONFIG_NAME));

    // init logger
    AppConfig::RegisterLogger();

    // register exit handler
    atexit(ExitHandler);

    // register unhandled exception dumper
    wb_coredump::RegisterUnhandledExceptionAutoDumper(config.coredump_prefix(), config.coredump_path(), config.crashdumper(), config.i18n_path(), config.theme_path(), true);
    wb_coredump::RegisterExceptionExitCallback(ExceptionExtHandler);
    wb_coredump::ChangeDumperLanguage(config.language());
    wb_coredump::ChangeTheme(config.current_theme_name());

    // qt application
    Q_INIT_RESOURCE(wxbox);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#ifdef WXBOX_XSTYLE_QUICK
    QQuickWindow::setSceneGraphBackend(QSGRendererInterface::Software);
#endif
    QApplication app(argc, argv);

    // init xstyle
    xstyle_manager.RegisterI18nFolder(QString(config.i18n_path().c_str()));
    xstyle_manager.RegisterThemeFolder(QString(config.theme_path().c_str()));
    xstyle_manager.RegisterDefaultTheme(XSTYLE_DEFAULT_THEME_URL);
    xstyle_manager.ChangeLanguage(QString(config.language().c_str()));
    xstyle_manager.ChangeTheme(QString(config.current_theme_name().c_str()));

    // main window
    MainWindow window;
    if (!window.CheckSystemVersionSupported()) {
        return 0;
    }
    singleton.RecordWindowId(window.winId());

    window.show();
    return app.exec();
}
