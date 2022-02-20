#ifndef __WXBOX_APP_CONFIG_HPP
#define __WXBOX_APP_CONFIG_HPP

#include <spdlog/sinks/stdout_color_sinks.h>

REGISTER_CONFIG_KEY(WXBOX_SERVER_URI);
REGISTER_CONFIG_KEY(WXBOX_CLIENT_RECONNECT_INTERVAL);
REGISTER_CONFIG_KEY(WXBOX_LANGUAGE);
REGISTER_CONFIG_KEY(WXBOX_I18N_PATH);
REGISTER_CONFIG_KEY(WXBOX_THEME_PATH);
REGISTER_CONFIG_KEY(WXBOX_THEME_NAME);
REGISTER_CONFIG_KEY(WXBOX_PLUGINS_RELPATH);
REGISTER_CONFIG_KEY(WXBOX_PLUGIN_LONG_TASK_TIMEOUT);
REGISTER_CONFIG_KEY(WXBOX_COREDUMP_PATH);
REGISTER_CONFIG_KEY(WXBOX_COREDUMP_PREFIX);
REGISTER_CONFIG_KEY(WXBOX_CRASHDUMPER);
REGISTER_CONFIG_KEY(WXBOX_LOG_PATH);
REGISTER_CONFIG_KEY(WXBOX_LOG_BASENAME);
REGISTER_CONFIG_KEY(WXBOX_LOG_MAX_ROTATING_FILE_COUNT);
REGISTER_CONFIG_KEY(WXBOX_LOG_MAX_SINGLE_FILE_SIZE);
REGISTER_CONFIG_KEY(WXBOX_LOG_AUTO_FLUSH_INTERVAL_SEC);
REGISTER_CONFIG_KEY(WXBOX_WECHAT_INSTALLATION_DIR);
REGISTER_CONFIG_KEY(WXBOX_WECHAT_MODULE_DIR);
REGISTER_CONFIG_KEY(WXBOX_WECHAT_FEATURE_RELPATH);
REGISTER_CONFIG_KEY(WXBOX_WECHAT_FEATURE_REPO_ROOT_URL);
REGISTER_CONFIG_KEY(WXBOX_WECHAT_FEATURE_UPDATE_TIMESTAMP);
REGISTER_CONFIG_KEY(WXBOX_WECHAT_MULTI_BLOXING_QUOTA);
REGISTER_CONFIG_KEY(WXBOX_WECHAT_STATUS_MONITOR_INTERVAL);
REGISTER_CONFIG_KEY(WXBOX_CLOSE_IS_MINIMIZE_TO_TRAY);
REGISTER_CONFIG_KEY(WXBOX_LOADING_ICON_TYPE);
REGISTER_CONFIG_KEY(WXBOX_LOADING_ICON_ANIMATION_USE_CACHE);

#if WXBOX_IN_WINDOWS_OS
static constexpr auto DUMPER_EXT_NAME = ".exe";
#else
static constexpr auto DUMPER_EXT_NAME = "";
#endif

class AppConfig final : public wb_config::Config
{
  private:
    AppConfig()
      : wb_config::Config()
    {
    }

    explicit AppConfig(const std::string& configPath)
      : wb_config::Config(configPath)
    {
    }

