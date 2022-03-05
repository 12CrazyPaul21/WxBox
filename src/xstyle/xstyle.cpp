#include <xstyle/xstyle.h>

//
// xstyle i18n
//

QString xstyle::XStyleManager::CurrentLanguage()
{
    std::shared_lock<std::shared_mutex> lock(mutexXStyleGlobal);
    return currentLanguage;
}

QString xstyle::XStyleManager::i18nFolder()
{
    std::shared_lock<std::shared_mutex> lock(mutexXStyleGlobal);
    return i18nFolderPath;
}

void xstyle::XStyleManager::RegisterI18nFolder(const QString& path)
{
    std::lock_guard<std::shared_mutex> lock(mutexXStyleGlobal);
    i18nFolderPath = path;
}

std::vector<std::pair<QString, QString>> xstyle::XStyleManager::i18ns()
{
    std::vector<std::pair<QString, QString>> result;
    QString                                  _i18nPath = i18nFolder();

    if (_i18nPath.isEmpty() || !QFile::exists(_i18nPath)) {
        return result;
    }

    for (auto fileInfo : QDir(_i18nPath).entryInfoList(QStringList("*.qm"), QDir::Files | QDir::Readable)) {
        QTranslator trans;
        if (trans.load(fileInfo.absoluteFilePath())) {
            result.emplace_back(std::make_pair<QString, QString>(fileInfo.baseName(), trans.translate("xstyle_meta", "English")));
        }
    }

    return result;
}

void xstyle::XStyleManager::ChangeLanguage(const QString& language)
{
    std::lock_guard<std::shared_mutex> lock(mutexXStyleGlobal);

    if (!currentLanguage.compare(language)) {
        return;
    }

    QString languagePath = QDir(i18nFolderPath).absoluteFilePath(language + ".qm");
    if (!QFile::exists(languagePath)) {
        return;
    }

    if (translator.load(languagePath)) {
        qApp->installTranslator(&translator);
    }

    currentLanguage = language;
}

QString xstyle::XStyleManager::Translate(const QString& context, const QString& key)
{
    return QCoreApplication::translate(context.toLocal8Bit(), key.toLocal8Bit());
}

//
// xstyle theme
//

QString xstyle::XStyleManager::CurrentTheme()
{
    std::shared_lock<std::shared_mutex> lock(mutexXStyleGlobal);
    return currentTheme;
}

QString xstyle::XStyleManager::ThemeFolder()
{
    std::shared_lock<std::shared_mutex> lock(mutexXStyleGlobal);
    return themeFolderPath;
}

void xstyle::XStyleManager::RegisterThemeFolder(const QString& path)
{
    std::lock_guard<std::shared_mutex> lock(mutexXStyleGlobal);
    themeFolderPath = path;
}

std::vector<QString> xstyle::XStyleManager::ThemeList()
{
    std::vector<QString> themeList;
    QString              _themeFolder = ThemeFolder();

    if (_themeFolder.isEmpty() || !QFile::exists(_themeFolder)) {
        return themeList;
    }

    for (auto fileInfo : QDir(_themeFolder).entryInfoList(QStringList("*.rcc"), QDir::Files | QDir::Readable)) {
        themeList.emplace_back(fileInfo.baseName());
    }

    return themeList;
}

void xstyle::XStyleManager::RegisterDefaultTheme(const QString& themeUrl)
{
    std::lock_guard<std::shared_mutex> lock(mutexXStyleGlobal);
    defaultThemeUrl   = themeUrl;
    currentStyleSheet = defaultThemeStyleSheet = GenerateDefaultStyle(themeUrl);
}

QString xstyle::XStyleManager::GetDefaultThemeUrl()
{
    std::shared_lock<std::shared_mutex> lock(mutexXStyleGlobal);
    return defaultThemeUrl;
}

