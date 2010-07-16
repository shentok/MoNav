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
#include <QDebug>
#include <QSettings>
#include <QInputDialog>

#include "rendererbase.h"
#include "utils/utils.h"

RendererBase::RendererBase()
{
	loaded = false;
	setupPolygons();
	tileSize = 256;
	settingsDialog = NULL;
}

RendererBase::~RendererBase()
{
	if ( settingsDialog != NULL )
		delete settingsDialog;
}

void RendererBase::reset()
{
	cache.clear();
	unload();
}

void RendererBase::setupPolygons()
{
	double leftPointer  = 135.0 / 180.0 * M_PI;
	double rightPointer = -135.0 / 180.0 * M_PI;
	arrow << QPointF( cos( leftPointer ), sin( leftPointer ) );
	arrow << QPointF( 1, 0 );
	arrow << QPointF( cos( rightPointer ), sin( rightPointer ) );
	arrow << QPointF( -3.0 / 8, 0 );
}

void RendererBase::SetInputDirectory( const QString& dir )
{
	directory = dir;
}

void RendererBase::ShowSettings()
{
	assert( loaded );
	settingsDialog->exec();
	if ( !settingsDialog->getSettings( &settings ) )
		return;
	cache.setMaxCost( 1024 * 1024 * settings.cacheSize );
}

bool RendererBase::LoadData()
{
	if ( loaded )
		reset();

	if ( settingsDialog == NULL )
		settingsDialog = new BRSettingsDialog();
	if ( !settingsDialog->getSettings( &settings ) )
		return false;
	cache.setMaxCost( 1024 * 1024 * settings.cacheSize );

	if ( !load() )
		return false;
	loaded = true;
	return true;
}

ProjectedCoordinate RendererBase::Move( int shiftX, int shiftY, const PaintRequest& request )
{
	if ( !loaded )
		return request.center;
	ProjectedCoordinate center = request.center;
	if ( request.rotation != 0 ) {
		int newX, newY;
		QTransform transform;
		transform.rotate( -request.rotation );
		transform.map( shiftX, shiftY, &newX, &newY );
		shiftX = newX;
		shiftY = newY;
	}
	center.x -= ( double ) shiftX / tileSize / ( 1u << request.zoom ) / ( request.virtualZoom );
	center.y -= ( double ) shiftY / tileSize / ( 1u << request.zoom ) / ( request.virtualZoom );
	return center;
}

ProjectedCoordinate RendererBase::PointToCoordinate( int shiftX, int shiftY, const PaintRequest& request )
{
	return Move( -shiftX, -shiftY, request );
}

void RendererBase::setSlot( QObject* obj, const char* slot )
{
	connect( this, SIGNAL(changed()), obj, slot );
}

long long RendererBase::tileID( int x, int y, int zoom )
{
	assert( zoom < 24 );
	long long id = y;
	id = ( id << 24 ) | x;
	id = ( id << 5 ) | zoom;
	return id;
}

