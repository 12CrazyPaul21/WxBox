#include <mainwindow.h>

static void ExitHandler()
{
    google::protobuf::ShutdownProtobufLibrary();
    spdlog::drop_all();
    AppConfig::singleton().submit();
}

static void ExceptionExitHandler()
{
    spdlog::error("wxbox is crash");
    ExitHandler();
}

int main(int argc, char* argv[])
{
    wb_process::AppSingleton singleton("WxBox_App", true);

    // load application config
    AppConfig& config = AppConfig::singleton();
    config.load(wb_file::JoinPath(wb_file::GetProcessRootPath(), AppConfig::APP_CONFIG_NAME));

    // qt application
    Q_INIT_RESOURCE(wxbox);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#ifdef WXBOX_XSTYLE_QUICK
    QQuickWindow::setSceneGraphBackend(QSGRendererInterface::Software);
#endif
    QApplication app(argc, argv);

    // splash screen
    SHOW_SPLASH_SCREEN();

    // change language
    xstyle_manager.RegisterI18nFolder(QString(config.i18n_path().c_str()));
    xstyle_manager.ChangeLanguage(QString(config.language().c_str()));

    // init logger
    SPLASH_MESSAGE("Register Logger");
    AppConfig::RegisterLogger();

    // register exit handler
    SPLASH_MESSAGE("Register Exit Handler");
    atexit(ExitHandler);

    // register unhandled exception dumper
    SPLASH_MESSAGE("Register Core Dumper");
    wb_coredump::RegisterUnhandledExceptionAutoDumper(config.coredump_prefix(), config.coredump_path(), config.crashdumper(), config.i18n_path(), config.theme_path(), true);
    wb_coredump::RegisterExceptionExitCallback(ExceptionExitHandler);
    wb_coredump::ChangeDumperLanguage(config.language());
    wb_coredump::ChangeTheme(config.current_theme_name());

    // init xstyle
    SPLASH_MESSAGE("Init XStyle Theme");
    xstyle_manager.RegisterThemeFolder(QString(config.theme_path().c_str()));
    xstyle_manager.RegisterDefaultTheme(XSTYLE_DEFAULT_THEME_URL);
    xstyle_manager.ChangeTheme(QString(config.current_theme_name().c_str()));

    //
    // main window
    //

    SPLASH_MESSAGE("Instantiation Main Window");
    MainWindow window;

    SPLASH_MESSAGE("Check System Version Supported");
    if (!window.CheckSystemVersionSupported()) {
        return 0;
    }

    SPLASH_MESSAGE("Init WxBox");
    if (!window.InitWxBox(splash)) {
        return 0;
    }

    SPLASH_MESSAGE("Record Singleton Window ID");
    singleton.RecordWindowId(window.winId());

    SPLASH_MESSAGE("Show Main Window");
    window.show();

    SPLASH_FINISH(&window);
    return app.exec();
}
