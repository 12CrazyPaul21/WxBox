#include <xstyle/xstylewindow.h>

//
// XStyleWindow
//

XStyleWindow::XStyleWindow(const QString& name, QWidget* xstyleParent, bool deleteWhenClose)
  : QMainWindow(nullptr)
  , xstyleParent(xstyleParent)
  , winName(name)
  , nativeDragMode(false)
  , xstyleOpacity(1.0f)
  , closeIsMinimizeToTray(false)
  , inited(false)
  , isThemeDirty(true)
  , inDraging(false)
  , ptBeginDrag(0, 0)
  , isFadeOut(false)
  , wantToClose(false)
  , readyForCloseCounter(0)
  , lastClickIconTimeStamp(0)
  , inActiveTitleColorDesc("")
  , container(nullptr)
  , containerLayout(nullptr)
  , titlePanel(nullptr)
  , titlePanelLayout(nullptr)
  , labelWindowIcon(nullptr)
  , labelWindowTitle(nullptr)
  , titleOperationLayout(nullptr)
  , btnMin(nullptr)
  , btnMax(nullptr)
  , btnClose(nullptr)
  , bodyContainer(nullptr)
  , footerPanel(nullptr)
{
    if (deleteWhenClose) {
        setAttribute(Qt::WA_DeleteOnClose);
    }

    setObjectName(name + "_xstylewindow");
    setProperty("class", "xstylewindow");
    installEventFilter(this);
}

XStyleWindow::~XStyleWindow()
{
}

void XStyleWindow::SetupXStyleTheme()
{
    SetStyleSheetSync(xstyle_manager.StyleSheet());
    isThemeDirty = true;
}

void XStyleWindow::SetupXStyleLayout()
{
    // default window size
    resize(DEFAULT_XSTYLE_WINDOW_WIDTH, DEFAULT_XSTYLE_WINDOW_HEIGHT);

    //
    // container for the entire window
    //

    container = new QWidget(this);
    container->setObjectName(winName + QString::fromUtf8("_container"));
    container->setProperty("class", "xstylewindow_container");

    containerLayout = new QVBoxLayout(container);
    containerLayout->setSpacing(0);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setObjectName(winName + QString::fromUtf8("_container_layout"));
    containerLayout->setProperty("class", "xstylewindow_container_layout");

    //
    // title panel
    //

    titlePanel = new QFrame(container);
    titlePanel->setObjectName(winName + QString::fromUtf8("_title_panel"));
    titlePanel->setProperty("class", "xstylewindow_title_panel");
    titlePanel->setFixedHeight(GetTitleHeight());
    titlePanel->setLayout(new QHBoxLayout(titlePanel));

    // add icon, title label and operation layout
    labelWindowIcon = new QLabel(titlePanel);
    labelWindowIcon->setObjectName(winName + QString::fromUtf8("_window_icon"));
    labelWindowTitle = new QLabel(winName, titlePanel);
    labelWindowTitle->setObjectName(winName + QString::fromUtf8("_title_label"));
    labelWindowTitle->setProperty("class", "xstylewindow_title_label");
    titleOperationLayout = new QHBoxLayout();

    titlePanelLayout = dynamic_cast<QHBoxLayout*>(titlePanel->layout());
    titlePanelLayout->addWidget(labelWindowIcon);
    titlePanelLayout->addWidget(labelWindowTitle);
    titlePanelLayout->addItem(titleOperationLayout);

    // add min, max and close btn
    btnMin   = new XStyleButton("min", titlePanel);
    btnMax   = new XStyleButton("max", titlePanel);
    btnClose = new XStyleButton("close", titlePanel);
    titleOperationLayout->addWidget(btnMin);
    titleOperationLayout->addWidget(btnMax);
    titleOperationLayout->addWidget(btnClose);

    // set object and class
    QString winBtnClassName = winName + QString::fromUtf8("_title_button");
    btnMin->setObjectName(winName + QString::fromUtf8("_min_button"));
    btnMax->setObjectName(winName + QString::fromUtf8("_max_button"));
    btnClose->setObjectName(winName + QString::fromUtf8("_close_button"));
    btnMin->setProperty("class", winBtnClassName + QString::fromUtf8(" xstylewindow_titile_button xstylewindow_min_button"));
    btnMax->setProperty("class", winBtnClassName + QString::fromUtf8(" xstylewindow_titile_button xstylewindow_max_button"));
    btnClose->setProperty("class", winBtnClassName + QString::fromUtf8(" xstylewindow_titile_button xstylewindow_close_button"));

    // connect signal
    btnMax->setEnabled(false);
    connect(btnMin, SIGNAL(clicked()), this, SLOT(showMinimized()));
    connect(btnClose, SIGNAL(clicked()), this, SLOT(close()));

    //
    // body container
    //

    bodyContainer = new QFrame(container);
    bodyContainer->setObjectName(winName + QString::fromUtf8("_body_container"));
    bodyContainer->setProperty("class", "xstylewindow_body_container");
    bodyContainer->resize(DEFAULT_XSTYLE_WINDOW_WIDTH, DEFAULT_XSTYLE_WINDOW_HEIGHT);

    //
    // footer panel
    //

    footerPanel = new QFrame(container);
    footerPanel->setObjectName(winName + QString::fromUtf8("_footer_panel"));
    footerPanel->setProperty("class", "xstylewindow_footer_panel");
    footerPanel->setFixedHeight(GetFooterHeight());
    footerPanel->setLayout(new QHBoxLayout(footerPanel));

    containerLayout->addWidget(titlePanel);
    containerLayout->addWidget(bodyContainer);
    containerLayout->addWidget(footerPanel);

    this->setCentralWidget(container);
    QMetaObject::connectSlotsByName(this);
}

