#ifndef __WXBOX_APP_CONFIG_HPP
#define __WXBOX_APP_CONFIG_HPP

REGISTER_CONFIG_KEY(WXBOX_PLUGINS_RELPATH);
REGISTER_CONFIG_KEY(WXBOX_LANGUAGE);
REGISTER_CONFIG_KEY(WXBOX_COREDUMP_PATH);
REGISTER_CONFIG_KEY(WXBOX_WECHAT_INSTALLATION_DIR);
REGISTER_CONFIG_KEY(WXBOX_WECHAT_MULTI_BLOXING_QUOTA);

class AppConfig final : public wb_config::Config
{
  private:
    AppConfig()
      : wb_config::Config()
    {
    }

    AppConfig(const std::string& configPath)
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
            return std::move(value);       \
        }                                  \
    }

        CHECK_DEFAULT_CONFIG(WXBOX_PLUGINS_RELPATH);
        CHECK_DEFAULT_CONFIG(WXBOX_LANGUAGE);
        CHECK_DEFAULT_CONFIG(WXBOX_COREDUMP_PATH);
        CHECK_DEFAULT_CONFIG(WXBOX_WECHAT_INSTALLATION_DIR);
        CHECK_DEFAULT_CONFIG(WXBOX_WECHAT_MULTI_BLOXING_QUOTA);

        return std::move(value);
    }

    static AppConfig& singleton()
    {
        static AppConfig s_app_config;
        return s_app_config;
    }

    static constexpr char APP_CONFIG_NAME[] = "config.yml";
};

#endif  // #ifndef __WXBOX_APP_CONFIG_HPP