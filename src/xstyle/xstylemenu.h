#ifndef __X_STYLE_MENU_H
#define __X_STYLE_MENU_H

#include <QObject>
#include <QMenu>

#include <unordered_map>

#include <xstyle/xstyle.h>

//
// XStyleMenu
//

class XStyleMenu : public QMenu
{
    Q_OBJECT

    using XStyleMenuHandler = std::function<void()>;

  public:
    explicit XStyleMenu(QWidget* parent = nullptr);
    explicit XStyleMenu(const char* title, QWidget* parent = nullptr);
    explicit XStyleMenu(const char* rootTitle, const char* title, QWidget* parent = nullptr);
    ~XStyleMenu();

    virtual void changeEvent(QEvent* event) override;

    QAction* popup(const QPoint& pos);
    void     retranslateUi();

    // sub menu visitor
    XStyleMenu& operator[](const char* key);

    // action visitor
    QAction& action(const char* name);

    // connect
    bool connectAction(const char* name, QObject* receiver, XStyleMenuHandler handler);

    QAction& pushSeparator();
    QMenu&   pushMenu(const char* title);
    QAction& pushAction(const char* name, QObject* receiver = nullptr, XStyleMenuHandler handler = nullptr, const QString& iconUrl = "", const QKeySequence& shortcut = QKeySequence());

    // change action status
    void show(const char* name);
    void hide(const char* name);
    void disable(const char* name);
    void enable(const char* name);

  protected:
    QString                                  rootTitle;
    std::unordered_map<QString, QAction*>    actions;
    std::unordered_map<QString, XStyleMenu*> subMenus;
};

#endif  // #ifndef __X_STYLE_MENU_H