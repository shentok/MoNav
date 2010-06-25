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

#include <QDir>
#include <QPainter>
#include <algorithm>
#include "mapnikrendererclient.h"

MapnikRendererClient::MapnikRendererClient()
{
	maxZoom = -1;
	tileSize = 1;
}

MapnikRendererClient::~MapnikRendererClient()
{

}

void MapnikRendererClient::unload()
{
	boxes.clear();
	cache.clear();
}

QString MapnikRendererClient::GetName()
{
	return "Mapnik Renderer";
}

void MapnikRendererClient::SetInputDirectory( const QString& dir )
{
	directory = dir;
}

void MapnikRendererClient::ShowSettings()
{

}

bool MapnikRendererClient::LoadData()
{
	if ( loaded )
		unload();

	QDir dir( directory );
	QString filename = dir.filePath( "Mapnik Renderer" );
	QFile configFile( filename );
	configFile.open( QIODevice::ReadOnly );
	QDataStream configData( &configFile );
	quint32 zoom, size;
	configData >> size >> zoom;
	maxZoom = zoom;
	tileSize = size;
	for ( int i = 0; i <= maxZoom; i++ )
	{
		quint32 minX, maxX, minY, maxY;
		configData >> minX >> maxX >> minY >> maxY;
		if ( configData.status() == QDataStream::ReadPastEnd )
			return false;
		Box box;
		box.minX = minX;
		box.maxX = maxX;
		box.minY = minY;
		box.maxY = maxY;
		boxes.push_back( box );
	}
	loaded = true;
	return true;
}

int MapnikRendererClient::GetMaxZoom()
{
	if ( !loaded )
		return -1;
	return maxZoom;
}

ProjectedCoordinate MapnikRendererClient::Move( ProjectedCoordinate center, int shiftX, int shiftY, int zoom )
{
	if ( !loaded )
		return center;
	center.x -= ( double ) shiftX / tileSize / ( 1u << zoom );
	center.y -= ( double ) shiftY / tileSize / ( 1u << zoom );
	return center;
}

ProjectedCoordinate MapnikRendererClient::PointToCoordinate( ProjectedCoordinate center, int shiftX, int shiftY, int zoom )
{
	if ( !loaded )
		return center;
	center.x += ( ( double ) shiftX ) / tileSize / ( 1u << zoom );
	center.y += ( ( double ) shiftY ) / tileSize / ( 1u << zoom );
	return center;
}

ProjectedCoordinate MapnikRendererClient::ZoomInOn( ProjectedCoordinate center, ProjectedCoordinate zoomPoint, int /*zoom*/ )
{
	if ( !loaded )
		return center;
	center.x = center.x + zoomPoint.x;
	center.y = center.y + zoomPoint.y;
	return center;
}

ProjectedCoordinate MapnikRendererClient::ZoomOutOn( ProjectedCoordinate center, ProjectedCoordinate zoomPoint, int /*zoom*/ )
{
	if ( !loaded )
		return center;
	center.x = center.x - zoomPoint.x / 2;
	center.y = center.y - zoomPoint.y / 2;
	return center;
}

bool MapnikRendererClient::SetPoints( QVector< UnsignedCoordinate > p )
{
	if ( !loaded )
		return false;
	points = p;
	return true;
}

bool MapnikRendererClient::SetEdges( QVector< int > s, QVector< UnsignedCoordinate > e )
{
	if ( !loaded )
		return false;
	segmentLengths = s;
	edges = e;
	return true;
}

bool MapnikRendererClient::SetPosition( UnsignedCoordinate coordinate, double heading )
{
	if ( !loaded )
		return false;
	return false;
}

