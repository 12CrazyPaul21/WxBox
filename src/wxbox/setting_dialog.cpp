#include "setting_dialog.h"
#include "ui_setting_dialog.h"

//
// store helper
//

SET_SETTING_VALUE_TEMPLATE(QLineEdit, setText, text);
SET_SETTING_VALUE_TEMPLATE(QSlider, setValue, value);
SET_SETTING_VALUE_TEMPLATE(QCheckBox, setChecked, isChecked);
SET_SETTING_VALUE_TEMPLATE(QComboBox, setCurrentIndex, currentIndex);
SET_SETTING_VALUE_SPECIALIZATION_TEMPLATE(QComboBox, QString, setCurrentText, currentIndex);

GET_SETTING_VALUE_TEMPLATE(int, 0);
GET_SETTING_VALUE_TEMPLATE(bool, false);
GET_SETTING_VALUE_TEMPLATE(QString, "");

inline QVariant UpdateCurrentSettingValue(QWidget* settingItem, bool confirm)
{
#define UPDATE_CURRENT_SETTING_VALUE(ITEM_TYPE, GET_METHOD)     \
    {                                                           \
        auto v = ((ITEM_TYPE*)(settingItem))->GET_METHOD();     \
        if (!confirm) {                                         \
            SetCurrentSettingValue((ITEM_TYPE*)settingItem, v); \
        }                                                       \
        newValue = v;                                           \
    }

    QVariant newValue;

    if (settingItem->inherits("QLineEdit")) {
        UPDATE_CURRENT_SETTING_VALUE(QLineEdit, text);
    }
    else if (settingItem->inherits("QSlider")) {
        UPDATE_CURRENT_SETTING_VALUE(QSlider, value);
    }
    else if (settingItem->inherits("QCheckBox")) {
        UPDATE_CURRENT_SETTING_VALUE(QCheckBox, isChecked);
    }
    else if (settingItem->inherits("QComboBox")) {
        UPDATE_CURRENT_SETTING_VALUE(QComboBox, currentIndex);
    }

    return newValue;
}

//
// WxBoxSettingDialog
//

#define WXBOX_SETTING_DIALOG_NAME "WxBoxSettingDialog"
#define WXBOX_SETTING_DIALOG_TITLE "Setting"

WxBoxSettingDialog::WxBoxSettingDialog(QWidget* parent, bool deleteWhenClose)
  : XStyleWindow(WXBOX_SETTING_DIALOG_NAME, parent, deleteWhenClose)
  , ui(new Ui::WxBoxSettingDialog)
  , config(AppConfig::singleton())
  , neededRestartToApplyAll(0)
  , waitingLoop(nullptr)
{
    SetupXStyleUi(ui);
    SetWindowTitle(Translate(WXBOX_SETTING_DIALOG_TITLE));
}

WxBoxSettingDialog::~WxBoxSettingDialog()
{
    delete ui;
}

void WxBoxSettingDialog::RetranslateUi()
{
    XStyleWindow::RetranslateUi();

    if (ui) {
        ui->retranslateUi(this);
        SetWindowTitle(Translate(WXBOX_SETTING_DIALOG_TITLE));
    }
}

bool WxBoxSettingDialog::eventFilter(QObject* obj, QEvent* e)
{
    if (obj != this && e->type() == QEvent::Type::Wheel) {
        return true;
    }

    return false;
}

