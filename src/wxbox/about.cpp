#include "about.h"
#include "ui_about.h"

#define ABOUT_WXBOX_NAME "AboutWxboxDialog"
#define ABOUT_WXBOX_TITLE "About WxBox"

AboutWxBoxDialog::AboutWxBoxDialog(QWidget* parent, bool deleteWhenClose)
  : XStyleWindow(ABOUT_WXBOX_NAME, parent, deleteWhenClose)
  , ui(new Ui::AboutWxBoxDialog)
{
    SetupXStyleUi(ui);
    SetWindowTitle(Translate(ABOUT_WXBOX_TITLE));

    QObject::connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AboutWxBoxDialog::close);
}

AboutWxBoxDialog::~AboutWxBoxDialog()
{
    delete ui;
}

void AboutWxBoxDialog::RetranslateUi()
{
    XStyleWindow::RetranslateUi();

    if (ui) {
        ui->retranslateUi(this);
        SetWindowTitle(Translate(ABOUT_WXBOX_TITLE));
    }
}