#ifndef __X_STYLE_WINDOW_H
#define __X_STYLE_WINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QOperatingSystemVersion>
#include <QLayout>
#include <QFrame>
#include <QPushButton>
#include <QLabel>
#include <QCloseEvent>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QPainter>
#include <QtMath>
#include <QDateTime>
#include <QResource>
#include <QFileInfo>
#include <QTextStream>
#include <QDir>
#include <QTranslator>
#include <QToolTip>
#include <QThread>
#include <QDesktopWidget>
#include <QScreen>
#include <QPixmapCache>

#include <vector>
#include <unordered_map>

#if _WIN32
#include <QtWin>
#include <Windows.h>

#define WM_XSTYLE_WAKE_UP WM_USER + 0x502

#endif

#include <xstyle/xstyle.h>
#include <xstyle/xstylebutton.hpp>
#include <qt_shadow_helper.hpp>

enum class WindowLoadingIconType
{
    DontUse = 0,
    OnlyTitleIcon,
    BothTitleIconAndTaskbarIcon
};

//
// XStyleWindow
//

#define RegisterXStyleWindowProperty(Type, PropertyName, MemberName, Suffix, DefaultValue) \
    DefineXStyleProperty(Type, PropertyName, MemberName, Suffix, DefaultValue)             \
        Q_PROPERTY(Type PropertyName READ Get##Suffix WRITE Set##Suffix DESIGNABLE true SCRIPTABLE true)

#define RegisterXStyleWindowPropertyWithOutSetter(Type, PropertyName, MemberName, Suffix, DefaultValue) \
    DefineXStylePropertyWithOutSetter(Type, PropertyName, MemberName, Suffix, DefaultValue)             \
        Q_PROPERTY(Type PropertyName READ Get##Suffix WRITE Set##Suffix DESIGNABLE true SCRIPTABLE true)

class XStyleWindow : public QMainWindow
{
    Q_OBJECT

  public:
    //
    // custom property for style
    //

    RegisterXStyleWindowProperty(int, title_height, titleHeight, TitleHeight, DEFAULT_XSTYLE_WINDOW_TITLE_HEIGHT);
    RegisterXStyleWindowProperty(int, footer_height, footerHeight, FooterHeight, DEFAULT_XSTYLE_WINDOW_FOOTER_HEIGHT);
    RegisterXStyleWindowProperty(int, window_icon_size, windowIconSize, WindowIconSize, DEFAULT_XSTYLE_WINDOW_WINDOW_ICON_SIZE);
    RegisterXStyleWindowProperty(QString, window_icon_url, windowIconUrl, WindowIconUrl, DEFAULT_XSTYLE_WINDOW_WINDOW_ICON_URL);
    RegisterXStyleWindowProperty(int, title_button_width, titleButtonWidth, TitleButtonWidth, DEFAULT_XSTYLE_WINDOW_TITLE_BUTTON_WIDTH);
    RegisterXStyleWindowProperty(int, title_button_height, titleButtonHeight, TitleButtonHeight, DEFAULT_XSTYLE_WINDOW_TITLE_BUTTON_HEIGHT);
    RegisterXStyleWindowProperty(int, reserve_border_width, reserveBorderWidth, ReserveBorderWidth, DEFAULT_XSTYLE_WINDOW_RESERVE_BORDER_WIDTH);
    RegisterXStyleWindowProperty(int, border_radius, borderRadius, BorderRadius, DEFAULT_XSTYLE_WINDOW_BORDER_RADIUS);
    RegisterXStyleWindowProperty(int, reserve_shadow_width, reserveShadowWidth, ReserveShadowWidth, DEFAULT_XSTYLE_WINDOW_RESERVE_SHADOW_WIDTH);
    RegisterXStyleWindowProperty(QColor, shadow_color, shadowColor, ShadowColor, DEFAULT_XSTYLE_WINDOW_SHADOW_COLOR);
    RegisterXStyleWindowProperty(int, window_fade_out_duration, msWindowFadeOutDuration, WindowFadeOutDuration, DEFAULT_XSTYLE_WINDOW_FADE_OUT_DURATION);
    RegisterXStyleWindowProperty(QColor, window_inactive_overlay_color, windowInActiveOverlayColor, WindowInActiveOverlayColor, DEFAULT_XSTYLE_WINDOW_INACTIVE_OVERLAY_COLOR);

    RegisterXStyleWindowProperty(int, window_loading_icon_rotate_duration, windowLoadingIconRotateDuration, WindowLoadingIconRotateDuration, DEFAULT_XSTYLE_WINDOW_WINDOW_LOADING_ICON_ROTATE_DURATION);
    RegisterXStyleWindowPropertyWithOutSetter(QString, window_loading_icon_url, windowLoadingIconUrl, WindowLoadingIconUrl, DEFAULT_XSTYLE_WINDOW_WINDOW_LOADING_ICON_URL);
    RegisterXStyleWindowPropertyWithOutSetter(int, window_loading_icon_rotation, windowLoadingIconRotation, WindowLoadingIconRotation, 0);

  public:
    explicit XStyleWindow(const QString& name, QWidget* xstyleParent = nullptr, bool deleteWhenClose = false);
    ~XStyleWindow();

    void changeEvent(QEvent* event);
    void paintEvent(QPaintEvent* event);
    void closeEvent(QCloseEvent* evnet);
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

    Q_INVOKABLE void show()
    {
        SetMinimizeButtonEnabled(true);
        setWindowModality(Qt::NonModal);
        SyncParentAlwaysTopMost();

        if (!isHidden()) {
            showNormal();
            activateWindow();
            return;
        }

        QMainWindow::show();
        QApplication::processEvents();

        fadeIn();
        CompleteShow();
    }

    Q_INVOKABLE void showApplicationModal()
    {
        SetMinimizeButtonEnabled(false);
        setWindowModality(Qt::ApplicationModal);
        SyncParentAlwaysTopMost();

        if (!isHidden()) {
            showNormal();
            activateWindow();
            return;
        }

        setWindowOpacity(0);
        QMainWindow::show();
        QApplication::processEvents();

        if (xstyleParent) {
            QPoint pt = xstyleParent->mapToGlobal(QPoint(0, 0));
            move(pt.x() + (xstyleParent->width() - width()) / 2, pt.y() + (xstyleParent->height() - height()) / 2);
        }

        fadeIn(true);
        CompleteShow();
    }

    Q_INVOKABLE void quit()
    {
        if (!closeIsMinimizeToTray) {
            emit this->close();
            return;
        }

        if (CanClose()) {
            fadeOut(true);
            hide();
            qApp->quit();
        }
    }

    Q_INVOKABLE void CenterInDesktop()
    {
        auto desktopCenter = QGuiApplication::screenAt(QCursor::pos())->availableGeometry().center();
        move(desktopCenter.x() - width() * 0.5, desktopCenter.y() - height() * 0.5);
    }

    Q_INVOKABLE void fadeIn(bool force = false)
    {
        if (!isFadeOut && !force) {
            return;
        }
        isFadeOut = false;

        QPropertyAnimation* fadeInAnimation = new QPropertyAnimation(this, "windowOpacity", this);
        fadeInAnimation->setDuration(GetWindowFadeOutDuration());
        fadeInAnimation->setStartValue(0);
        fadeInAnimation->setEndValue(xstyleOpacity);
        fadeInAnimation->setEasingCurve(QEasingCurve::InQuad);
        fadeInAnimation->start();

        while (fadeInAnimation->state() != QAbstractAnimation::Stopped) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }

        delete fadeInAnimation;
    }

    Q_INVOKABLE void fadeOut(bool force = false)
    {
        if (isFadeOut && !force) {
            return;
        }
        isFadeOut = true;

        QPropertyAnimation* fadeOutAnimation = new QPropertyAnimation(this, "windowOpacity", this);
        fadeOutAnimation->setDuration(GetWindowFadeOutDuration());
        fadeOutAnimation->setStartValue(xstyleOpacity);
        fadeOutAnimation->setEndValue(0);
        fadeOutAnimation->setEasingCurve(QEasingCurve::OutQuad);
        fadeOutAnimation->start();

        while (fadeOutAnimation->state() != QAbstractAnimation::Stopped) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }

        delete fadeOutAnimation;
    }

    Q_INVOKABLE void SetNativeDragWindowMode(bool nativeMode)
    {
        nativeDragMode = nativeMode;
    }

    Q_INVOKABLE void SetWindowTitle(const QString& title)
    {
        if (this->labelWindowTitle) {
            this->labelWindowTitle->setText(title);
        }
    }

    Q_INVOKABLE void SetWindowTitleVisible(bool enabled)
    {
        if (this->labelWindowTitle) {
            this->labelWindowTitle->setVisible(enabled);
        }
    }

    Q_INVOKABLE void SetWindowIconVisible(bool enabled)
    {
        if (this->labelWindowIcon) {
            this->labelWindowIcon->setVisible(enabled);
        }
    }

    Q_INVOKABLE void SetMinimizeButtonEnabled(bool enabled)
    {
        if (this->btnMin) {
            btnMin->setEnabled(enabled);
            setWindowFlag(Qt::WindowMinimizeButtonHint, enabled);
        }
    }

    Q_INVOKABLE void SetCloseButtonEnabled(bool enabled)
    {
        if (this->btnClose) {
            this->btnClose->setEnabled(enabled);
            setWindowFlag(Qt::WindowCloseButtonHint, enabled);
            setWindowFlag(Qt::WindowSystemMenuHint, enabled);
        }
    }

    Q_INVOKABLE void SetXStyleWindowOpacity(qreal level)
    {
        xstyleOpacity = level;
        setWindowOpacity(xstyleOpacity);
    }

    Q_INVOKABLE virtual void TurnCloseIsMinimizeToTray(bool toTray)
    {
        closeIsMinimizeToTray = toTray;
    }

    Q_INVOKABLE void SyncParentAlwaysTopMost()
    {
        if (!xstyleParent) {
            return;
        }

        TurnAlwaysTopMost(
#if _WIN32
            GetWindowLong((HWND)(xstyleParent->winId()), GWL_EXSTYLE) & WS_EX_TOPMOST
#else
            xstyleParent->windowFlags() & Qt::WindowStaysOnTopHint
#endif
        );
    }

    Q_INVOKABLE bool IsAlwaysTopMost()
    {
#if _WIN32
        return GetWindowLong((HWND)winId(), GWL_EXSTYLE) & WS_EX_TOPMOST;
#else
        return windowFlags() & Qt::WindowStaysOnTopHint;
#endif
    }

    Q_INVOKABLE virtual void TurnAlwaysTopMost(bool enabled)
    {
        if (IsAlwaysTopMost() == enabled) {
            return;
        }

#if _WIN32
        SetWindowPos((HWND)winId(), enabled ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#else
        setWindowFlag(Qt::WindowStaysOnTopHint, enabled);
#endif
    }

    //
    // i18n
    //

    QString Translate(const QString& key)
    {
        return QCoreApplication::translate(objectName().toLocal8Bit(), key.toLocal8Bit());
    }

    QStringList TranslateStringList(const QStringList& strs)
    {
        QStringList result;
        for (auto str : strs) {
            result.append(Translate(str));
        }
        return result;
    }

    virtual void RetranslateUi()
    {
    }

    //
    // style
    //

    void ApplyTheme(bool extractXStyle = false);

    inline void SetStyleSheetSync(const QString& styleSheet)
    {
        setStyleSheet(styleSheet);
        QApplication::processEvents();
    }

    inline void AddStyle(const QString& styleSheet)
    {
        SetStyleSheetSync(this->styleSheet() + styleSheet);
    }

    //
    // mission
    //

    virtual void BeginMission()
    {
        IgnoreForClose();

        if (missionCounter.ref() && missionCounter == 1) {
            emit missionBegined();
            QCoreApplication::processEvents();
        }
    }

    virtual void CloseMission()
    {
        if (!missionCounter.deref()) {
            emit missionClosed();
            QCoreApplication::processEvents();
        }

        ReadyForClose();
    }

    Q_INVOKABLE WindowLoadingIconType GetWindowLoadingIconType() const
    {
        return windowLoadingIconType;
    }

    Q_INVOKABLE void SetWindowLoadingIconType(WindowLoadingIconType type)
    {
        windowLoadingIconType = type;
    }

    Q_INVOKABLE bool IsUseLoadingIconAnimationCache() const
    {
        return useLoadingIconAnimationCache;
    }

    Q_INVOKABLE void SetUseLoadingIconAnimationCache(bool useCache)
    {
        useLoadingIconAnimationCache = useCache;
    }

    Q_INVOKABLE void SetWindowLoadingIconUrl(QString url)
    {
        if (!QFile(url).exists()) {
            return;
        }
        loadingIcon = QIcon(url);
        loadingIconAnimationPixmapBuffer.clear();
    }

    Q_INVOKABLE void SetWindowLoadingIconRotation(int rotation)
    {
        windowLoadingIconRotation = rotation;
        if (missionCounter && windowLoadingIconType != WindowLoadingIconType::DontUse) {
            UpdateLoadingIcon(rotation);
        }
    }

  protected:
    void SetupXStyleTheme();
    void SetupXStyleLayout();
    void SetupXStyleWindow();
    void RecalcWindowFixedSize();
    void RelayoutTitlePanel();
    void InitPlatformRelatedFeature();

    template<typename XStyleBodyUi>
    void SetupXStyleUi(XStyleBodyUi ui = nullptr)
    {
        SetupXStyleTheme();
        SetupXStyleLayout();

        if (ui) {
            ui->setupUi(this->bodyContainer);
        }

        SetupXStyleWindow();

#if _WIN32
        SetNativeDragWindowMode(true);
#endif
    }

    void SetupXStyleEmptyUi()
    {
        SetupXStyleTheme();
        SetupXStyleLayout();
        SetupXStyleWindow();

#if _WIN32
        SetNativeDragWindowMode(true);
#endif
    }

    bool eventFilter(QObject* obj, QEvent* e) override;
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
#if _WIN32
    bool nativeEvent_Windows(const QByteArray& eventType, void* message, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, long* result);
    void EnabledWindowCaption(bool bEnabled);
    void DisableSystemMenuCloseItem();
#endif

    virtual void CompleteShow();
    virtual bool BeforeClose();
    virtual void MinimizedToTray();
    virtual void AfterThemeChanged(const QString& themeName);
    virtual void WindowActiveChanged(bool isActive);
    virtual bool DraggableAreaHitTest(int x, int y);

    //
    // close
    //

    Q_INVOKABLE void IgnoreForClose();
    Q_INVOKABLE void ReadyForClose(const bool canCloseWhenZero = true);
    Q_INVOKABLE bool CanClose();

    //
    // loading icon rotation animation
    //

    void BeginLoadingIconRotate()
    {
        if (loadingIconAnimation || windowLoadingIconType == WindowLoadingIconType::DontUse) {
            return;
        }

        windowLoadingIconRotation = 0;

        loadingIconAnimation = new QPropertyAnimation(this, "window_loading_icon_rotation", this);
        loadingIconAnimation->setDuration(windowLoadingIconRotateDuration);
        loadingIconAnimation->setStartValue(0);
        loadingIconAnimation->setEndValue(360);
        loadingIconAnimation->setEasingCurve(QEasingCurve::Linear);
        loadingIconAnimation->setLoopCount(-1);
        loadingIconAnimation->start(QPropertyAnimation::KeepWhenStopped);
    }

    void EndLoadingIconRotate()
    {
        if (!loadingIconAnimation) {
            return;
        }

        loadingIconAnimation->stop();
        while (loadingIconAnimation->state() != QAbstractAnimation::Stopped) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
        delete loadingIconAnimation;
        loadingIconAnimation = nullptr;

        WindowActiveChanged(isActiveWindow());
        if (windowLoadingIconType == WindowLoadingIconType::BothTitleIconAndTaskbarIcon) {
            setWindowIcon(windowTitleIcon.pixmap(labelWindowIcon->width(), labelWindowIcon->height(), QIcon::Normal, QIcon::On));
        }
    }

    void UpdateLoadingIcon(int angle)
    {
        if (windowLoadingIconType == WindowLoadingIconType::DontUse) {
            return;
        }

        QPixmap loadingPixmap;

        // QString pixmapId = QString("%1_loading_icon_angle_%2").arg(objectName()).arg(angle);
        // if (!QPixmapCache::find(pixmapId, &loadingPixmap)) {
        //     loadingPixmap = xstyle::RotatePixmap((loadingIcon.isNull() ? windowTitleIcon : loadingIcon).pixmap(labelWindowIcon->size(), QIcon::Normal, QIcon::On), windowLoadingIconRotation);
        //     QPixmapCache::insert(pixmapId, loadingPixmap);
        // }

        if (useLoadingIconAnimationCache) {
            if (loadingIconAnimationPixmapBuffer.find(angle) == loadingIconAnimationPixmapBuffer.end()) {
                loadingPixmap                           = xstyle::RotatePixmap((loadingIcon.isNull() ? windowTitleIcon : loadingIcon).pixmap(labelWindowIcon->size(), QIcon::Normal, QIcon::On), angle);
                loadingIconAnimationPixmapBuffer[angle] = loadingPixmap;
            }
            else {
                loadingPixmap = loadingIconAnimationPixmapBuffer[angle];
            }
        }
        else {
            loadingPixmap = xstyle::RotatePixmap((loadingIcon.isNull() ? windowTitleIcon : loadingIcon).pixmap(labelWindowIcon->size(), QIcon::Normal, QIcon::On), angle);
        }

        if (labelWindowIcon) {
            labelWindowIcon->setPixmap(loadingPixmap);
        }
        if (windowLoadingIconType == WindowLoadingIconType::BothTitleIconAndTaskbarIcon) {
            setWindowIcon(loadingPixmap);
        }
    }

  signals:
    void closed();
    void missionBegined();
    void missionClosed();

  public slots:
    virtual void ThemeChanged(QString themeName, QString styleSheet);

    virtual void OnBeginMission()
    {
        BeginLoadingIconRotate();
    }

    virtual void OnCloseMission()
    {
        EndLoadingIconRotate();
    }

  protected:
    // window property
    QWidget* xstyleParent;
    QString  winName;
    bool     nativeDragMode;
    qreal    xstyleOpacity;
    bool     closeIsMinimizeToTray;

    // flags
    bool   inited;
    bool   isThemeDirty;
    bool   inDraging;
    QPoint ptBeginDrag;

    // close flags
    bool                    isFadeOut;
    bool                    wantToClose;
    QAtomicInteger<int32_t> readyForCloseCounter;
    qint64                  lastClickIconTimeStamp;

    // mission flags
    QAtomicInteger<int32_t>          missionCounter;
    WindowLoadingIconType            windowLoadingIconType;
    QIcon                            loadingIcon;
    QPropertyAnimation*              loadingIconAnimation;
    bool                             useLoadingIconAnimationCache;
    std::unordered_map<int, QPixmap> loadingIconAnimationPixmapBuffer;

    // shadow
    QPixmap shadowBuffer;
    QPixmap inActiveShadowBuffer;

    // inactive overlay
    QString inActiveTitleColorDesc;

    // window icon
    QIcon windowTitleIcon;

    // ui components
    QWidget*      container;
    QVBoxLayout*  containerLayout;
    QFrame*       titlePanel;
    QHBoxLayout*  titlePanelLayout;
    QLabel*       labelWindowIcon;
    QLabel*       labelWindowTitle;
    QHBoxLayout*  titleOperationLayout;
    XStyleButton* btnMin;
    XStyleButton* btnMax;
    XStyleButton* btnClose;
    QFrame*       bodyContainer;
    QFrame*       footerPanel;
};

#endif  // __X_STYLE_WINDOW_H