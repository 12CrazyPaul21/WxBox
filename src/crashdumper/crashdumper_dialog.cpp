#include "crashdumper_dialog.h"
#include "ui_crashdumper_dialog.h"

CrashReportDialog::CrashReportDialog(QWidget* parent)
  : CrashReportDialog(parent, nullptr, nullptr)
{
}

CrashReportDialog::CrashReportDialog(QWidget* parent, wb_coredump::PCrashDumperRequest request, PCrashDumpReport dumpReport)
  : QDialog(parent)
  , ui(new Ui::CrashReportDialog)
  , request(request)
  , dumpReport(dumpReport)
{
    if (request) {
        auto languagePath = wb_file::JoinPath(request->i18nPath, std::string(request->language) + ".qm");
        if (wb_file::IsPathExists(languagePath)) {
            if (translator.load(QString::fromLocal8Bit(languagePath.c_str()))) {
                qApp->installTranslator(&translator);
            }
        }
    }

    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

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
}

CrashReportDialog::~CrashReportDialog()
{
    delete ui;
}

void CrashReportDialog::openDumpFolderInExplorer()
{
    if (request) {
        wb_file::OpenFolderInExplorer(request->dumpSinkPath);
    }
}

void CrashReportDialog::accept()
{
    QDialog::accept();

    if (!dumpReport || !ui->is_need_restart_program->isChecked()) {
        return;
    }

    wb_process::StartProcess(dumpReport->crashProgramFullPath, false);
}