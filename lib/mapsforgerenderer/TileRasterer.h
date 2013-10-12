#ifndef MAPSFORGE_TILERASTERER_H
#define MAPSFORGE_TILERASTERER_H

#include "renderer/PointTextContainer.h"
#include "renderer/ShapePaintContainer.h"
#include "renderer/SymbolContainer.h"
#include "renderer/WayTextContainer.h"

#include <QVector>
#include <QList>

class QPainter;

class TileRasterer
{
public:
	TileRasterer(QPainter *painter);

	void drawWays(const QVector<QVector<QList<ShapePaintContainer *> > > &ways);

	void drawSymbols(const QList<SymbolContainer> &symbols);

	void drawWayNames(const QList<WayTextContainer> &wayTexts);

	void drawNodes(const QList<PointTextContainer> &nodes);

private:
	QPainter *const m_painter;
};

#endif // MAPSFORGE_TILERASTERER_H
