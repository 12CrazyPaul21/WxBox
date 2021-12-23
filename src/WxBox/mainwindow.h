#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

private:
    Ui::MainWindow* ui;
	AboutWxBoxDialog aboutDialog;

public slots:
    void OpenAboutDialog();
};

#endif  // MAINWINDOW_H
