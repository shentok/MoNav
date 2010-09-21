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
#include "utils/qthelpers.h"

#include <mapnik/map.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/font_engine_freetype.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/load_map.hpp>
#include <omp.h>
#include <QFile>
#include <QTemporaryFile>
#include <QProcess>
#include <QtDebug>

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
	QString filename = fileInDirectory( outputDirectory, "Mapnik Renderer" );

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
				qCritical() << "Mapnik Renderer: failed to read routing Edges";
				return false;
			}
			if ( !importer->GetRoutingNodes( &inputNodes ) ) {
				qCritical() << "Mapnik Renderer: failed to read routing Nodes";
				return false;
			}
		}

		Timer time;

		mapnik::datasource_cache::instance()->register_datasources( settings.plugins.toAscii().constData() );
		QDir fonts( settings.fonts );
		mapnik::projection projection;
		projection = mapnik::projection( "+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +no_defs +over" );
		mapnik::freetype_engine::register_font( fonts.filePath( "DejaVuSans.ttf" ).toAscii().constData() );
		mapnik::freetype_engine::register_font( fonts.filePath( "DejaVuSans-Bold.ttf" ).toAscii().constData() );
		mapnik::freetype_engine::register_font( fonts.filePath( "DejaVuSans-Oblique.ttf" ).toAscii().constData() );
		mapnik::freetype_engine::register_font( fonts.filePath( "DejaVuSans-BoldOblique.ttf" ).toAscii().constData() );

		qDebug() << "Mapnik Renderer: initialized mapnik connection:" << time.restart() << "ms";

		int numThreads = omp_get_max_threads();
		qDebug() << "Mapnik Renderer: using" << numThreads << "threads";

		qDebug() << "Mapnik Renderer: x: " << box.min.x << "-" << box.max.x;
		qDebug() << "Mapnik Renderer: y: " << box.min.y << "-" << box.max.y;

		FileStream configData( filename );
		if ( !configData.open( QIODevice::WriteOnly ) )
			return false;

		configData << quint32( settings.tileSize ) << quint32( settings.zoomLevels.size() );

		mapnik::Map* maps[numThreads];
		mapnik::Image32* images[numThreads];
		for ( int i = 0; i < numThreads; i++ ) {
			maps[i] = new mapnik::Map;
			mapnik::load_map( *maps[i], settings.theme.toLocal8Bit().constData() );
			const int metaTileSize = settings.metaTileSize * settings.tileSize + 2 * settings.margin;
			images[i] = new mapnik::Image32( metaTileSize, metaTileSize );
		}

		qDebug() << "Mapnik Renderer: initialized thread data:" << time.restart() << "ms";

		long long tilesSkipped = 0;
		long long tiles = 0;
		long long metaTilesRendered = 0;
		long long pngcrushSaved = 0;

		std::vector< ZoomInfo > zoomInfo( settings.zoomLevels.size() );
		std::vector< MetaTile > tasks;

		for ( int zoomLevel = 0; zoomLevel < ( int ) settings.zoomLevels.size(); zoomLevel++ ) {
			ZoomInfo& info = zoomInfo[zoomLevel];
			int zoom = settings.zoomLevels[zoomLevel];

			info.minX = box.min.GetTileX( zoom );
			info.maxX = box.max.GetTileX( zoom ) + 1;
			info.minY = box.min.GetTileY( zoom );
			info.maxY = box.max.GetTileY( zoom ) + 1;

			if ( zoom <= settings.fullZoom ) {
				info.minX = info.minY = 0;
				info.maxX = info.maxY = 1 << zoom;
			} else {
				info.minX = std::max( 0 , info.minX - settings.tileMargin );
				info.maxX = std::min ( 1 << zoom, info.maxX + settings.tileMargin );
				info.minY = std::max( 0, info.minY - settings.tileMargin );
				info.maxY = std::min ( 1 << zoom, info.maxY + settings.tileMargin );
			}

			tiles += ( info.maxX - info.minX ) * ( info.maxY - info.minY );
			qDebug() << "Mapnik Renderer: [" << zoom << "] x:" << info.minX << "-" << info.maxX << "; y:" << info.minY << "-" << info.maxY;
			configData << quint32( zoom ) << quint32( info.minX ) << quint32( info.maxX ) << quint32( info.minY ) << quint32( info.maxY );

			int numberOfTiles = ( info.maxX - info.minX ) * ( info.maxY - info.minY );
			IndexElement dummyIndex;
			dummyIndex.start = dummyIndex.size = 0;
			info.index.resize( numberOfTiles, dummyIndex );

			for ( std::vector< IImporter::RoutingEdge >::const_iterator i = inputEdges.begin(), e = inputEdges.end(); i != e; ++i ) {
				int sourceX = inputNodes[i->source].coordinate.GetTileX( zoom );
				int sourceY = inputNodes[i->source].coordinate.GetTileY( zoom );
				int targetX = inputNodes[i->target].coordinate.GetTileX( zoom );
				int targetY = inputNodes[i->target].coordinate.GetTileY( zoom );
				if ( sourceX > targetX )
					std::swap( sourceX, targetX );
				if ( sourceY > targetY )
					std::swap( sourceY, targetY );
				sourceX = std::max( sourceX, info.minX );
				sourceX = std::min( sourceX, info.maxX - 1 );
				sourceY = std::max( sourceY, info.minY );
				sourceY = std::min( sourceY, info.maxY - 1 );
				targetX = std::max( targetX, info.minX );
				targetX = std::min( targetX, info.maxX - 1 );
				targetY = std::max( targetY, info.minY );
				targetY = std::min( targetY, info.maxY - 1 );
				for ( int x = sourceX; x <= targetX; ++x )
					for ( int y = sourceY; y <= targetY; ++y )
						info.index[( x - info.minX ) + ( y - info.minY ) * ( info.maxX - info.minX )].size = 1;
			}

			info.tilesFile = new QFile( filename + QString( "_%1_tiles" ).arg( zoom ) );
			if ( !openQFile( info.tilesFile, QIODevice::WriteOnly ) )
				return false;

			for ( int x = info.minX; x < info.maxX; x+= settings.metaTileSize ) {
				int metaTileSizeX = std::min( settings.metaTileSize, info.maxX - x );
				for ( int y = info.minY; y < info.maxY; y+= settings.metaTileSize ) {
					int metaTileSizeY = std::min( settings.metaTileSize, info.maxY - y );
					MetaTile tile;
					tile.zoom = zoomLevel;
					tile.x = x;
					tile.y = y;
					tile.metaTileSizeX = metaTileSizeX;
					tile.metaTileSizeY = metaTileSizeY;
					tasks.push_back( tile );
				}
			}
		}