void XStyleWindow::SetupXStyleWindow()
{
    // enable some effect feature
    qApp->setEffectEnabled(Qt::UI_FadeTooltip, true);
    qApp->setEffectEnabled(Qt::UI_AnimateTooltip, true);
    qApp->setEffectEnabled(Qt::UI_AnimateMenu, false);
    qApp->setEffectEnabled(Qt::UI_FadeMenu, false);

    // config window styles
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
#if _WIN32
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10) {
        setWindowFlag(Qt::WindowSystemMenuHint);
    }
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);
#endif

    // config some attribute
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    //setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAutoFillBackground(false);

    //
    // set shadow[deprecated]
    // occur: UpdateLayeredWindowIndirect failed for ptDst=...
    //
    // shadow = new QGraphicsDropShadowEffect(this);
    // setGraphicsEffect(shadow);

    // apply theme
    ApplyTheme(true);

    // move to desktop center
    CenterInDesktop();

    // config platform releated
    InitPlatformRelatedFeature();

    // register xstyle widget
    xstyle_manager.Register(this);
}

void XStyleWindow::ApplyTheme(bool extractXStyle)
{
    // recalc window fixed size
    RecalcWindowFixedSize();

    // relayout title panel
    RelayoutTitlePanel();

    if (isThemeDirty && extractXStyle) {
        // extract xStyle
        AddStyle(QString(R"(
			#%1_container {
				margin: %2px;
				border-top-left-radius: %3;
				border-top-right-radius: %3;
				border-bottom-left-radius: %3;
				border-bottom-right-radius: %3;
			}

			#%1_title_panel {
				border-top-left-radius: %3;
				border-top-right-radius: %3;
			}

			#%1_footer_panel {
				border-bottom-left-radius: %3;
				border-bottom-right-radius: %3;
			}

			#%1_close_button {
				qproperty-right_top_radius: %3;
			}
		)")
                     .arg(winName)
                     .arg(reserveShadowWidth)
                     .arg(borderRadius));

        isThemeDirty = false;
    }

    //
    // load window icon
    //

    QPixmap originalPixmap;
    if (!windowIconUrl.isEmpty() && QFile::exists(windowIconUrl)) {
        originalPixmap = QPixmap(windowIconUrl);
    }
    else {
        originalPixmap = QApplication::style()->standardPixmap(QStyle::StandardPixmap::SP_FileIcon);
    }

    // make inactive icon pixmap
    auto    mask = originalPixmap.createMaskFromColor(Qt::transparent);
    QPixmap inactivePixmap(originalPixmap.size());
    inactivePixmap.fill(windowInActiveOverlayColor);
    inactivePixmap.setMask(mask);

    // add icon
    windowIcon = QIcon();
    windowIcon.addPixmap(originalPixmap, QIcon::Normal, QIcon::On);
    windowIcon.addPixmap(inactivePixmap, QIcon::Normal, QIcon::Off);

    if (labelWindowIcon) {
        labelWindowIcon->setPixmap(windowIcon.pixmap(labelWindowIcon->width(), labelWindowIcon->height(), QIcon::Normal, QIcon::On));
    }

    //
    // inactive style
    //

    inActiveTitleColorDesc = QString("color: rgba(%1, %2, %3, %4);").arg(windowInActiveOverlayColor.red()).arg(windowInActiveOverlayColor.green()).arg(windowInActiveOverlayColor.blue()).arg(windowInActiveOverlayColor.alpha());

    //
    // refresh button background
    //

    if (btnMin && btnMax && btnClose) {
        btnMin->SetupStyle();
        btnMax->SetupStyle();
        btnClose->SetupStyle();
    }

    //
    // apply shadow
    //

    // shadow->setBlurRadius(GetReserveShadowWidth());
    // shadow->setOffset(0);
    // shadow->setColor(GetShadowColor());

    if (reserveShadowWidth) {
        // active window shadow
        shadowBuffer = QPixmap::fromImage(Shadow::paint(this, rect(), reserveShadowWidth * 2, reserveShadowWidth, shadowColor));

        // inactive window shadow
        auto inActiveShadowColor = shadowColor;
        inActiveShadowColor.setAlpha(100);
        inActiveShadowBuffer = QPixmap::fromImage(Shadow::paint(this, rect(), reserveShadowWidth * 1.5, reserveShadowWidth, inActiveShadowColor));
    }

    // repaint
    repaint();
}

