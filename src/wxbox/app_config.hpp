#ifndef __WXBOX_APP_CONFIG_HPP
#define __WXBOX_APP_CONFIG_HPP

REGISTER_CONFIG_KEY(WXBOX_LANGUAGE);
REGISTER_CONFIG_KEY(WXBOX_I18N_PATH);
REGISTER_CONFIG_KEY(WXBOX_THEME_PATH);
REGISTER_CONFIG_KEY(WXBOX_THEME_NAME);
REGISTER_CONFIG_KEY(WXBOX_PLUGINS_RELPATH);
REGISTER_CONFIG_KEY(WXBOX_COREDUMP_PATH);
REGISTER_CONFIG_KEY(WXBOX_COREDUMP_PREFIX);
REGISTER_CONFIG_KEY(WXBOX_CRASHDUMPER);
REGISTER_CONFIG_KEY(WXBOX_LOG_PATH);
REGISTER_CONFIG_KEY(WXBOX_LOG_BASENAME);
REGISTER_CONFIG_KEY(WXBOX_LOG_MAX_ROTATING_FILE_COUNT);
REGISTER_CONFIG_KEY(WXBOX_LOG_MAX_SINGLE_FILE_SIZE);
REGISTER_CONFIG_KEY(WXBOX_LOG_AUTO_FLUSH_INTERVAL_SEC);
REGISTER_CONFIG_KEY(WXBOX_WECHAT_INSTALLATION_DIR);
REGISTER_CONFIG_KEY(WXBOX_WECHAT_FEATURE_RELPATH);
REGISTER_CONFIG_KEY(WXBOX_WECHAT_MULTI_BLOXING_QUOTA);
REGISTER_CONFIG_KEY(WXBOX_CLOSE_IS_MINIMIZE_TO_TRAY);

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

        CHECK_DEFAULT_CONFIG(WXBOX_LANGUAGE);
        CHECK_DEFAULT_CONFIG(WXBOX_I18N_PATH);
        CHECK_DEFAULT_CONFIG(WXBOX_THEME_PATH);
        CHECK_DEFAULT_CONFIG(WXBOX_THEME_NAME);
        CHECK_DEFAULT_CONFIG(WXBOX_PLUGINS_RELPATH);
        CHECK_DEFAULT_CONFIG(WXBOX_COREDUMP_PATH);
        CHECK_DEFAULT_CONFIG(WXBOX_COREDUMP_PREFIX);
        CHECK_DEFAULT_CONFIG(WXBOX_CRASHDUMPER);
        CHECK_DEFAULT_CONFIG(WXBOX_LOG_PATH);
        CHECK_DEFAULT_CONFIG(WXBOX_LOG_BASENAME);
        CHECK_DEFAULT_CONFIG(WXBOX_LOG_MAX_ROTATING_FILE_COUNT);
        CHECK_DEFAULT_CONFIG(WXBOX_LOG_MAX_SINGLE_FILE_SIZE);
        CHECK_DEFAULT_CONFIG(WXBOX_LOG_AUTO_FLUSH_INTERVAL_SEC);
        CHECK_DEFAULT_CONFIG(WXBOX_WECHAT_INSTALLATION_DIR);
        CHECK_DEFAULT_CONFIG(WXBOX_WECHAT_FEATURE_RELPATH);
        CHECK_DEFAULT_CONFIG(WXBOX_WECHAT_MULTI_BLOXING_QUOTA);
        CHECK_DEFAULT_CONFIG(WXBOX_CLOSE_IS_MINIMIZE_TO_TRAY);

        return value;
    }

    //
    // i18n
    //

    std::string language()
    {
        return this->operator[](WXBOX_LANGUAGE_KEY).safe_as<std::string>();
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
    // wxbox
    //

    bool close_is_minimize_to_tray() const
    {
        return this->operator[](WXBOX_CLOSE_IS_MINIMIZE_TO_TRAY_KEY).safe_as<bool>();
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
            featuresPath = wb_file::JoinPath(rootPath, "/../../../../features");
        }
#endif

        return featuresPath;
    }

    std::string features_meta_file_path() const
    {
        return wb_file::JoinPath(features_path(), "features.yml");
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
            auto sinker = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(config.log_file_path(), config.log_max_single_file_size(), config.log_max_rotating_file_count());
            auto logger = std::make_shared<spdlog::logger>(config.log_name(), sinker);

            spdlog::register_logger(logger);
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