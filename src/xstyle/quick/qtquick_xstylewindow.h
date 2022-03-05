#ifndef __X_QTQUICK_STYLE_WINDOW_H
#define __X_QTQUICK_STYLE_WINDOW_H

#include <QtQuickControls2>
#include <QtQuickWidgets/QQuickWidget>
#include <QQuickView>
#include <QQuickWindow>

#include <xstyle/xstylewindow.h>

#define XSTYLE_QUICK_HOST_OBJECT_NAME "xwin"
#define DEFAULT_XSTYLE_QUICK_PLUGIN_URL "qrc:/plugin"

//
// XStyleQuickWindow
//

class XStyleQuickObject : public QObject
{
    friend class XStyleQuickWindow;

    Q_OBJECT

  public:
    explicit XStyleQuickObject(QObject* parent = nullptr)
      : QObject(parent)
    {
    }

    Q_INVOKABLE std::vector<std::pair<QString, QString>> i18ns() const
    {
        return xstyle_manager.i18ns();
    }

    Q_INVOKABLE void ChangeLanguage(const QString& language) const
    {
        xstyle_manager.ChangeLanguage(language);
    }

    Q_INVOKABLE std::vector<QString> GetThemeList() const
    {
        return xstyle_manager.ThemeList();
    }

    Q_INVOKABLE bool ChangeTheme(const QString& themeName) const
    {
        return xstyle_manager.ChangeTheme(themeName);
    }

  signals:
    void languageChanged();
};

class XStyleQuickWindow : public XStyleWindow
{
    friend class XStyleQuickObject;

    Q_OBJECT

  public:
    explicit XStyleQuickWindow(const QString& name, QWidget* parent = nullptr, bool deleteWhenClose = false);
    ~XStyleQuickWindow();

    Q_INVOKABLE inline QString GetPluginPath() const
    {
        return pluginPath;
    }

    Q_INVOKABLE inline void RegisterPluginPath(const QString& path)
    {
        pluginPath = path;
    }

  protected:
    void SetupXStyleQuick(QQuickWidget* quickWidget, const QString& sourceUrl);

    virtual void RetranslateUi() override
    {
        emit quickObject.languageChanged();
    }

  protected:
    QString           pluginPath;
    XStyleQuickObject quickObject;
};

#endif  // __X_QTQUICK_STYLE_WINDOW_H