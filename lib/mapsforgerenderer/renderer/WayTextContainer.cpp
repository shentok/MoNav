#include "WayTextContainer.h"

WayTextContainer::WayTextContainer(const Point &p1, const Point &p2, const QString &text, const QFont &font, const Paint &paint) :
	m_p1(p1),
	m_p2(p2),
	m_text(text),
	m_paint(paint),
	m_font(font)
{
}
