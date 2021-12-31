#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QTranslator>

#include "about.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

	virtual void changeEvent(QEvent* event) override;

	void changeLanguage(const std::string& language);

private:
    Ui::MainWindow*  ui;
    QTranslator      translator;
    AboutWxBoxDialog aboutDialog;

public slots:
    void OpenAboutDialog();
};

#endif  // MAINWINDOW_H