  public:
    wb_config::SafeYamlNode default_config(const std::string& keyPath) const override
    {
        wb_config::SafeYamlNode value;

#define CHECK_DEFAULT_CONFIG(KEY)          \
    {                                      \
        if (!keyPath.compare(KEY)) {       \
            value = DEFAULT_##KEY##_VALUE; \
            return value;                  \
        }                                  \
    }

        CHECK_DEFAULT_CONFIG(WXBOX_SERVER_URI);
        CHECK_DEFAULT_CONFIG(WXBOX_CLIENT_RECONNECT_INTERVAL);
        CHECK_DEFAULT_CONFIG(WXBOX_LANGUAGE);
        CHECK_DEFAULT_CONFIG(WXBOX_I18N_PATH);
        CHECK_DEFAULT_CONFIG(WXBOX_THEME_PATH);
        CHECK_DEFAULT_CONFIG(WXBOX_THEME_NAME);
        CHECK_DEFAULT_CONFIG(WXBOX_PLUGINS_RELPATH);
        CHECK_DEFAULT_CONFIG(WXBOX_PLUGIN_LONG_TASK_TIMEOUT);
        CHECK_DEFAULT_CONFIG(WXBOX_COREDUMP_PATH);
        CHECK_DEFAULT_CONFIG(WXBOX_COREDUMP_PREFIX);
        CHECK_DEFAULT_CONFIG(WXBOX_CRASHDUMPER);
        CHECK_DEFAULT_CONFIG(WXBOX_LOG_PATH);
        CHECK_DEFAULT_CONFIG(WXBOX_LOG_BASENAME);
        CHECK_DEFAULT_CONFIG(WXBOX_LOG_MAX_ROTATING_FILE_COUNT);
        CHECK_DEFAULT_CONFIG(WXBOX_LOG_MAX_SINGLE_FILE_SIZE);
        CHECK_DEFAULT_CONFIG(WXBOX_LOG_AUTO_FLUSH_INTERVAL_SEC);
        CHECK_DEFAULT_CONFIG(WXBOX_WECHAT_INSTALLATION_DIR);
        CHECK_DEFAULT_CONFIG(WXBOX_WECHAT_MODULE_DIR);
        CHECK_DEFAULT_CONFIG(WXBOX_WECHAT_FEATURE_RELPATH);
        CHECK_DEFAULT_CONFIG(WXBOX_WECHAT_FEATURE_REPO_ROOT_URL);
        CHECK_DEFAULT_CONFIG(WXBOX_WECHAT_FEATURE_UPDATE_TIMESTAMP);
        CHECK_DEFAULT_CONFIG(WXBOX_WECHAT_MULTI_BLOXING_QUOTA);
        CHECK_DEFAULT_CONFIG(WXBOX_WECHAT_STATUS_MONITOR_INTERVAL);
        CHECK_DEFAULT_CONFIG(WXBOX_CLOSE_IS_MINIMIZE_TO_TRAY);
        CHECK_DEFAULT_CONFIG(WXBOX_LOADING_ICON_TYPE);
        CHECK_DEFAULT_CONFIG(WXBOX_LOADING_ICON_ANIMATION_USE_CACHE);

        return value;
    }

    //
    // i18n
    //

    std::string language()
    {
        return this->operator[](WXBOX_LANGUAGE_KEY).safe_as<std::string>();
    }

    void change_language(const std::string& language)
    {
        this->operator[](WXBOX_LANGUAGE_KEY) = language;
        submit();
    }

    std::string i18n_path()
    {
        auto i18nSubPath = this->operator[](WXBOX_I18N_PATH_KEY).safe_as<std::string>();
        if (i18nSubPath.empty()) {
            return "";
        }

        auto rootPath = wxbox::util::file::GetProcessRootPath();
        auto i18nPath = wxbox::util::file::JoinPath(rootPath, i18nSubPath);

#if _DEBUG
        if (!wb_file::IsPathExists(i18nPath)) {
            i18nPath = wb_file::JoinPath(rootPath, "/../../../../assets/translations");
        }
#endif

        return i18nPath;
    }

    //
    // theme
    //

    std::string theme_path()
    {
        auto themeSubPath = this->operator[](WXBOX_THEME_PATH_KEY).safe_as<std::string>();
        if (themeSubPath.empty()) {
            return "";
        }

        auto rootPath  = wxbox::util::file::GetProcessRootPath();
        auto themePath = wxbox::util::file::JoinPath(rootPath, themeSubPath);

#if _DEBUG
        if (!wb_file::IsPathExists(themePath)) {
            themePath = wb_file::JoinPath(rootPath, "/../../../../assets/themes");
        }
#endif

        return themePath;
    }

    std::string current_theme_name()
    {
        return this->operator[](WXBOX_THEME_NAME_KEY).safe_as<std::string>();
    }

    void change_theme(const std::string& themeName)
    {
        this->operator[](WXBOX_THEME_NAME_KEY) = themeName;
        submit();
    }

    //
    // coredump
    //

    std::string coredump_path() const
    {
        auto coredumpPath = this->operator[](WXBOX_COREDUMP_PATH_KEY).safe_as<std::string>();
        if (coredumpPath.empty()) {
            return "";
        }

        return wxbox::util::file::JoinPath(wxbox::util::file::GetProcessRootPath(), coredumpPath);
    }

    std::string coredump_prefix() const
    {
        return this->operator[](WXBOX_COREDUMP_PREFIX_KEY).safe_as<std::string>();
    }

    std::string crashdumper() const
    {
        auto crashdumper = this->operator[](WXBOX_CRASHDUMPER_KEY).safe_as<std::string>();
        if (crashdumper.empty()) {
            return "";
        }

        auto rootPath        = wxbox::util::file::GetProcessRootPath();
        auto crashdumperPath = wxbox::util::file::JoinPath(rootPath, crashdumper) + DUMPER_EXT_NAME;

#if _DEBUG
        if (!wb_file::IsPathExists(crashdumperPath)) {
            crashdumperPath = wb_file::JoinPath(wb_file::JoinPath(rootPath, "/../crashdumper"), crashdumper) + DUMPER_EXT_NAME;
        }
#endif

        return crashdumperPath;
    }

    //
    // logger
    //

