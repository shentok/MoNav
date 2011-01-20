/*
Copyright 2010  Christian Vetter veaac.fdirct@gmail.com

This file is part of MoNav.

MoNav is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MoNav is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY {

}; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MoNav.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pluginmanager.h"

#include "interfaces/iimporter.h"
#include "interfaces/ipreprocessor.h"
#include "utils/qthelpers.h"
#include <QPluginLoader>
#include <QDir>
#include <QCoreApplication>
#include <QtDebug>
#include <QImage>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QtDebug>

struct PluginManager::PrivateImplementation {

	QVector< IImporter* > importerPlugins;
	QVector< IPreprocessor* > rendererPlugins;
	QVector< IPreprocessor* > routerPlugins;
	QVector< IPreprocessor* > gpsLookupPlugins;
	QVector< IPreprocessor* > addressLookupPlugins;

	QVector< QObject* > plugins;
	QVector< QPluginLoader* > pluginLoaders;

	bool testPlugin( QObject* plugin );

	QString inputFilename;
	QString outputDirectory;
	QString name;
	QString displayName;
	QString image;

	QFuture< bool > processingFuture;
	QFutureWatcher< bool > processingWatcher;

	template< class IPlugin >
	static int findPlugin( QVector< IPlugin > plugins, QString name );

};

void PluginManager::finish()
{
	emit finished( d->processingWatcher.result() );
}

template< class IPlugin >
int PluginManager::PrivateImplementation::findPlugin( QVector< IPlugin > plugins, QString name )
{
	// check if name uniquely specifies an importer plugin
	int index = -1;
	for ( int i = 0; i < plugins.size(); i++ ) {
		if ( plugins[i]->GetName() == name ) {
			if ( index != -1 ) {
				qCritical() << "Plugin name not unique:" << name;
				return -1;
			}
		}
	}
	if ( index == -1 )
		qCritical() << "Plugin name not found:" << name;

	return index;
}

PluginManager::PluginManager()
{
	d = new PrivateImplementation;
	connect( &d->processingWatcher, SIGNAL(finished()), this, SLOT(finish()) );
}
PluginManager::~PluginManager()
{
	delete d;
}

// returns the single instance of this class
PluginManager* PluginManager::instance()
{
	static PluginManager pluginManager;
	return &pluginManager;
}

// loads dynamic plugins and initializes dynamic and static plugins
bool PluginManager::loadPlugins()
{
	// dynamically load all available plugins
	QDir pluginDir( QCoreApplication::applicationDirPath() );
	if ( pluginDir.cd( "plugins_preprocessor" ) ) {
		foreach ( QString fileName, pluginDir.entryList( QDir::Files ) ) {
			QPluginLoader* loader = new QPluginLoader( pluginDir.absoluteFilePath( fileName ) );
			loader->setLoadHints( QLibrary::ExportExternalSymbolsHint );
			if ( !loader->load() )
				qDebug() << loader->errorString();

			if ( d->testPlugin( loader->instance() ) ) {
				d->pluginLoaders.push_back( loader );
			} else {
				loader->unload();
				delete loader;
			}
		}
	}

	// test static plugins
	foreach ( QObject *plugin, QPluginLoader::staticInstances() )
		d->testPlugin( plugin );

	if ( d->importerPlugins.size() == 0  || d->rendererPlugins.size() == 0  || d->routerPlugins.size() == 0 || d->gpsLookupPlugins.size() == 0  || d->addressLookupPlugins.size() == 0 ) {
		qWarning() << "plugin types are missing";
		return false;
	}

	return true;
}

// determines the type of the plugin
// adds it to the correct data structures
// returns whether this is a MoNav plugin
bool PluginManager::PrivateImplementation::testPlugin( QObject* plugin )
{
	bool needed = false;
	// importer plugin?
	if ( IImporter *interface = qobject_cast< IImporter* >( plugin ) )
	{
		importerPlugins.append( interface );
		needed = true;
	}
	// other preprocessing plugin?
	if ( IPreprocessor *interface = qobject_cast< IPreprocessor* >( plugin ) )
	{
		if ( interface->GetType() == IPreprocessor::Renderer )
			rendererPlugins.append( interface );
		if ( interface->GetType() == IPreprocessor::Router )
			routerPlugins.append( interface );
		if ( interface->GetType() == IPreprocessor::GPSLookup )
			gpsLookupPlugins.append( interface );
		if ( interface->GetType() == IPreprocessor::AddressLookup )
			addressLookupPlugins.append( interface );
		needed = true;
	}
	return needed;
}

bool PluginManager::unloadPlugins()
{
	foreach( QPluginLoader* pluginLoader, d->pluginLoaders )
	{
		pluginLoader->unload();
		delete pluginLoader;
	}
	// delete static instances
	// Qt does not do this, although the documentation states otherwise
	// TODO: CHECK FOR EACH NOW QT VERSION
	foreach ( QObject *plugin, QPluginLoader::staticInstances() )
		delete plugin;

	return true;
}

// returns a list of plugins
// plugins are uniquely identified by their name
QStringList PluginManager::importerPlugins()
{
	QStringList names;
	foreach( IImporter* importer, d->importerPlugins ) {
		names.push_back( importer->GetName() );
	}
	return names;
}
QStringList PluginManager::routerPlugins()
{
	QStringList names;
	foreach( IPreprocessor* plugin, d->routerPlugins ) {
		names.push_back( plugin->GetName() );
	}
	return names;
}
QStringList PluginManager::gpsLookupPlugins()
{
	QStringList names;
	foreach( IPreprocessor* plugin, d->gpsLookupPlugins ) {
		names.push_back( plugin->GetName() );
	}
	return names;
}
QStringList PluginManager::rendererPlugins()
{
	QStringList names;
	foreach( IPreprocessor* plugin, d->rendererPlugins ) {
		names.push_back( plugin->GetName() );
	}
	return names;
}
QStringList PluginManager::addressLookupPlugins()
{
	QStringList names;
	foreach( IPreprocessor* plugin, d->addressLookupPlugins ) {
		names.push_back( plugin->GetName() );
	}
	return names;
}

// returns pointers to the plugins
QVector< QObject* > PluginManager::plugins()
{
	return d->plugins;
}

// input filename for processing
QString PluginManager::inputFile()
{
	return d->inputFilename;
}
// name of the data set to be created
QString PluginManager::name()
{
	return d->name;
}
// description of the data set to be created
QString PluginManager::description()
{
	return d->displayName;
}
// image filename of the data set to be created
QString PluginManager::image()
{
	return d->image;
}

// map data directory that will contain everything
QString PluginManager::outputDirectory()
{
	return d->outputDirectory;
}

// waits until all current processing step is finished
void PluginManager::waitForFinish()
{
	d->processingFuture.waitForFinished();
	return;
}
// is a plugin currently processing?
bool PluginManager::processing()
{
	return !d->processingFuture.isFinished();
}


void PluginManager::setInputFile( QString filename )
{
	d->inputFilename = filename;
}

void PluginManager::setName( QString name )
{
	d->name = name;
}

void PluginManager::setDisplayName( QString displayName )
{
	d->displayName = displayName;
}

void PluginManager::setImage( QString image )
{
	d->image = image;
}

void PluginManager::setOutputDirectory( QString directory )
{
	d->outputDirectory = directory;
}

// saves settings
bool PluginManager::saveSettings( QSettings* settings )
{
	settings->setValue( "input", d->inputFilename );
	settings->setValue( "output", d->outputDirectory );
	settings->setValue( "name", d->name );
	settings->setValue( "displayName", d->displayName );
	settings->setValue( "image", d->image );

	foreach ( IImporter* plugin, d->importerPlugins ) {
		if ( !plugin->SaveSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->routerPlugins ) {
		if ( !plugin->SaveSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->rendererPlugins ) {
		if ( !plugin->SaveSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->gpsLookupPlugins ) {
		if ( !plugin->SaveSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->addressLookupPlugins ) {
		if ( !plugin->SaveSettings( settings ) )
			return false;
	}

	return true;
}

// restores settings
bool PluginManager::loadSettings( QSettings* settings )
{
	d->inputFilename = settings->value( "input" ).toString();
	d->outputDirectory = settings->value( "output" ).toString();
	d->name = settings->value( "name" ).toString();
	d->displayName = settings->value( "displayName" ).toString();
	d->image = settings->value( "image" ).toString();

	foreach ( IImporter* plugin, d->importerPlugins ) {
		if ( !plugin->LoadSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->routerPlugins ) {
		if ( !plugin->LoadSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->rendererPlugins ) {
		if ( !plugin->LoadSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->gpsLookupPlugins ) {
		if ( !plugin->LoadSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->addressLookupPlugins ) {
		if ( !plugin->LoadSettings( settings ) )
			return false;
	}

	return true;
}

bool runImporter( IImporter* importer )
{
	return false;
}

bool runRouting( QString directory, IImporter* importer, IPreprocessor* router, IPreprocessor* gpsLookup )
{
	return false;
}

bool runRendering( QString directory, IImporter* importer, IPreprocessor* renderer )
{
	return false;
}

bool runAddressLookup( QString directory, IImporter* importer, IPreprocessor* addressLookup )
{
	return false;
}

// process plugins asynchronously, fails if processing is underway
// accessing the plugins during processing is forbidden

bool PluginManager::processImporter( QString plugin, bool async )
{
	int index = d->findPlugin( d->importerPlugins, plugin );
	if ( index == -1 )
		return false;

	// find / create importer directory
	QDir dir( d->outputDirectory );
	if ( !dir.exists( "tmp" ) )
		dir.mkdir( "tmp" );
	dir.cd( "tmp" );
	d->importerPlugins[index]->SetOutputDirectory( dir.path() );

	if ( !async ) {
		return runImporter( d->importerPlugins[index] );
	}

	// run async and return
	d->processingFuture = QtConcurrent::run( runImporter, d->importerPlugins[index] );
	d->processingWatcher.setFuture( d->processingFuture );
	return true;
}

bool PluginManager::processRoutingModule( QString moduleName, QString importer, QString router, QString gpsLookup, bool async )
{
	int importerIndex = d->findPlugin( d->importerPlugins, importer );
	int routerIndex = d->findPlugin( d->routerPlugins, router );
	int gpsLookupIndex = d->findPlugin( d->gpsLookupPlugins, gpsLookup );
	if ( importerIndex == -1 || routerIndex == -1 || gpsLookupIndex == -1 )
		return false;

	d->importerPlugins[importerIndex]->SetOutputDirectory( fileInDirectory( d->outputDirectory, "tmp") );

	// find / create module directory
	QDir dir( d->outputDirectory );
	if ( !dir.exists( moduleName ) )
		dir.mkdir( moduleName );
	dir.cd( moduleName );

	if ( !async ) {
		return runRouting( dir.path(), d->importerPlugins[importerIndex], d->routerPlugins[routerIndex], d->gpsLookupPlugins[gpsLookupIndex] );
	}

	// run async and return
	d->processingFuture = QtConcurrent::run( runRouting, dir.path(), d->importerPlugins[importerIndex], d->routerPlugins[routerIndex], d->gpsLookupPlugins[gpsLookupIndex] );
	d->processingWatcher.setFuture( d->processingFuture );
	return true;
}

bool PluginManager::processRenderingModule( QString moduleName, QString importer, QString renderer, bool async )
{
	int importerIndex = d->findPlugin( d->importerPlugins, importer );
	int rendererIndex = d->findPlugin( d->rendererPlugins, renderer );
	if ( importerIndex == -1 || rendererIndex == -1 )
		return false;

	d->importerPlugins[importerIndex]->SetOutputDirectory( fileInDirectory( d->outputDirectory, "tmp") );

	// find / create module directory
	QDir dir( d->outputDirectory );
	if ( !dir.exists( moduleName ) )
		dir.mkdir( moduleName );
	dir.cd( moduleName );

	if ( !async ) {
		return runRendering( dir.path(), d->importerPlugins[importerIndex], d->rendererPlugins[rendererIndex] );
	}

	// run async and return
	d->processingFuture = QtConcurrent::run( runRendering, dir.path(), d->importerPlugins[importerIndex], d->rendererPlugins[rendererIndex] );
	d->processingWatcher.setFuture( d->processingFuture );
	return true;
}

bool PluginManager::processAddressLookupModule( QString moduleName, QString importer, QString addressLookup, bool async )
{
	int importerIndex = d->findPlugin( d->importerPlugins, importer );
	int addressLookupIndex = d->findPlugin( d->rendererPlugins, addressLookup );
	if ( importerIndex == -1 || addressLookupIndex == -1 )
		return false;

	d->importerPlugins[importerIndex]->SetOutputDirectory( fileInDirectory( d->outputDirectory, "tmp") );

	// find / create module directory
	QDir dir( d->outputDirectory );
	if ( !dir.exists( moduleName ) )
		dir.mkdir( moduleName );
	dir.cd( moduleName );

	if ( !async ) {
		return runAddressLookup( dir.path(), d->importerPlugins[importerIndex], d->addressLookupPlugins[addressLookupIndex] );
	}

	// run async and return
	d->processingFuture = QtConcurrent::run( runAddressLookup, dir.path(), d->importerPlugins[importerIndex], d->addressLookupPlugins[addressLookupIndex] );
	d->processingWatcher.setFuture( d->processingFuture );
	return true;
}

// writes main config file
// contains description of the map package
bool PluginManager::writeConfig()
{
	QSettings pluginSettings( fileInDirectory( d->outputDirectory, "MoNav.ini" ), QSettings::IniFormat );

	pluginSettings.setValue( "configVersion", 2 );

	pluginSettings.setValue( "name", d->name );
	pluginSettings.setValue( "description", d->displayName );
	QImage image( d->image );
	if ( !image.isNull() )
		pluginSettings.setValue( "image", image );

	pluginSettings.sync();
	if ( pluginSettings.status() != QSettings::NoError )
		return false;

	return true;
}
// deletes temporary files
bool PluginManager::deleteTemporaryFiles()
{
	QDir dir( d->outputDirectory );
	dir.cd( "tmp" );
	if ( !dir.exists() )
		return true;
	foreach ( QString filename, dir.entryList() ) {
		QFile::remove( dir.absoluteFilePath( filename ) );
	}
	return true;
}

