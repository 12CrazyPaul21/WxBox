//
// There is a code snippet that works for a custom QGraphicsEffect
// from : https://forum.qt.io/topic/77576/painting-shadow-around-a-parentless-qwidget
//

#ifndef __QT_HELPER_SHADOW_H
#define __QT_HELPER_SHADOW_H

#include <QtWidgets/qgraphicseffect.h>
#include <QtGui/qpainter.h>
#include <QtWidgets/qwidget.h>

// src/widgets/effects/qpixmapfilter.cpp
Q_DECL_IMPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);

class Shadow : public QGraphicsEffect {
public:
	enum Side {
		Left = 0x1,
		Right = 0x2,
		Bottom = 0x4,
		Top = 0x8,
		Around = Left | Top | Right | Bottom,
	};
	Q_DECLARE_FLAGS(Sides, Side);

	Shadow(QObject *parent = 0) :
		QGraphicsEffect(parent)
	{
	}

	Shadow(const QColor &c, qreal distance, qreal radius, Sides sides = Side::Around, QObject *parent = 0) :
		QGraphicsEffect(parent)
	{
		setColor(c);
		setBlurRadius(radius);
		setDistance(distance);
		setSides(sides);
	}

	Sides sides() const {
		return _side;
	}

	void setSides(Sides s) {
		_side = s;
		updateBoundingRect();
	}

	QColor color() const {
		return _color;
	}

	void setColor(const QColor &c) {
		_color = c;
		updateBoundingRect();
	}

	qreal blurRadius() const {
		return _blurRadius;
	}

	void setBlurRadius(qreal br) {
		_blurRadius = br;
		updateBoundingRect();
	}

	qreal distance() const {
		return _distance;
	}

	void setDistance(qreal d) {
		_distance = d;
		updateBoundingRect();
	}

	QRectF boundingRectFor(const QRectF& r) const override {
		qreal _delta = blurRadius() + distance();
		return r.marginsAdded(QMarginsF(
			(sides() & Side::Left) ? _delta : 0,
			(sides() & Side::Top) ? _delta : 0,
			(sides() & Side::Right) ? _delta : 0,
			(sides() & Side::Bottom) ? _delta : 0
		));
	}

	// Return a pixmap with target painted into it with margin = offset
	static QPixmap grab(QWidget *target, const QRect &rect, int offset)
	{
		auto result = QPixmap(rect.size());
		auto r = rect.marginsRemoved(QMargins(offset, offset, offset, offset));
		result.fill(Qt::transparent);
		{
			QPainter p;
			p.begin(&result);
			target->render(&p, QPoint(offset, offset), r);
			p.end();
		}
		return result;
	}

	// Return a background blurred QImage to Draw as the widget's shadow
	static QImage paint(QWidget *target, const QRect &box, qreal radius, qreal distance, const QColor &c, Sides sides = Side::Around)
	{

		const auto _source = grab(target, box, distance);
		if (_source.isNull() || distance <= 0) return QImage();

		QImage _backgroundImage(box.size(), QImage::Format_ARGB32_Premultiplied);
		_backgroundImage.fill(0);

		QPainter _backgroundPainter(&_backgroundImage);
		_backgroundPainter.drawPixmap(QPointF(), _source);
		_backgroundPainter.end();

		QImage blurredImage(_backgroundImage.size(), QImage::Format_ARGB32_Premultiplied);
		blurredImage.fill(0);

		{
			QPainter blurPainter(&blurredImage);
			qt_blurImage(&blurPainter, _backgroundImage, radius, true, false);
			blurPainter.end();
		}
		_backgroundImage = blurredImage;

		_backgroundPainter.begin(&_backgroundImage);
		_backgroundPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		auto margin = _backgroundImage.rect().marginsRemoved(QMargins(
			(sides & Left) ? 0 : distance,
			(sides & Top) ? 0 : distance,
			(sides & Right) ? 0 : distance,
			(sides & Bottom) ? 0 : distance
		));
		_backgroundPainter.fillRect(margin, c);
		_backgroundPainter.end();
		return _backgroundImage;
	}

protected:
	void draw(QPainter *painter) override
	{
		if ((blurRadius() + distance()) <= 0) {
			drawSource(painter);
			return;
		}

		QPoint _offset;
		QPixmap _pixmap = sourcePixmap(Qt::DeviceCoordinates, &_offset, QGraphicsEffect::PadToEffectiveBoundingRect);
		if (_pixmap.isNull()) return;

		QTransform _transform = painter->worldTransform();
		painter->setWorldTransform(QTransform());

		QSize _backgroundSize = QSize(_pixmap.size().width() + 2 * distance(), _pixmap.size().height() + 2 * distance());
		QImage _temp(_backgroundSize, QImage::Format_ARGB32_Premultiplied);

		QPixmap scaled = _pixmap.scaled(_backgroundSize);
		_temp.fill(0);

		QPainter _tempPainter(&_temp);
		_tempPainter.setCompositionMode(QPainter::CompositionMode_Source);
		_tempPainter.drawPixmap(QPointF(-distance(), -distance()), scaled);
		_tempPainter.end();

		QImage blurred(_temp.size(), QImage::Format_ARGB32_Premultiplied);
		blurred.fill(0);

		QPainter blurPainter(&blurred);
		qt_blurImage(&blurPainter, _temp, blurRadius(), true, false);
		blurPainter.end();
		_temp = blurred;

		_tempPainter.begin(&_temp);
		_tempPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		_tempPainter.fillRect(_temp.rect(), color());
		_tempPainter.end();

		painter->drawImage(_offset, _temp);
		painter->drawPixmap(_offset, _pixmap, QRectF());
		painter->setWorldTransform(_transform);
	}

private:
	Sides _side;
	QColor _color;
	qreal _distance;
	qreal _blurRadius;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Shadow::Sides)

#endif // #ifndef __QT_HELPER_SHADOW_H