/* tile-write.cpp. Writes openstreetmap map tiles using a series of rules
(defined near the top) and a pre-processed database file of roads, paths,
and areas arranged as a quad-tree in the same fashion as the tiles.

Current issues:
    No text support - would be easy to add.
*/

#include "mapsforgetilewriter.h"

#include <QImage>
#include <QPainter>

MapsforgeTileWriter::MapsforgeTileWriter(const QString &fileName, RenderTheme *renderTheme) :
	m_databaseFile(fileName, this),
	m_tileFactory(&m_databaseFile, renderTheme)
{
}

MapsforgeTileWriter::~MapsforgeTileWriter()
{
}

//The main entry point into the class. Draw the map tile described by x, y
//and zoom, and save it into _imgname if it is not an empty string. The tile
//is held in memory and the raw image data can be accessed by get_img_data()
void MapsforgeTileWriter::draw_image(int x, int y, int zoom, int magnification)
{
	QImage image = m_tileFactory.createTile(x, y, zoom, magnification);
	const int width = image.width();
	const int height = image.height();

	QPainter painter(&image);

	painter.setPen(Qt::black);
	painter.drawLine(0, 0, 0, height);
	painter.drawLine(0, 0, width, 0);

	emit image_finished(x, y, zoom, magnification, image);
}
