#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , aboutDialog(this)
  , ui(new Ui::MainWindow)
  , init(false)
  , wantToClose(false)
  , readyForCloseCounter(0)
{
    changeLanguage("zh_cn");
    ui->setupUi(this);

    // ---------------------------------------
    //             click to test
    // ---------------------------------------

    auto button = new QPushButton("test", ui->centralWidget);
    connect(button, SIGNAL(clicked()), this, SLOT(clickTest()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

//
// Events
//

void MainWindow::changeEvent(QEvent* event)
{
    switch (event->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            QMainWindow::changeEvent(event);
    }
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    if (!init) {
        init = true;
        QTimer::singleShot(10, this, SLOT(InitWxBox()));
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (init) {
        emit DeinitWxBox();
        init = false;
    }

    wantToClose = true;
    readyForCloseCounter <= 0 ? event->accept() : event->ignore();
}

//
// Slots
//

void MainWindow::InitWxBox()
{
    startWxBoxServer();
}

void MainWindow::DeinitWxBox()
{
    stopWxBoxServer();
}

void MainWindow::OpenAboutDialog()
{
    aboutDialog.show();
}

void MainWindow::WxBoxServerStatusChange(const WxBoxServerStatus oldStatus, const WxBoxServerStatus newStatus)
{
    switch (newStatus) {
        case WxBoxServerStatus::Started:
            break;
        case WxBoxServerStatus::StartServiceFailed:
            readyForClose(false);
            QTimer::singleShot(100, this, [&]() {
                QMessageBox::information(this, "Warning!!!", "start WxBoxServer failed, close application now...");
                emit this->close();
            });
            break;
        case WxBoxServerStatus::Interrupted:
        case WxBoxServerStatus::Stopped:
            readyForClose();
            break;
    }
}

//
// Methods
//

void MainWindow::changeLanguage(const std::string& language)
{
    // zh_cn or en
    if (translator.load(QString().asprintf(":/translations/%s.qm", language.c_str()))) {
        qApp->installTranslator(&translator);
    }
}

void MainWindow::ignoreForClose()
{
    readyForCloseCounter.ref();
}

void MainWindow::readyForClose(const bool canCloseWhenZero)
{
    readyForCloseCounter.deref();
    if (wantToClose && readyForCloseCounter <= 0 && canCloseWhenZero) {
        emit this->close();
    }
}

void MainWindow::startWxBoxServer()
{
    if (worker.isRunning()) {
        return;
    }

    wxbox::WxBoxServer* server = wxbox::WxBoxServer::NewWxBoxServer();

    // connect slots
    QObject::connect(&worker, SIGNAL(finished()), server, SLOT(deleteLater()));
    QObject::connect(&worker, SIGNAL(shutdown()), server, SLOT(shutdown()), Qt::DirectConnection);
    QObject::connect(server, &wxbox::WxBoxServer::WxBoxServerStatusChange, this, &MainWindow::WxBoxServerStatusChange, Qt::QueuedConnection);
    QObject::connect(this, &MainWindow::WxBoxServerEvent, server, &wxbox::WxBoxServer::WxBoxServerEvent, Qt::QueuedConnection);

    // start WxBoxServer
    worker.startServer(server);

    // prevent the window from being closed before the service ends
    ignoreForClose();
}

void MainWindow::stopWxBoxServer()
{
    if (worker.isRunning()) {
        worker.stopServer();
    }
}

// ---------------------------------------
//             click to test
// ---------------------------------------

void MainWindow::clickTest()
{
    //emit WxBoxServerEvent();
    stopWxBoxServer();
    //startWxBoxServer();
}