QString xstyle::XStyleManager::GenerateDefaultStyle(const QString& themeUrl)
{
    QString defaultStyleSheet =
        QString(R"(
.xstylewindow {
    qproperty-title_height: %1;
    qproperty-footer_height: %2;
    qproperty-title_button_width: %12;
    qproperty-title_button_height: %13;
    qproperty-reserve_border_width: %3;
    qproperty-reserve_shadow_width: %4;
    qproperty-shadow_color: %5;
    qproperty-border_radius: %10;
    qproperty-window_fade_out_duration: %11;
}
		
.xstylewindow_container {
    background: %6;
}

.xstylewindow_title_panel {
    background: %7;
}

.xstylewindow_title_label {
    font: normal %19 %20px "%21";
    color: %22;
}

.xstylewindow_body_container {
    background: %8;
}

.xstylewindow_footer_panel {
    background: %9;
}
		
.xstylewindow_title_button {
    border: %17px solid transparent;
    border-radius: 0px;
}

.xstylewindow_title_button:hover {
    border:%17px solid %18;
}

.xstylewindow_min_button {
    qproperty-icon_url: "%14";
}

.xstylewindow_max_button {
    qproperty-icon_url: "%15";
}

.xstylewindow_close_button {
    qproperty-icon_url: "%16";
}

.xstylemessagebox_label_message {
	font: normal 16px "Microsoft YaHei";
    color: white;
}

QMenu {
    background-color: rgba(0, 0, 26, 200);
    color: white;
    border: 1px solid rgba(245, 245, 240, 240);
    border-radius: 1px;
    padding: 0px;
}

QMenu::separator {
    height: 1px;
    background: white;
	margin: 1px;
}

QMenu::item {
    background-color: transparent;
}

QMenu::item:selected {
    background-color: rgba(175, 175, 182, 120);
}

QMenu::item:disabled {
    color: gray;
}
	)")
            .arg(DEFAULT_XSTYLE_WINDOW_TITLE_HEIGHT)
            .arg(DEFAULT_XSTYLE_WINDOW_FOOTER_HEIGHT)
            .arg(DEFAULT_XSTYLE_WINDOW_RESERVE_BORDER_WIDTH)
            .arg(DEFAULT_XSTYLE_WINDOW_RESERVE_SHADOW_WIDTH)
            .arg(DEFAULT_XSTYLE_WINDOW_SHADOW_COLOR.name())
            .arg(DEFAULT_XSTYLE_WINDOW_CONTAINER_BACKGROUND)
            .arg(DEFAULT_XSTYLE_WINDOW_TITLE_BACKGROUND)
            .arg(DEFAULT_XSTYLE_WINDOW_BODY_CONTAINER_BACKGROUND)
            .arg(DEFAULT_XSTYLE_WINDOW_FOOTER_BACKGROUND)
            .arg(DEFAULT_XSTYLE_WINDOW_BORDER_RADIUS)
            .arg(DEFAULT_XSTYLE_WINDOW_FADE_OUT_DURATION)
            .arg(DEFAULT_XSTYLE_WINDOW_TITLE_BUTTON_WIDTH)
            .arg(DEFAULT_XSTYLE_WINDOW_TITLE_BUTTON_HEIGHT)
            .arg(STANDARD_XSTYLE_WINDOW_TITLE_MIN_BUTTON_ICON)
            .arg(STANDARD_XSTYLE_WINDOW_TITLE_MAX_BUTTON_ICON)
            .arg(STANDARD_XSTYLE_WINDOW_TITLE_CLOSE_BUTTON_ICON)
            .arg(DEFAULT_XSTYLE_WINDOW_TITLE_BUTTON_BORDER_WIDTH)
            .arg(DEFAULT_XSTYLE_WINDOW_TITLE_BUTTON_BORDER_COLOR)
            .arg(DEFAULT_XSTYLE_WINDOW_TITLE_LABEL_FONT_WEIGHT)
            .arg(DEFAULT_XSTYLE_WINDOW_TITLE_LABEL_FONT_SIZE)
            .arg(DEFAULT_XSTYLE_WINDOW_TITLE_LABEL_FONT_FAMILY)
            .arg(DEFAULT_XSTYLE_WINDOW_TITLE_LABEL_FONT_COLOR);

    if (!themeUrl.isEmpty() && QFile::exists(themeUrl)) {
        defaultStyleSheet += LoadTheme(themeUrl);
    }

    return defaultStyleSheet;
}

QString xstyle::XStyleManager::DefaultStyle()
{
    {
        std::shared_lock<std::shared_mutex> lock(mutexXStyleGlobal);
        if (!defaultThemeStyleSheet.isEmpty()) {
            return defaultThemeStyleSheet;
        }
    }

    RegisterDefaultTheme(defaultThemeUrl);

    {
        std::shared_lock<std::shared_mutex> lock(mutexXStyleGlobal);
        return defaultThemeStyleSheet;
    }
}

