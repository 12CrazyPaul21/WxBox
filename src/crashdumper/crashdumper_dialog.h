#ifndef __WXBOX_CRASH_DUMPER_DIALOG_H
#define __WXBOX_CRASH_DUMPER_DIALOG_H

#include <QApplication>
#include <QTranslator>
#include <QPushButton>
#include <QDialog>

#undef signals
#include <crashdumper.h>
#define signals Q_SIGNALS

namespace Ui {
    class CrashReportDialog;
}

class CrashReportDialog final : public QDialog
{
    Q_OBJECT

  public:
    explicit CrashReportDialog(QWidget* parent = nullptr);
    explicit CrashReportDialog(QWidget* parent, wb_coredump::PCrashDumperRequest request, PCrashDumpReport dumpReport);
    ~CrashReportDialog();

  public slots:
    void openDumpFolderInExplorer();
    void accept() override;

  private:
    Ui::CrashReportDialog*           ui;
    QTranslator                      translator;
    wb_coredump::PCrashDumperRequest request;
    PCrashDumpReport                 dumpReport;
};

#endif  // #ifndef __WXBOX_CRASH_DUMPER_DIALOG_H
