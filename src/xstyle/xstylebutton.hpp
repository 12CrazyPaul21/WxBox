#ifndef __X_STYLE_BUTTON_H
#define __X_STYLE_BUTTON_H

#include <QPushButton>
#include <QEvent>
#include <QFile>
#include <QPropertyAnimation>
#include <QStylePainter>
#include <QStyleOptionButton>
#include <QBitmap>
#include <QPainter>
#include <QApplication>

#include <xstyle/xstyle.h>
#include <xstyle/xstylecommon.h>

//
// XStyleButton
//

#define RegisterXStyleButtonProperty(Type, PropertyName, MemberName, Suffix, DefaultValue) \
    DefineXStyleProperty(Type, PropertyName, MemberName, Suffix, DefaultValue)             \
        Q_PROPERTY(Type PropertyName READ Get##Suffix WRITE Set##Suffix DESIGNABLE true SCRIPTABLE true)

class XStyleButton : public QPushButton
{
    Q_OBJECT

  public:
    RegisterXStyleButtonProperty(QColor, normal_color, normalColor, NormalColor, DEFAULT_XSTYLE_BUTTON_NORMAL_COLOR);
    RegisterXStyleButtonProperty(QColor, disabled_color, disabledColor, DisabledColor, DEFAULT_XSTYLE_BUTTON_DISABLED_COLOR);
    RegisterXStyleButtonProperty(QColor, hover_color, hoverColor, HoverColor, DEFAULT_XSTYLE_BUTTON_HOVER_COLOR);
    RegisterXStyleButtonProperty(QColor, down_color, downColor, DownColor, DEFAULT_XSTYLE_BUTTON_DOWN_COLOR);

    RegisterXStyleButtonProperty(int, left_top_radius, leftTopRadius, LeftTopRadius, DEFAULT_XSTYLE_BUTTON_LEFT_TOP_RADIUS);
    RegisterXStyleButtonProperty(int, left_bottom_radius, leftBottomRadius, LeftBottomRadius, DEFAULT_XSTYLE_BUTTON_LEFT_BOTTOM_RADIUS);
    RegisterXStyleButtonProperty(int, right_top_radius, rightTopRadius, RightTopRadius, DEFAULT_XSTYLE_BUTTON_RIGHT_TOP_RADIUS);
    RegisterXStyleButtonProperty(int, right_bottom_radius, rightBottomRadius, RightBottomRadius, DEFAULT_XSTYLE_BUTTON_RIGHT_BOTTOM_RADIUS);

    RegisterXStyleButtonProperty(int, animation_duration, animationDuration, AnimationDuration, DEFAULT_XSTYLE_BUTTON_ANIMATION_DURATION);

    RegisterXStyleButtonProperty(QString, icon_url, iconUrl, IconUrl, DEFAULT_XSTYLE_BUTTON_ICON_URL);
    RegisterXStyleButtonProperty(QColor, icon_normal_overlay_color, iconNormalOverlayColor, IconNormalOverlayColor, DEFAULT_XSTYLE_BUTTON_ICON_NORMAL_OVERLAY_COLOR);
    RegisterXStyleButtonProperty(QColor, icon_hover_overlay_color, iconHoverOverlayColor, IconHoverOverlayColor, DEFAULT_XSTYLE_BUTTON_ICON_HOVER_OVERLAY_COLOR);
    RegisterXStyleButtonProperty(QColor, icon_disabled_overlay_color, iconDisabledOverlayColor, IconDisabledOverlayColor, DEFAULT_XSTYLE_BUTTON_ICON_DISABLED_OVERLAY_COLOR);

  public:
    explicit XStyleButton(QWidget* parent = nullptr)
      : XStyleButton("", parent)
    {
    }

    explicit XStyleButton(const QString& text, QWidget* parent = nullptr)
      : QPushButton(text, parent)
      , currentBackgroundColor(DEFAULT_XSTYLE_BUTTON_NORMAL_COLOR)
      , backgroundTransition(nullptr)
    {
        this->installEventFilter(this);
    }

    QPainterPath ButtonBackgroundPath()
    {
        QPainterPath path;

        int left, top, right, bottom;
        rect().getCoords(&left, &top, &right, &bottom);

        // right top
        auto rightTopDiam = rightTopRadius * 2;
        path.moveTo(right, top + rightTopRadius);
        path.arcTo(QRect(right - rightTopDiam, top, rightTopDiam, rightTopDiam), 0.0, 90.0);

        // left top
        auto leftTopDiam = leftTopRadius * 2;
        path.lineTo(left + leftTopRadius, top);
        path.arcTo(QRect(left, top, leftTopDiam, leftTopDiam), 90.0, 90.0);

        // left bottom
        auto leftBottomDiam = leftBottomRadius * 2;
        path.lineTo(left, bottom - leftBottomRadius);
        path.arcTo(QRect(left, bottom - leftBottomDiam, leftBottomDiam, leftBottomDiam), 180.0, 90);

        // right bottom
        auto rightBottomDiam = rightBottomRadius * 2;
        path.lineTo(right - rightBottomRadius, bottom);
        path.arcTo(QRect(right - rightBottomDiam, bottom - rightBottomDiam, rightBottomDiam, rightBottomDiam), 270.0, 90.0);

        path.closeSubpath();
        return path;
    }

    void SetupIcon(QIcon&& icon)
    {
        // original icon pixmap and mask
        auto originalIconPixmap = icon.pixmap(iconSize(), QIcon::Normal, QIcon::State::On);
        auto mask               = originalIconPixmap.createMaskFromColor(Qt::transparent);

        // normal status icon
        QPixmap normalIconPixmap(originalIconPixmap.size());
        normalIconPixmap.fill(iconNormalOverlayColor);
        normalIconPixmap.setMask(mask);
        icon.addPixmap(normalIconPixmap, QIcon::Normal, QIcon::On);

        // hover status icon
        QPixmap hoverIconPixmap(originalIconPixmap.size());
        hoverIconPixmap.fill(iconHoverOverlayColor);
        hoverIconPixmap.setMask(mask);
        icon.addPixmap(hoverIconPixmap, QIcon::Selected, QIcon::On);

        // disabled status icon
        QPixmap disabledIconPixmap(originalIconPixmap.size());
        disabledIconPixmap.fill(iconDisabledOverlayColor);
        disabledIconPixmap.setMask(mask);
        icon.addPixmap(disabledIconPixmap, QIcon::Disabled, QIcon::On);

        setText("");
        setIcon(std::move(icon));
    }

    void SetupStyle()
    {
        // generate background path
        backgroundPath = ButtonBackgroundPath();

        // generate style sheet
        auto style = QString(R"(border-top-left-radius: %1;border-top-right-radius: %2;border-bottom-left-radius: %3;border-bottom-right-radius: %4;)").arg(leftTopRadius).arg(rightTopRadius).arg(leftBottomRadius).arg(rightBottomRadius);

        // button icon
        if (!iconUrl.isEmpty()) {
            QIcon icon;

            // load icon
            if (!iconUrl.compare(STANDARD_XSTYLE_WINDOW_TITLE_MIN_BUTTON_ICON)) {
                icon = QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarMinButton);
            }
            else if (!iconUrl.compare(STANDARD_XSTYLE_WINDOW_TITLE_MAX_BUTTON_ICON)) {
                icon = QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarMaxButton);
            }
            else if (!iconUrl.compare(STANDARD_XSTYLE_WINDOW_TITLE_CLOSE_BUTTON_ICON)) {
                icon = QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarCloseButton);
            }
            else if (QFile::exists(iconUrl)) {
                icon = QIcon(iconUrl);
            }

            // setup icon
            if (icon.availableSizes().count()) {
                SetupIcon(std::move(icon));
            }
        }

        // apply style sheet
        setStyleSheet(style);

        // init background color
        TransitionBackgroundColor(isEnabled() ? normalColor : disabledColor);
    }

  protected:
    void TransitionBackgroundColor(const QColor& targetColor)
    {
        if (!isVisible()) {
            currentBackgroundColor = targetColor;
            return;
        }

        if (backgroundTransition) {
            backgroundTransition->stop();
        }

        backgroundTransition = new QVariantAnimation(this);
        backgroundTransition->setDuration(animationDuration);
        backgroundTransition->setStartValue(currentBackgroundColor);
        backgroundTransition->setEndValue(targetColor);

        connect(backgroundTransition, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
            currentBackgroundColor = value.value<QColor>();
            repaint();
        });

        connect(backgroundTransition, &QVariantAnimation::destroyed, this, [this]() {
            backgroundTransition = nullptr;
            repaint();
        });

        backgroundTransition->start(QAbstractAnimation::DeleteWhenStopped);
    }

    bool eventFilter(QObject* obj, QEvent* e) override
    {
        Q_UNUSED(obj);

        switch (e->type()) {
            case QEvent::HoverEnter:
                if (!isEnabled()) {
                    break;
                }
                TransitionBackgroundColor(hoverColor);
                break;

            case QEvent::HoverLeave:
                if (!isEnabled()) {
                    break;
                }
                TransitionBackgroundColor(normalColor);
                break;

            case QEvent::MouseButtonPress:
                if (!isEnabled()) {
                    break;
                }
                TransitionBackgroundColor(downColor);
                break;

            case QEvent::MouseButtonRelease:
                if (!isEnabled()) {
                    break;
                }
                TransitionBackgroundColor(normalColor);
                break;

            case QEvent::EnabledChange:
                TransitionBackgroundColor(isEnabled() ? normalColor : disabledColor);
                break;
        }

        return false;
    }

    void paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);

        QStylePainter      painter(this);
        QStyleOptionButton option;

        initStyleOption(&option);

        // draw background
        painter.setRenderHint(painter.Antialiasing, true);
        painter.fillPath(backgroundPath, currentBackgroundColor);
        painter.setRenderHint(painter.Antialiasing, false);

        // select icon
        option.icon = icon().pixmap(iconSize(), isEnabled() ? ((option.state & QStyle::State_MouseOver) ? QIcon::Selected : QIcon::Normal) : QIcon::Disabled, QIcon::On);

        // draw button
        painter.drawControl(QStyle::CE_PushButton, option);
    }

  protected:
    QPainterPath       backgroundPath;
    QColor             currentBackgroundColor;
    QVariantAnimation* backgroundTransition;
};

#endif  // #ifndef __X_STYLE_BUTTON_H