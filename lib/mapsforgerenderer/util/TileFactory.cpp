/* tile-write.cpp. Writes openstreetmap map tiles using a series of rules
(defined near the top) and a pre-processed database file of roads, paths,
and areas arranged as a quad-tree in the same fashion as the tiles.

Current issues:
    No text support - would be easy to add.
*/

#include "TileFactory.h"

#include "osmarender/RenderTheme.h"
#include "mapsforgereader/TileId.h"
#include "DatabaseRenderer.h"
#include "TileRasterer.h"

#include <qmath.h>
#include <QImage>
#include <QPainter>

namespace Mapsforge {

const double TileFactory::STROKE_INCREASE = 1.5;
const byte TileFactory::STROKE_MIN_ZOOM_LEVEL = 12;

TileFactory::TileFactory(QIODevice *device, RenderTheme *renderTheme) :
	m_mapDatabase(device),
	m_renderTheme(renderTheme),
	m_previousMagnification(1),
	m_previousZoomLevel(std::numeric_limits<byte>::min())
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

	if (zoom != m_previousZoomLevel || magnification != m_previousMagnification) {
		const int zoomLevelDiff = qMax(zoom - STROKE_MIN_ZOOM_LEVEL, 0);
		const float strokeWidth = (float) qPow(STROKE_INCREASE, zoomLevelDiff) * magnification;
		m_renderTheme->scaleStrokeWidth(strokeWidth);
		m_previousZoomLevel = zoom;
	}

	if (magnification != m_previousMagnification) {
		m_renderTheme->scaleTextSize(magnification);
		m_previousMagnification = magnification;
	}

	DatabaseRenderer databaseRenderer(m_renderTheme);
	const VectorTile vectorTile = m_mapDatabase.readMapData(tileId);
	foreach (const PointOfInterest &pointOfInterest, vectorTile.pointsOfInterest()) {
		databaseRenderer.matchPointOfInterest(pointOfInterest, tileId, tileSize);
	}

	foreach (const Way &way, vectorTile.ways()) {
		databaseRenderer.matchWay(way, tileId, tileSize);
	}

	if (vectorTile.isWater()) {
		databaseRenderer.matchWater(tileId, tileSize);
	}

	QImage image = QImage(tileSize, tileSize, QImage::Format_ARGB32_Premultiplied);
	image.fill(m_renderTheme->getMapBackground().rgba());
	QPainter painter(&image);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
	painter.setPen(Qt::NoPen);
	TileRasterer rasterer(&painter);
	rasterer.drawWays(databaseRenderer.ways());
	rasterer.drawSymbols(databaseRenderer.waySymbols());
	rasterer.drawSymbols(databaseRenderer.pointSymbols());
	rasterer.drawWayNames(databaseRenderer.wayNames());
	rasterer.drawNodes(databaseRenderer.nodes());
	rasterer.drawNodes(databaseRenderer.areaLabels());

	return image;
}

} // namespace mapsforge
