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

#include "mapdata.h"
#include "utils/qthelpers.h"
#include <QSettings>
#include <QPluginLoader>
#include <QApplication>

struct MapData::PrivateImplementation {

	enum { ConfigVersion = 1 };

	QString path;

	bool addressLookupRequired;
	bool gpsLookupRequired;
	bool rendererRequired;
	bool routerRequired;

	bool loaded;
	bool informationLoaded;

	QString addressLookupName;
	QString gpsLookupName;
	QString rendererName;
	QString routerName;

	// stores pointers to all dynamically loaded plugins
	QList< QPluginLoader* > plugins;
	// stores a pointer to the current renderer plugin
	IRenderer* renderer;
	// stores a pointer to the current address lookup plugin
	IAddressLookup* addressLookup;
	// stores a pointer to the current GPS lookup plugin
	IGPSLookup* gpsLookup;
	// stores a pointer to the current router plugin
	IRouter* router;

	int addressLookupFileFormatVersion;
	int gpsLookupFileFormatVersion;
	int rendererFileFormatVersion;
	int routerFileFormatVersion;

	QString name;
	QString description;
	QImage image;

	bool testPlugin( QObject* plugin );
};

bool MapData::PrivateImplementation::testPlugin( QObject* plugin )
{
	bool needed = false;
	if ( IRenderer *interface = qobject_cast< IRenderer* >( plugin ) ) {
		if ( interface->GetName() == rendererName ) {
			renderer = interface;
			needed = true;
		}
	}
	if ( IAddressLookup *interface = qobject_cast< IAddressLookup* >( plugin ) ) {
		if ( interface->GetName() == addressLookupName )
		{
			addressLookup = interface;
			needed = true;
		}
	}
	if ( IGPSLookup *interface = qobject_cast< IGPSLookup* >( plugin ) ) {
		if ( interface->GetName() == gpsLookupName ) {
			gpsLookup = interface;
			needed = true;
		}
	}
	if ( IRouter *interface = qobject_cast< IRouter* >( plugin ) ) {
		if ( interface->GetName() == routerName ) {
			router = interface;
			needed = true;
		}
	}
	return needed;
}

MapData::MapData() :
	QObject( NULL ), d( new PrivateImplementation )
{
	d->addressLookupRequired = true;
	d->gpsLookupRequired = true;
	d->rendererRequired = true;
	d->routerRequired = true;

	d->loaded = false;
	d->informationLoaded = false;

	d->addressLookup = NULL;
	d->gpsLookup = NULL;
	d->renderer = NULL;
	d->router = NULL;
}

MapData::~MapData()
{
	//delete static plugins
	foreach ( QObject *plugin, QPluginLoader::staticInstances() )
		delete plugin;

	delete d;
}

MapData* MapData::instance()
{
	static MapData mapData;
	return &mapData;
}

QString MapData::path() const
{
	return d->path;
}

void MapData::setPath( QString path )
{
	unload();
	d->path = path;
}

bool MapData::pluginRequired( PluginType plugin ) const
{
	switch( plugin ) {
	case AddressLookup:
		return d->addressLookupRequired;
	case GPSLookup:
		return d->gpsLookupRequired;
	case Renderer:
		return d->rendererRequired;
	case Router:
		return d->routerRequired;
	}
}

void MapData::setPluginRequired( PluginType plugin, bool required )
{
	switch( plugin ) {
	case AddressLookup:
		d->addressLookupRequired = required;
		break;
	case GPSLookup:
		d->gpsLookupRequired = required;
		break;
	case Renderer:
		d->rendererRequired = required;
		break;
	case Router:
		d->routerRequired = required;
		break;
	}
}

bool MapData::containsMapData() const
{
	QString configFilename = fileInDirectory( d->path, "MoNav.ini" );
	return QFile::exists( configFilename );
}

bool MapData::loaded() const
{
	return d->loaded;
}

bool MapData::load()
{
	if ( !loadInformation() )
		return false;

	if ( !canBeLoaded() )
		return false;

	if ( d->addressLookupRequired ) {
		d->addressLookup->SetInputDirectory( d->path );
		if ( !d->addressLookup->LoadData() )
			return false;
	}
	if ( d->gpsLookupRequired ) {
		d->gpsLookup->SetInputDirectory( d->path );
		if ( !d->gpsLookup->LoadData() )
			return false;
	}
	if ( d->rendererRequired ) {
		d->renderer->SetInputDirectory( d->path );
		if ( !d->renderer->LoadData() )
			return false;
	}
	if ( d->routerRequired ) {
		d->router->SetInputDirectory( d->path );
		if ( !d->router->LoadData() )
			return false;
	}

	d->loaded = true;
	emit dataLoaded();
	return true;
}

bool MapData::unload()
{
	if ( d->loaded ) {
	/*if ( d->addressLookup != NULL )
		d->addressLookup->Unload();
	if ( d->gpsLookup != NULL )
		d->gpsLookup->Unload();
	if ( d->renderer != NULL )
		d->renderer->UnloadData();
	if ( d->router != NULL )
		d->router->UnloadData();*/
	}

	d->informationLoaded = false;

	if ( !d->loaded )
		return true;

	d->addressLookup = NULL;
	d->gpsLookup = NULL;
	d->renderer = NULL;
	d->router = NULL;

	foreach( QPluginLoader* pluginLoader, d->plugins )
	{
		pluginLoader->unload();
		delete pluginLoader;
	}
	d->plugins.clear();

	d->loaded = false;

	emit dataUnloaded();
	return true;
}