void WxBoxSettingDialog::InitWidgets()
{
    QObject::connect(this, SIGNAL(closed()), this, SLOT(Closed()));

    //
    // Disabled Some Settings
    //

    // ui->labelLoadingIconType->setVisible(false);
    // ui->comboBoxLoadingIconType->setVisible(false);

    //
    // action button
    //

    QObject::connect(ui->btnApply, &QPushButton::clicked, this, &WxBoxSettingDialog::Apply);
    QObject::connect(ui->btnConfirm, &QPushButton::clicked, this, &WxBoxSettingDialog::Confirm);
    QObject::connect(ui->btnCancel, &QPushButton::clicked, this, &WxBoxSettingDialog::Cancel);

    //
    // Regular tab
    //

    // RPC groupbox
    ui->lineEditWxBoxServerPort->setValidator(new QIntValidatorFixup(1024, 65535, this));
    RegisterLineEditSettingItem(ui->lineEditWxBoxServerPort, true);
    RegisterSliderSettingItem(ui->sliderWxBoxClientReconnectInterval, ui->lineEditWxBoxClientReconnectIntervalValue);

    // window groupbox
    RegisterCheckBoxSettingItem(ui->checkBoxCloseIsMinimizeToTray);
    RegisterCheckBoxSettingItem(ui->checkBoxAlwaysTopMost);
    RegisterCheckBoxSettingItem(ui->checkBoxLoadingIconAnimationUseCache);
    RegisterComboBoxSettingItem(ui->comboBoxLoadingIconType);

    // language and theme groupbox
    RegisterComboBoxSettingItem(ui->comboBoxLanguage);
    RegisterComboBoxSettingItem(ui->comboBoxTheme);

    // coredump groupbox
    RegisterLineEditSettingItem(ui->lineEditCoreDumpPrefix);

    // log groupbox
    RegisterLineEditSettingItem(ui->lineEditLogBasename, true);
    RegisterSliderSettingItem(ui->sliderLogMaxRotatingFileCount, ui->lineEditLogMaxRotatingFileCountValue, true);
    RegisterSliderSettingItem(ui->sliderLogMaxSingleFileSize, ui->lineEditLogMaxSingleFileSizeValue, true);
    RegisterSliderSettingItem(ui->sliderLogAutoFlushInterval, ui->lineEditLogAutoFlushIntervalValue);

    //
    // Plugin tab
    //

    RegisterSliderSettingItem(ui->sliderPluginLongTaskTimeout, ui->lineEditPluginLongTaskTimeoutValue);
    RegisterSliderSettingItem(ui->sliderPluginLogMaxLine, ui->lineEditPluginLogMaxLineValue);
    RegisterSliderSettingItem(ui->sliderPluginCommandMaxHistoryLine, ui->lineEditPluginCommandMaxHistoryLineValue);
    RegisterSliderSettingItem(ui->sliderPluginCommandMaxHistoryPersistenceLine, ui->lineEditPluginCommandMaxHistoryPersistenceLineValue);

    //
    // Feature tab
    //

    RegisterLineEditSettingItem(ui->lineEditFeatureRepoRootURL);
    RegisterCheckBoxSettingItem(ui->checkBoxAvoidRevokeMessage);
    RegisterCheckBoxSettingItem(ui->checkBoxEnableRawMessageHook);
    RegisterCheckBoxSettingItem(ui->checkBoxEnableSendTextMessageHook);

    //
    // WeChat tab
    //

    RegisterLineEditSettingItem(ui->lineEditWeChatInstallationPath);
    RegisterLineEditSettingItem(ui->lineEditWeChatCoreModulePath);
    RegisterSliderSettingItem(ui->sliderClientStatusUpdateInterval, ui->lineEditClientStatusUpdateIntervalValue);

    //
    // Path Visiter
    //

    RegisterPathVisitor(ui->btnVisit18nPath, ui->lineEditi18nPath);
    RegisterPathVisitor(ui->btnVisitThemePath, ui->lineEditThemePath);
    RegisterPathVisitor(ui->btnVisitCoreDumpPath, ui->lineEditCoreDumpPath);
    RegisterPathVisitor(ui->btnVisitLogPath, ui->lineEditLogPath);
    RegisterPathVisitor(ui->btnVisitPluginsPath, ui->lineEditPluginsPath);
    RegisterPathVisitor(ui->btnVisitFeaturePath, ui->lineEditFeaturePath);
    RegisterPathVisitor(ui->btnVisitFeatureRepoRootURL, ui->lineEditFeatureRepoRootURL);
    RegisterPathVisitor(ui->btnVisitWeChatInstallationPath, ui->lineEditWeChatInstallationPath);
    RegisterPathVisitor(ui->btnVisitWeChatCoreModulePath, ui->lineEditWeChatCoreModulePath);

    //
    // Folder Selector
    //

    RegisterPathSelector(ui->btnSelectWeChatInstallationPath, ui->lineEditWeChatInstallationPath, Translate("Choice WeChat Installation Path"));
    RegisterPathSelector(ui->btnSelectWeChatCoreModulePath, ui->lineEditWeChatCoreModulePath, Translate("Choice WeChat Core Module Path"));

    //
    // filter wheel event
    //

    ui->sliderWxBoxClientReconnectInterval->installEventFilter(this);
    ui->comboBoxLoadingIconType->installEventFilter(this);
    ui->comboBoxLanguage->installEventFilter(this);
    ui->comboBoxTheme->installEventFilter(this);
    ui->sliderLogMaxRotatingFileCount->installEventFilter(this);
    ui->sliderLogMaxSingleFileSize->installEventFilter(this);
    ui->sliderLogAutoFlushInterval->installEventFilter(this);
}

