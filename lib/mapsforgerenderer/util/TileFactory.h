#ifndef MAPSFORGE_TILE_FACTORY_H
#define MAPSFORGE_TILE_FACTORY_H

#include "mapsforgereader/MapDatabase.h"

#include <QImage>

class QIODevice;

namespace Mapsforge {

class TileFactory
{
public:
    TileFactory(QIODevice *device);
    ~TileFactory();

    /**
     * Draw the map tile described by x, y and zoom.
     */
    QImage createTile(int x, int y, int zoom, int magnification);

private:
    MapDatabase m_mapDatabase;
};

} // namespace Mapsforge

#endif
