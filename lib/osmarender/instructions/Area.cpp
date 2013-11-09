#include "Area.h"

#include "../RenderCallback.h"

Area::Area(const QBrush &fill, const QPen &stroke, float strokeWidth, int level) :
	m_fill(fill),
	m_stroke(stroke),
	m_level(level),
	m_strokeWidth(strokeWidth)
{
	m_stroke.setWidthF(strokeWidth);
}

void Area::renderNode(RenderCallback *renderCallback, const QList<Tag> &tags) const
{
	Q_UNUSED(renderCallback)
	Q_UNUSED(tags)

	// do nothing
}

void Area::renderWay(RenderCallback *renderCallback, const QList<Tag> &tags) const
{
	Q_UNUSED(tags)

	renderCallback->renderArea(m_fill, m_stroke, m_level);
}

void Area::scaleStrokeWidth(float scaleFactor)
{
	m_stroke.setWidthF(m_strokeWidth * scaleFactor);
}

void Area::scaleTextSize(float scaleFactor)
{
	Q_UNUSED(scaleFactor)

	// do nothing
}
