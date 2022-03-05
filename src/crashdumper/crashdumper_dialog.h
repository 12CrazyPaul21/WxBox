#ifndef __WXBOX_CRASH_DUMPER_DIALOG_H
#define __WXBOX_CRASH_DUMPER_DIALOG_H

#include <QApplication>
#include <QTranslator>
#include <QPushButton>
#include <QDialog>
#include <QStyleFactory>

#undef signals
#include <crashdumper.h>
#define signals Q_SIGNALS

#include <xstyle/xstylewindow.h>

#include <config.h>

namespace Ui {
    class CrashReportWidget;
}

class CrashReportDialog final : public XStyleWindow
{
    Q_OBJECT

  public:
    explicit CrashReportDialog(QWidget* xstyleParent = nullptr);
    explicit CrashReportDialog(QWidget* xstyleParent, wb_coredump::PCrashDumperRequest request, PCrashDumpReport dumpReport);
    ~CrashReportDialog();

  protected:
    void RetranslateUi();

  public slots:
    void openDumpFolderInExplorer();
    void confirm();

  private:
    Ui::CrashReportWidget*           ui;
    wb_coredump::PCrashDumperRequest request;
    PCrashDumpReport                 dumpReport;
};

#endif  // #ifndef __WXBOX_CRASH_DUMPER_DIALOG_H
