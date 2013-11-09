#include "Circle.h"

#include "../RenderCallback.h"

Circle::Circle(const QBrush &fill, const QPen &stroke, float radius, float strokeWidth, bool scaleRadius, int level) :
	m_fill(fill),
	m_stroke(stroke),
	m_level(level),
	m_radius(radius),
	m_renderRadius(0),
	m_strokeWidth(strokeWidth),
	m_scaleRadius(scaleRadius)
{
	if (!m_scaleRadius) {
		m_renderRadius = m_radius;
		m_stroke.setWidthF(m_strokeWidth);
	}
}

void Circle::renderNode(RenderCallback *renderCallback, const QList<Tag> &tags) const
{
	Q_UNUSED(tags)

	renderCallback->renderPointOfInterestCircle(m_renderRadius, m_fill, m_stroke, m_level);
}

void Circle::renderWay(RenderCallback *renderCallback, const QList<Tag> &tags) const
{
	Q_UNUSED(renderCallback)
	Q_UNUSED(tags)
	// do nothing
}

void Circle::scaleStrokeWidth(float scaleFactor)
{
	if (m_scaleRadius) {
		m_renderRadius = m_radius * scaleFactor;
		m_stroke.setWidthF(m_strokeWidth * scaleFactor);
	}
}

void Circle::scaleTextSize(float scaleFactor)
{
	Q_UNUSED(scaleFactor)
	// do nothing
}
