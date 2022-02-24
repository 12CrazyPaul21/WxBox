#include <wxbox_client_status_model.h>

#include <QHeaderView>

WxBoxClientStatusModel::WxBoxClientStatusModel(QObject* parent)
  : QStyledItemDelegate(parent)
  , modelContainer(nullptr)
  , statusItemModel(this)
{
    setObjectName("WxBoxClientStatusModel");
}

wb_process::PID WxBoxClientStatusModel::selection()
{
    if (!modelContainer) {
        return 0;
    }

    for (const auto& item : modelContainer->selectionModel()->selectedRows()) {
        auto pid = getPid(item.row());
        if (pid) {
            return pid;
        }
    }

    return 0;
}

std::set<wb_process::PID> WxBoxClientStatusModel::selections()
{
    std::set<wb_process::PID> result;

    if (!modelContainer) {
        return result;
    }

    for (const auto& item : modelContainer->selectionModel()->selectedRows()) {
        auto pid = getPid(item.row());
        if (pid) {
            result.insert(pid);
        }
    }

    return result;
}

bool WxBoxClientStatusModel::add(WxBoxClientStatusItemPtr item)
{
    if (!item || items.find(item->pid) != items.end()) {
        return false;
    }

    item->buildColumns();
    item->update();
    statusItemModel.appendRow(item->columns());

    items.emplace(item->pid, std::move(item));
    return true;
}

void WxBoxClientStatusModel::remove(wb_process::PID pid)
{
    auto it = items.find(pid);
    if (it == items.end()) {
        return;
    }

    auto index = statusItemModel.indexFromItem(it->second->statusItem);
    if (!index.isValid()) {
        items.erase(it);
        return;
    }

    statusItemModel.removeRow(index.row());
    items.erase(it);
}

void WxBoxClientStatusModel::removes(const std::set<wb_process::PID>& pids)
{
    for (auto pid : pids) {
        remove(pid);
    }
}

void WxBoxClientStatusModel::clear()
{
    statusItemModel.clear();
    items.clear();
}

void WxBoxClientStatusModel::resize(bool inited)
{
    if (!modelContainer) {
        return;
    }

    auto header = modelContainer->horizontalHeader();
    if (!header) {
        return;
    }

    header->resizeSections(QHeaderView::ResizeMode::ResizeToContents);

    if (inited) {
        header->resizeSection(1, header->sectionSize(1) + 30);
        header->resizeSection(3, header->sectionSize(3) + 60);
        header->resizeSection(4, header->sectionSize(4) + 80);
        header->setSectionResizeMode(5, QHeaderView::Stretch);
    }
}

void WxBoxClientStatusModel::applyTheme(const QString& statusIconUrls, const QString& loginStatusIconUrls)
{
    if (statusIconUrls.isEmpty() || loginStatusIconUrls.isEmpty()) {
        return;
    }

    auto statusIconUrlSplited      = statusIconUrls.split('|');
    auto loginStatusIconUrlSplited = loginStatusIconUrls.split('|');

    if (statusIconUrlSplited.count() != 3 || loginStatusIconUrlSplited.count() != 3) {
        return;
    }

    //
    // status
    //

    statusIcons      = QIcon();
    loginStatusIcons = QIcon();

    // Independent
    if (QFile(statusIconUrlSplited[0]).exists()) {
        statusIcons.addPixmap(QPixmap(statusIconUrlSplited[0]), QIcon::Disabled, QIcon::On);
    }

    // Injected
    if (QFile(statusIconUrlSplited[1]).exists()) {
        statusIcons.addPixmap(QPixmap(statusIconUrlSplited[1]), QIcon::Active, QIcon::On);
    }

    // Normal
    if (QFile(statusIconUrlSplited[2]).exists()) {
        statusIcons.addPixmap(QPixmap(statusIconUrlSplited[2]), QIcon::Normal, QIcon::On);
    }

    //
    // login status
    //

    // Not Connected
    if (QFile(loginStatusIconUrlSplited[0]).exists()) {
        loginStatusIcons.addPixmap(QPixmap(loginStatusIconUrlSplited[0]), QIcon::Disabled, QIcon::On);
    }

    // Logged-In
    if (QFile(loginStatusIconUrlSplited[1]).exists()) {
        loginStatusIcons.addPixmap(QPixmap(loginStatusIconUrlSplited[1]), QIcon::Normal, QIcon::On);
    }

    // Not Logged-In
    if (QFile(loginStatusIconUrlSplited[2]).exists()) {
        loginStatusIcons.addPixmap(QPixmap(loginStatusIconUrlSplited[2]), QIcon::Active, QIcon::On);
    }
}

void WxBoxClientStatusModel::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    auto item = statusItemModel.itemFromIndex(index);
    if (!item) {
        return;
    }

    auto itemData = item->data(Qt::UserRole);
    if (itemData.isNull()) {
        return;
    }

    //
    // calc paint area rect
    //

    QRectF rect;
    auto   size     = (qMin(option.rect.width(), option.rect.height()) / 2.0f);
    auto   qsize    = QSize(size, size);
    auto   halfSize = size / 2.0f;
    auto   deltaX   = option.rect.width() / 2.0f - halfSize;
    auto   deltaY   = option.rect.height() / 2.0f - halfSize;
    rect.setX(option.rect.x() + deltaX);
    rect.setY(option.rect.y() + deltaY);
    rect.setSize(qsize);

    //
    // select icon pixmap
    //

    QPixmap pixmap;

    if (index.column() == 0) {
        //
        // status
        //

        switch ((WxBoxClientItemStatus)itemData.toInt()) {
            case WxBoxClientItemStatus::Independent:
                pixmap = statusIcons.pixmap(qsize, QIcon::Disabled, QIcon::On);
                break;
            case WxBoxClientItemStatus::Injected:
                pixmap = statusIcons.pixmap(qsize, QIcon::Active, QIcon::On);
                break;
            case WxBoxClientItemStatus::Normal:
                pixmap = statusIcons.pixmap(qsize, QIcon::Normal, QIcon::On);
                break;
        }
    }
    else if (index.column() == 2) {
        //
        // login status
        //

        if (item->toolTip().isEmpty()) {
            pixmap = loginStatusIcons.pixmap(qsize, QIcon::Disabled, QIcon::On);
        }
        else {
            pixmap = loginStatusIcons.pixmap(qsize, itemData.toBool() ? QIcon::Normal : QIcon::Active, QIcon::On);
        }
    }
    else {
        return;
    }

    //
    // paint status icon
    //

    if (!pixmap.isNull()) {
        painter->save();
        painter->drawPixmap(rect.x(), rect.y(), rect.width(), rect.height(), pixmap);
        painter->restore();
    }
}