bool MapData::informationLoaded() const
{
	return d->informationLoaded;
}

bool MapData::loadInformation()
{
	if ( !unload() )
		return false;

	QString configFilename = fileInDirectory( d->path, "MoNav.ini" );
	if ( !QFile::exists( configFilename ) ) {
		qCritical() << "no config file can be found at:" << d->path;
		return false;
	}

	QSettings config( configFilename, QSettings::IniFormat );

	int configVersion = config.value( "configVersion" ).toInt();
	if ( configVersion != d->ConfigVersion ) {
		qCritical() << "config version not compatible:" << configVersion << "vs" << d->ConfigVersion;
		return false;
	}

	d->name = config.value( "name" ).toString();
	d->description = config.value( "description" ).toString();
	d->image = config.value( "image" ).value< QImage >();

	d->addressLookupName = config.value( "addressLookup" ).toString();
	d->gpsLookupName = config.value( "gpsLookup" ).toString();
	d->rendererName = config.value( "renderer" ).toString();
	d->routerName = config.value( "router" ).toString();

	d->addressLookupFileFormatVersion = config.value( "addressLookupFileFormatVersion" ).toInt();
	d->gpsLookupFileFormatVersion = config.value( "gpsLookupFileFormatVersion" ).toInt();
	d->rendererFileFormatVersion = config.value( "rendererFileFormatVersion" ).toInt();
	d->routerFileFormatVersion = config.value( "routerFileFormatVersion" ).toInt();

	QDir pluginDir( QApplication::applicationDirPath() );
	if ( pluginDir.cd( "plugins_client" ) ) {
		foreach ( QString fileName, pluginDir.entryList( QDir::Files ) ) {
			QPluginLoader* loader = new QPluginLoader( pluginDir.absoluteFilePath( fileName ) );
			if ( !loader->load() )
				qDebug( "%s", loader->errorString().toAscii().constData() );
			if ( d->testPlugin( loader->instance() ) )
				d->plugins.append( loader );
			else {
				loader->unload();
				delete loader;
			}
		}
	}

	foreach ( QObject *plugin, QPluginLoader::staticInstances() )
		d->testPlugin( plugin );

	d->informationLoaded = true;
	emit informationChanged();
	return true;
}

bool MapData::canBeLoaded() const
{
	if ( d->addressLookupRequired ) {
		if ( !pluginPresent( AddressLookup ) )
			return false;
		if ( !fileFormatCompatible( AddressLookup ) )
			return false;
	}
	if ( d->gpsLookupRequired ) {
		if ( !pluginPresent( GPSLookup ) )
			return false;
		if ( !fileFormatCompatible( GPSLookup ) )
			return false;
	}
	if ( d->rendererRequired ) {
		if ( !pluginPresent( Renderer ) )
			return false;
		if ( !fileFormatCompatible( Renderer ) )
			return false;
	}
	if ( d->routerRequired ) {
		if ( !pluginPresent( Router ) )
			return false;
		if ( !fileFormatCompatible( Router ) )
			return false;
	}

	return true;
}

QString MapData::name() const
{
	return d->name;
}

QString MapData::description() const
{
	return d->description;
}

QImage MapData::image() const
{
	return d->image;
}

QString MapData::pluginName( PluginType plugin ) const
{
	switch( plugin ) {
	case AddressLookup:
		return d->addressLookupName;
	case GPSLookup:
		return d->gpsLookupName;
	case Renderer:
		return d->rendererName;
	case Router:
		return d->routerName;
	}
}

bool MapData::pluginPresent( PluginType plugin ) const
{
	switch( plugin ) {
	case AddressLookup:
		return d->addressLookup != NULL;
	case GPSLookup:
		return d->gpsLookup != NULL;
	case Renderer:
		return d->renderer != NULL;
	case Router:
		return d->router != NULL;
	}
}

int MapData::fileFormatVersion( PluginType plugin ) const
{
	switch( plugin ) {
	case AddressLookup:
		return d->addressLookupFileFormatVersion;
	case GPSLookup:
		return d->gpsLookupFileFormatVersion;
	case Renderer:
		return d->rendererFileFormatVersion;
	case Router:
		return d->routerFileFormatVersion;
	}
}

bool MapData::fileFormatCompatible( PluginType plugin ) const
{
	return true;
	switch( plugin ) {
	case AddressLookup:
		if ( pluginPresent( AddressLookup ) )
			return false;
		return d->addressLookup->IsCompatible( d->addressLookupFileFormatVersion );
	case GPSLookup:
		if ( pluginPresent( GPSLookup ) )
			return false;
		return d->gpsLookup->IsCompatible( d->gpsLookupFileFormatVersion );
	case Renderer:
		if ( pluginPresent( Renderer ) )
			return false;
		return d->renderer->IsCompatible( d->rendererFileFormatVersion );
	case Router:
		if ( pluginPresent( Router ) )
			return false;
		return d->router->IsCompatible( d->routerFileFormatVersion );
	}
}

IAddressLookup* MapData::addressLookup()
{
	return d->addressLookup;
}

IGPSLookup* MapData::gpsLookup()
{
	return d->gpsLookup;
}

IRenderer* MapData::renderer()
{
	return d->renderer;
}

IRouter* MapData::router()
{
	return d->router;
}

