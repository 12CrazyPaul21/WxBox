#ifndef __ABOUT_WXBOX_CONTACT_LIST_DIALOG_H
#define __ABOUT_WXBOX_CONTACT_LIST_DIALOG_H

#include <QStandardItemModel>

#include <xstyle/xstylewindow.h>

#undef signals
#include <utils/common.h>
#define signals Q_SIGNALS

#define WxBoxContactListDialogHeader                 \
    {                                                \
        "NickName", "Remark", "WeChatNumber", "WXID" \
    }

namespace Ui {
    class ContactListDialog;
}

class ContactListDialog final : public XStyleWindow
{
    Q_OBJECT

  public:
    explicit ContactListDialog(QWidget* parent = nullptr, bool deleteWhenClose = false);
    ~ContactListDialog();

    void DisplayContactList(const QString& ownerWxNumber, const std::vector<wb_wx::WeChatContact>& contacts);
    void ResetHeader(bool _inited = true);

  protected:
    void RetranslateUi();

  public slots:
    void ContactListDialogClosed();

  private:
    Ui::ContactListDialog* ui;
    QStandardItemModel     contactItemModel;
};

#endif  // #ifndef __ABOUT_WXBOX_CONTACT_LIST_DIALOG_H