void XStyleWindow::RecalcWindowFixedSize()
{
    int fixedWidth = 0, fixedHeight = 0;

    if (!container || !bodyContainer) {
        return;
    }

    // get body container size
    QSize size = this->bodyContainer->size();
    fixedWidth += size.width();
    fixedHeight += size.height();

    // min width
    fixedWidth = std::max(fixedWidth, titleButtonWidth * 6);
    size.setWidth(fixedWidth);
    this->bodyContainer->setFixedSize(size);

    // add title and footer height
    fixedHeight += GetTitleHeight() + GetFooterHeight();

    // add reverse border and shadow width
    int reverseMargin = GetReserveBorderWidth() + GetReserveShadowWidth();
    this->container->setContentsMargins(QMargins(reverseMargin, reverseMargin, reverseMargin, reverseMargin));
    fixedWidth += reverseMargin * 2;
    fixedHeight += reverseMargin * 2;

    // set full window fixed size
    setFixedSize(QSize(fixedWidth, fixedHeight));
    this->container->setFixedSize(QSize(fixedWidth, fixedHeight));
}

void XStyleWindow::RelayoutTitlePanel()
{
    // update title panel height
    if (titlePanel) {
        titlePanel->setFixedHeight(GetTitleHeight());
    }

    // title : operationPanel = 8 : 1
    if (titlePanelLayout) {
        titlePanelLayout->setContentsMargins(QMargins(0, 0, 0, 0));
        titlePanelLayout->setStretch(0, 1);
        titlePanelLayout->setStretch(1, 6);
        titlePanelLayout->setStretch(2, 3);
        titlePanelLayout->setSpacing(1);
    }

    // set window icon layout
    if (labelWindowIcon) {
        labelWindowIcon->setMinimumSize(QSize(windowIconSize, windowIconSize));
        labelWindowIcon->setMaximumSize(QSize(windowIconSize, windowIconSize));
        labelWindowIcon->setContentsMargins(QMargins(4, 4, 4, 4));
        labelWindowIcon->setScaledContents(true);
        labelWindowIcon->installEventFilter(this);
    }

    if (labelWindowTitle) {
        // retain title label size when hidden
        QSizePolicy titleSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        titleSizePolicy.setRetainSizeWhenHidden(true);
        labelWindowTitle->setSizePolicy(titleSizePolicy);

        // set title label style
        labelWindowTitle->setContentsMargins(QMargins(5, 0, 5, 0));
    }

    // set title operation layout
    if (titleOperationLayout) {
        titleOperationLayout->setSpacing(0);
        titleOperationLayout->setAlignment(Qt::AlignTop);
        titleOperationLayout->setSizeConstraint(QLayout::SetFixedSize);
    }

    if (btnMin && btnMax && btnClose) {
        //
        // config button
        //

        QSizePolicy btnSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        btnSizePolicy.setHorizontalStretch(0);
        btnSizePolicy.setVerticalStretch(0);

        btnSizePolicy.setHeightForWidth(btnMin->sizePolicy().hasHeightForWidth());
        btnMin->setSizePolicy(btnSizePolicy);
        btnMin->setMinimumSize(QSize(titleButtonWidth, titleButtonHeight));
        btnMin->setMaximumSize(QSize(titleButtonWidth, titleButtonHeight));
        btnMin->installEventFilter(this);

        btnSizePolicy.setHeightForWidth(btnMax->sizePolicy().hasHeightForWidth());
        btnMax->setSizePolicy(btnSizePolicy);
        btnMax->setMinimumSize(QSize(titleButtonWidth, titleButtonHeight));
        btnMax->setMaximumSize(QSize(titleButtonWidth, titleButtonHeight));
        btnMax->installEventFilter(this);

        btnSizePolicy.setHeightForWidth(btnClose->sizePolicy().hasHeightForWidth());
        btnClose->setSizePolicy(btnSizePolicy);
        btnClose->setMinimumSize(QSize(titleButtonWidth, titleButtonHeight));
        btnClose->setMaximumSize(QSize(titleButtonWidth, titleButtonHeight));
        btnClose->installEventFilter(this);

        //
        // no focus
        //

        btnMin->setFocusPolicy(Qt::NoFocus);
        btnMax->setFocusPolicy(Qt::NoFocus);
        btnClose->setFocusPolicy(Qt::NoFocus);
    }
}