QString xstyle::XStyleManager::StyleSheet()
{
    {
        std::shared_lock<std::shared_mutex> lock(mutexXStyleGlobal);
        if (!currentStyleSheet.isEmpty()) {
            return currentStyleSheet;
        }
    }

    RegisterDefaultTheme(defaultThemeUrl);

    {
        std::shared_lock<std::shared_mutex> lock(mutexXStyleGlobal);
        return currentStyleSheet;
    }
}

QString xstyle::XStyleManager::LoadTheme(const QString& themeQssUrl)
{
    if (!QFile::exists(themeQssUrl)) {
        return "";
    }

    // open xstyle theme qss file
    QFile qssFile(themeQssUrl);
    if (!qssFile.open(QIODevice::ReadOnly)) {
        return "";
    }

    // check qss file size, must be less than 1MB
    if (qssFile.size() > XSTYLE_THEME_QSS_MUST_BE_LESS_THAN_1MB) {
        qssFile.close();
        return "";
    }

    // check whether valid
    QTextStream stream(&qssFile);
    QString     xstyleHeader = stream.readLine();
    if (xstyleHeader.compare(XSTYLE_THEME_QSS_HEADER)) {
        qssFile.close();
        return "";
    }

    // read all style
    QString styleSheet = stream.readAll();
    qssFile.close();
    return styleSheet;
}

QString xstyle::XStyleManager::LoadThemeByName(const QString& themeName)
{
    return LoadTheme(QString(":/XStyleTheme/%1/%1.qss").arg(themeName));
}

bool xstyle::XStyleManager::RegisterThemeResource(const QString& themeName)
{
    if (themeResourceMaps.find(themeName) != themeResourceMaps.end()) {
        return true;
    }

    QString rccPath = QString("%1%2%3.rcc").arg(themeFolderPath).arg(QDir::separator()).arg(themeName);
    if (!QFile::exists(rccPath)) {
        return false;
    }

    if (!QResource::registerResource(rccPath)) {
        return false;
    }

    themeResourceMaps[themeName] = rccPath;
    return true;
}

void xstyle::XStyleManager::UnRegisterThemeResource(const QString& themeName)
{
    auto it = themeResourceMaps.find(themeName);
    if (it == themeResourceMaps.end()) {
        return;
    }

    QResource::unregisterResource(it->second);
    themeResourceMaps.erase(it);
}

bool xstyle::XStyleManager::ChangeTheme(const QString& themeName)
{
    std::lock_guard<std::shared_mutex> lock(mutexXStyleGlobal);

    if (!currentTheme.compare(themeName)) {
        return true;
    }

    // change to default theme
    if (themeName.isEmpty()) {
        if (!currentTheme.isEmpty()) {
            UnRegisterThemeResource(currentTheme);
        }

        currentStyleSheet = defaultThemeStyleSheet;
        currentTheme      = "";
        emit themeChanged(currentTheme, currentStyleSheet);
        return true;
    }

    // register theme
    if (!RegisterThemeResource(themeName)) {
        return false;
    }

    // load theme style sheet
    QString themeStyleSheet = defaultThemeStyleSheet + LoadThemeByName(themeName);

    // unregister previous theme
    if (!currentTheme.isEmpty()) {
        UnRegisterThemeResource(currentTheme);
    }

    // update
    currentTheme      = themeName;
    currentStyleSheet = themeStyleSheet;
    emit themeChanged(currentTheme, currentStyleSheet);

    return true;
}

//
// xstyle helper
//

QPixmap xstyle::RotatePixmap(const QPixmap& pixmap, qreal angle)
{
    QPixmap rotatedPixmap(pixmap.size());
    rotatedPixmap.fill(Qt::transparent);

    qreal halfWidth  = pixmap.width() / 2.0;
    qreal halfHeight = pixmap.height() / 2.0;

    QPainter painter(&rotatedPixmap);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.translate(halfWidth, halfHeight);
    painter.rotate(angle);
    painter.translate(-halfWidth, -halfHeight);
    painter.drawPixmap(0, 0, pixmap);
    painter.end();

    return rotatedPixmap;
}