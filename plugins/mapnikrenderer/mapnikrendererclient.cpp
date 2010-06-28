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
#include <cmath>
#include "mapnikrendererclient.h"
#include "utils/utils.h"

MapnikRendererClient::MapnikRendererClient()
{
	maxZoom = -1;
	tileSize = 1;
	setupPolygons();
}

MapnikRendererClient::~MapnikRendererClient()
{

}

void MapnikRendererClient::unload()
{
	boxes.clear();
	cache.clear();
}

void MapnikRendererClient::setupPolygons()
{
	double leftPointer  = 135.0 / 180.0 * M_PI;
	double rightPointer = -135.0 / 180.0 * M_PI;
	arrow << QPointF( cos( leftPointer ), sin( leftPointer ) );
	arrow << QPointF( 1, 0 );
	arrow << QPointF( cos( rightPointer ), sin( rightPointer ) );
	arrow << QPointF( -3.0 / 8, 0 );
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
	if ( fmod( rotation / 90, 1 ) < 0.01 )
		rotation = 90 * floor( rotation / 90 );
	else if ( fmod( rotation / 90, 1 ) > 0.99 )
		rotation = 90 * ceil( rotation / 90 );

	double zoomFactor = ( double ) ( 1 << zoomLevel ) * tileSize;
	painter->translate( sizeX / 2, sizeY / 2 );
	if ( virtualZoom > 0 )
		painter->scale( virtualZoom, virtualZoom );
	painter->rotate( rotation );
	painter->translate( -center.x * zoomFactor, -center.y * zoomFactor );
	if ( fabs( rotation ) > 1 || virtualZoom > 0 ) {
		painter->setRenderHint( QPainter::SmoothPixmapTransform );
		painter->setRenderHint( QPainter::Antialiasing );
		painter->setRenderHint( QPainter::HighQualityAntialiasing );
	}

	QTransform transform = painter->worldTransform();
	QTransform inverseTransform = transform.inverted();

	const int xWidth = boxes[zoomLevel].maxX - boxes[zoomLevel].minX;
	const int yWidth = boxes[zoomLevel].maxY - boxes[zoomLevel].minY;

	QPointF corner1 = inverseTransform.map( QPointF( 0, 0 ) );
	QPointF corner2 = inverseTransform.map( QPointF( 0, sizeY ) );
	QPointF corner3 = inverseTransform.map( QPointF( sizeX, 0 ) );
	QPointF corner4 = inverseTransform.map( QPointF( sizeX, sizeY ) );

	int minX = floor( std::min( corner1.x(), std::min( corner2.x(), std::min( corner3.x(), corner4.x() ) ) ) / tileSize );
	int maxX = ceil( std::max( corner1.x(), std::max( corner2.x(), std::max( corner3.x(), corner4.x() ) ) ) / tileSize );
	int minY = floor( std::min( corner1.y(), std::min( corner2.y(), std::min( corner3.y(), corner4.y() ) ) ) / tileSize );
	int maxY = ceil( std::max( corner1.y(), std::max( corner2.y(), std::max( corner3.y(), corner4.y() ) ) ) / tileSize );

	QDir dir( directory );
	QString filename = dir.filePath( "Mapnik Renderer" );
	QFile indexFile( filename + QString( "_%1_index" ).arg( zoomLevel ) );
	indexFile.open( QIODevice::ReadOnly );
	QFile imageFile( filename + QString( "_%1_tiles" ).arg( zoomLevel ) );
	imageFile.open( QIODevice::ReadOnly );

	for ( int x = minX; x < maxX; ++x ) {
		for ( int y = minY; y < maxY; ++y ) {
			const int xID = x - boxes[zoomLevel].minX;
			const int yID = y - boxes[zoomLevel].minY;
			const long long indexPosition = yID * xWidth + xID;

			QPixmap* tile = NULL;
			if ( xID >= 0 && xID < xWidth && yID >= 0 && yID < yWidth ) {
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
				painter->drawPixmap( x * tileSize, y * tileSize, *tile );
			else
				painter->fillRect( x * tileSize, y * tileSize, tileSize, tileSize, QColor( 241, 238 , 232, 255 ) );
		}
	}

	if ( points.size() > 0 ) {
		QPen oldPen = painter->pen();
		QBrush oldBrush = painter->brush();
		painter->setRenderHint( QPainter::Antialiasing );

		for ( int i = 0; i < points.size(); i++ ) {
			painter->setPen( QPen( QColor( 0, 0, 128 ), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
			ProjectedCoordinate pos = points[i].ToProjectedCoordinate();
			QPointF mapped = transform.map( QPointF( pos.x * zoomFactor, pos.y * zoomFactor ) );
			if ( mapped.x() < 3 || mapped.y() < 3 || mapped.x() >= sizeX - 3 || mapped.y() >= sizeY - 3 ) {
				//clip an imaginary line from the screen center to pos at the screen boundaries
				ProjectedCoordinate start( mapped.x(), mapped.y() );
				ProjectedCoordinate end( sizeX / 2, sizeY / 2 );
				clipEdge( &start, &end, ProjectedCoordinate( 10, 10 ), ProjectedCoordinate( sizeX - 10, sizeY - 10) );

				QPointF position = inverseTransform.map( QPointF( start.x, start.y ) );
				QMatrix arrowMatrix;
				arrowMatrix.translate( position.x(), position.y() );
				arrowMatrix.rotate( atan2( mapped.y() - sizeY / 2, mapped.x() - sizeX / 2 ) * 360 / 2 / M_PI );
				arrowMatrix.scale( 8, 8 );

				painter->setBrush( QColor( 0, 0, 128 ) );
				painter->drawPolygon( arrowMatrix.map( arrow ) );
				painter->setBrush( QColor( 255, 0, 0 ) );
				painter->setPen( QPen( QColor( 255, 0, 0 ), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
				painter->drawPolygon( arrowMatrix.map( arrow ) );
			}
			else {
				painter->setBrush( oldBrush );
				painter->drawEllipse( pos.x * zoomFactor - 8, pos.y * zoomFactor - 8, 16, 16);
				painter->setPen( QPen( QColor( 255, 0, 0 ), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
				painter->drawEllipse( pos.x * zoomFactor - 8, pos.y * zoomFactor - 8, 16, 16);
			}
		}

		painter->setBrush( oldBrush );
		painter->setPen( oldPen );
		painter->setRenderHint( QPainter::Antialiasing, false );
	}

	return true;
}


Q_EXPORT_PLUGIN2( mapnikrendererclient, MapnikRendererClient )
