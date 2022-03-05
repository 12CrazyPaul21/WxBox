#include "crashdumper_dialog.h"
#include "ui_crashdumper_dialog.h"

#define CRASH_DUMPER_NAME "CrashDumper"
#define CRASH_DUMPER_TITLE "WxBox - " CRASH_DUMPER_NAME

CrashReportDialog::CrashReportDialog(QWidget* xstyleParent)
  : CrashReportDialog(xstyleParent, nullptr, nullptr)
{
}

CrashReportDialog::CrashReportDialog(QWidget* xstyleParent, wb_coredump::PCrashDumperRequest request, PCrashDumpReport dumpReport)
  : XStyleWindow(CRASH_DUMPER_NAME, xstyleParent, false)
  , ui(new Ui::CrashReportWidget)
  , request(request)
  , dumpReport(dumpReport)
{
    // setup xstyle ui
    qApp->setStyle(QStyleFactory::create("Fusion"));
    SetupXStyleUi(ui);

    // update title
    SetWindowTitle(Translate(CRASH_DUMPER_TITLE));

    // wxbox repo link
    QLabel* link = new QLabel(Translate(R"(visit wxbox repository to submit the issue)") + R"( : <a href=)" WXBOX_REPOSITORY_URL R"(>wxbox gitee repo</a>)");
    link->setObjectName("CrashDumper_link_label");
    link->setOpenExternalLinks(true);
    this->footerPanel->layout()->addWidget(link);

    // information
    if (dumpReport) {
        ui->label_crash_program->setText(QString::fromLocal8Bit(dumpReport->crashProgram.c_str()));
        ui->label_process_bits->setText(dumpReport->is64Process ? "64-bits" : "32-bits");
        ui->label_crash_date->setText(dumpReport->date.c_str());
        ui->label_error_type->setText(dumpReport->exceptionTypeDescription.c_str());
        ui->label_crash_module->setText(QString::fromLocal8Bit(wb_file::ToFileName(dumpReport->crashModule).c_str()));
        ui->label_crash_address->setText(QString::asprintf(dumpReport->is64Process ? "0x%.16X" : "0x%.8X", dumpReport->crashInstructionAddress));
        ui->label_module_path->setText(QString::fromLocal8Bit(dumpReport->crashModule.c_str()));
        ui->label_module_path->setToolTip(dumpReport->crashModule.c_str());
    }

    QObject::connect(ui->button_open_dump_folder, &QPushButton::clicked, this, &CrashReportDialog::openDumpFolderInExplorer);
    QObject::connect(ui->button_confirm, &QPushButton::clicked, this, &CrashReportDialog::confirm);
}

CrashReportDialog::~CrashReportDialog()
{
    delete ui;
}

void CrashReportDialog::RetranslateUi()
{
    XStyleWindow::RetranslateUi();
    if (this->bodyContainer) {
        ui->retranslateUi(this->bodyContainer);
    }
}

void CrashReportDialog::openDumpFolderInExplorer()
{
    if (request) {
        auto coredumpFileName = std::string(request->dumpPrefix) + "-MiniDump-" + request->crashTimestamp + ".dmp";
        auto coredumpFilePath = wb_file::JoinPath(request->dumpSinkPath, coredumpFileName);
        wb_file::OpenFileInExplorer(coredumpFilePath);
    }
}

void CrashReportDialog::confirm()
{
    if (dumpReport && ui->is_need_restart_program->isChecked()) {
        wb_process::StartProcess(dumpReport->crashProgramFullPath, false);
    }

    close();
}