#pragma omp parallel for schedule( dynamic )
			for ( int i = 0; i < ( int ) tasks.size(); i++ ) {

				int metaTileSizeX = tasks[i].metaTileSizeX;
				int metaTileSizeY = tasks[i].metaTileSizeY;
				int x = tasks[i].x;
				int y = tasks[i].y;
				int zoomLevel = tasks[i].zoom;
				int zoom = settings.zoomLevels[zoomLevel];
				ZoomInfo& info = zoomInfo[zoomLevel];

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

				std::string data;
				int skipped = 0;
				int saved = 0;
				for ( int subX = 0; subX < metaTileSizeX; ++subX ) {
					for ( int subY = 0; subY < metaTileSizeY; ++subY ) {
						int indexNumber = ( y + subY - info.minY ) * ( info.maxX - info.minX ) + x + subX - info.minX;
						mapnik::image_view<mapnik::ImageData32> view = image.get_view( subX * settings.tileSize + settings.margin, subY * settings.tileSize + settings.margin, settings.tileSize, settings.tileSize );
						std::string result;
						if ( !settings.deleteTiles || info.index[( x + subX - info.minX ) + ( y + subY - info.minY ) * ( info.maxX - info.minX )].size == 1 ) {
							if ( settings.reduceColors )
								result = mapnik::save_to_string( view, "png256" );
							else
								result = mapnik::save_to_string( view, "png" );

							if ( settings.pngcrush ) {
								QTemporaryFile tempOut;
								tempOut.open();
								tempOut.write( result.data(), result.size() );
								tempOut.close();

								QTemporaryFile tempIn;
								tempIn.open();

								QProcess pngcrush;
								pngcrush.start( "pngcrush", QStringList() << tempOut.fileName() << tempIn.fileName() );
								if ( pngcrush.waitForStarted() && pngcrush.waitForFinished() ) {
									QByteArray buffer = tempIn.readAll();
									if ( buffer.size() != 0 && buffer.size() < ( int ) result.size() ) {
										saved += result.size() - buffer.size();
										result.assign( buffer.constData(), buffer.size() );
									}
								}
							}
						}

						info.index[indexNumber].start = data.size();
						info.index[indexNumber].size = result.size();
						data += result;
					}
				}

				qint64 position;
#pragma omp critical
				{
					position = info.tilesFile->pos();
					info.tilesFile->write( data.data(), data.size() );

					metaTilesRendered++;
					tilesSkipped += skipped;
					pngcrushSaved += saved;
					qDebug() << "Mapnik Renderer: [" << zoom << "], thread" << threadID << ", metatiles:" << metaTilesRendered << "/" << tasks.size();
				}

				for ( int subX = 0; subX < metaTileSizeX; ++subX ) {
					for ( int subY = 0; subY < metaTileSizeY; ++subY ) {
						int indexNumber = ( y + subY - info.minY ) * ( info.maxX - info.minX ) + x + subX - info.minX;
						info.index[indexNumber].start += position;
					}
				}
			}

		for ( int i = 0; i < numThreads; i++ ) {
			delete maps[i];
			delete images[i];
		}

		for ( int zoomLevel = 0; zoomLevel < ( int ) settings.zoomLevels.size(); zoomLevel++ ) {
			const ZoomInfo& info = zoomInfo[zoomLevel];
			int zoom = settings.zoomLevels[zoomLevel];
			QFile indexFile( filename + QString( "_%1_index" ).arg( zoom ) );
			if ( !openQFile( &indexFile, QIODevice::WriteOnly ) )
				return false;
			for ( int i = 0; i < ( int ) info.index.size(); i++ ) {
				indexFile.write( ( const char* ) &info.index[i].start, sizeof( info.index[i].start ) );
				indexFile.write( ( const char* ) &info.index[i].size, sizeof( info.index[i].size ) );
			}
			delete info.tilesFile;
		}

		if ( settings.deleteTiles )
			qDebug() << "Mapnik Renderer: removed" << tilesSkipped << "tiles";
		if ( settings.pngcrush )
			qDebug() << "Mapnik Renderer: PNGcrush saved" << pngcrushSaved / 1024 / 1024 << "MB";

		qDebug() << "Mapnik Renderer: finished:" << time.restart() << "ms";

	} catch ( const mapnik::config_error & ex ) {
		qCritical( "Mapnik Renderer: ### Configuration error: %s", ex.what() );
		return false;
	} catch ( const std::exception & ex ) {
		qCritical( "Mapnik Renderer: ### STD error: %s", ex.what() );
		return false;
	} catch ( ... ) {
		qCritical( "Mapnik Renderer: ### Unknown error" );
		return false;
	}
	return true;
}

Q_EXPORT_PLUGIN2( mapnikrenderer, MapnikRenderer )