    std::string log_file_path() const
    {
        auto rootPath = wxbox::util::file::GetProcessRootPath();
        auto subPath  = log_sub_path();
        auto logName  = log_name();
        return wb_file::JoinPath(wb_file::JoinPath(rootPath, subPath), logName) + ".log";
    }

    std::string log_sub_path() const
    {
        return this->operator[](WXBOX_LOG_PATH_KEY).safe_as<std::string>();
    }

    std::string log_name() const
    {
        auto logName = this->operator[](WXBOX_LOG_BASENAME_KEY).safe_as<std::string>();
        return logName.empty() ? "WxBox" : logName;
    }

    int log_max_rotating_file_count() const
    {
        return this->operator[](WXBOX_LOG_MAX_ROTATING_FILE_COUNT_KEY).safe_as<int>();
    }

    int log_max_single_file_size() const
    {
        return this->operator[](WXBOX_LOG_MAX_SINGLE_FILE_SIZE_KEY).safe_as<int>();
    }

    int log_auto_flush_interval_sec() const
    {
        return this->operator[](WXBOX_LOG_AUTO_FLUSH_INTERVAL_SEC_KEY).safe_as<int>();
    }

    spdlog::level::level_enum log_level() const
    {
#ifdef _DEBUG
        return spdlog::level::debug;
#else
        return spdlog::level::info;
#endif
    }

    std::string log_pattern() const
    {
        return "[thread %t] %+";
    }

    //
    // feature
    //

    std::string features_path() const
    {
        auto featuresRealPath = this->operator[](WXBOX_WECHAT_FEATURE_RELPATH_KEY).safe_as<std::string>();
        if (featuresRealPath.empty()) {
            return "";
        }

        auto rootPath     = wxbox::util::file::GetProcessRootPath();
        auto featuresPath = wxbox::util::file::JoinPath(rootPath, featuresRealPath);

#if _DEBUG
        if (!wb_file::IsPathExists(featuresPath)) {
            featuresPath = wb_file::JoinPath(rootPath, "/../../../features");
        }

        if (!wb_file::IsPathExists(featuresPath)) {
            featuresPath = wb_file::JoinPath(rootPath, "/../../../../features");
        }
#endif

        return wxbox::util::file::JoinPath(wxbox::util::file::JoinPath(featuresPath, WXBOX_PLATFORM_NAME),
#if WXBOX_CPU_IS_X86
                                           "x86"
#else
                                           "x64"
#endif
        );
    }

    std::string features_repo_root_url() const
    {
        return this->operator[](WXBOX_WECHAT_FEATURE_REPO_ROOT_URL_KEY).safe_as<std::string>();
    }

    void change_features_repo_root_url(const std::string& url)
    {
        this->operator[](WXBOX_WECHAT_FEATURE_REPO_ROOT_URL_KEY) = url;
        submit();
    }

    std::string features_list_url() const
    {
        return wb_file::JoinUrl(features_repo_root_url(), std::string(WXBOX_PLATFORM_NAME) + "-feature-list.txt");
    }

    std::string feature_update_timestamp() const
    {
        return this->operator[](WXBOX_WECHAT_FEATURE_UPDATE_TIMESTAMP_KEY).safe_as<std::string>();
    }

    void update_feature_update_timestamp(const std::string& timestamp)
    {
        this->operator[](WXBOX_WECHAT_FEATURE_UPDATE_TIMESTAMP_KEY) = timestamp;
        submit();
    }

    //
    // wx
    //

    std::string wechat_installation_dir() const
    {
        return this->operator[](WXBOX_WECHAT_INSTALLATION_DIR_KEY).safe_as<std::string>();
    }

    void change_wechat_installation_dir(const std::string& path)
    {
        this->operator[](WXBOX_WECHAT_INSTALLATION_DIR_KEY) = path;
        submit();
    }

    std::string wechat_module_dir() const
    {
        return this->operator[](WXBOX_WECHAT_MODULE_DIR_KEY).safe_as<std::string>();
    }

    void change_wechat_module_dir(const std::string& path)
    {
        this->operator[](WXBOX_WECHAT_MODULE_DIR_KEY) = path;
        submit();
    }

    int wechat_status_monitor_interval() const
    {
        return this->operator[](WXBOX_WECHAT_STATUS_MONITOR_INTERVAL_KEY).safe_as<int>();
    }

    void change_wechat_status_monitor_interval(int interval)
    {
        this->operator[](WXBOX_WECHAT_STATUS_MONITOR_INTERVAL_KEY) = interval;
        submit();
    }

    //
    // wxbox
    //

    bool close_is_minimize_to_tray() const
    {
        return this->operator[](WXBOX_CLOSE_IS_MINIMIZE_TO_TRAY_KEY).safe_as<bool>();
    }

