#ifndef __WXBOX_DOWNLOAD_DIALOG_H
#define __WXBOX_DOWNLOAD_DIALOG_H

#include <tuple>
#include <fstream>

#include <utils/common.h>

#if WXBOX_IN_WINDOWS_OS
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

#include <xstyle/xstylewindow.h>
#include <xstyle/xstylemessagebox.h>

#include <internal/downloader.hpp>

namespace Ui {
    class DownloadDialog;
}

enum class FileDownloadStatus
{
    Success = 0,
    Canceled,
    CreateFolderFailed,
    PersistentFailed,
    NetworkError
};

class DownloadDialog final : public XStyleWindow
{
    friend class MainWindow;

    Q_OBJECT

  public:
    explicit DownloadDialog(QWidget* parent = nullptr, bool deleteWhenClose = false);
    ~DownloadDialog();

    void SetStatus(const QString& status);
    void SetProgress(qint64 progress, qint64 total);

    void beginMission();
    void closeMission();

    std::tuple<bool, QByteArray> get(const QUrl& url);
    FileDownloadStatus           download(const std::string& sinkFolder, const std::vector<std::pair<std::string, std::string>>& fileList);

  protected:
    void RetranslateUi();

  public slots:
    void cancel();

  private:
    Ui::DownloadDialog*              ui;
    wxbox::internal::WxBoxDownloader downloader;
#if WXBOX_IN_WINDOWS_OS
    QWinTaskbarButton    taskbarButton;
    QWinTaskbarProgress* taskbarProgress;
#endif

    std::mutex        mutex;
    std::atomic<bool> isCancel;
};

#endif  // #ifndef __WXBOX_DOWNLOAD_DIALOG_H