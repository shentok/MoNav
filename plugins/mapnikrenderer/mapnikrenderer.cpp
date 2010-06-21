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

#include <QLibrary>

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
	bool aborted = false;

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

		qDebug( "Initialize Database Link" );

		QDir pluginDir( settings.plugins );
		foreach ( QString fileName, pluginDir.entryList( QDir::Files ) ) {
			QLibrary* loader = new QLibrary( pluginDir.absoluteFilePath( fileName ) );
			loader->load();
			qDebug( "%s is libary: %i", pluginDir.absoluteFilePath( fileName ).toAscii().constData(), loader->isLoaded() );
			if ( !loader->isLoaded() )
				qDebug( "Reason: %s", loader->errorString().toAscii().constData() );
		}

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
		omp_set_nested( true );

		mapnik::Map* maps[numThreads];
		mapnik::Image32* images[numThreads];
		for ( int i = 0; i < numThreads; i++ ) {
			maps[i] = new mapnik::Map;
			mapnik::load_map( *maps[i], settings.theme.toAscii().constData() );
			const int metaTileSize = settings.metaTileSize * settings.tileSize + 2 * settings.margin;
			images[i] = new mapnik::Image32( metaTileSize, metaTileSize );
		}
		qDebug( "Using %d Threads", numThreads );

		long long progressMax = 0;
		long long progressValue = 0;
		for ( int zoom = 0; zoom <= settings.maxZoom; zoom++ )
		{
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
			long long x = ( maxX - minX + settings.metaTileSize - 1 ) / settings.metaTileSize;
			long long y = ( maxY - minY + settings.metaTileSize - 1 ) / settings.metaTileSize;
			progressMax += x * y;
		}

		QProgressDialog progress( tr( "Rendering tiles..." ), tr( "Abort Rendering" ), 0, progressMax );
		progress.setWindowModality( Qt::WindowModal );
		progress.setMinimumDuration( 0 );
		progress.setValue( 0 );

		for ( int zoom = 0; zoom <= settings.maxZoom; zoom++ ) {

			if ( aborted )
				break;

			int tilesRendered = 0;
			int tilesSkipped = 0;
			int metaTilesRendered = 0;

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
			/*if ( removeEmpty ) {
				occupancy.resize( maxX - minX );
				for ( int x = minX; x < maxX; ++x )
					occupancy[x - minX].resize( maxY - minY, false );
				for ( std::vector< InputEdge >::const_iterator i = inputEdges.begin(), e = inputEdges.end(); i != e; ++i ) {
					int sourceX = inputCoordinates[i->source].GetTileX( zoom );
					int sourceY = inputCoordinates[i->source].GetTileY( zoom );
					int targetX = inputCoordinates[i->target].GetTileX( zoom );
					int targetY = inputCoordinates[i->target].GetTileY( zoom );
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
			}*/

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

			progress.setValue( progressValue );
#pragma omp parallel for schedule( dynamic )
			for ( int i = 0; i < ( int ) tasks.size(); i++ ) {

				if ( aborted )
					continue;

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
					progressValue++;
					if ( omp_get_thread_num() == 0 )
					{
						progress.setLabelText( QString( "Zoom: %1, Tiles: %2, Metatiles: %3" ).arg( zoom ).arg( tilesRendered ).arg( metaTilesRendered ) );
						progress.setValue( progressValue );
						if ( progress.wasCanceled() )
						{
							aborted = true;
						}
					}
					qDebug( "Zoom: %d, Tiles: %d, Metatiles: %d, Thread %d", zoom, tilesRendered, metaTilesRendered, threadID );
				}
			}
			qDebug( "Zoom %d: Removed %d of %d tiles", zoom, tilesSkipped, tilesRendered );
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
	return !aborted;
}

Q_EXPORT_PLUGIN2( MapnikRenderer, MapnikRenderer )
