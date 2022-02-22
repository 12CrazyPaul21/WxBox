#ifndef __WXBOX_CLIENT_STATUS_MODEL_H
#define __WXBOX_CLIENT_STATUS_MODEL_H

#include <QObject>
#include <QStandardItemModel>
#include <QTableView>

#undef signals
#include <utils/common.h>
#define signals Q_SIGNALS

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

    QStandardItem* statusItem;
    QStandardItem* pidItem;

    _WxBoxClientStatusItem()
      : valid(false)
      , status(WxBoxClientItemStatus::Independent)
      , pid(0)
      , statusItem(nullptr)
      , pidItem(nullptr)
    {
    }

    void buildColumns()
    {
        if (valid) {
            return;
        }

        // dont need to free manually
        statusItem = new QStandardItem;
        pidItem    = new QStandardItem(QString("%1").arg(pid));

        // init item
        pidItem->setData((unsigned long long)pid, Qt::UserRole);

        valid = true;
    }

    void update()
    {
        if (!valid) {
            return;
        }

        statusItem->setText(QString("%1").arg((int)status));
    }

    QList<QStandardItem*> columns()
    {
        return QList<QStandardItem*>({statusItem, pidItem});
    }

    static std::shared_ptr<_WxBoxClientStatusItem> New()
    {
        return std::make_shared<_WxBoxClientStatusItem>();
    }

} WxBoxClientStatusItem, *PWxBoxClientStatusItem;

using WxBoxClientStatusItemPtr = std::shared_ptr<WxBoxClientStatusItem>;

class WxBoxClientStatusModel : public QObject
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

  protected:
    QTableView*                                                   modelContainer;
    QStandardItemModel                                            statusItemModel;
    std::unordered_map<wb_process::PID, WxBoxClientStatusItemPtr> items;
};

#endif  // #ifndef __WXBOX_CLIENT_STATUS_MODEL_H