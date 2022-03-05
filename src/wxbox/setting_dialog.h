#ifndef __WXBOX_SETTING_DIALOG_H
#define __WXBOX_SETTING_DIALOG_H

#include <QValidator>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>

#undef signals
#include <utils/common.h>
#define signals Q_SIGNALS

#include <xstyle/xstylewindow.h>
#include <xstyle/xstylemessagebox.h>

#include <app_log.hpp>
#include <app_config.hpp>

//
// store helper
//

#define SETTING_VALUE_KEY "CURRENT_SETTING_ITEM_VALUE"

template<typename QT_WIDGET_TYPE, typename VALUE_TYPE>
void SetCurrentSettingValue(QT_WIDGET_TYPE* widget, const VALUE_TYPE& value);

template<typename VALUE_TYPE>
VALUE_TYPE GetCurrentSettingValue(QWidget* widget);

#define SET_SETTING_VALUE_TEMPLATE(TYPE, SET_METHOD, GET_METHOD)       \
    template<typename VALUE_TYPE>                                      \
    void SetCurrentSettingValue(TYPE* widget, const VALUE_TYPE& value) \
    {                                                                  \
        if (!widget) {                                                 \
            return;                                                    \
        }                                                              \
                                                                       \
        widget->SET_METHOD(value);                                     \
        widget->setProperty(SETTING_VALUE_KEY, widget->GET_METHOD());  \
    }

#define SET_SETTING_VALUE_SPECIALIZATION_TEMPLATE(TYPE, VALUE_TYPE, SET_METHOD, GET_METHOD) \
    template<>                                                                              \
    void SetCurrentSettingValue(TYPE* widget, const VALUE_TYPE& value)                      \
    {                                                                                       \
        if (!widget) {                                                                      \
            return;                                                                         \
        }                                                                                   \
                                                                                            \
        widget->SET_METHOD(value);                                                          \
        widget->setProperty(SETTING_VALUE_KEY, widget->GET_METHOD());                       \
    }

#define GET_SETTING_VALUE_TEMPLATE(TYPE, DEFAULT_VALUE)   \
    template<>                                            \
    TYPE GetCurrentSettingValue(QWidget* widget)          \
    {                                                     \
        if (!widget) {                                    \
            return DEFAULT_VALUE;                         \
        }                                                 \
                                                          \
        auto value = widget->property(SETTING_VALUE_KEY); \
        if (value.canConvert<TYPE>()) {                   \
            return value.value<TYPE>();                   \
        }                                                 \
                                                          \
        return DEFAULT_VALUE;                             \
    }

namespace Ui {
    class WxBoxSettingDialog;
}

class QIntValidatorFixup final : public QIntValidator
{
    Q_OBJECT

  public:
    explicit QIntValidatorFixup(QObject* parent = nullptr)
      : QIntValidator(parent)
    {
    }

    QIntValidatorFixup(int bottom, int top, QObject* parent)
      : QIntValidator(bottom, top, parent)
    {
    }

    virtual void fixup(QString& input) const override
    {
        input = QString("%1").arg(WXBOX_CLAMP(input.toInt(), bottom(), top()));
    }
};

//
// WxBoxSettingDialog
//

class WxBoxSettingDialog final : public XStyleWindow
{
    Q_OBJECT

    using WxBoxSettingChangedHandler = std::function<void(const QString& name, const QVariant& newValue)>;

  public:
    explicit WxBoxSettingDialog(QWidget* parent = nullptr, bool deleteWhenClose = false);
    ~WxBoxSettingDialog();

    virtual bool eventFilter(QObject* obj, QEvent* e) Q_DECL_OVERRIDE;

    inline void RegisterSettingChangedHandler(WxBoxSettingChangedHandler _handler)
    {
        handler = _handler;
    }

    void InitWidgets();
    void LoadSetting();
    int  ModifySetting(int tabIndex = 0);

    void Apply(bool confirm = false);

    void Confirm()
    {
        Apply(true);
        close();
    }

    void Cancel()
    {
        close();
    }

  protected:
    void RetranslateUi();

