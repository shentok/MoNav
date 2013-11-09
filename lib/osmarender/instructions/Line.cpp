#include "Line.h"

#include "../RenderCallback.h"

Line::Line(const QPen &stroke, float strokeWidth, int level) :
	m_stroke(stroke),
	m_strokeWidth(strokeWidth),
	m_level(level)
{
	m_stroke.setWidthF(strokeWidth);
}

void Line::renderNode(RenderCallback */*renderCallback*/, const QList<Tag> &/*tags*/) const
{
	// do nothing
}

void Line::renderWay(RenderCallback *renderCallback, const QList<Tag> &/*tags*/) const
{
	renderCallback->renderWay(this->m_stroke, this->m_level);
}

void Line::scaleStrokeWidth(float scaleFactor)
{
	this->m_stroke.setWidthF(this->m_strokeWidth * scaleFactor);
}

void Line::scaleTextSize(float /*scaleFactor*/)
{
	// do nothing
}
