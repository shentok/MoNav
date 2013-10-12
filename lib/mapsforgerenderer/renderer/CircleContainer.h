#ifndef CIRCLECONTAINER_H
#define CIRCLECONTAINER_H

#include "ShapeContainer.h"

#include <QBrush>
#include <QPen>
#include <QPoint>

typedef QPoint Point;

class CircleBrushContainer : public ShapeContainer
{
public:
	CircleBrushContainer(const Point &point, qreal radius, const QBrush &brush);

	void render(QPainter *painter) const;

private:
	Point m_center;
	qreal m_radius;
	QBrush m_brush;
};

class CirclePenContainer : public ShapeContainer
{
public:
	CirclePenContainer(const Point &point, qreal radius, const QPen &pen);

	void render(QPainter *painter) const;

private:
	Point m_center;
	qreal m_radius;
	QPen m_pen;
};

#endif // CIRCLECONTAINER_H
