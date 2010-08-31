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

#include "mapnikrendererclient.h"
#include "utils/qthelpers.h"
#include <QtDebug>

MapnikRendererClient::MapnikRendererClient()
{
	fileZoom = -1;
	indexFile = NULL;
	tileFile = NULL;
}

MapnikRendererClient::~MapnikRendererClient()
{
}

void MapnikRendererClient::unload()
{
	boxes.clear();
	if ( indexFile != NULL )
		delete indexFile;
	if ( tileFile != NULL )
		delete tileFile;
	indexFile = NULL;
	tileFile = NULL;
}

QString MapnikRendererClient::GetName()
{
	return "Mapnik Renderer";
}

bool MapnikRendererClient::load()
{
	QString filename = fileInDirectory( directory, "Mapnik Renderer" );
	FileStream configData( filename );
	if ( !configData.open( QIODevice::ReadOnly ) )
		return false;

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

	fileZoom = -1;
	indexFile = new QFile( this );
	tileFile = new QFile( this );

	return true;
}

int MapnikRendererClient::GetMaxZoom()
{
	if ( !loaded )
		return -1;
	return maxZoom;
}

bool MapnikRendererClient::loadTile( int x, int y, int zoom, QPixmap** tile )
{
	const Box& box = boxes[zoom];
	if ( x < box.minX || x >= box.maxX )
		return false;
	if ( y < box.minY || y >= box.maxY )
		return false;

	if ( fileZoom != zoom ) {
		indexFile->close();
		tileFile->close();

		QString filename = fileInDirectory( directory, "Mapnik Renderer" );
		indexFile->setFileName( filename + QString( "_%1_index" ).arg( zoom ) );
		if ( !openQFile( indexFile, QIODevice::ReadOnly ) )
			return false;
		tileFile->setFileName( filename + QString( "_%1_tiles" ).arg( zoom ) );
		if ( !openQFile( tileFile, QIODevice::ReadOnly ) )
			return false;
		fileZoom = zoom;
	}

	qint64 indexPosition = qint64( y - box.minY ) * ( box.maxX - box.minX ) + ( x - box.minX );
	indexFile->seek( indexPosition * 2 * sizeof( qint64 ) );
	qint64 start, end;
	indexFile->read( ( char* ) &start, sizeof( start ) );
	indexFile->read( ( char* ) &end, sizeof( end ) );
	if ( start == end )
		return false;

	tileFile->seek( start );
	QByteArray data = tileFile->read( end - start );
	*tile = new QPixmap( tileSize, tileSize );
	if ( !( *tile )->loadFromData( data, "PNG" ) ) {
		qDebug() << "Failed to load picture:" << x << y << zoom << " data: (" << start << "-" << end << ")";
		delete *tile;
		return false;
	}
	return true;
}

Q_EXPORT_PLUGIN2( mapnikrendererclient, MapnikRendererClient )
