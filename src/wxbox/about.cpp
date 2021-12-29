#include "about.h"
#include "ui_about.h"

AboutWxBoxDialog::AboutWxBoxDialog(QWidget* parent)
  : QDialog(parent)
  , ui(new Ui::AboutWxBoxDialog)
{
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

AboutWxBoxDialog::~AboutWxBoxDialog()
{
    delete ui;
}

void AboutWxBoxDialog::changeEvent(QEvent* event)
{
    switch (event->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            QWidget::changeEvent(event);
    }
}
