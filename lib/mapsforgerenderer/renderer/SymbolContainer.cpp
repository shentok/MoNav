#include "SymbolContainer.h"


SymbolContainer::SymbolContainer(const QImage &symbol, const Point &point) :
	m_symbol(symbol),
	m_point(point),
	m_alignCenter(false),
	m_theta(0)
{
}

SymbolContainer::SymbolContainer(const QImage &symbol, const Point &point, bool alignCenter, float theta) :
	m_symbol(symbol),
	m_point(point),
	m_alignCenter(alignCenter),
	m_theta(theta)
{
}