    inline void SetSettingItemRequireRestartToApply(QWidget* widget, bool requireRestartToApply)
    {
        widget->setProperty("SETTING_APPLY_NEED_RESTART", requireRestartToApply);
    }

    inline bool IsSettingItemRequireRestartToApply(QWidget* widget)
    {
        auto value = widget->property("SETTING_APPLY_NEED_RESTART");
        if (value.canConvert<bool>()) {
            return value.value<bool>();
        }
        return false;
    }

    inline void RegisterPathVisitor(QPushButton* triggerBtn, QLineEdit* pathLineEdit)
    {
        QObject::connect(triggerBtn, &QPushButton::clicked, this, [this, pathLineEdit]() {
            auto path = pathLineEdit->text();
            if (path.isEmpty()) {
                return;
            }
            wb_file::OpenFolderInExplorer(path.toLocal8Bit().data());
        });
    }

    inline void RegisterPathSelector(QPushButton* triggerBtn, QLineEdit* pathLineEdit, const QString& title)
    {
        QObject::connect(triggerBtn, &QPushButton::clicked, this, [this, pathLineEdit, title]() {
            auto newPath = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, title, pathLineEdit->text()));
            if (!newPath.isEmpty()) {
                pathLineEdit->setText(newPath);
            }
        });
    }

    inline void RegisterLineEditSettingItem(QLineEdit* valueEdit, bool requireRestartToApply = false)
    {
        QObject::connect(valueEdit, &QLineEdit::textChanged, this, [this, valueEdit](const QString& value) {
            SettingChanged(valueEdit, QVariant::fromValue(value));
        });

        SetSettingItemRequireRestartToApply(valueEdit, requireRestartToApply);
    }

    inline void RegisterSliderSettingItem(QSlider* slider, QLineEdit* valueEdit, bool requireRestartToApply = false)
    {
        valueEdit->setValidator(new QIntValidatorFixup(slider->minimum(), slider->maximum(), this));
        QObject::connect(slider, &QSlider::valueChanged, this, [this, slider, valueEdit](int value) {
            valueEdit->setText(QString("%1").arg(value));
            SettingChanged(slider, QVariant::fromValue(value));
        });
        QObject::connect(valueEdit, &QLineEdit::textChanged, this, [this, slider](const QString& value) {
            slider->setValue(value.toInt());
        });

        SetSettingItemRequireRestartToApply(slider, requireRestartToApply);
    }

    inline void RegisterCheckBoxSettingItem(QCheckBox* checkbox, bool requireRestartToApply = false)
    {
        QObject::connect(checkbox, &QCheckBox::stateChanged, this, [this, checkbox](int state) {
            WXBOX_UNREF(state);
            SettingChanged(checkbox, QVariant::fromValue(checkbox->isChecked()));
        });

        SetSettingItemRequireRestartToApply(checkbox, requireRestartToApply);
    }

    inline void RegisterComboBoxSettingItem(QComboBox* combo, bool requireRestartToApply = false)
    {
        QObject::connect(combo, &QComboBox::currentTextChanged, this, [this, combo](const QString& index) {
            WXBOX_UNREF(index);
            SettingChanged(combo, QVariant::fromValue(combo->currentIndex()));
        });

        SetSettingItemRequireRestartToApply(combo, requireRestartToApply);
    }

    void ApplyChanged(QWidget* settingItem, bool confirm);
    void SettingChanged(QWidget* settingItem, const QVariant& newValue);

  public slots:
    void Closed()
    {
        changedList.clear();

        if (waitingLoop) {
            waitingLoop->exit(neededRestartToApplyAll);
            waitingLoop = nullptr;
        }
    }

  private:
    Ui::WxBoxSettingDialog* ui;

    AppConfig&                               config;
    std::vector<std::pair<QString, QString>> i18ns;
    std::vector<QString>                     themes;

    int                        neededRestartToApplyAll;
    QEventLoop*                waitingLoop;
    std::set<QWidget*>         changedList;
    WxBoxSettingChangedHandler handler;
};

#endif  // #ifndef __WXBOX_SETTING_DIALOG_H
