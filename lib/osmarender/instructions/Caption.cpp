#include "Caption.h"

#include "../RenderCallback.h"

Caption::Caption(const Paint &fill, const Paint &stroke, const QFont &font, float dy, float fontSize, const QString &caption) :
	RenderInstruction(),
	m_fill(fill),
	m_stroke(stroke),
	m_font(font),
	m_dy(dy),
	m_fontSize(fontSize),
	m_tagsKey(caption)
{
	m_font.setPointSizeF(fontSize);
}

void Caption::renderNode(RenderCallback *renderCallback, const QList<Tag> &tags) const
{
	if (m_tagsKey.isEmpty()) {
		return;
	}

	foreach (const Tag &tag, tags) {
		if (tag.first == m_tagsKey) {
			renderCallback->renderPointOfInterestCaption(tag.second, m_dy, m_font, m_fill, m_stroke);
			break;
		}
	}
}

void Caption::renderWay(RenderCallback *renderCallback, const QList<Tag> &tags) const
{
	if (m_tagsKey.isEmpty()) {
		return;
	}

	foreach (const Tag &tag, tags) {
		if (tag.first == m_tagsKey) {
			renderCallback->renderAreaCaption(tag.second, m_dy, m_font, m_fill, m_stroke);
			break;
		}
	}
}

void Caption::scaleStrokeWidth(float /*scaleFactor*/)
{
	// do nothing
}

void Caption::scaleTextSize(float scaleFactor)
{
	m_font.setPointSizeF(m_fontSize * scaleFactor);
}
