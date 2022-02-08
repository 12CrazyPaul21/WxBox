#include <xstyle/quick/qtquick_xstylewindow.h>

//
// XStyleQuickWindow
//

XStyleQuickWindow::XStyleQuickWindow(const QString& name, QWidget* parent, bool deleteWhenClose)
  : XStyleWindow(name, parent, deleteWhenClose)
  , pluginPath(DEFAULT_XSTYLE_QUICK_PLUGIN_URL)
  , quickObject(this)
{
}

XStyleQuickWindow::~XStyleQuickWindow()
{
}

void XStyleQuickWindow::SetupXStyleQuick(QQuickWidget* quickWidget, const QString& sourceUrl)
{
    if (!quickWidget || sourceUrl.isEmpty()) {
        return;
    }

    quickWidget->engine()->addImportPath(pluginPath);
    quickWidget->engine()->rootContext()->setContextProperty(XSTYLE_QUICK_HOST_OBJECT_NAME, &quickObject);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quickWidget->setFixedSize(quickWidget->size());
    quickWidget->setSource(QUrl(sourceUrl));
}