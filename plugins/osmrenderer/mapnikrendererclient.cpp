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
#include <QSettings>
#include <QInputDialog>
#include <QtDebug>

MapnikRendererClient::MapnikRendererClient()
{
	fileZoom = -1;
	indexFile = NULL;
	tileFile = NULL;
}

MapnikRendererClient::~MapnikRendererClient()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Mapnik Renderer" );
	settings.setValue( "cacheSize", 1024 * 1024 * cacheSize );
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
		indexFile.close();
		tileFile->close();

		QDir dir( directory );
		QString filename = dir.filePath( "Mapnik Renderer" );
		indexFile->setFileName( filename + QString( "_%1_index" ).arg( zoom ) );
		indexFile->open( QIODevice::ReadOnly );
		tileFile->setFileName( filename + QString( "_%1_tiles" ).arg( zoom ) );
		tileFile->open( QIODevice::ReadOnly );
		fileZoom = zoom;
	}

	qint64 indexPosition = qint64( y - box.minY ) * ( box.maxX - box.minX ) + ( x - box.minX );
	indexFile.seek( indexPosition * 2 * sizeof( qint64 ) );
	qint64 start, end;
	indexFile.read( ( char* ) &start, sizeof( start ) );
	indexFile.read( ( char* ) &end, sizeof( end ) );
	if ( start != end ) {
		tileFile->seek( start );
		QByteArray data = tileFile->read( end - start );
		if ( !( *tile )->loadFromData( data, "PNG" ) ) {
			qDebug() << "Failed to load picture:" << id << " data: (" << start << "-" << end << ")";
			return false;
		}
	}

}

Q_EXPORT_PLUGIN2( mapnikrendererclient, MapnikRendererClient )
