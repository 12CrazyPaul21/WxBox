#include <xstyle/xstylemenu.h>

XStyleMenu::XStyleMenu(QWidget* parent)
  : XStyleMenu("", parent)
{
}

XStyleMenu::XStyleMenu(const char* title, QWidget* parent)
  : XStyleMenu(title, title, parent)
{
}

XStyleMenu::XStyleMenu(const char* rootTitle, const char* title, QWidget* parent)
  : QMenu(xstyle_manager.Translate(rootTitle, title), parent)
  , rootTitle(rootTitle)
{
}

XStyleMenu::~XStyleMenu()
{
}

void XStyleMenu::changeEvent(QEvent* event)
{
    switch (event->type()) {
        case QEvent::LanguageChange:
            retranslateUi();
            break;
        default:
            QMenu::changeEvent(event);
    }
}

void XStyleMenu::retranslateUi()
{
    if (rootTitle.isEmpty()) {
        return;
    }

    for (auto action : actions) {
        action.second->setText(xstyle_manager.Translate(rootTitle, action.first));
    }

    for (auto subMenu : subMenus) {
        subMenu.second->setTitle(xstyle_manager.Translate(rootTitle, subMenu.first));
    }
}

QAction* XStyleMenu::popup(const QPoint& pos)
{
    return exec(pos);
}

XStyleMenu& XStyleMenu::operator[](const char* key)
{
    if (subMenus.find(key) == subMenus.end()) {
        subMenus[key] = new XStyleMenu(rootTitle.toStdString().c_str(), key, this);
        addMenu(subMenus[key]);
    }
    return *subMenus[key];
}

QAction& XStyleMenu::action(const char* name)
{
    if (actions.find(name) == actions.end()) {
        actions[name] = new QAction(xstyle_manager.Translate(rootTitle, name).toStdString().c_str(), this);
        addAction(actions[name]);
    }
    return *actions[name];
}

QAction& XStyleMenu::pushSeparator()
{
    return *addSeparator();
}

QMenu& XStyleMenu::pushMenu(const char* title)
{
    if (subMenus.find(title) != subMenus.end()) {
        return *subMenus[title];
    }

    subMenus[title] = new XStyleMenu(rootTitle.toStdString().c_str(), title, this);
    addMenu(subMenus[title]);
    return *subMenus[title];
}

QAction& XStyleMenu::pushAction(const char* name, QObject* receiver, XStyleMenuHandler handler, const QString& iconUrl, const QKeySequence& shortcut)
{
    QAction* action = nullptr;

    if (actions.find(name) != actions.end()) {
        action = actions[name];
    }
    else {
        action        = new QAction(xstyle_manager.Translate(rootTitle, name).toStdString().c_str(), this);
        actions[name] = action;
        addAction(action);
    }

    if (!shortcut.isEmpty()) {
        action->setShortcut(shortcut);
    }

    if (!iconUrl.isEmpty()) {
        action->setIcon(QIcon(iconUrl));
    }

    if (receiver && handler) {
        QObject::connect(action, &QAction::triggered, receiver, handler);
    }

    return *action;
}