#ifndef __X_STYLE_H
#define __X_STYLE_H

#include <QObject>
#include <QCoreApplication>
#include <QTranslator>
#include <QDir>
#include <QFile>
#include <QColor>
#include <QResource>
#include <QTextStream>
#include <QPixmap>
#include <QPainter>

#include <vector>
#include <unordered_map>
#include <shared_mutex>

//
// default xstyle window style
//

#define DEFAULT_XSTYLE_WINDOW_SHADOW_COLOR QColor("black")
#define DEFAULT_XSTYLE_WINDOW_INACTIVE_OVERLAY_COLOR QColor("gray")

#define DEFAULT_XSTYLE_WINDOW_WIDTH 500
#define DEFAULT_XSTYLE_WINDOW_HEIGHT 500

#define DEFAULT_XSTYLE_WINDOW_FADE_OUT_DURATION 300

#define DEFAULT_XSTYLE_WINDOW_TITLE_HEIGHT 50
#define DEFAULT_XSTYLE_WINDOW_FOOTER_HEIGHT 20
#define DEFAULT_XSTYLE_WINDOW_WINDOW_ICON_SIZE 30
#define DEFAULT_XSTYLE_WINDOW_WINDOW_ICON_URL ""
#define DEFAULT_XSTYLE_WINDOW_TITLE_BUTTON_WIDTH 40
#define DEFAULT_XSTYLE_WINDOW_TITLE_BUTTON_HEIGHT 40
#define DEFAULT_XSTYLE_WINDOW_TITLE_BUTTON_BORDER_WIDTH 1
#define DEFAULT_XSTYLE_WINDOW_TITLE_BUTTON_BORDER_COLOR "black"

#define DEFAULT_XSTYLE_WINDOW_TITLE_LABEL_FONT_SIZE 12
#define DEFAULT_XSTYLE_WINDOW_TITLE_LABEL_FONT_WEIGHT "bold"
#define DEFAULT_XSTYLE_WINDOW_TITLE_LABEL_FONT_FAMILY "Microsoft YaHei"
#define DEFAULT_XSTYLE_WINDOW_TITLE_LABEL_FONT_COLOR "rgba(255, 255, 255, 100)"

#define DEFAULT_XSTYLE_WINDOW_RESERVE_BORDER_WIDTH 2
#define DEFAULT_XSTYLE_WINDOW_BORDER_RADIUS 4
#define DEFAULT_XSTYLE_WINDOW_RESERVE_SHADOW_WIDTH 6

#define DEFAULT_XSTYLE_WINDOW_CONTAINER_BACKGROUND "rgb(0, 0, 77)"
#define DEFAULT_XSTYLE_WINDOW_TITLE_BACKGROUND "rgb(31, 39, 100)"
#define DEFAULT_XSTYLE_WINDOW_BODY_CONTAINER_BACKGROUND "rgba(31, 39, 100, 150)"
#define DEFAULT_XSTYLE_WINDOW_FOOTER_BACKGROUND "rgb(31, 39, 100)"

//
// default xstyle button style
//

#define DEFAULT_XSTYLE_BUTTON_NORMAL_COLOR QColor("transparent")
#define DEFAULT_XSTYLE_BUTTON_DISABLED_COLOR QColor("transparent")
#define DEFAULT_XSTYLE_BUTTON_HOVER_COLOR QColor(120, 120, 120, 100)
#define DEFAULT_XSTYLE_BUTTON_DOWN_COLOR QColor(150, 150, 150, 200)

#define DEFAULT_XSTYLE_BUTTON_ICON_NORMAL_OVERLAY_COLOR QColor(255, 255, 255, 100)
#define DEFAULT_XSTYLE_BUTTON_ICON_HOVER_OVERLAY_COLOR QColor(255, 255, 255, 200)
#define DEFAULT_XSTYLE_BUTTON_ICON_DISABLED_OVERLAY_COLOR QColor(0, 0, 0, 100)

constexpr int DEFAULT_XSTYLE_BUTTON_LEFT_TOP_RADIUS     = 0;
constexpr int DEFAULT_XSTYLE_BUTTON_LEFT_BOTTOM_RADIUS  = 0;
constexpr int DEFAULT_XSTYLE_BUTTON_RIGHT_TOP_RADIUS    = 0;
constexpr int DEFAULT_XSTYLE_BUTTON_RIGHT_BOTTOM_RADIUS = 0;

