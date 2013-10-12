#include "CircleContainer.h"

#include <QPainter>

CircleBrushContainer::CircleBrushContainer(const Point &point, qreal radius, const QBrush &brush) :
	ShapeContainer(),
	m_center(point),
	m_radius(radius),
	m_brush(brush)
{
}

void CircleBrushContainer::render(QPainter *painter) const
{
	painter->setBrush(m_brush);

	painter->drawEllipse(m_center.x(), m_center.y(), m_radius, m_radius);
}


CirclePenContainer::CirclePenContainer(const Point &point, qreal radius, const QPen &pen) :
	ShapeContainer(),
	m_center(point),
	m_radius(radius),
	m_pen(pen)
{
}

void CirclePenContainer::render(QPainter *painter) const
{
	painter->setPen(m_pen);

	painter->drawEllipse(m_center.x(), m_center.y(), m_radius, m_radius);
}
