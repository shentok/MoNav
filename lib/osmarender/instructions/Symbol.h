#ifndef MAPSFORGE_SYMBOL_H
#define MAPSFORGE_SYMBOL_H

#include "../RenderInstruction.h"

#include <QImage>

class Symbol : public RenderInstruction
{
public:
	Symbol(const QImage &symbol);

	void renderNode(RenderCallback *renderCallback, const QList<Tag> &tags) const;

	void renderWay(RenderCallback *renderCallback, const QList<Tag> &tags) const;

	void scaleStrokeWidth(float scaleFactor);

	void scaleTextSize(float scaleFactor);

private:
	QImage m_symbol;
};

#endif // MAPSFORGE_SYMBOL_H