constexpr int DEFAULT_XSTYLE_BUTTON_ANIMATION_DURATION = 200;

#define DEFAULT_XSTYLE_BUTTON_ICON_URL ""

#define STANDARD_XSTYLE_WINDOW_TITLE_MIN_BUTTON_ICON "#qt_standard_style/min_button"
#define STANDARD_XSTYLE_WINDOW_TITLE_MAX_BUTTON_ICON "#qt_standard_style/max_button"
#define STANDARD_XSTYLE_WINDOW_TITLE_CLOSE_BUTTON_ICON "#qt_standard_style/close_button"

#define STANDARD_XSTYLE_MESSAGE_BOX_INFORMATION_BUTTON_ICON "#qt_standard_style/information"
#define STANDARD_XSTYLE_MESSAGE_BOX_WARNING_BUTTON_ICON "#qt_standard_style/warning"
#define STANDARD_XSTYLE_MESSAGE_BOX_ERROR_BUTTON_ICON "#qt_standard_style/error"

//
// xstyle property
//

#define XSTYLE_THEME_QSS_HEADER "// #XStyleWindowTheme#"
#define XSTYLE_DEFAULT_THEME_URL ":/XStyleTheme/DefaultTheme/DefaultTheme.qss"
#define XSTYLE_THEME_QSS_MUST_BE_LESS_THAN_1MB 1048576

namespace xstyle {

    //
    // xstyle class
    //

    class XStyleManager final : public QObject
    {
        Q_OBJECT

      private:
        explicit XStyleManager(QObject* parent = nullptr)
          : QObject(parent)
        {
        }

        ~XStyleManager()
        {
            if (!currentTheme.isEmpty()) {
                UnRegisterThemeResource(currentTheme);
            }
        }

      public:
        static XStyleManager& singleton()
        {
            static XStyleManager _manager(nullptr);
            return _manager;
        }

        //
        // xstyle object register & unregister
        //

        template<typename XStyleObjectType>
        void Register(XStyleObjectType* obj)
        {
            if (!obj) {
                return;
            }

            QObject::connect(this, &XStyleManager::themeChanged, obj, &XStyleObjectType::ThemeChanged, Qt::QueuedConnection);
        }

        template<typename XStyleObjectType>
        void UnRegister(XStyleObjectType* obj)
        {
            if (!obj) {
                return;
            }

            QObject::disconnect(this, &XStyleManager::themeChanged, obj, &XStyleObjectType::ThemeChanged, Qt::QueuedConnection);
        }

        //
        // i18n
        //

        QString                                  CurrentLanguage();
        QString                                  i18nFolder();
        void                                     RegisterI18nFolder(const QString& path);
        std::vector<std::pair<QString, QString>> i18ns();
        void                                     ChangeLanguage(const QString& language);
        QString                                  Translate(const QString& context, const QString& key);

        //
        // theme
        //

        QString              CurrentTheme();
        QString              ThemeFolder();
        std::vector<QString> ThemeList();
        void                 RegisterThemeFolder(const QString& path);
        void                 RegisterDefaultTheme(const QString& themeUrl = "");
        QString              GetDefaultThemeUrl();
        QString              GenerateDefaultStyle(const QString& defaultThemeUrl = "");
        QString              DefaultStyle();
        QString              StyleSheet();
        bool                 ChangeTheme(const QString& themeName);

      private:
        QString LoadTheme(const QString& themeQssUrl);
        QString LoadThemeByName(const QString& themeName);
        bool    RegisterThemeResource(const QString& themeName);
        void    UnRegisterThemeResource(const QString& themeName);

      signals:
        void themeChanged(QString themeName, QString styleSheet);

      private:
        std::shared_mutex mutexXStyleGlobal;

        // i18n
        QString     i18nFolderPath;
        QString     currentLanguage;
        QTranslator translator;

        // theme
        QString                              themeFolderPath;
        QString                              defaultThemeUrl;
        QString                              defaultThemeStyleSheet;
        QString                              currentTheme;
        QString                              currentStyleSheet;
        std::unordered_map<QString, QString> themeResourceMaps;
    };

    //
    // xstyle helper
    //

    QPixmap RotatePixmap(const QPixmap& pixmap, qreal angle);
}

#define xstyle_manager xstyle::XStyleManager::singleton()

#endif  // #ifndef __X_STYLE_WINDOW_H