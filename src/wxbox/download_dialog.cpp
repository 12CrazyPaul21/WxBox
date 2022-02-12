#include <download_dialog.h>
#include "ui_download.h"

#define WXBOX_DOWNLOAD_DIALOG_NAME "DownloadDialog"
#define WXBOX_DOWNLOAD_DIALOG_TITLE ""

DownloadDialog::DownloadDialog(QWidget* parent, bool deleteWhenClose)
  : XStyleWindow(WXBOX_DOWNLOAD_DIALOG_NAME, parent, deleteWhenClose)
  , ui(new Ui::DownloadDialog)
#if WXBOX_IN_WINDOWS_OS
  , taskbarButton(this)
  , taskbarProgress(nullptr)
#endif
  , downloader(this)
  , isCancel(false)
{
    SetupXStyleUi(ui);
    SetWindowTitle(Translate(WXBOX_DOWNLOAD_DIALOG_TITLE));

    // ui
    ui->labelStatus->setObjectName(QString::fromUtf8("DownloadDialog_labelStatus"));
    SetCloseButtonEnabled(false);
#if WXBOX_IN_WINDOWS_OS
    taskbarButton.setWindow(windowHandle());
    taskbarProgress = taskbarButton.progress();
    taskbarProgress->setVisible(true);
#endif

    // event
    QObject::connect(ui->btnCancel, SIGNAL(clicked()), this, SLOT(cancel()));
}

DownloadDialog::~DownloadDialog()
{
    delete ui;
}

void DownloadDialog::RetranslateUi()
{
    XStyleWindow::RetranslateUi();

    if (ui) {
        ui->retranslateUi(this);
        SetWindowTitle(Translate(WXBOX_DOWNLOAD_DIALOG_TITLE));
    }
}

void DownloadDialog::SetStatus(const QString& status)
{
    ui->labelStatus->setText(status);
}

void DownloadDialog::SetProgress(qint64 progress, qint64 total)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(progress);
#if WXBOX_IN_WINDOWS_OS
    taskbarProgress->setMaximum(total);
    taskbarProgress->setValue(progress);
#endif
}

void DownloadDialog::beginMission()
{
    SetProgress(0, 0);
    showApplicationModal();
}

void DownloadDialog::closeMission()
{
    cancel();
    close();
}

void DownloadDialog::cancel()
{
    isCancel.store(true);
    downloader.cancel();
}

std::tuple<bool, QByteArray> DownloadDialog::get(const QUrl& url)
{
    std::lock_guard<std::mutex> lock(mutex);

    isCancel.store(false);
    SetProgress(0, 0);

    QEventLoop                   loop;
    std::tuple<bool, QByteArray> result;

    QTimer::singleShot(10, [&]() {
        downloader.download(
            url,
            [this](const QUrl& url, qint64 progress, qint64 total) {
                Q_UNUSED(url);

                ui->progressBar->setValue(progress);
                ui->progressBar->setMaximum(total < 0 ? 0 : total);
            },
            [&](const QUrl& url, const QByteArray& bytes) {
                Q_UNUSED(url);

                std::get<1>(result) = bytes;
                loop.exit(0);
            },
            [&](const QUrl& url, const QNetworkReply::NetworkError& error, const QString& errorString) -> bool {
                Q_UNUSED(url);

                if (error == QNetworkReply::NetworkError::OperationCanceledError) {
                    loop.exit(1);
                    return false;
                }

                if (xstyle::error(this, Translate("Download Failed"), errorString, XStyleMessageBoxButtonType::RetrySkip) == XStyleMessageBoxButton::Retry) {
                    return true;
                }

                loop.exit(1);
                return false;
            });
    });

    std::get<0>(result) = !loop.exec();
    return result;
}

FileDownloadStatus DownloadDialog::download(const std::string& sinkFolder, const std::vector<std::pair<std::string, std::string>>& fileList)
{
    std::lock_guard<std::mutex> lock(mutex);

    isCancel.store(false);
    SetProgress(0, 0);

    QDir extraPath(QString::fromLocal8Bit(sinkFolder.c_str()));
    if (!extraPath.exists() && !extraPath.mkpath(".")) {
        xstyle::error(this, Translate("Create Folder Failed"), extraPath.absolutePath());
        return FileDownloadStatus::CreateFolderFailed;
    }

    FileDownloadStatus lastError = FileDownloadStatus::Success;

    for (auto item : fileList) {
        std::string filename = item.first;

        downloader.download(
            QUrl(item.second.c_str()),
            [this, filename](const QUrl& url, qint64 progress, qint64 total) {
                Q_UNUSED(url);

                ui->labelStatus->setText(Translate("Downloading : ") + filename.c_str());
                SetProgress(progress, total < 0 ? 0 : total);
            },
            [&, filename](const QUrl& url, const QByteArray& bytes) {
                Q_UNUSED(url);

                std::ofstream stream(wb_file::JoinPath(sinkFolder, filename), std::ios::out | std::ios::trunc | std::ios::binary);
                if (!stream.is_open()) {
                    lastError = FileDownloadStatus::PersistentFailed;
                    xstyle::error(this, Translate("Save File Failed"), filename.c_str());
                    return;
                }

                stream.write(bytes, bytes.length());
                stream.flush();
                stream.close();
            },
            [&](const QUrl& url, const QNetworkReply::NetworkError& error, const QString& errorString) -> bool {
                Q_UNUSED(url);

                if (error == QNetworkReply::NetworkError::OperationCanceledError) {
                    return false;
                }

                if (xstyle::error(this, Translate("Download Failed"), errorString, XStyleMessageBoxButtonType::RetrySkip) == XStyleMessageBoxButton::Retry) {
                    return true;
                }

                lastError = FileDownloadStatus::NetworkError;
                return false;
            });
    }

    while (downloader.isRunning()) {
        QCoreApplication::processEvents();
        QThread::msleep(10);
    }

    if (isCancel.load()) {
        lastError = FileDownloadStatus::Canceled;
    }

    return lastError;
}