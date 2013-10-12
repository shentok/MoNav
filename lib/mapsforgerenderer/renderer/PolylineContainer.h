#ifndef POLYLINECONTAINER_H
#define POLYLINECONTAINER_H

#include "ShapeContainer.h"

#include <QBrush>
#include <QVector>
#include <QPen>
#include <QPoint>

typedef QPoint Point;

class PolylineBrushContainer : public ShapeContainer
{
public:
	PolylineBrushContainer(const QVector<QVector<Point> > &coordinates = QVector<QVector<Point> >(), const QBrush &brush = QBrush());

	void render(QPainter *painter) const;

private:
	QVector<QVector<Point> > m_coordinates;
	QBrush m_brush;
};

class PolylinePenContainer : public ShapeContainer
{
public:
	PolylinePenContainer(const QVector<QVector<Point> > &coordinates = QVector<QVector<Point> >(), const QPen &pen = QPen());

	void render(QPainter *painter) const;

private:
	QVector<QVector<Point> > m_coordinates;
	QPen m_pen;
};

#endif // POLYLINECONTAINER_H
