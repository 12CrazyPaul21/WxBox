#ifndef __X_STYLE_MESSAGE_BOX_H
#define __X_STYLE_MESSAGE_BOX_H

#include <xstyle/xstylewindow.h>
#include <QLayout>
#include <QPixmap>
#include <QFrame>

#define MESSAGE_WXBOX_NAME "XStyleMessageBox"

typedef enum class XStyleMessageBoxType
{
    Message = 0,
    Information,
    Warning,
    Error
} XStyleMessageBoxType;

typedef enum class XStyleMessageBoxButton
{
    Close = 0,
    NoButton,
    Ok,
    Yes,
    No,
    Confirm,
    Cancel,
    _Count
} XStyleMessageBoxButton;

typedef enum class XStyleMessageBoxButtonType
{
    NoButton = 0,
    Ok,
    Confirm,
    ConfirmCancel,
    YesNo
} XStyleMessageBoxButtonType;

//
// XStyleMessageBox
//

#define XSTYLE_MESSAGE_BOX_ICON_FIXED_WIDTH 40
#define XSTYLE_MESSAGE_BOX_ICON_FIXED_HEIGHT 40

#define RegisterXStyleMessageBoxProperty(Type, PropertyName, MemberName, Suffix, DefaultValue) \
    DefineXStyleProperty(Type, PropertyName, MemberName, Suffix, DefaultValue)                 \
        Q_PROPERTY(Type PropertyName READ Get##Suffix WRITE Set##Suffix DESIGNABLE true SCRIPTABLE true)

class XStyleMessageBox : public XStyleWindow
{
    Q_OBJECT

    RegisterXStyleMessageBoxProperty(QString, information_icon_url, informationIconUrl, InformationIconUrl, STANDARD_XSTYLE_MESSAGE_BOX_INFORMATION_BUTTON_ICON);
    RegisterXStyleMessageBoxProperty(QString, warning_icon_url, warningIconUrl, WarningIconUrl, STANDARD_XSTYLE_MESSAGE_BOX_WARNING_BUTTON_ICON);
    RegisterXStyleMessageBoxProperty(QString, error_icon_url, errorIconUrl, ErrorIconUrl, STANDARD_XSTYLE_MESSAGE_BOX_ERROR_BUTTON_ICON);

  public:
    explicit XStyleMessageBox(QWidget* parent, const QString& title, const QString& msg, XStyleMessageBoxType type = XStyleMessageBoxType::Information, XStyleMessageBoxButtonType flags = XStyleMessageBoxButtonType::Ok, bool deleteWhenClose = false)
      : XStyleWindow(MESSAGE_WXBOX_NAME, parent, deleteWhenClose)
      , exitCode(XStyleMessageBoxButton::Close)
      , title(title)
      , message(msg)
      , msgType(type)
      , btnFlags(flags)
      , buttonLayout(nullptr)
    {
        setProperty("class", this->property("class").toString() + " xstylemessagebox");
        SetupXStyleUi(this);
    }

    ~XStyleMessageBox()
    {
    }

    void setupUi(QFrame* bodyFrame)
    {
        SetupMessageBox(bodyFrame);
    }

    XStyleMessageBoxButton exec()
    {
        QEventLoop loop;
        connect(this, SIGNAL(destroyed()), &loop, SLOT(quit()));
        connect(this, &XStyleMessageBox::closed, this, [this, &loop]() {
            loop.exit((int)exitCode);
        });
        xstyleParent ? showApplicationModal() : show();
        XStyleMessageBoxButton retval = (XStyleMessageBoxButton)loop.exec();

        if (retval < XStyleMessageBoxButton::Close || retval >= XStyleMessageBoxButton::_Count) {
            retval = XStyleMessageBoxButton::Close;
        }

        if (retval == XStyleMessageBoxButton::Close) {
            if (btnFlags == XStyleMessageBoxButtonType::ConfirmCancel) {
                retval = XStyleMessageBoxButton::Cancel;
            }
            else if (btnFlags == XStyleMessageBoxButtonType::YesNo) {
                retval = XStyleMessageBoxButton::No;
            }
        }

        return (XStyleMessageBoxButton)retval;
    }

  protected:
    void SetupMessageBox(QFrame* bodyFrame);
    void SetupMessageLayout(QFrame* bodyFrame);
    void PushButton(const char* text, XStyleMessageBoxButton btnFlag, QFrame* bodyFrame);
    void SetupButtonLayout(QFrame* bodyFrame);

  protected:
    QString                    title;
    QString                    message;
    XStyleMessageBoxType       msgType;
    XStyleMessageBoxButtonType btnFlags;
    XStyleMessageBoxButton     exitCode;

    QHBoxLayout* msgLayout;
    QHBoxLayout* buttonLayout;

    QLabel* labelMessageIcon;
    QLabel* labelMessageText;
};

namespace xstyle {
    XStyleMessageBoxButton message(QWidget* parent, const QString& title, const QString& msg, XStyleMessageBoxButtonType flags = XStyleMessageBoxButtonType::Ok);
    XStyleMessageBoxButton information(QWidget* parent, const QString& title, const QString& msg, XStyleMessageBoxButtonType flags = XStyleMessageBoxButtonType::Ok);
    XStyleMessageBoxButton warning(QWidget* parent, const QString& title, const QString& msg, XStyleMessageBoxButtonType flags = XStyleMessageBoxButtonType::Ok);
    XStyleMessageBoxButton error(QWidget* parent, const QString& title, const QString& msg, XStyleMessageBoxButtonType flags = XStyleMessageBoxButtonType::Ok);
}

#endif  // #ifndef __X_STYLE_MESSAGE_BOX_H