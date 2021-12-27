#ifndef __ABOUT_WXBOX_DIALOG_H
#define __ABOUT_WXBOX_DIALOG_H

#include <QDialog>

namespace Ui {
	class AboutWxBoxDialog;
}

class AboutWxBoxDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutWxBoxDialog(QWidget* parent = nullptr);
	~AboutWxBoxDialog();

	virtual void changeEvent(QEvent* event) override;

private:
    Ui::AboutWxBoxDialog* ui;
};

#endif  // #ifndef __ABOUT_WXBOX_DIALOG_H