void WxBoxSettingDialog::LoadSetting()
{
    //
    // action button
    //

    ui->btnApply->setEnabled(false);
    ui->btnConfirm->setEnabled(true);
    ui->btnCancel->setEnabled(true);

    //
    // Regular tab
    //

    auto serverURI = QString(config.wxbox_server_uri().c_str());
    SetCurrentSettingValue(ui->lineEditWxBoxServerURI, serverURI);
    auto port = QUrl("http://" + serverURI).port();
    SetCurrentSettingValue(ui->lineEditWxBoxServerPort, QString("%1").arg(port >= 1024 ? port : 1024));

    SetCurrentSettingValue(ui->sliderWxBoxClientReconnectInterval, config.wxbox_client_reconnect_interval());
    SetCurrentSettingValue(ui->checkBoxCloseIsMinimizeToTray, config.close_is_minimize_to_tray());
    SetCurrentSettingValue(ui->checkBoxAlwaysTopMost, config.always_top_most());
    SetCurrentSettingValue(ui->checkBoxLoadingIconAnimationUseCache, config.loading_icon_animation_use_cache());
    SetCurrentSettingValue(ui->comboBoxLoadingIconType, config.loading_icon_type());

    //
    // i18n and theme groupbox
    //

    i18ns  = xstyle_manager.i18ns();
    themes = xstyle_manager.ThemeList();

    auto    currentLanguage = config.language();
    QString languageIndex   = "";

    ui->comboBoxLanguage->clear();
    for (auto language : i18ns) {
        ui->comboBoxLanguage->addItem(language.second);
        if (!language.first.compare(currentLanguage.c_str())) {
            languageIndex = language.second;
        }
    }

    ui->comboBoxTheme->clear();
    ui->comboBoxTheme->addItem("DefaultTheme");
    for (auto theme : themes) {
        ui->comboBoxTheme->addItem(theme);
    }

    SetCurrentSettingValue(ui->lineEditi18nPath, QString(config.i18n_path().c_str()));
    SetCurrentSettingValue(ui->comboBoxLanguage, languageIndex);
    SetCurrentSettingValue(ui->lineEditThemePath, QString(config.theme_path().c_str()));
    SetCurrentSettingValue(ui->comboBoxTheme, QString(config.current_theme_name().c_str()));

    //
    // CoreDump groupbox
    //

    SetCurrentSettingValue(ui->lineEditCoreDumpPath, QString(config.coredump_path().c_str()));
    SetCurrentSettingValue(ui->lineEditCoreDumpPrefix, QString(config.coredump_prefix().c_str()));
    SetCurrentSettingValue(ui->lineEditCoreDumper, QString(config.crashdumper().c_str()));

    //
    // Log groupbox
    //

    SetCurrentSettingValue(ui->lineEditLogPath, QString(config.log_file_path().c_str()));
    SetCurrentSettingValue(ui->lineEditLogBasename, QString(config.log_name().c_str()));
    SetCurrentSettingValue(ui->sliderLogMaxRotatingFileCount, config.log_max_rotating_file_count());
    SetCurrentSettingValue(ui->sliderLogMaxSingleFileSize, config.log_max_single_file_size() / 1048576);
    SetCurrentSettingValue(ui->sliderLogAutoFlushInterval, config.log_auto_flush_interval_sec());

    //
    // Plugin tab
    //

    SetCurrentSettingValue(ui->lineEditPluginsPath, QString(config.plugins_root().c_str()));
    SetCurrentSettingValue(ui->sliderPluginLongTaskTimeout, config.plugin_long_task_timeout());
    SetCurrentSettingValue(ui->sliderPluginLogMaxLine, config.plugin_log_max_line());
    SetCurrentSettingValue(ui->sliderPluginCommandMaxHistoryLine, config.plugin_command_max_history_line());
    SetCurrentSettingValue(ui->sliderPluginCommandMaxHistoryPersistenceLine, config.plugin_command_max_history_persistence_line());

    //
    // Feature tab
    //

    SetCurrentSettingValue(ui->lineEditFeaturePath, QString(config.features_path().c_str()));
    SetCurrentSettingValue(ui->lineEditFeatureRepoRootURL, QString(config.features_repo_root_url().c_str()));
    SetCurrentSettingValue(ui->lineEditFeatureVersionTimestamp, QString(config.feature_update_timestamp().c_str()));
    SetCurrentSettingValue(ui->checkBoxAvoidRevokeMessage, config.wechat_avoid_revoke_message());
    SetCurrentSettingValue(ui->checkBoxEnableRawMessageHook, config.wechat_enable_raw_message_hook());
    SetCurrentSettingValue(ui->checkBoxEnableSendTextMessageHook, config.wechat_enable_send_text_message_hook());

    //
    // WeChat tab
    //

    SetCurrentSettingValue(ui->lineEditWeChatInstallationPath, QString(config.wechat_installation_dir().c_str()));
    SetCurrentSettingValue(ui->lineEditWeChatCoreModulePath, QString(config.wechat_module_dir().c_str()));
    SetCurrentSettingValue(ui->sliderClientStatusUpdateInterval, config.wechat_status_monitor_interval());
}

