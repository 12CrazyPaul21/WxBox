#include <wxbox_client_status_model.h>

WxBoxClientStatusModel::WxBoxClientStatusModel(QObject* parent)
  : QObject(parent)
  , modelContainer(nullptr)
  , statusItemModel(this)
{
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