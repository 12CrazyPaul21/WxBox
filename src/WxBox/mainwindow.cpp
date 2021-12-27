#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , aboutDialog(this)
  , ui(new Ui::MainWindow)
{
    changeLanguage("zh_cn");
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::OpenAboutDialog()
{
    aboutDialog.show();
}

void MainWindow::changeEvent(QEvent* event)
{
    switch (event->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            QWidget::changeEvent(event);
    }
}

void MainWindow::changeLanguage(const std::string& language)
{
	// zh_cn or en
    if (translator.load(QString().asprintf(":/translations/%s.qm", language.c_str()))) {
        qApp->installTranslator(&translator);
	}
}