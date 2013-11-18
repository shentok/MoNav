#ifndef MAPSFORGE_TILE_FACTORY_H
#define MAPSFORGE_TILE_FACTORY_H

#include "mapsforgereader/MapDatabase.h"

#include <QImage>

class QIODevice;

class DependencyCache;
class RenderTheme;

namespace Mapsforge {

class TileFactory
{
public:
	TileFactory(QIODevice *device, RenderTheme *renderTheme);
	~TileFactory();

	/**
	 * Draw the map tile described by x, y and zoom.
	 */
	QImage createTile(int x, int y, int zoom, int magnification);

private:
	MapDatabase m_mapDatabase;
	RenderTheme *const m_renderTheme;
	DependencyCache *const m_dependencyCache;

	int m_previousMagnification;
	byte m_previousZoomLevel;

	static const double STROKE_INCREASE;
	static const byte STROKE_MIN_ZOOM_LEVEL;
};

} // namespace Mapsforge

#endif
