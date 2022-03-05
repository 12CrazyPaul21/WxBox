#include "contact_list_dialog.h"
#include "ui_contact_list_dialog.h"

#define CONTACT_LIST_DIALOG_WXBOX_NAME "ContactListDialog"

ContactListDialog::ContactListDialog(QWidget* parent, bool deleteWhenClose)
  : XStyleWindow(CONTACT_LIST_DIALOG_WXBOX_NAME, parent, deleteWhenClose)
  , ui(new Ui::ContactListDialog)
  , contactItemModel(this)
{
    SetupXStyleUi(ui);
    SetWindowTitle("");

    ui->viewContactList->setModel(&contactItemModel);
    ui->viewContactList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ResetHeader(true);

    QObject::connect(this, SIGNAL(closed()), this, SLOT(ContactListDialogClosed()));
}

ContactListDialog::~ContactListDialog()
{
    delete ui;
}

void ContactListDialog::RetranslateUi()
{
    XStyleWindow::RetranslateUi();

    if (ui) {
        ui->retranslateUi(this);
        contactItemModel.setHorizontalHeaderLabels(TranslateStringList(WxBoxContactListDialogHeader));
    }
}

void ContactListDialog::DisplayContactList(const QString& ownerWxNumber, const std::vector<wb_wx::WeChatContact>& contacts)
{
    SetWindowTitle(ownerWxNumber + Translate(" All Contacts :"));

    contactItemModel.removeRows(0, contactItemModel.rowCount());
    for (auto contact : contacts) {
        QStandardItem* nicknameItem = new QStandardItem(contact.nickname.c_str());
        nicknameItem->setToolTip(nicknameItem->text());

        QStandardItem* remarkItem = new QStandardItem(contact.remark.c_str());
        remarkItem->setToolTip(remarkItem->text());

        QStandardItem* wxnumberItem = new QStandardItem(contact.wxnumber.c_str());
        wxnumberItem->setToolTip(wxnumberItem->text());

        QStandardItem* wxidItem = new QStandardItem(contact.wxid.c_str());
        wxidItem->setToolTip(wxidItem->text());

        contactItemModel.appendRow(QList<QStandardItem*>({nicknameItem, remarkItem, wxnumberItem, wxidItem}));
    }

    showApplicationModal();
}

void ContactListDialog::ResetHeader(bool _inited)
{
    contactItemModel.setHorizontalHeaderLabels(TranslateStringList(WxBoxContactListDialogHeader));

    auto header = ui->viewContactList->horizontalHeader();
    if (!header) {
        return;
    }

    header->resizeSections(QHeaderView::ResizeToContents);

    if (_inited) {
        header->resizeSection(0, header->sectionSize(0) + 50);
        header->resizeSection(1, header->sectionSize(1) + 40);
        header->resizeSection(2, header->sectionSize(2) + 70);
        header->setSectionResizeMode(3, QHeaderView::Stretch);
    }
}

void ContactListDialog::ContactListDialogClosed()
{
    contactItemModel.removeRows(0, contactItemModel.rowCount());
}