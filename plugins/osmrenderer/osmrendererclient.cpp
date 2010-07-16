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
#include <QNetworkReply>
#include <QDebug>
#include <QDesktopServices>

#include "osmrendererclient.h"
#include "utils/utils.h"

OSMRendererClient::OSMRendererClient()
{
	network = NULL;
	diskCache = NULL;
}

OSMRendererClient::~OSMRendererClient()
{
	unload();
}

void OSMRendererClient::unload()
{
	if ( network != NULL )
		delete network;
	network = NULL;
}

QString OSMRendererClient::GetName()
{
	return "OSM Renderer";
}

bool OSMRendererClient::load()
{
	network = new QNetworkAccessManager( this );
	diskCache = new QNetworkDiskCache( this );
	QString cacheDir = QDesktopServices::storageLocation( QDesktopServices::CacheLocation );
	qDebug() << "set disk cache to: " << cacheDir;
	diskCache->setCacheDirectory( cacheDir );
	network->setCache( diskCache );
	connect( network, SIGNAL(finished(QNetworkReply*)), this, SLOT(finished(QNetworkReply*)) );
	tileSize = 256;
	return true;
}

int OSMRendererClient::GetMaxZoom()
{
	return 18;
}

void OSMRendererClient::finished( QNetworkReply* reply ) {
	long long id = reply->request().attribute( QNetworkRequest::User ).toLongLong();
	if ( reply->error() ) {
		qDebug() << "failed to get: " << reply->url();
		return;
	}

	QImage image;
	if ( !image.load( reply, 0 ) ) {
		qDebug() << "failed to load image: " << id;
		return;
	}
	QPixmap* tile = new QPixmap( QPixmap::fromImage( image ) );
	cache.insert( id, tile , tileSize * tileSize * tile->depth() / 8 );
	reply->deleteLater();
	emit changed();
}

bool OSMRendererClient::loadTile( int x, int y, int zoom, QPixmap** tile )
{
		long long id = tileID( x, y, zoom );

		QString path = "http://tile.openstreetmap.org/%1/%2/%3.png";
		QUrl url = QUrl( path.arg( zoom ).arg( x ).arg( y ) );
		QNetworkRequest request;
		request.setUrl( url );
		request.setRawHeader( "User-Agent", "MoNav OSM Renderer 1.0" );
		request.setAttribute( QNetworkRequest::User, QVariant( id ) );
		request.setAttribute( QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache );
		network->get( request );

		// was the tile loaded immediately? Do not overwrite in this case
		if ( cache.contains( id ) ) {
			*tile = cache.object( id );
			return true;
		}
		return false;
}

Q_EXPORT_PLUGIN2( osmrendererclient, OSMRendererClient )
