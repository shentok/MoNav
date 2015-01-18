#ifndef MAPSFORGE_TILE_WRITER_H
#define MAPSFORGE_TILE_WRITER_H

#include <QObject>

#include "mapsforgerenderer/util/TileFactory.h"

#include <QIODevice>

class QImage;
class RenderTheme;

class MapsforgeTileWriter : public QObject
{
	Q_OBJECT

public:
	MapsforgeTileWriter(QIODevice *mapDatabase, RenderTheme *renderTheme);
	~MapsforgeTileWriter();

public slots:
	void draw_image(int x, int y, int zoom, int magnification);

signals:
	void image_finished(int x, int y, int zoom, int magnification, const QImage &data);

private:
	Mapsforge::TileFactory m_tileFactory;
};

#endif
