#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QTranslator>
#include <QCloseEvent>
#include <QAtomicInteger>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>

#include "about.h"
#include "wxbox_server.hpp"

namespace Ui {
    class MainWindow;
}

using wxbox::WxBoxServerStatus;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    //
    // Constructor
    //

    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    //
    // Events
    //

    virtual void changeEvent(QEvent* event) override;
    virtual void showEvent(QShowEvent* event) override;
    virtual void closeEvent(QCloseEvent* event) override;

    //
    // Public Methods
    //

    void changeLanguage(const std::string& language);
    void startWxBoxServer();
    void stopWxBoxServer();

  private:
    //
    // Private Methods
    //

    void ignoreForClose();
    void readyForClose(const bool canCloseWhenZero = true);

  signals:
    void WxBoxServerEvent();

  public slots:
    void InitWxBox();
    void DeinitWxBox();
    void OpenAboutDialog();
    void WxBoxServerStatusChange(const WxBoxServerStatus oldStatus, const WxBoxServerStatus newStatus);
    void clickTest();

  private:
    bool                    init;
    bool                    wantToClose;
    QAtomicInteger<int32_t> readyForCloseCounter;

    Ui::MainWindow*          ui;
    QTranslator              translator;
    AboutWxBoxDialog         aboutDialog;
    wxbox::WxBoxServerWorker worker;
};

#endif  // MAINWINDOW_H
