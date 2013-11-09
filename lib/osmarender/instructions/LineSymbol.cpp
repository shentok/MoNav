#include "LineSymbol.h"

#include "../RenderCallback.h"

LineSymbol::LineSymbol(bool alignCenter, const QImage &bitmap, bool repeat) :
	m_alignCenter(alignCenter),
	m_bitmap(bitmap),
	m_repeat(repeat)
{
}

void LineSymbol::renderNode(RenderCallback */*renderCallback*/, const QList<Tag> &/*tags*/) const
{
	// do nothing
}

void LineSymbol::renderWay(RenderCallback *renderCallback, const QList<Tag> &tags) const
{
	Q_UNUSED(tags)

	renderCallback->renderWaySymbol(m_bitmap, m_alignCenter, m_repeat);
}

void LineSymbol::scaleStrokeWidth(float /*scaleFactor*/)
{
	// do nothing
}

void LineSymbol::scaleTextSize(float /*scaleFactor*/)
{
	// do nothing
}