void XStyleWindow::InitPlatformRelatedFeature()
{
#if _WIN32
    if (!QtWin::isCompositionEnabled()) {
        return;
    }

    // turn on some aero effect(maximize and minimize transition)
    EnabledWindowCaption(true);
#endif
}

void XStyleWindow::CompleteShow()
{
    inited = true;
}

bool XStyleWindow::BeforeClose()
{
    return true;
}

void XStyleWindow::ThemeChanged(QString themeName, QString styleSheet)
{
    SetStyleSheetSync(styleSheet);
    isThemeDirty = true;
    ApplyTheme(true);
    AfterThemeChanged(themeName);
}

void XStyleWindow::AfterThemeChanged(const QString& themeName)
{
    Q_UNUSED(themeName)
}

void XStyleWindow::WindowActiveChanged(bool isActive)
{
    if (!labelWindowTitle || !labelWindowIcon) {
        return;
    }

    if (isActive) {
        labelWindowTitle->setStyleSheet("");
        labelWindowIcon->setPixmap(windowIcon.pixmap(labelWindowIcon->width(), labelWindowIcon->height(), QIcon::Normal, QIcon::On));
    }
    else {
        labelWindowTitle->setStyleSheet(inActiveTitleColorDesc);
        labelWindowIcon->setPixmap(windowIcon.pixmap(labelWindowIcon->width(), labelWindowIcon->height(), QIcon::Normal, QIcon::Off));
    }
}

void XStyleWindow::IgnoreForClose()
{
    readyForCloseCounter.ref();
}

void XStyleWindow::ReadyForClose(const bool canCloseWhenZero)
{
    if (readyForCloseCounter > 0) {
        readyForCloseCounter.deref();
    }

    if (wantToClose && readyForCloseCounter <= 0 && canCloseWhenZero) {
        if (closeIsMinimizeToTray) {
            quit();
        }
        else {
            emit close();
        }
    }
}

bool XStyleWindow::CanClose()
{
    wantToClose = true;

    if (inited) {
        inited = !BeforeClose();
    }

    return readyForCloseCounter <= 0 && inited == false;
}

void XStyleWindow::closeEvent(QCloseEvent* event)
{
    // minimize to tray
    if (closeIsMinimizeToTray) {
        fadeOut(true);
        hide();
        event->ignore();
        return;
    }

    if (!CanClose()) {
        event->ignore();
        return;
    }

    fadeOut();
    emit closed();
    event->accept();
}

void XStyleWindow::changeEvent(QEvent* event)
{
    switch (event->type()) {
        case QEvent::LanguageChange:
            RetranslateUi();
            break;
        default:
            QMainWindow::changeEvent(event);
    }
}

void XStyleWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    if (reserveShadowWidth && !shadowBuffer.isNull() && !inActiveShadowBuffer.isNull()) {
        QPainter painter(this);
        painter.drawPixmap(QPoint(), isActiveWindow() ? shadowBuffer : inActiveShadowBuffer);
        painter.end();
    }
}

void XStyleWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (!nativeDragMode && inDraging) {
        move(event->globalPos() - ptBeginDrag);
    }
}