bool MapnikRendererClient::Paint( QPainter* painter, ProjectedCoordinate center, int zoomLevel, double rotation, double virtualZoom )
{
	if ( !loaded )
		return false;

	int sizeX = painter->device()->width();
	int sizeY = painter->device()->height();

	if ( sizeX <= 1 && sizeY <= 1 )
		return true;

	bool drawFast = true;
	if ( fabs( rotation ) > 0.01 || virtualZoom > 0 )
	{
		drawFast = false;
		painter->translate( centerX, centerY );
		painter->rotate( rotation );
		painter->translate( -centerX, -centerY );
		if ( rotation != 90 && rotation != 180 && rotation != 270 ) {
			painter->setRenderHint( QPainter::SmoothPixmapTransform );
			painter->setRenderHint( QPainter::Antialiasing );
			painter->setRenderHint( QPainter::HighQualityAntialiasing );
		}
	}

	const int centerX = sizeX / 2;
	const int centerY = sizeY / 2;

	const int xWidth = boxes[zoomLevel].maxX - boxes[zoomLevel].minX;
	const int yWidth = boxes[zoomLevel].maxY - boxes[zoomLevel].minY;
	const double left = center.x * ( 1u << zoomLevel ) - ( ( double ) centerX ) / tileSize;
	const double top = center.y * ( 1u << zoomLevel ) - ( ( double ) centerY ) / tileSize;

	if ( !drawFast )
	{
		/*TODO*/
	}

	double minXBias = 0;
	double minYBias = 0;
	double maxXBias = ( double ) sizeX / tileSize;
	double maxYBias = ( double ) sizeY / tileSize;

	if ( !drawFast )
	{

	}

	const int minX = floor( left + minXBias ) - boxes[zoomLevel].minX;
	const int minY = floor( top + minYBias ) - boxes[zoomLevel].minY;
	const int maxX = ceil( maxXBias + left ) - boxes[zoomLevel].minX;
	const int maxY = ceil( maxYBias + top ) - boxes[zoomLevel].minY;

	const int leftX = left - boxes[zoomLevel].minX;
	const int topY = top - boxes[zoomLevel].minY;
	const int leftSubX = ( left - boxes[zoomLevel].minX - leftX ) * tileSize;
	const int leftSubY = ( top - boxes[zoomLevel].minY - topY ) * tileSize;

	QDir dir( directory );
	QString filename = dir.filePath( "Mapnik Renderer" );
	QFile indexFile( filename + QString( "_%1_index" ).arg( zoomLevel ) );
	indexFile.open( QIODevice::ReadOnly );
	QFile imageFile( filename + QString( "_%1_tiles" ).arg( zoomLevel ) );
	imageFile.open( QIODevice::ReadOnly );

	for ( int x = minX; x < maxX; ++x ) {
		for ( int y = minY; y < maxY; ++y ) {
			const int xPos = ( x - leftX ) * tileSize - leftSubX;
			const int yPos = ( y - topY ) * tileSize - leftSubY;
			const int indexPosition = y * xWidth + x;

			if ( !drawFast ) {
				//TODO
			}

			QPixmap* tile = NULL;
			if ( y >= 0 && y < yWidth && x >= 0 && x < xWidth ) {
				long long id = ( indexPosition << 8 ) + zoomLevel;
				if ( !cache.contains( id ) )
				{
					indexFile.seek( indexPosition * 2 * sizeof( qint64 ) );
					qint64 start, end;
					indexFile.read( ( char* ) &start, sizeof( start ) );
					indexFile.read( ( char* ) &end, sizeof( end ) );
					if ( start != end )
					{
						uchar* data = new uchar[end - start];
						imageFile.seek( start );
						imageFile.read( ( char * ) data, end - start );
						tile = new QPixmap();
						if ( !tile->loadFromData( data, end - start, "PNG" ) )
						{
							qDebug( "failed to load picture" );
							delete tile;
							tile = NULL;
						}
						cache.insert( id, tile, 1 );
					}
				}
				else
				{
					tile = cache.object( id );
				}
			}

			if ( tile != NULL )
				painter->drawPixmap( xPos, yPos, *tile );
			else
				painter->fillRect( xPos, yPos, tileSize, tileSize, QColor( 241, 238 , 232, 255 ) );
		}
	}

	if ( points.size() > 0 ) {
		QPen oldPen = painter->pen();
		painter->setRenderHint( QPainter::Antialiasing );
		painter->setPen( QPen( QColor( 0, 0, 255 ), 6, Qt::SolidLine, Qt::RoundCap ) );
		for ( int i = 0; i < points.size(); i++ ) {
			ProjectedCoordinate pos = points[i].ToProjectedCoordinate();
			pos.x = ( pos.x - center.x ) * ( 1 << zoomLevel ) * tileSize + centerX;
			pos.y = ( pos.y - center.y ) * ( 1 << zoomLevel ) * tileSize + centerY;
			painter->drawEllipse( pos.x - 8, pos.y - 8, 16, 16);
		}
		painter->setPen( QPen( QColor( 255, 0, 0 ), 4, Qt::SolidLine, Qt::RoundCap ) );
		for ( int i = 0; i < points.size(); i++ ) {
			ProjectedCoordinate pos = points[i].ToProjectedCoordinate();
			pos.x = ( pos.x - center.x ) * ( 1 << zoomLevel ) * tileSize + centerX;
			pos.y = ( pos.y - center.y ) * ( 1 << zoomLevel ) * tileSize + centerY;
			painter->drawEllipse( pos.x - 8, pos.y - 8, 16, 16);
		}
		painter->setPen( oldPen );
		painter->setRenderHint( QPainter::Antialiasing, false );
	}

	return true;
}


Q_EXPORT_PLUGIN2( mapnikrendererclient, MapnikRendererClient )
