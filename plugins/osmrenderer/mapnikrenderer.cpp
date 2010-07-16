/*
Copyright 2010  Christian Vetter veaac.fdirct@gmail.com

This file is part of MoNav.

MoNav is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MoNav is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MoNav.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mapnikrenderer.h"

#include <mapnik/map.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/font_engine_freetype.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/load_map.hpp>
#include <omp.h>
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QProgressDialog>
#include <QTemporaryFile>
#include <QProcess>
#include <QtDebug>

//#include <QLibrary>

MapnikRenderer::MapnikRenderer()
{
	settingsDialog = NULL;
}

MapnikRenderer::~MapnikRenderer()
{
	if ( settingsDialog != NULL )
		delete settingsDialog;
}

QString MapnikRenderer::GetName()
{
	return "Mapnik Renderer";
}

MapnikRenderer::Type MapnikRenderer::GetType()
{
	return Renderer;
}

void MapnikRenderer::SetOutputDirectory( const QString& dir )
{
	outputDirectory = dir;
}

void MapnikRenderer::ShowSettings()
{
	if ( settingsDialog == NULL )
		settingsDialog = new MRSettingsDialog();
	settingsDialog->exec();
}

bool MapnikRenderer::Preprocess( IImporter* importer )
{
	if ( settingsDialog == NULL )
		settingsDialog = new MRSettingsDialog();
	QDir directory( outputDirectory );
	QString filename = directory.filePath( "Mapnik Renderer" );

	try {
		MRSettingsDialog::Settings settings;
		if ( !settingsDialog->getSettings( &settings ) )
			return false;
		IImporter::BoundingBox box;
		if ( !importer->GetBoundingBox( &box ) )
			return false;
		std::vector< IImporter::RoutingEdge > inputEdges;
		std::vector< IImporter::RoutingNode > inputNodes;
		if ( settings.deleteTiles ) {
			if ( !importer->GetRoutingEdges( &inputEdges ) ) {
				qCritical() << "Failed to read routing Edges";
				return false;
			}
			if ( !importer->GetRoutingNodes( &inputNodes ) ) {
				qCritical() << "Failed to read routing Nodes";
				return false;
			}
		}

		qDebug( "Initialize Database Link" );

		/*QDir pluginDir( settings.plugins );
		foreach ( QString fileName, pluginDir.entryList( QDir::Files ) ) {
			QLibrary* loader = new QLibrary( pluginDir.absoluteFilePath( fileName ) );
			loader->load();
			qDebug( "%s is libary: %i", pluginDir.absoluteFilePath( fileName ).toAscii().constData(), loader->isLoaded() );
			if ( !loader->isLoaded() )
				qDebug( "Reason: %s", loader->errorString().toAscii().constData() );
		}*/

		mapnik::datasource_cache::instance()->register_datasources( settings.plugins.toAscii().constData() );

		QDir fonts( settings.fonts );
		mapnik::projection projection;
		projection = mapnik::projection( "+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +no_defs +over" );
		mapnik::freetype_engine::register_font( fonts.filePath( "DejaVuSans.ttf" ).toAscii().constData() );
		mapnik::freetype_engine::register_font( fonts.filePath( "DejaVuSans-Bold.ttf" ).toAscii().constData() );
		mapnik::freetype_engine::register_font( fonts.filePath( "DejaVuSans-Oblique.ttf" ).toAscii().constData() );
		mapnik::freetype_engine::register_font( fonts.filePath( "DejaVuSans-BoldOblique.ttf" ).toAscii().constData() );

		qDebug( "X: %u - %u", box.min.x, box.max.x );
		qDebug( "Y: %u - %u", box.min.y, box.max.y );

		MapnikConfig config;
		config.maxZoom = settings.maxZoom;
		config.tileSize = settings.tileSize;
		QFile configFile( filename );
		configFile.open( QIODevice::WriteOnly );
		QDataStream configData( &configFile );

		configData << quint32( config.tileSize ) << quint32( config.maxZoom );

		int numThreads = omp_get_max_threads();

		mapnik::Map* maps[numThreads];
		mapnik::Image32* images[numThreads];
		for ( int i = 0; i < numThreads; i++ ) {
			maps[i] = new mapnik::Map;
			mapnik::load_map( *maps[i], settings.theme.toAscii().constData() );
			const int metaTileSize = settings.metaTileSize * settings.tileSize + 2 * settings.margin;
			images[i] = new mapnik::Image32( metaTileSize, metaTileSize );
		}
		qDebug( "Using %d Threads", numThreads );

		for ( int zoom = 0; zoom <= settings.maxZoom; zoom++ ) {

			int tilesRendered = 0;
			int tilesSkipped = 0;
			int metaTilesRendered = 0;
			long long pngcrushSaved = 0;

			int minX = box.min.GetTileX( zoom );
			int maxX = box.max.GetTileX( zoom ) + 1;
			int minY = box.min.GetTileY( zoom );
			int maxY = box.max.GetTileY( zoom ) + 1;

			if ( zoom <= settings.minZoom ) {
				minX = minY = 0;
				maxX = maxY = 1 << zoom;
			}
			else {
				minX = std::max( 0 , minX - settings.tileMargin );
				maxX = std::min ( 1 << zoom, maxX + settings.tileMargin );
				minY = std::max( 0, minY - settings.tileMargin );
				maxY = std::min ( 1 << zoom, maxY + settings.tileMargin );
			}

			std::vector< std::vector< bool > > occupancy;
			if ( settings.deleteTiles ) {
				occupancy.resize( maxX - minX );
				for ( int x = minX; x < maxX; ++x )
					occupancy[x - minX].resize( maxY - minY, false );
				for ( std::vector< IImporter::RoutingEdge >::const_iterator i = inputEdges.begin(), e = inputEdges.end(); i != e; ++i ) {
					int sourceX = inputNodes[i->source].coordinate.GetTileX( zoom );
					int sourceY = inputNodes[i->source].coordinate.GetTileY( zoom );
					int targetX = inputNodes[i->target].coordinate.GetTileX( zoom );
					int targetY = inputNodes[i->target].coordinate.GetTileY( zoom );
					if ( sourceX > targetX )
						std::swap( sourceX, targetX );
					if ( sourceY > targetY )
						std::swap( sourceY, targetY );
					sourceX = std::max( sourceX, minX );
					sourceX = std::min( sourceX, maxX - 1 );
					sourceY = std::max( sourceY, minY );
					sourceY = std::min( sourceY, maxY - 1 );
					targetX = std::max( targetX, minX );
					targetX = std::min( targetX, maxX - 1 );
					targetY = std::max( targetY, minY );
					targetY = std::min( targetY, maxY - 1 );
					for ( int x = sourceX; x <= targetX; ++x ) {
						for ( int y = sourceY; y <= targetY; ++y ) {
							occupancy[x - minX][y - minY] = true;
						}
					}
				}
			}

			configData << quint32( minX ) << quint32( maxX ) << quint32( minY ) << quint32( maxY );

			qDebug( "Zoom: %d; X: %d - %d; Y: %d - %d", zoom, minX, maxX, minY, maxY );
			int numberOfTiles = ( maxX - minX ) * ( maxY - minY );
			std::vector< MapnikIndexElement > index( numberOfTiles );
			qint64 position = 0;
			QFile tilesFile( filename + QString( "_%1_tiles" ).arg( zoom ) );
			tilesFile.open( QIODevice::WriteOnly );

			std::vector< MapnikMetaTile > tasks;
			for ( int x = minX; x < maxX; x+= settings.metaTileSize ) {
				int metaTileSizeX = std::min( settings.metaTileSize, maxX - x );
				for ( int y = minY; y < maxY; y+= settings.metaTileSize ) {
					int metaTileSizeY = std::min( settings.metaTileSize, maxY - y );
					MapnikMetaTile tile;
					tile.x = x;
					tile.y = y;
					tile.metaTileSizeX = metaTileSizeX;
					tile.metaTileSizeY = metaTileSizeY;
					tasks.push_back( tile );
				}
			}


#pragma omp parallel for schedule( dynamic )
			for ( int i = 0; i < ( int ) tasks.size(); i++ ) {

				int metaTileSizeX = tasks[i].metaTileSizeX;
				int metaTileSizeY = tasks[i].metaTileSizeY;
				int x = tasks[i].x;
				int y = tasks[i].y;

				int threadID = omp_get_thread_num();
				mapnik::Map& map = *maps[threadID];
				mapnik::Image32& image = *images[threadID];

				map.resize( metaTileSizeX * settings.tileSize + 2 * settings.margin, metaTileSizeY * settings.tileSize + 2 * settings.margin );

				ProjectedCoordinate drawTopLeft( x - 1.0 * settings.margin / settings.tileSize, y - 1.0 * settings.margin / settings.tileSize, zoom );
				ProjectedCoordinate drawBottomRight( x + metaTileSizeX + 1.0 * settings.margin / settings.tileSize, y + metaTileSizeY + 1.0 * settings.margin / settings.tileSize, zoom );
				GPSCoordinate drawTopLeftGPS = drawTopLeft.ToGPSCoordinate();
				GPSCoordinate drawBottomRightGPS = drawBottomRight.ToGPSCoordinate();
				projection.forward( drawTopLeftGPS.longitude, drawBottomRightGPS.latitude );
				projection.forward( drawBottomRightGPS.longitude, drawTopLeftGPS.latitude );
				mapnik::Envelope<double> boundingBox( drawTopLeftGPS.longitude, drawTopLeftGPS.latitude, drawBottomRightGPS.longitude, drawBottomRightGPS.latitude );
				map.zoomToBox( boundingBox );
				mapnik::agg_renderer<mapnik::Image32> renderer( map, image );
				renderer.apply();

				for ( int subX = 0; subX < metaTileSizeX; ++subX ) {
					for ( int subY = 0; subY < metaTileSizeY; ++subY ) {
						int indexNumber = ( y + subY - minY ) * ( maxX - minX ) + x + subX - minX;
						mapnik::image_view<mapnik::ImageData32> view = image.get_view( subX * settings.tileSize + settings.margin, subY * settings.tileSize + settings.margin, settings.tileSize, settings.tileSize );
						std::string result;
						if ( !settings.deleteTiles || occupancy[x + subX - minX][y + subY - minY] ) {
							if ( settings.reduceColors )
								result = mapnik::save_to_string( view, "png256" );
							else
								result = mapnik::save_to_string( view, "png" );

							if ( settings.pngcrush )
							{
								QTemporaryFile tempOut;
								tempOut.open();
								tempOut.write( result.data(), result.size() );
								tempOut.close();

								QTemporaryFile tempIn;
								tempIn.open();

								QProcess pngcrush;
								pngcrush.start( "pngcrush", QStringList() << tempOut.fileName() << tempIn.fileName() );
								if ( pngcrush.waitForStarted() && pngcrush.waitForFinished() )
								{
									QByteArray buffer = tempIn.readAll();
									if ( buffer.size() != 0 && buffer.size() < ( int ) result.size() )
									{
#pragma omp critical
										pngcrushSaved += result.size() - buffer.size();
										result.assign( buffer.constData(), buffer.size() );
									}
								}
							}
						}
						else {
#pragma omp critical
							tilesSkipped++;
						}

#pragma omp critical
						{
							tilesFile.write( result.data(), result.size() );
							tilesRendered++;
							index[indexNumber].start = position;
							position += result.size();
							index[indexNumber].end = position;
						}
					}
				}
#pragma omp critical
				{
					metaTilesRendered++;
					qDebug( "Zoom: %d, Thread %d, Metatiles: %d, Tiles: %d / %d", zoom, threadID, metaTilesRendered, tilesRendered, ( maxX - minX ) * ( maxY - minY ) );
				}
			}
			qDebug( "Zoom %d: Removed %d of %d tiles", zoom, tilesSkipped, tilesRendered );
			if ( settings.pngcrush )
				qDebug( "PNGcrush saved %lld KB -> %lld KB [%lld %%]", pngcrushSaved / 1024, position / 1024, 100 * pngcrushSaved / position );
			QFile indexFile( filename + QString( "_%1_index" ).arg( zoom ) );
			indexFile.open( QIODevice::WriteOnly );
			for ( int i = 0; i < ( int ) index.size(); i++ )
			{
				indexFile.write( ( const char* ) &index[i].start, sizeof( index[i].start ) );
				indexFile.write( ( const char* ) &index[i].end, sizeof( index[i].end ) );
			}
		}

		for ( int i = 0; i < numThreads; i++ ) {
			delete maps[i];
			delete images[i];
		}
	} catch ( const mapnik::config_error & ex ) {
		qCritical( "### Configuration error: %s", ex.what() );
		return false;
	} catch ( const std::exception & ex ) {
		qCritical( "### STD error: %s", ex.what() );
		return false;
	} catch ( ... ) {
		qCritical( "### Unknown error" );
		return false;
	}
	return true;
}

Q_EXPORT_PLUGIN2( mapnikrenderer, MapnikRenderer )
