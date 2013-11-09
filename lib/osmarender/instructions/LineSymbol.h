#ifndef MAPSFORGE_LINESYMBOL_H
#define MAPSFORGE_LINESYMBOL_H

#include "../RenderInstruction.h"

#include <QImage>

class LineSymbol : public RenderInstruction
{
public:
	LineSymbol(bool alignCenter, const QImage &bitmap, bool repeat);

	void renderNode(RenderCallback *renderCallback, const QList<Tag> &tags) const;

	void renderWay(RenderCallback *renderCallback, const QList<Tag> &tags) const;

	void scaleStrokeWidth(float scaleFactor);

	void scaleTextSize(float scaleFactor);

private:
	bool m_alignCenter;
	QImage m_bitmap;
	bool m_repeat;
};

#endif // MAPSFORGE_LINESYMBOL_H
