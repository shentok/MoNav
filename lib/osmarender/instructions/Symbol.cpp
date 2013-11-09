#include "Symbol.h"

#include "../RenderCallback.h"

Symbol::Symbol(const QImage &symbol) :
	m_symbol(symbol)
{
}

void Symbol::renderNode(RenderCallback *renderCallback, const QList<Tag> &/*tags*/) const
{
	renderCallback->renderPointOfInterestSymbol(m_symbol);
}

void Symbol::renderWay(RenderCallback *renderCallback, const QList<Tag> &/*tags*/) const
{
	renderCallback->renderAreaSymbol(m_symbol);
}

void Symbol::scaleStrokeWidth(float /*scaleFactor*/)
{
	// do nothing
}

void Symbol::scaleTextSize(float /*scaleFactor*/)
{
	// do nothing
}
