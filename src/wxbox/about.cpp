#include "about.h"
#include "ui_about.h"
#include "config.h"

#define _adtr(MESSAGE) Translate(MESSAGE)

#define ABOUT_WXBOX_NAME "AboutWxBoxDialog"
#define ABOUT_WXBOX_TITLE "About WxBox"

AboutWxBoxDialog::AboutWxBoxDialog(QWidget* parent, bool deleteWhenClose)
  : XStyleWindow(ABOUT_WXBOX_NAME, parent, deleteWhenClose)
  , ui(new Ui::AboutWxBoxDialog)
{
    SetupXStyleUi(ui);
    SetWindowTitle(Translate(ABOUT_WXBOX_TITLE));
    UpdateAboutContent();

    QObject::connect(ui->buttonBox, &QPushButton::clicked, this, &AboutWxBoxDialog::close);
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
        UpdateAboutContent();
    }
}

void AboutWxBoxDialog::UpdateAboutContent()
{
#define GENERATE_A_TAG(URL) R"(<a href=)" URL R"(>)" URL R"(</a>)"

    QString aboutContent = QString(
                               R"(<b>WxBox Version : %1</b><br/>
<b>WxBot Version : %2</b><br/>
<b>%3</b> : %4<br/>
<br/>
<b>%5</b> :<br/>
&nbsp;&nbsp;&nbsp;&nbsp;gRPC-v1.41.1 : %6<br/>
&nbsp;&nbsp;&nbsp;&nbsp;protobuf-v3.17.3 : %7<br/>
&nbsp;&nbsp;&nbsp;&nbsp;spdlog-v1.9.2 : %8<br/>
&nbsp;&nbsp;&nbsp;&nbsp;meson-v0.60.2 : %9<br/>
&nbsp;&nbsp;&nbsp;&nbsp;lua-v5.4.3 : %10<br/>
&nbsp;&nbsp;&nbsp;&nbsp;yaml-cpp-v0.7.0 : %11<br/>
&nbsp;&nbsp;&nbsp;&nbsp;googletest-v1.11.0 : %12<br/>
<br/>
<b>%13</b> :<br/>
&nbsp;&nbsp;&nbsp;&nbsp;iconfont : %14<br/>
)")
                               .arg(WXBOX_VERSION)
                               .arg(WXBOT_VERSION)
                               .arg(_adtr("Project Repository URL"))
                               .arg(GENERATE_A_TAG(WXBOX_REPOSITORY_URL))
                               .arg(_adtr("Thirdparty Project Repository URL"))
                               .arg(GENERATE_A_TAG("https://github.com/grpc/grpc"))
                               .arg(GENERATE_A_TAG("https://github.com/protocolbuffers/protobuf"))
                               .arg(GENERATE_A_TAG("https://github.com/gabime/spdlog"))
                               .arg(GENERATE_A_TAG("https://github.com/mesonbuild/meson"))
                               .arg(GENERATE_A_TAG("https://github.com/lua/lua"))
                               .arg(GENERATE_A_TAG("https://github.com/jbeder/yaml-cpp"))
                               .arg(GENERATE_A_TAG("https://github.com/google/googletest"))
                               .arg(_adtr("Thirdparty Resource URL"))
                               .arg(GENERATE_A_TAG("https://www.iconfont.cn"));
    ui->labelAboutContent->setText(aboutContent);
}