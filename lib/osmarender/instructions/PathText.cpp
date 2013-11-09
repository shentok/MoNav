#include "PathText.h"

#include "../RenderCallback.h"

PathText::PathText(const Paint &fill, const Paint &stroke, const QFont &font, float fontSize, const QString &text) :
	RenderInstruction(),
	m_fill(fill),
	m_stroke(stroke),
	m_font(font),
	m_fontSize(fontSize),
	m_textKey(text)
{
	m_font.setPointSizeF(fontSize);
}

void PathText::renderNode(RenderCallback */*renderCallback*/, const QList<Tag> &/*tags*/) const
{
	// do nothing
}

void PathText::renderWay(RenderCallback *renderCallback, const QList<Tag> &tags) const
{
	if (m_textKey.isEmpty()) {
		return;
	}

	foreach (const Tag &tag, tags) {
		if (tag.first == m_textKey) {
			renderCallback->renderWayText(tag.second, m_font, m_fill, m_stroke);
			break;
		}
	}
}

void PathText::scaleStrokeWidth(float /*scaleFactor*/)
{
	// do nothing
}

void PathText::scaleTextSize(float scaleFactor)
{
	m_font.setPointSizeF(m_fontSize * scaleFactor);
}
