#ifndef __WXBOX_INTERNAL_DOWNLOADER_H
#define __WXBOX_INTERNAL_DOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <functional>
#include <atomic>
#include <deque>
#include <tuple>
#include <mutex>

namespace wxbox {
    namespace internal {

        using WxBoxDownloadProgress = std::function<void(const QUrl& url, qint64 progress, qint64 total)>;
        using WxBoxDownloadSuccess  = std::function<void(const QUrl& url, const QByteArray& bytes)>;
        using WxBoxDownloadError    = std::function<void(const QUrl& url, const QNetworkReply::NetworkError& error, const QString& errorString)>;

        class WxBoxDownloader : public QObject
        {
            Q_OBJECT

          public:
            explicit WxBoxDownloader(QObject* parent = nullptr)
              : QObject(parent)
              , running(false)
            {
            }

            void download(const QUrl& url, WxBoxDownloadProgress progressCallback, WxBoxDownloadSuccess successCallback, WxBoxDownloadError errorCallback)
            {
                bool alreadyRunning = false;
                running.compare_exchange_strong(alreadyRunning, true);

                if (alreadyRunning) {
                    inqueue(url, progressCallback, successCallback, errorCallback);
                    return;
                }

                doDownload(url, progressCallback, successCallback, errorCallback);
            }

            void cancel()
            {
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    queue.clear();
                }
                emit triggerCancel();
            }

            bool isRunning()
            {
                return running;
            }

          private:
            inline void reset()
            {
                running.store(false);
            }

            void inqueue(const QUrl& url, WxBoxDownloadProgress progressCallback, WxBoxDownloadSuccess successCallback, WxBoxDownloadError errorCallback)
            {
                std::lock_guard<std::mutex> lock(mutex);
                queue.push_back(std::make_tuple(url, progressCallback, successCallback, errorCallback));
            }

            void rolling()
            {
                decltype(queue)::value_type item;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    if (queue.empty()) {
                        reset();
                        return;
                    }

                    item = std::move(queue.front());
                    queue.pop_front();
                }
                doDownload(std::get<0>(item), std::get<1>(item), std::get<2>(item), std::get<3>(item));
            }

            void doDownload(const QUrl& url, WxBoxDownloadProgress progressCallback, WxBoxDownloadSuccess successCallback, WxBoxDownloadError errorCallback)
            {
                // check whether scheme is supported
                QNetworkAccessManager* networkManager = new QNetworkAccessManager();
                // if (!networkManager->supportedSchemes().contains(url.scheme())) {
                //     delete networkManager;
                //     return false;
                // }

                // new request
                QNetworkRequest   request(url);
                QSslConfiguration sslConfig = request.sslConfiguration();
                sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
                sslConfig.setProtocol(QSsl::AnyProtocol);
                request.setSslConfiguration(sslConfig);

                // execute get request
                QNetworkReply* reply = networkManager->get(request);

                //
                // connect slot
                //

                QObject::connect(reply, &QNetworkReply::downloadProgress, this, [url, progressCallback](qint64 progress, qint64 total) {
                    if (progressCallback) {
                        progressCallback(url, progress, total >= 0 ? total : progress);
                    }
                });

                // QObject::connect(reply, &QIODevice::readyRead, receiver, [reply]() {
                //
                // });

                QObject::connect(reply, &QNetworkReply::finished, [this, reply, networkManager, url, progressCallback, successCallback, errorCallback]() {
                    if (reply->error() == QNetworkReply::NoError) {
                        // check whether it's redirect
                        QUrl redirectUrl = reply->attribute(QNetworkRequest::Attribute::RedirectionTargetAttribute).toUrl();
                        if (!redirectUrl.isEmpty()) {
                            reply->deleteLater();
                            networkManager->deleteLater();
                            doDownload(redirectUrl, progressCallback, successCallback, errorCallback);
                            return;
                        }

                        if (successCallback) {
                            successCallback(url, reply->readAll());
                        }
                    }
                    else {
                        if (errorCallback) {
                            errorCallback(url, reply->error(), reply->errorString());
                        }
                    }

                    reply->deleteLater();
                    networkManager->deleteLater();
                    rolling();
                });

                QObject::connect(this, &WxBoxDownloader::triggerCancel, reply, &QNetworkReply::abort);
            }

          signals:
            void triggerCancel();

          private:
            std::mutex                                                                                    mutex;
            std::atomic<bool>                                                                             running;
            std::deque<std::tuple<QUrl, WxBoxDownloadProgress, WxBoxDownloadSuccess, WxBoxDownloadError>> queue;
        };
    }
}

#endif  // #ifndef __WXBOX_INTERNAL_DOWNLOADER_H