#ifndef MAPSFORGE_PATHTEXT_H
#define MAPSFORGE_PATHTEXT_H

#include "../RenderInstruction.h"

#include <QFont>
#include <QPen>

typedef QPen Paint;

class PathText : public RenderInstruction
{
public:
	PathText(const Paint &fill, const Paint &stroke, const QFont &font, float fontSize, const QString &text);

	void renderNode(RenderCallback *renderCallback, const QList<Tag> &tags) const;

	void renderWay(RenderCallback *renderCallback, const QList<Tag> &tags) const;

	void scaleStrokeWidth(float scaleFactor);

	void scaleTextSize(float scaleFactor);

private:
	Paint m_fill;
	Paint m_stroke;
	QFont m_font;
	float m_fontSize;
	QString m_textKey;
};

#endif // MAPSFORGE_PATHTEXT_H