bool RendererBase::Paint( QPainter* painter, const PaintRequest& request )
{
	if ( !loaded )
		return false;
	if ( request.zoom < 0 || request.zoom > GetMaxZoom() )
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

	double tileFactor = 1u << request.zoom;
	double zoomFactor = tileFactor * tileSize;
	painter->translate( sizeX / 2, sizeY / 2 );
	if ( request.virtualZoom > 0 )
		painter->scale( request.virtualZoom, request.virtualZoom );
	painter->rotate( rotation );

	if ( settings.filter && ( fmod( rotation, 90 ) != 0 || request.virtualZoom > 1 ) )
		painter->setRenderHint( QPainter::SmoothPixmapTransform );

	if ( settings.hqAntiAliasing )
		painter->setRenderHint( QPainter::HighQualityAntialiasing );

	QTransform transform = painter->worldTransform();
	QTransform inverseTransform = transform.inverted();

	const int xWidth = 1 << request.zoom;
	const int yWidth = 1 << request.zoom;

	QRect boundingBox = inverseTransform.mapRect( QRect(0, 0, sizeX, sizeY ) );

	int minX = floor( ( double ) boundingBox.x() / tileSize + request.center.x * tileFactor );
	int maxX = ceil( ( double ) boundingBox.right() / tileSize + request.center.x * tileFactor );
	int minY = floor( ( double ) boundingBox.y() / tileSize + request.center.y * tileFactor );
	int maxY = ceil( ( double ) boundingBox.bottom() / tileSize + request.center.y * tileFactor );

	int posX = ( minX - request.center.x * tileFactor ) * tileSize;
	for ( int x = minX; x < maxX; ++x ) {
		int posY = ( minY - request.center.y * tileFactor ) * tileSize;
		for ( int y = minY; y < maxY; ++y ) {

			QPixmap* tile = NULL;
			if ( x >= 0 && x < xWidth && y >= 0 && y < yWidth ) {
				long long id = tileID( x, y, request.zoom );
				if ( !cache.contains( id ) ) {
					if ( !loadTile( x, y, request.zoom, &tile ) ) {
						tile = new QPixmap( tileSize, tileSize );
						tile->fill( QColor( 241, 238 , 232, 255 ) );
					}

					long long minCacheSize = 2 * tileSize * tileSize * tile->depth() / 8 * ( maxX - minX ) * ( maxY - minY );
					if ( cache.maxCost() < minCacheSize ) {
						qDebug() << "had to increase cache size to accomodate all tiles for at least two images: " << minCacheSize / 1024 / 1024 << " MB";
						cache.setMaxCost( minCacheSize );
					}

					cache.insert( id, tile, tileSize * tileSize * tile->depth() / 8 );
				}
				else {
					tile = cache.object( id );
				}
			}

			if ( tile != NULL )
				painter->drawPixmap( posX, posY, *tile );
			else
				painter->fillRect( posX, posY,  tileSize, tileSize, QColor( 241, 238 , 232, 255 ) );
			posY += tileSize;
		}
		posX += tileSize;
	}

	if ( settings.antiAliasing )
		painter->setRenderHint( QPainter::Antialiasing );

	if ( request.edgeSegments.size() > 0 && request.edges.size() > 0 ) {
		int position = 0;
		for ( int i = 0; i < request.edgeSegments.size(); i++ ) {
			QVector< ProjectedCoordinate > line;
			for ( ; position < request.edgeSegments[i]; position++ ) {
				ProjectedCoordinate pos = request.edges[position].ToProjectedCoordinate();
				line.push_back( ProjectedCoordinate( ( pos.x - request.center.x ) * zoomFactor, ( pos.y - request.center.y ) * zoomFactor ) );
			}
			drawPolyline( painter, boundingBox, line, QColor( 0, 0, 128, 128 ) );
		}
	}

	if ( request.route.size() > 0 ) {
		QVector< ProjectedCoordinate > line;
		for ( int i = 0; i < request.route.size(); i++ ){
			ProjectedCoordinate pos = request.route[i].ToProjectedCoordinate();
			line.push_back( ProjectedCoordinate( ( pos.x - request.center.x ) * zoomFactor, ( pos.y - request.center.y ) * zoomFactor ) );
		}
		drawPolyline( painter, boundingBox, line, QColor( 0, 0, 128, 128 ) );
	}

	if ( request.POIs.size() > 0 ) {
		for ( int i = 0; i < request.POIs.size(); i++ ) {
			ProjectedCoordinate pos = request.POIs[i].ToProjectedCoordinate();
			drawIndicator( painter, transform, inverseTransform, ( pos.x - request.center.x ) * zoomFactor, ( pos.y - request.center.y ) * zoomFactor, sizeX, sizeY, request.virtualZoom, QColor( 196, 0, 0 ), QColor( 0, 0, 196 ) );
		}
	}

	if ( request.target.x != 0 || request.target.y != 0 )
	{
		ProjectedCoordinate pos = request.target.ToProjectedCoordinate();
		drawIndicator( painter, transform, inverseTransform, ( pos.x - request.center.x ) * zoomFactor, ( pos.y - request.center.y ) * zoomFactor, sizeX, sizeY, request.virtualZoom, QColor( 0, 0, 128 ), QColor( 255, 0, 0 ) );
	}

	if ( request.position.x != 0 || request.position.y != 0 )
	{
		ProjectedCoordinate pos = request.position.ToProjectedCoordinate();
		drawIndicator( painter, transform, inverseTransform, ( pos.x - request.center.x ) * zoomFactor, ( pos.y - request.center.y ) * zoomFactor, sizeX, sizeY, request.virtualZoom, QColor( 0, 128, 0 ), QColor( 255, 255, 0 ) );
		drawArrow( painter, ( pos.x - request.center.x ) * zoomFactor, ( pos.y - request.center.y ) * zoomFactor, request.heading - 90, QColor( 0, 128, 0 ), QColor( 255, 255, 0 ) );
	}

	return true;
}

void RendererBase::drawArrow( QPainter* painter, int x, int y, double rotation, QColor outer, QColor inner )
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

void RendererBase::drawIndicator( QPainter* painter, const QTransform& transform, const QTransform& inverseTransform, int x, int y, int sizeX, int sizeY, int virtualZoom, QColor outer, QColor inner )
{
	QPoint mapped = transform.map( QPoint( x, y ) );
	int margin = 9 * virtualZoom;
	if ( mapped.x() < margin || mapped.y() < margin || mapped.x() >= sizeX - margin || mapped.y() >= sizeY - margin ) {
		//clip an imaginary line from the screen center to pos at the screen boundaries
		ProjectedCoordinate start( mapped.x(), mapped.y() );
		ProjectedCoordinate end( sizeX / 2, sizeY / 2 );
		clipEdge( &start, &end, ProjectedCoordinate( margin, margin ), ProjectedCoordinate( sizeX - margin, sizeY - margin ) );
		QPoint position = inverseTransform.map( QPoint( start.x, start.y ) );
		double heading = atan2( mapped.y() - sizeY / 2.0, mapped.x() - sizeX / 2.0 ) * 360 / 2 / M_PI;
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

void RendererBase::drawPolyline( QPainter* painter, const QRect& boundingBox, QVector< ProjectedCoordinate > line, QColor color )
{
	painter->setPen( QPen( color, 8, Qt::SolidLine, Qt::FlatCap ) );

	QVector< bool > isInside;

	for ( int i = 0; i < line.size(); i++ ) {
		ProjectedCoordinate pos = line[i];
		QPoint point( pos.x, pos.y );
		isInside.push_back( boundingBox.contains( point ) );
	}

	QVector< bool > draw = isInside;
	for ( int i = 1; i < line.size(); i++ ) {
		if ( isInside[i - 1] )
			draw[i] = true;
		if ( isInside[i] )
			draw[i - 1] = true;
	}

	QVector< QPoint > polygon;
	ProjectedCoordinate lastCoord;
	for ( int i = 0; i < line.size(); i++ ) {
		if ( !draw[i] ) {
			painter->drawPolyline( polygon.data(), polygon.size() );
			polygon.clear();
			continue;
		}
		ProjectedCoordinate pos = line[i];
		if ( fabs( pos.x - lastCoord.x ) + fabs( pos.y - lastCoord.y ) < 5 ) {
			isInside.push_back( false );
			continue;
		}
		QPoint point( pos.x, pos.y );
		polygon.push_back( point );
		lastCoord = pos;
	}
	painter->drawPolyline( polygon.data(), polygon.size() );
}