void XStyleWindow::mousePressEvent(QMouseEvent* event)
{
    if (!nativeDragMode) {
        ptBeginDrag = event->pos();
        if (DraggableAreaHitTest(ptBeginDrag.x(), ptBeginDrag.y())) {
            inDraging = true;
            setMouseTracking(true);
        }
    }
}

void XStyleWindow::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);

    if (!nativeDragMode && inDraging) {
        inDraging = false;
        setMouseTracking(false);
    }
}

bool XStyleWindow::eventFilter(QObject* obj, QEvent* e)
{
    Q_UNUSED(obj);

    auto eventType = e->type();

    if (obj == labelWindowIcon && eventType == QEvent::MouseButtonDblClick) {
        close();
    }
    else if (eventType == QEvent::ToolTip) {
        QString msg = "";

        if (obj == btnMin) {
            msg = xstyle_manager.Translate("xstyle_meta", "Minimize");
        }
        else if (obj == btnMax) {
            msg = xstyle_manager.Translate("xstyle_meta", "Maximize");
        }
        else if (obj == btnClose) {
            msg = xstyle_manager.Translate("xstyle_meta", "Close");
        }
        else {
            return false;
        }

        QPushButton* btn = static_cast<QPushButton*>(obj);
        if (btn->isEnabled()) {
            QHelpEvent* helpEvent = static_cast<QHelpEvent*>(e);
            QToolTip::showText(helpEvent->globalPos(), msg);
        }
        else {
            e->ignore();
        }
        return true;
    }
    else if (obj == this) {
        switch (eventType) {
            case QEvent::WindowActivate:
                WindowActiveChanged(true);
                break;
            case QEvent::WindowDeactivate:
                WindowActiveChanged(false);
                break;
        }
    }

    return false;
}

bool XStyleWindow::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
#if _WIN32
    MSG* msg = reinterpret_cast<MSG*>(message);
    return nativeEvent_Windows(eventType, message, msg->hwnd, msg->message, msg->wParam, msg->lParam, result);
#else
    return QMainWindow::nativeEvent(eventType, message, result);
#endif
}

#if _WIN32

bool XStyleWindow::nativeEvent_Windows(const QByteArray& eventType, void* message, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, long* result)
{
    Q_UNUSED(wParam);

    switch (msg) {
        case WM_XSTYLE_WAKE_UP: {
            show();
            break;
        }

        case WM_ENTERSIZEMOVE: {
            EnabledWindowCaption(false);
            inDraging = true;
            break;
        }

        case WM_MOVE: {
            if (!inDraging) {
                EnabledWindowCaption(false);
                QTimer::singleShot(10, [this]() {
                    EnabledWindowCaption(true);
                });
            }
            break;
        }

        case WM_EXITSIZEMOVE: {
            EnabledWindowCaption(true);
            inDraging = false;
            break;
        }

        case WM_NCCALCSIZE: {
            // remove nonclient area
            *result = 0;
            return true;
        }

        case WM_NCHITTEST: {
            if (!nativeDragMode) {
                break;
            }

            auto  screenPt = MAKEPOINTS(lParam);
            POINT clientPt = {screenPt.x, screenPt.y};
            ScreenToClient(hWnd, &clientPt);
            if (DraggableAreaHitTest(clientPt.x, clientPt.y)) {
                *result = HTCAPTION;
                return true;
            }
            break;
        }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}

void XStyleWindow::EnabledWindowCaption(bool bEnabled)
{
    HWND  hWnd     = (HWND)this->winId();
    DWORD style    = ::GetWindowLong(hWnd, GWL_STYLE);
    DWORD newStyle = bEnabled ? (style | WS_CAPTION) : (style & ~WS_CAPTION);

    if (style != newStyle) {
        ::SetWindowLong(hWnd, GWL_STYLE, newStyle);
    }
}

#endif

bool XStyleWindow::DraggableAreaHitTest(int x, int y)
{
    QPoint pt(x, y);

    if (!this->titlePanel) {
        return false;
    }

    // if (!this->titlePanel->frameGeometry().contains(pt)) {
    //     return false;
    // }

    if (y > this->titlePanel->y() + this->titlePanel->height()) {
        return false;
    }

    for (auto& child : this->titlePanel->findChildren<QWidget*>()) {
        if (!child->objectName().compare(winName + QString::fromUtf8("_title_label"))) {
            continue;
        }

        // if (child->underMouse()) {
        //     return false;
        // }

        if (child->rect().contains(child->mapFrom(this, pt))) {
            return false;
        }
    }

    return true;
}