int WxBoxSettingDialog::ModifySetting(int tabIndex)
{
    if (waitingLoop) {
        return 0;
    }

    ui->tabWidget->setCurrentIndex(tabIndex);
    LoadSetting();

    bool enableAllTab = (tabIndex != 3);
    ui->tabRegular->setEnabled(enableAllTab);
    ui->tabPlugin->setEnabled(enableAllTab);
    ui->tabFeature->setEnabled(enableAllTab);

    QEventLoop loop;
    waitingLoop             = &loop;
    neededRestartToApplyAll = 0;

    QTimer::singleShot(10, [this]() {
        xstyleParent ? showApplicationModal() : show();
    });

    return loop.exec();
}

void WxBoxSettingDialog::Apply(bool confirm)
{
    if (changedList.empty()) {
        return;
    }

    neededRestartToApplyAll = 0;

    for (auto settingItem : changedList) {
        if (IsSettingItemRequireRestartToApply(settingItem)) {
            neededRestartToApplyAll |= !settingItem->whatsThis().compare("WxBoxServerPort") ? 2 : 1;
        }

        // apply change
        ApplyChanged(settingItem, confirm);
    }

    if (neededRestartToApplyAll) {
        QString msg = Translate("Need Restart WxBox To Apply All Change");
        if (neededRestartToApplyAll & 2) {
            msg = QString("<center>%1</center>(<font color=\"red\">%2</font>)").arg(msg).arg(Translate("Change WxBox Server Port Must Restart WxBox. Otherwise, The Newly WxBox Client Cannot Be Connected"));
        }

        xstyle::warning(this, "", msg);
    }

    ui->btnApply->setEnabled(false);
    changedList.clear();
}

void WxBoxSettingDialog::ApplyChanged(QWidget* settingItem, bool confirm)
{
    QVariant newValue        = UpdateCurrentSettingValue(settingItem, confirm);
    QString  settingItemName = settingItem->whatsThis();

    if (!settingItemName.compare("Language")) {
        auto index = newValue.value<int>();
        if (!i18ns.empty() && index < (int)i18ns.size()) {
            newValue = i18ns.at(index).first;
        }
    }
    else if (!settingItemName.compare("Theme")) {
        auto index = newValue.value<int>();
        if (index <= 0) {
            newValue = "";
        }
        else if (!themes.empty() && --index < (int)themes.size()) {
            newValue = themes.at(index);
        }
    }
    else if (!settingItemName.compare("AlwaysTopMost") && !confirm) {
        TurnAlwaysTopMost(newValue.value<bool>());
    }

    if (handler) {
        handler(settingItemName, newValue);
    }
}

void WxBoxSettingDialog::SettingChanged(QWidget* settingItem, const QVariant& newValue)
{
    if (!waitingLoop) {
        return;
    }

    if (settingItem == ui->lineEditWxBoxServerPort) {
        ui->lineEditWxBoxServerURI->setText("localhost:" + ui->lineEditWxBoxServerPort->text());
    }

    // check whether value is changed
    if (settingItem->property(SETTING_VALUE_KEY).compare(newValue)) {
        // add to changed list
        if (changedList.find(settingItem) == changedList.end()) {
            changedList.insert(settingItem);
        }
    }
    else {
        changedList.erase(settingItem);
    }

    ui->btnApply->setEnabled(!changedList.empty());
}