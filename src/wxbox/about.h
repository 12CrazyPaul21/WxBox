#ifndef __ABOUT_WXBOX_DIALOG_H
#define __ABOUT_WXBOX_DIALOG_H

#include <xstyle/xstylewindow.h>

namespace Ui {
    class AboutWxBoxDialog;
}

class AboutWxBoxDialog final : public XStyleWindow
{
    Q_OBJECT

  public:
    explicit AboutWxBoxDialog(QWidget* parent = nullptr, bool deleteWhenClose = false);
    ~AboutWxBoxDialog();

  protected:
    void RetranslateUi();

  private:
    Ui::AboutWxBoxDialog* ui;
};

#endif  // #ifndef __ABOUT_WXBOX_DIALOG_H
