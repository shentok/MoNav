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
#include <QDir>

struct MapData::PrivateImplementation {

	enum { ConfigVersion = 2 };

	QString path;

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

	QString name;
	QImage image;

	QVector< MapData::Module > routingModules;
	QVector< MapData::Module > renderingModules;
	QVector< MapData::Module > addressLookupModules;

	bool testPlugin( QObject* plugin );
	void findRoutingModules();
	void findRenderingModules();
	void findAddressLookupModules();
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
	d->loaded = false;
	d->informationLoaded = false;

	d->addressLookup = NULL;
	d->gpsLookup = NULL;
	d->renderer = NULL;
	d->router = NULL;
}

MapData::~MapData()
{
	delete d;
}

void MapData::deleteStaticPlugins()
{
	//delete static plugins
	foreach ( QObject *plugin, QPluginLoader::staticInstances() )
		delete plugin;
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

bool MapData::containsMapData() const
{
	QString configFilename = fileInDirectory( d->path, "MoNav.ini" );
	return QFile::exists( configFilename );
}

bool MapData::loaded() const
{
	return d->loaded;
}

bool MapData::load( const Module& routingModule, const Module& renderingModule, const Module& addressLookupModule )
{
	if ( !informationLoaded() ) {
		qCritical() << "Information not loaded";
		return false;
	}

	if ( routingModule.plugins.size() != 2 ) {
		qCritical() << "Illegal routing module passed";
		return false;
	}
	d->routerName = routingModule.plugins[0];
	d->gpsLookupName = routingModule.plugins[1];

	if ( renderingModule.plugins.size() != 1 ) {
		qCritical() << "Illegal rendering module passed";
		return false;
	}
	d->rendererName = renderingModule.plugins[0];

	if ( addressLookupModule.plugins.size() != 1 ) {
		qCritical() << "Illegal address lookup module passed";
		return false;
	}
	d->addressLookupName = addressLookupModule.plugins[0];

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

	// check for success and unload plugins otherwise

	// check if all plugins were found
	bool success = true;
	if ( d->router == NULL ) {
		qCritical() << "Router plugin missing:" << d->routerName;
		success = false;
	}
	if ( d->gpsLookup == NULL ) {
		qCritical() << "GPS Lookup plugin missing:" << d->gpsLookupName;
		success = false;
	}
	if ( d->renderer == NULL ) {
		qCritical() << "Renderer plugin missing:" << d->rendererName;
		success = false;
	}
	if ( d->addressLookup == NULL ) {
		qCritical() << "Address lookup plugin missing:" << d->addressLookupName;
	}

	// check if file formats are compatible
	if ( success ) {
		if ( !d->router->IsCompatible( routingModule.fileFormats[0] ) ) {
			qCritical() << "Router file format not compatible";
			success = false;
		}
		if ( !d->gpsLookup->IsCompatible( routingModule.fileFormats[1] ) ) {
			qCritical() << "GPS Lookup file format not compatible";
			success = false;
		}
		if ( !d->renderer->IsCompatible( renderingModule.fileFormats[0] ) ) {
			qCritical() << "Renderer file format not compatible";
			success = false;
		}
		if ( !d->addressLookup->IsCompatible( addressLookupModule.fileFormats[0] ) ) {
			qCritical() << "Address Lookup file format not compatible";
			success = false;
		}
	}

	// check if data can be loaded
	if ( success ) {
		d->router->SetInputDirectory( routingModule.path );
		d->gpsLookup->SetInputDirectory( routingModule.path );
		d->renderer->SetInputDirectory( renderingModule.path );
		d->addressLookup->SetInputDirectory( addressLookupModule.path );
		if ( !d->router->LoadData() ) {
			qCritical() << "Failed to load router data";
			success = false;
		}
		if ( !d->gpsLookup->LoadData() ) {
			qCritical() << "Failed to load gps lookup data";
			success = false;
		}
		if ( !d->renderer->LoadData() ) {
			qCritical() << "Failed to load renderer data";
			success = false;
		}
		if ( !d->addressLookup->LoadData() ) {
			qCritical() << "Failed to load address lookup data";
			success = false;
		}

		/*
		if ( !success ) {
			d->router->Unload();
			d->gpsLookup->Unload();
			d->renderer->Unload();
			d->addressLookup->Unload();
		}
		*/
	}

	if ( !success ) {
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

void MapData::PrivateImplementation::findRoutingModules()
{
	routingModules.clear();
	// get potentially interesting subdirs
	QDir dir( path );
	dir.setNameFilters( QStringList( "routing_*" ) );
	QStringList subDirs = dir.entryList( QDir::Dirs, QDir::Name );

	// check each dir whether it contains suitable data
	for ( int i = 0; i < subDirs.size(); i++ ) {
		QString configFilename = fileInDirectory( dir.filePath( subDirs[i] ), "Module.ini" );
		if ( !QFile::exists( configFilename ) )
			continue;

		QSettings config( configFilename, QSettings::IniFormat );

		int configVersion = config.value( "configVersion" ).toInt();
		if ( configVersion != ConfigVersion ) {
			qCritical() << "config version found in" << configFilename << "not compatible:" << configVersion << "vs" << ConfigVersion;
			continue;
		}

		MapData::Module module;
		module.name = config.value( "name", "No Name" ).toString();
		module.path = dir.filePath( subDirs[i] );
		module.plugins.push_back( config.value( "router" ).toString() );
		module.plugins.push_back( config.value( "gpsLookup" ).toString() );
		module.fileFormats.push_back( config.value( "routerFileFormatVersion", -1 ).toInt() );
		module.fileFormats.push_back( config.value( "gpsLookupFileFormatVersion", -1 ).toInt() );

		routingModules.push_back( module );
	}
}

void MapData::PrivateImplementation::findRenderingModules()
{
	renderingModules.clear();
	// get potentially interesting subdirs
	QDir dir( path );
	dir.setNameFilters( QStringList( "rendering_*" ) );
	QStringList subDirs = dir.entryList( QDir::Dirs, QDir::Name );

	// check each dir whether it contains suitable data
	for ( int i = 0; i < subDirs.size(); i++ ) {
		QString configFilename = fileInDirectory( dir.filePath( subDirs[i] ), "Module.ini" );
		if ( !QFile::exists( configFilename ) )
			continue;

		QSettings config( configFilename, QSettings::IniFormat );

		int configVersion = config.value( "configVersion" ).toInt();
		if ( configVersion != ConfigVersion ) {
			qCritical() << "config version found in" << configFilename << "not compatible:" << configVersion << "vs" << ConfigVersion;
			continue;
		}

		MapData::Module module;
		module.name = config.value( "name", "No Name" ).toString();
		module.path = dir.filePath( subDirs[i] );
		module.plugins.push_back( config.value( "renderer" ).toString() );
		module.fileFormats.push_back( config.value( "rendererFileFormatVersion", -1 ).toInt() );

		renderingModules.push_back( module );
	}
}

void MapData::PrivateImplementation::findAddressLookupModules()
{
	addressLookupModules.clear();
	// get potentially interesting subdirs
	QDir dir( path );
	dir.setNameFilters( QStringList( "address_*" ) );
	QStringList subDirs = dir.entryList( QDir::Dirs, QDir::Name );

	// check each dir whether it contains suitable data
	for ( int i = 0; i < subDirs.size(); i++ ) {
		QString configFilename = fileInDirectory( dir.filePath( subDirs[i] ), "Module.ini" );
		if ( !QFile::exists( configFilename ) )
			continue;

		QSettings config( configFilename, QSettings::IniFormat );

		int configVersion = config.value( "configVersion" ).toInt();
		if ( configVersion != ConfigVersion ) {
			qCritical() << "config version found in" << configFilename << "not compatible:" << configVersion << "vs" << ConfigVersion;
			continue;
		}

		MapData::Module module;
		module.name = config.value( "name", "No Name" ).toString();
		module.path = dir.filePath( subDirs[i] );
		module.plugins.push_back( config.value( "addressLookup" ).toString() );
		module.fileFormats.push_back( config.value( "addressLookupFileFormatVersion", -1 ).toInt() );

		addressLookupModules.push_back( module );
	}
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
	d->image = config.value( "image" ).value< QImage >();
	if ( d->image.isNull() ) {
		d->image.load( ":images/map.png" );
	}

	// search for modules
	d->findRoutingModules();
	d->findRenderingModules();
	d->findAddressLookupModules();

	d->informationLoaded = true;
	emit informationChanged();
	return true;
}

QVector< MapData::Module > MapData::modules( ModuleType plugin ) const
{
	if ( !d->informationLoaded )
		return QVector< MapData::Module >();
	switch ( plugin ) {
	case Routing:
		return d->routingModules;
	case Rendering:
		return d->renderingModules;
	case AddressLookup:
		return d->addressLookupModules;
	}
	// should never reach this code
	return QVector< MapData::Module >();
}

QString MapData::name() const
{
	return d->name;
}

QImage MapData::image() const
{
	return d->image;
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

