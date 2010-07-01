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

bool MapnikRendererClient::Paint( QPainter* painter, const PaintRequest& request )
{
	if ( !loaded )
		return false;
	if ( request.zoom < 0 || request.zoom > maxZoom )
		return false;

	int sizeX = painter->device()->width();
	int sizeY = painter->device()->height();

	if ( sizeX <= 1 && sizeY <= 1 )
		return true;
	double rotation = request.rotation;
	if ( fmod( rotation / 90, 1 ) < 0.01 )
		rotation = 90 * floor( rotation / 90 );
	else if ( fmod( rotation / 90, 1 ) > 0.99 )
		rotation = 90 * ceil( rotation / 90 );

	double zoomFactor = ( double ) ( 1 << request.zoom ) * tileSize;
	painter->translate( sizeX / 2, sizeY / 2 );
	if ( request.virtualZoom > 0 )
		painter->scale( request.virtualZoom, request.virtualZoom );
	painter->rotate( rotation );
	painter->translate( -request.center.x * zoomFactor, -request.center.y * zoomFactor );
	if ( fabs( rotation ) > 1 || request.virtualZoom > 0 ) {
		painter->setRenderHint( QPainter::SmoothPixmapTransform );
		painter->setRenderHint( QPainter::Antialiasing );
		painter->setRenderHint( QPainter::HighQualityAntialiasing );
	}

	QTransform transform = painter->worldTransform();
	QTransform inverseTransform = transform.inverted();

	const int xWidth = boxes[request.zoom].maxX - boxes[request.zoom].minX;
	const int yWidth = boxes[request.zoom].maxY - boxes[request.zoom].minY;

	QRectF boundingBox = inverseTransform.mapRect( QRectF(0, 0, sizeX, sizeY ) );

	int minX = floor( boundingBox.x() / tileSize );
	int maxX = ceil( boundingBox.right() / tileSize );
	int minY = floor( boundingBox.y() / tileSize );
	int maxY = ceil( boundingBox.bottom() / tileSize );

	QDir dir( directory );
	QString filename = dir.filePath( "Mapnik Renderer" );
	QFile indexFile( filename + QString( "_%1_index" ).arg( request.zoom ) );
	indexFile.open( QIODevice::ReadOnly );
	QFile imageFile( filename + QString( "_%1_tiles" ).arg( request.zoom ) );
	imageFile.open( QIODevice::ReadOnly );

	for ( int x = minX; x < maxX; ++x ) {
		for ( int y = minY; y < maxY; ++y ) {
			const int xID = x - boxes[request.zoom].minX;
			const int yID = y - boxes[request.zoom].minY;
			const long long indexPosition = yID * xWidth + xID;

			QPixmap* tile = NULL;
			if ( xID >= 0 && xID < xWidth && yID >= 0 && yID < yWidth ) {
				long long id = ( indexPosition << 8 ) + request.zoom;
				if ( !cache.contains( id ) ) {
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
				else {
					tile = cache.object( id );
				}
			}

			if ( tile != NULL )
				painter->drawPixmap( x * tileSize, y * tileSize, *tile );
			else
				painter->fillRect( x * tileSize, y * tileSize, tileSize, tileSize, QColor( 241, 238 , 232, 255 ) );
		}
	}

	painter->setRenderHint( QPainter::Antialiasing );

	if ( request.edgeSegments.size() > 0 && request.edges.size() > 0 ) {
		painter->setPen( QPen( QColor( 0, 0, 128, 128 ), 8, Qt::SolidLine, Qt::FlatCap ) );

		int position = 0;
		for ( int i = 0; i < request.edgeSegments.size(); i++ ) {
			QPolygonF polygon;
			for ( ; position < request.edgeSegments[i]; position++ ) {
				ProjectedCoordinate pos = request.edges[position].ToProjectedCoordinate();
				polygon << QPointF( pos.x * zoomFactor, pos.y * zoomFactor );
			}
			painter->drawPolyline( polygon );
		}
	}

	if ( request.route.size() > 0 ) {
		painter->setPen( QPen( QColor( 0, 0, 128, 128 ), 8, Qt::SolidLine, Qt::FlatCap ) );

		QPolygonF polygon;
		QVector< QPointF > p;
		int step = 1;
		for ( int i = 1; i < request.route.size(); i+= step ) {
			ProjectedCoordinate pos = request.route[i].ToProjectedCoordinate();
			polygon << QPointF( pos.x * zoomFactor, pos.y * zoomFactor );
		}
		painter->drawPolyline( polygon );
	}

	if ( request.POIs.size() > 0 ) {
		for ( int i = 0; i < request.POIs.size(); i++ ) {
			ProjectedCoordinate pos = request.POIs[i].ToProjectedCoordinate();
			drawIndicator( painter, transform, inverseTransform, pos.x * zoomFactor, pos.y * zoomFactor, sizeX, sizeY, QColor( 196, 0, 0 ), QColor( 0, 0, 196 ) );
		}
	}

	if ( request.target.x != 0 || request.target.y != 0 )
	{
		ProjectedCoordinate pos = request.target.ToProjectedCoordinate();
		drawIndicator( painter, transform, inverseTransform, pos.x * zoomFactor, pos.y * zoomFactor, sizeX, sizeY, QColor( 0, 0, 128 ), QColor( 255, 0, 0 ) );
	}

	if ( request.position.x != 0 || request.position.y != 0 )
	{
		ProjectedCoordinate pos = request.position.ToProjectedCoordinate();
		drawIndicator( painter, transform, inverseTransform, pos.x * zoomFactor, pos.y * zoomFactor, sizeX, sizeY, QColor( 0, 128, 0 ), QColor( 255, 255, 0 ) );
		drawArrow( painter, pos.x * zoomFactor, pos.y * zoomFactor, request.heading * 360 / 2 / M_PI - 90, QColor( 0, 128, 0 ), QColor( 255, 255, 0 ) );
	}

	return true;
}

void MapnikRendererClient::drawArrow( QPainter* painter, int x, int y, double rotation, QColor outer, QColor inner )
{
	QMatrix arrowMatrix;
	arrowMatrix.translate( x, y );
	arrowMatrix.rotate( rotation );
	arrowMatrix.scale( 8, 8 );

	painter->setPen( QPen( outer, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
	painter->setBrush( outer );
	painter->drawPolygon( arrowMatrix.map( arrow ) );
	painter->setPen( QPen( inner, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
	painter->setBrush( inner );
	painter->drawPolygon( arrowMatrix.map( arrow ) );
}

void MapnikRendererClient::drawIndicator( QPainter* painter, const QTransform& transform, const QTransform& inverseTransform, int x, int y, int sizeX, int sizeY, QColor outer, QColor inner )
{
	QPointF mapped = transform.map( QPointF( x, y ) );
	if ( mapped.x() < 3 || mapped.y() < 3 || mapped.x() >= sizeX - 3 || mapped.y() >= sizeY - 3 ) {
		//clip an imaginary line from the screen center to pos at the screen boundaries
		ProjectedCoordinate start( mapped.x(), mapped.y() );
		ProjectedCoordinate end( sizeX / 2, sizeY / 2 );
		clipEdge( &start, &end, ProjectedCoordinate( 10, 10 ), ProjectedCoordinate( sizeX - 10, sizeY - 10) );
		QPointF position = inverseTransform.map( QPointF( start.x, start.y ) );
		double heading = atan2( mapped.y() - sizeY / 2, mapped.x() - sizeX / 2 ) * 360 / 2 / M_PI;
		drawArrow( painter, position.x(), position.y(), heading, outer, inner );
	}
	else {
		painter->setBrush( Qt::NoBrush );
		painter->setPen( QPen( outer, 5 ) );
		painter->drawEllipse( x - 8, y - 8, 16, 16);
		painter->setPen( QPen( inner, 2 ) );
		painter->drawEllipse( x - 8, y - 8, 16, 16);
	}
}


Q_EXPORT_PLUGIN2( mapnikrendererclient, MapnikRendererClient )
