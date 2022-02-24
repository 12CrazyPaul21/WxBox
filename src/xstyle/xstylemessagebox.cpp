#include <xstyle/xstylemessagebox.h>

//
// XStyleMessageBox
//

void XStyleMessageBox::SetupMessageBox(QFrame* bodyFrame)
{
    bodyFrame->setLayout(new QVBoxLayout(bodyFrame));

    SetWindowTitle(title);
    SetupMessageLayout(bodyFrame);

    if (btnFlags != XStyleMessageBoxButtonType::NoButton) {
        SetupButtonLayout(bodyFrame);
    }

    bodyFrame->adjustSize();
    bodyFrame->resize(bodyFrame->width(), bodyFrame->height());
}

void XStyleMessageBox::SetupMessageLayout(QFrame* bodyFrame)
{
    msgLayout = new QHBoxLayout();
    msgLayout->setContentsMargins(QMargins(5, 5, 5, 5));
    msgLayout->setSpacing(10);

    // message icon
    if (msgType != XStyleMessageBoxType::Report && msgType != XStyleMessageBoxType::Message) {
        labelMessageIcon = new QLabel(bodyFrame);
        labelMessageIcon->setFixedSize(QSize(XSTYLE_MESSAGE_BOX_ICON_FIXED_WIDTH, XSTYLE_MESSAGE_BOX_ICON_FIXED_HEIGHT));
        labelMessageIcon->setScaledContents(true);

        QStyle::StandardPixmap standardPixmapId = QStyle::StandardPixmap::SP_MessageBoxInformation;
        QString                pixmapUrl        = informationIconUrl;

        switch (msgType) {
            case XStyleMessageBoxType::Warning:
                standardPixmapId = QStyle::StandardPixmap::SP_MessageBoxWarning;
                pixmapUrl        = warningIconUrl;
                break;
            case XStyleMessageBoxType::Error:
                standardPixmapId = QStyle::StandardPixmap::SP_MessageBoxCritical;
                pixmapUrl        = errorIconUrl;
                break;
        }

        QPixmap icon;
        if (!pixmapUrl.isEmpty() && pixmapUrl.at(0) != '#' && QFile::exists(pixmapUrl)) {
            icon = QPixmap(pixmapUrl);
        }
        else {
            icon = QApplication::style()->standardPixmap(standardPixmapId);
        }

        labelMessageIcon->setPixmap(icon);
        msgLayout->addWidget(labelMessageIcon);
    }

    // message text
    labelMessageText = new QLabel(bodyFrame);
    labelMessageText->setProperty("class", "xstylemessagebox_label_message");
    labelMessageText->setText(message);
    labelMessageText->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    labelMessageText->adjustSize();
    if (msgType == XStyleMessageBoxType::Report) {
        labelMessageText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }
    msgLayout->addWidget(labelMessageText);

    bodyFrame->layout()->addItem(msgLayout);
}

void XStyleMessageBox::PushButton(const char* text, XStyleMessageBoxButton btnFlag, QFrame* bodyFrame)
{
    if (!buttonLayout) {
        return;
    }

    auto btn = new QPushButton(xstyle_manager.Translate("xstyle_meta", text), bodyFrame);
    btn->setProperty("class", QString("xstylemessage_button_%1").arg(text));
    connect(btn, &QPushButton::clicked, this, [this, btnFlag]() {
        exitCode = btnFlag;
        emit close();
    });
    btn->setFixedSize(btn->width(), btn->height());
    buttonLayout->addWidget(btn);
}

void XStyleMessageBox::SetupButtonLayout(QFrame* bodyFrame)
{
    buttonLayout = new QHBoxLayout();
    buttonLayout->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(20);

    switch (btnFlags) {
        case XStyleMessageBoxButtonType::Ok:
            PushButton("Ok", XStyleMessageBoxButton::Ok, bodyFrame);
            break;
        case XStyleMessageBoxButtonType::Confirm:
            PushButton("Confirm", XStyleMessageBoxButton::Confirm, bodyFrame);
            break;
        case XStyleMessageBoxButtonType::ConfirmCancel:
            PushButton("Confirm", XStyleMessageBoxButton::Confirm, bodyFrame);
            PushButton("Cancel", XStyleMessageBoxButton::Cancel, bodyFrame);
            break;
        case XStyleMessageBoxButtonType::YesNo:
            PushButton("Yes", XStyleMessageBoxButton::Yes, bodyFrame);
            PushButton("No", XStyleMessageBoxButton::No, bodyFrame);
            break;
        case XStyleMessageBoxButtonType::RetrySkip:
            PushButton("Retry", XStyleMessageBoxButton::Retry, bodyFrame);
            PushButton("Skip", XStyleMessageBoxButton::Skip, bodyFrame);
            break;
    }

    bodyFrame->layout()->addItem(buttonLayout);
}

//
// XStyleMessageBox warpper
//

XStyleMessageBoxButton xstyle::report(QWidget* parent, const QString& title, const QString& msg, XStyleMessageBoxButtonType flags)
{
    return XStyleMessageBox(parent, title, msg, XStyleMessageBoxType::Report, flags, false).exec();
}

XStyleMessageBoxButton xstyle::message(QWidget* parent, const QString& title, const QString& msg, XStyleMessageBoxButtonType flags)
{
    return XStyleMessageBox(parent, title, msg, XStyleMessageBoxType::Message, flags, false).exec();
}

XStyleMessageBoxButton xstyle::information(QWidget* parent, const QString& title, const QString& msg, XStyleMessageBoxButtonType flags)
{
    return XStyleMessageBox(parent, title, msg, XStyleMessageBoxType::Information, flags, false).exec();
}

XStyleMessageBoxButton xstyle::warning(QWidget* parent, const QString& title, const QString& msg, XStyleMessageBoxButtonType flags)
{
    return XStyleMessageBox(parent, title, msg, XStyleMessageBoxType::Warning, flags, false).exec();
}

XStyleMessageBoxButton xstyle::error(QWidget* parent, const QString& title, const QString& msg, XStyleMessageBoxButtonType flags)
{
    return XStyleMessageBox(parent, title, msg, XStyleMessageBoxType::Error, flags, false).exec();
}