#include "PointTextContainer.h"

#include <QFontMetrics>

PointTextContainer::PointTextContainer(const QString &text, const Point &point, const QFont &font, const Paint &paintFront) :
	m_text(text),
	m_paintFront(paintFront),
	m_paintBack(Qt::NoPen),
	m_font(font),
	m_symbol(0),
	m_point(point),
	m_boundary(Point(0, 0), QSize(QFontMetrics(font).width(text), QFontMetrics(font).height()))
{
	m_paintBack.setColor(Qt::transparent);
}

PointTextContainer::PointTextContainer(const QString &text, const Point &point, const QFont &font, const Paint &paintFront, const Paint &paintBack) :
	m_text(text),
	m_paintFront(paintFront),
	m_paintBack(paintBack),
	m_font(font),
	m_symbol(0),
	m_point(point),
	m_boundary(Point(0, 0), QSize(QFontMetrics(font).width(text), QFontMetrics(font).height()))
{
}

PointTextContainer::PointTextContainer(const QString &text, const Point &point, const QFont &font, const Paint &paintFront, const Paint &paintBack, const SymbolContainer *symbol) :
	m_text(text),
	m_paintFront(paintFront),
	m_paintBack(paintBack),
	m_font(font),
	m_symbol(symbol),
	m_point(point),
	m_boundary(Point(0, 0), QSize(QFontMetrics(font).width(text), QFontMetrics(font).height()))
{
}
