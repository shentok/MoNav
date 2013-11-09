/* tile-write.cpp. Writes openstreetmap map tiles using a series of rules
(defined near the top) and a pre-processed database file of roads, paths,
and areas arranged as a quad-tree in the same fashion as the tiles.

Current issues:
    No text support - would be easy to add.
*/

#include "TileFactory.h"

#include "osmarender/RenderTheme.h"
#include "mapsforgereader/TileId.h"

#include <QDebug>
#include <QImage>
#include <QPainter>
#include <QPolygon>

namespace Mapsforge {

TileFactory::TileFactory(QIODevice *device, RenderTheme *renderTheme) :
	m_mapDatabase(device),
	m_renderTheme(renderTheme)
{
}

TileFactory::~TileFactory()
{
	delete m_renderTheme;
}

QImage TileFactory::createTile(int x, int y, int zoom, int magnification)
{
	const TileId tileId(x, y, zoom);
	const unsigned int tileSize = m_mapDatabase.getMapFileInfo().tilePixelSize() * magnification;
	const VectorTile vectorTile = m_mapDatabase.readMapData(tileId);

	QImage image(tileSize, tileSize, QImage::Format_ARGB32_Premultiplied);
	image.fill(QColor(241, 238 , 232, 255).rgba());
	QPainter painter(&image);
	painter.translate(-QPoint(tileId.pixelX(tileSize), tileId.pixelY(tileSize)));

	foreach (const Way &way, vectorTile.ways()) {
		foreach (const QVector<LatLong> &points, way.geoPoints()) {
			QPolygonF polygon;
			foreach (const LatLong &point, points) {
				const double pointX = MercatorProjection::longitudeToPixelX(point.lon(), zoom, tileSize);
				const double pointY = MercatorProjection::latitudeToPixelY(point.lat(), zoom, tileSize);
				polygon << QPointF(pointX, pointY);
			}

			if (polygon.size() >= 2) {
				painter.drawPolyline(polygon);
			}
		}
	}

	return image;
}

} // namespace mapsforge
