#include "PolylineContainer.h"

#include <QPainter>

PolylineBrushContainer::PolylineBrushContainer(const QVector<QVector<Point> > &coordinates, const QBrush &brush) :
	m_coordinates(coordinates),
	m_brush(brush)
{
}

void PolylineBrushContainer::render(QPainter *painter) const
{
	painter->setBrush(m_brush);

	QPainterPath path;
	foreach (const QVector<Point> &points, m_coordinates) {
		if (points.size() < 1)
			continue;

		path.moveTo(points[0].x(), points[0].y());
		for (int i = 1; i < points.size(); ++i) {
			const Point point = points[i];
			path.lineTo(point.x(), point.y());
		}
	}

	painter->drawPath(path);
}

PolylinePenContainer::PolylinePenContainer(const QVector<QVector<Point> > &coordinates, const QPen &pen) :
	m_coordinates(coordinates),
	m_pen(pen)
{
}

void PolylinePenContainer::render(QPainter *painter) const
{
	painter->setPen(m_pen);

	QPainterPath path;
	foreach (const QVector<Point> &points, m_coordinates) {
		if (points.size() < 1)
			continue;

		path.moveTo(points[0].x(), points[0].y());
		for (int i = 1; i < points.size(); ++i) {
			const Point point = points[i];
			path.lineTo(point.x(), point.y());
		}
	}

	painter->drawPath(path);
}
