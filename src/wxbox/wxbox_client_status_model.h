#ifndef __WXBOX_CLIENT_STATUS_MODEL_H
#define __WXBOX_CLIENT_STATUS_MODEL_H

#include <QObject>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTableView>

#undef signals
#include <utils/common.h>
#define signals Q_SIGNALS

#include <xstyle/xstyle.h>

#define WxBoxClientStatusHeader                                      \
    {                                                                \
        "Status", "PID", "Login", "NickName", "WeChatNumber", "WXID" \
    }

#define DEFAULT_WXBOX_CLIENT_MODEL_STATUS_ICON_URLS ":/XStyleTheme/DefaultTheme/image/independent_status.png|:/XStyleTheme/DefaultTheme/image/injected_status.png|:/XStyleTheme/DefaultTheme/image/normal_status.png"
#define DEFAULT_WXBOX_CLIENT_MODEL_LOGIN_STATUS_ICON_URLS ":/XStyleTheme/DefaultTheme/image/disabled_login_status.png|:/XStyleTheme/DefaultTheme/image/logined_status.png|:/XStyleTheme/DefaultTheme/image/not_logined_status.png"

enum class WxBoxClientItemStatus
{
    Independent,
    Injected,
    Normal
};

typedef struct _WxBoxClientStatusItem
{
    bool                  valid;
    WxBoxClientItemStatus status;
    wb_process::PID       pid;
    bool                  logined;
    QString               nickname;
    QString               wxnumber;
    QString               wxid;

    QStandardItem* statusItem;
    QStandardItem* pidItem;
    QStandardItem* loginedItem;
    QStandardItem* nicknameItem;
    QStandardItem* wxnumberItem;
    QStandardItem* wxidItem;

    _WxBoxClientStatusItem()
      : valid(false)
      , status(WxBoxClientItemStatus::Independent)
      , pid(0)
      , logined(false)
      , statusItem(nullptr)
      , pidItem(nullptr)
      , loginedItem(nullptr)
      , nicknameItem(nullptr)
      , wxnumberItem(nullptr)
      , wxidItem(nullptr)
    {
    }

    void buildColumns()
    {
        if (valid) {
            return;
        }

        // dont need to free manually
        statusItem   = new QStandardItem();
        pidItem      = new QStandardItem(QString("%1").arg(pid));
        loginedItem  = new QStandardItem;
        nicknameItem = new QStandardItem;
        wxnumberItem = new QStandardItem;
        wxidItem     = new QStandardItem;

        // init item
        pidItem->setData((unsigned long long)pid, Qt::UserRole);
        pidItem->setToolTip(pidItem->text());
        pidItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setData((int)status, Qt::UserRole);
        loginedItem->setData(logined, Qt::UserRole);

        valid = true;
    }

    void update()
    {
        if (!valid) {
            return;
        }

        statusItem->setData((int)status, Qt::UserRole);
        loginedItem->setData(logined, Qt::UserRole);

        switch (status) {
            case WxBoxClientItemStatus::Independent:
                statusItem->setToolTip(xstyle_manager.Translate("xstyle_meta", "Not Injected"));
                break;
            case WxBoxClientItemStatus::Injected:
                statusItem->setToolTip(xstyle_manager.Translate("xstyle_meta", "Injected"));
                break;
            case WxBoxClientItemStatus::Normal:
                statusItem->setToolTip(xstyle_manager.Translate("xstyle_meta", "Connected WxBot"));
                break;
        }

        if (status != WxBoxClientItemStatus::Normal) {
            loginedItem->setToolTip("");

            nicknameItem->setText("");
            nicknameItem->setToolTip("");

            wxnumberItem->setText("");
            wxnumberItem->setToolTip("");

            wxidItem->setText("");
            wxidItem->setToolTip("");
        }
        else {
            loginedItem->setToolTip(xstyle_manager.Translate("xstyle_meta", logined ? "Logged-In" : "Not Logged-In"));

            nicknameItem->setText(nickname);
            nicknameItem->setToolTip(nickname);

            wxnumberItem->setText(wxnumber);
            wxnumberItem->setToolTip(wxnumber);

            wxidItem->setText(wxid);
            wxidItem->setToolTip(wxid);
        }
    }

    QList<QStandardItem*> columns()
    {
        return QList<QStandardItem*>({statusItem, pidItem, loginedItem, nicknameItem, wxnumberItem, wxidItem});
    }

    static std::shared_ptr<_WxBoxClientStatusItem> New()
    {
        return std::make_shared<_WxBoxClientStatusItem>();
    }

} WxBoxClientStatusItem, *PWxBoxClientStatusItem;

using WxBoxClientStatusItemPtr = std::shared_ptr<WxBoxClientStatusItem>;

class WxBoxClientStatusModel : public QStyledItemDelegate
{
    Q_OBJECT

  public:
    WxBoxClientStatusModel(QObject* parent = nullptr);

    inline QTableView* container() const
    {
        return modelContainer;
    }

    inline void setContainer(QTableView* p)
    {
        modelContainer = p;

        if (p) {
            p->setItemDelegateForColumn(0, this);
            p->setItemDelegateForColumn(2, this);
        }
    }

    inline QStandardItemModel& model() noexcept
    {
        return statusItemModel;
    }

    inline bool count() const noexcept
    {
        return items.size();
    }

    inline bool exist(wb_process::PID pid) const noexcept
    {
        return items.find(pid) != items.end();
    }

    inline WxBoxClientStatusItemPtr get(wb_process::PID pid) noexcept
    {
        auto it = items.find(pid);
        if (it == items.end()) {
            return nullptr;
        }
        return it->second;
    }

    inline wb_process::PID getPid(int row) noexcept
    {
        auto itemModel = statusItemModel.item(row, 1);
        if (!itemModel) {
            return 0;
        }

        auto itemData = itemModel->data(Qt::UserRole);
        if (itemData.isNull()) {
            return 0;
        }

        return itemData.toULongLong();
    }

    inline std::unordered_map<wb_process::PID, WxBoxClientStatusItemPtr>& all()
    {
        return items;
    }

    wb_process::PID           selection();
    std::set<wb_process::PID> selections();

    bool add(WxBoxClientStatusItemPtr item);
    void remove(wb_process::PID pid);
    void removes(const std::set<wb_process::PID>& pids);
    void clear();

    void resize(bool inited = true);
    void applyTheme(const QString& statusIconUrls, const QString& loginStatusIconUrls);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  protected:
    QTableView*                                                   modelContainer;
    QStandardItemModel                                            statusItemModel;
    QIcon                                                         statusIcons;
    QIcon                                                         loginStatusIcons;
    std::unordered_map<wb_process::PID, WxBoxClientStatusItemPtr> items;
};

#endif  // #ifndef __WXBOX_CLIENT_STATUS_MODEL_H