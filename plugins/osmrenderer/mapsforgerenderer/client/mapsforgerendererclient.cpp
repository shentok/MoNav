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

#include "mapsforgerendererclient.h"
#include "mapsforgetilewriter.h"
#include "utils/qthelpers.h"

#include <QImage>
#include <QThread>

MapsforgeRendererClient::MapsforgeRendererClient() :
	twriter( NULL ),
	m_renderThread( NULL )
{
}

MapsforgeRendererClient::~MapsforgeRendererClient()
{
	unload();
}

void MapsforgeRendererClient::unload()
{
	disconnect( this, SIGNAL(drawImage(int,int,int,int)) );
	disconnect( this, SLOT(tileLoaded(int,int,int,int,QImage)) );
	if ( twriter != NULL ) {
		disconnect( twriter, SIGNAL(image_finished(int,int,int,int,QImage)), this, SIGNAL(changed()) );
		twriter->deleteLater();
		twriter = NULL;
	}
	if ( m_renderThread != NULL ) {
		m_renderThread->quit();
		m_renderThread->wait();
		m_renderThread->deleteLater();
		m_renderThread = NULL;
	}
}

QString MapsforgeRendererClient::GetName()
{
	return "Mapsforge Renderer";
}

bool MapsforgeRendererClient::IsCompatible( int fileFormatVersion )
{
	return fileFormatVersion == 1;
}

bool MapsforgeRendererClient::load()
{
	for(int i=0;i<=GetMaxZoom();i++)
		m_zoomLevels.push_back(i);

	const QString filename = fileInDirectory( m_directory, "map.map" );
	twriter = new MapsforgeTileWriter( filename );
	m_renderThread = new QThread( this );
	twriter->moveToThread( m_renderThread );
	connect( this, SIGNAL(drawImage(int,int,int,int)), twriter, SLOT(draw_image(int,int,int,int)) );
	connect( twriter, SIGNAL(image_finished(int,int,int,int,QImage)), this, SLOT(tileLoaded(int,int,int,int,QImage)) );
	connect( twriter, SIGNAL(image_finished(int,int,int,int,QImage)), this, SIGNAL(changed()) );
	m_renderThread->start();
	return true;
}

int MapsforgeRendererClient::GetMaxZoom()
{
	return 18;
}

QPixmap* MapsforgeRendererClient::loadTile( int x, int y, int zoom, int magnification )
{
	emit drawImage( x, y, zoom, magnification );
	return 0;
}

void MapsforgeRendererClient::tileLoaded( int x, int y, int zoom, int magnification, const QImage &img )
{
	if ( this->m_magnification != magnification )
		return;

	const QSize tileSize = img.size();
	QPixmap* tile = new QPixmap( QPixmap::fromImage( img.scaled( tileSize * magnification ) ) );
	long long id = tileID( x, y, zoom );
	m_cache.insert( id, tile, 256 * 256 * magnification * magnification * tile->depth() / 8 );
}

Q_EXPORT_PLUGIN2( mapsforgerendererclient, MapsforgeRendererClient )
