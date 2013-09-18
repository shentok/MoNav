/* tile-write.cpp. Writes openstreetmap map tiles using a series of rules
(defined near the top) and a pre-processed database file of roads, paths,
and areas arranged as a quad-tree in the same fashion as the tiles.

Current issues:
    No text support - would be easy to add.
*/

#include "../../utils/qthelpers.h"
#include "../../utils/coordinates.h"

#include <mapnik/agg_renderer.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/graphics.hpp>
#include <mapnik/image_view.hpp>
#include <mapnik/config_error.hpp>

#include <string>
#include <QDebug>
#include <QImage>

#include "mapniktilewrite.h"


MapnikTileWriter::MapnikTileWriter( const QString &dir ) :
	m_projection( mapnik::projection( "+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +no_defs +over" ) ),
	m_theme("/windows/d/openstreetmap-carto/osm.xml"),
	m_tileSize( 256 ),
	m_margin( 128 )
{
	Timer time;

	qDebug() << "Mapnik Renderer: initialized mapnik connection:" << time.restart() << "ms";

	mapnik::load_map( m_map, m_theme.toLocal8Bit().constData() );
}

MapnikTileWriter::~MapnikTileWriter()
{
}

//The main entry point into the class. Draw the map tile described by x, y
//and zoom, and save it into _imgname if it is not an empty string. The tile
//is held in memory and the raw image data can be accessed by get_img_data()
void MapnikTileWriter::draw_image(int x, int y, int zoomLevel, int magnification)
{
	try {
		const int metaTileSize = m_tileSize + 2 * m_margin;

		m_map.resize( m_tileSize + 2 * m_margin, m_tileSize + 2 * m_margin );

		ProjectedCoordinate drawTopLeft( x - 1.0 * m_margin / m_tileSize, y - 1.0 * m_margin / m_tileSize, zoomLevel );
		ProjectedCoordinate drawBottomRight( x + 1 + 1.0 * m_margin / m_tileSize, y + 1 + 1.0 * m_margin / m_tileSize, zoomLevel );
		GPSCoordinate drawTopLeftGPS = drawTopLeft.ToGPSCoordinate();
		GPSCoordinate drawBottomRightGPS = drawBottomRight.ToGPSCoordinate();
		m_projection.forward( drawTopLeftGPS.longitude, drawBottomRightGPS.latitude );
		m_projection.forward( drawBottomRightGPS.longitude, drawTopLeftGPS.latitude );
		const mapnik::box2d<double> boundingBox( drawTopLeftGPS.longitude, drawTopLeftGPS.latitude, drawBottomRightGPS.longitude, drawBottomRightGPS.latitude );
		m_map.zoom_to_box( boundingBox );

		mapnik::image_32 image( metaTileSize, metaTileSize );

		mapnik::agg_renderer<mapnik::image_32> renderer( m_map, image );
		renderer.apply();

		const mapnik::image_view<mapnik::image_data_32> view = image.get_view( m_margin, m_margin, m_tileSize, m_tileSize );
		const std::string data = mapnik::save_to_string( view, "png" );
		const QByteArray array( data.c_str(), data.length() );
		const QImage im = QImage::fromData(array);

		emit image_finished( x, y, zoomLevel, magnification, im );
	} catch ( const mapnik::config_error & ex ) {
		qCritical( "Mapnik Renderer: ### Configuration error: %s", ex.what() );
	} catch ( const std::exception & ex ) {
		qCritical( "Mapnik Renderer: ### STD error: %s", ex.what() );
	} catch ( ... ) {
		qCritical( "Mapnik Renderer: ### Unknown error" );
	}
}