    void change_close_is_minimize_to_tray(bool toTray)
    {
        this->operator[](WXBOX_CLOSE_IS_MINIMIZE_TO_TRAY_KEY) = toTray;
        submit();
    }

    int loading_icon_type() const
    {
        int type = this->operator[](WXBOX_LOADING_ICON_TYPE_KEY).safe_as<int>();
        if (type < 0 || type > 2) {
            type = 0;
        }
        return type;
    }

    void change_loading_icon_type(int type)
    {
        if (type < 0 || type > 2) {
            type = 0;
        }
        this->operator[](WXBOX_LOADING_ICON_TYPE_KEY) = type;
        submit();
    }

    bool loading_icon_animation_use_cache() const
    {
        return this->operator[](WXBOX_LOADING_ICON_ANIMATION_USE_CACHE_KEY).safe_as<bool>();
    }

    void change_loading_icon_animation_use_cache(bool useCache)
    {
        this->operator[](WXBOX_LOADING_ICON_ANIMATION_USE_CACHE_KEY) = useCache;
        submit();
    }

    //
    // wxbox server
    //

    std::string wxbox_server_uri() const
    {
        return this->operator[](WXBOX_SERVER_URI_KEY).safe_as<std::string>();
    }

    void change_wxbox_server_uri(const std::string& uri)
    {
        this->operator[](WXBOX_SERVER_URI_KEY) = uri;
        submit();
    }

    int wxbox_client_reconnect_interval() const
    {
        return this->operator[](WXBOX_CLIENT_RECONNECT_INTERVAL_KEY).safe_as<int>();
    }

    void change_wxbox_client_reconnect_interval(int interval)
    {
        this->operator[](WXBOX_CLIENT_RECONNECT_INTERVAL_KEY) = interval;
    }

    //
    // wxbot
    //

    std::string wxbot_root_path() const
    {
#ifndef _DEBUG
        return wxbox::util::file::GetProcessRootPath();
#else
        auto rootPath      = wxbox::util::file::GetProcessRootPath();
        auto wxbotRootPath = wb_file::JoinPath(rootPath, "/../wxbot/");
        if (!wb_file::IsPathExists(wxbotRootPath)) {
            wxbotRootPath = wb_file::JoinPath(rootPath, "/../src/wxbot/");
        }
        return wxbotRootPath;
#endif
    }

    //
    // plugin
    //

    std::string plugins_root() const
    {
        auto pluginsRelpath = this->operator[](WXBOX_PLUGINS_RELPATH_KEY).safe_as<std::string>();
        if (pluginsRelpath.empty()) {
            return "";
        }

        auto rootPath        = wxbox::util::file::GetProcessRootPath();
        auto pluginsRootPath = wxbox::util::file::JoinPath(rootPath, pluginsRelpath);

#if _DEBUG
        if (!wb_file::IsPathExists(pluginsRootPath)) {
            pluginsRootPath = wb_file::JoinPath(rootPath, "/../../../../" + pluginsRelpath);
        }
#endif

        return pluginsRootPath;
    }

    int plugin_long_task_timeout() const
    {
        return this->operator[](WXBOX_PLUGIN_LONG_TASK_TIMEOUT_KEY).safe_as<int>();
    }

    void change_plugin_long_task_timeout(int timeout)
    {
        this->operator[](WXBOX_PLUGIN_LONG_TASK_TIMEOUT_KEY) = timeout;
    }

    //
    // Static Methods
    //

    static AppConfig& singleton()
    {
        static AppConfig s_app_config;
        return s_app_config;
    }

    static bool RegisterLogger()
    {
        const AppConfig& config = singleton();
        bool             retval = false;

        try {
            std::vector<spdlog::sink_ptr> sinks;
#ifdef _DEBUG
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
#endif
            sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(config.log_file_path(), config.log_max_single_file_size(), config.log_max_rotating_file_count()));

            auto logger      = std::make_shared<spdlog::logger>(config.log_name(), std::begin(sinks), std::end(sinks));
            auto wxbotLogger = std::make_shared<spdlog::logger>(WXBOT_LOG_NAME, std::begin(sinks), std::end(sinks));

            spdlog::register_logger(logger);
            spdlog::register_logger(wxbotLogger);

            spdlog::set_default_logger(logger);

            spdlog::flush_every(std::chrono::seconds(config.log_auto_flush_interval_sec()));
            spdlog::set_level(config.log_level());
            spdlog::set_pattern(config.log_pattern());

            retval = true;
        }
        catch (const spdlog::spdlog_ex& /*e*/) {
        }
        catch (const std::exception& /*e*/) {
        }

        return retval;
    }

    static constexpr char APP_CONFIG_NAME[] = "config.yml";
};

#endif  // #ifndef __WXBOX_APP_CONFIG_HPP