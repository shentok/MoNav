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

#include "utils/commandlineparser.h"
#include "utils/formattedoutput.h"
#include "pluginmanager.h"
#include "interfaces/iconsolesettings.h"
#include "utils/log.h"

#include <QtPlugin>
#include <QSettings>
#include <QtCore/QCoreApplication>

Q_IMPORT_PLUGIN( mapnikrenderer );
Q_IMPORT_PLUGIN( contractionhierarchies );
Q_IMPORT_PLUGIN( gpsgrid );
Q_IMPORT_PLUGIN( unicodetournamenttrie );
Q_IMPORT_PLUGIN( osmrenderer );
Q_IMPORT_PLUGIN( qtilerenderer );
Q_IMPORT_PLUGIN( osmimporter );

class Commands : public IConsoleSettings {
	Q_INTERFACES( IConsoleSettings );

public:

	Commands()
	{
		listPlugins = false;
		importing = true;
		config = false;
		del = false;
		verbose = false;
	}

	virtual QString GetModuleName()
	{
		return "Global";
	}

	virtual bool GetSettingsList( QVector< Setting >* settings )
	{
		settings->push_back( Setting( "i", "input", "input file", "filename" ) );
		settings->push_back( Setting( "o", "ouput", "output directory", "directory" ) );

		settings->push_back( Setting( "", "name", "map package name", "string" ) );
		settings->push_back( Setting( "", "image", "map package image", "image filename" ) );

		settings->push_back( Setting( "", "plugins", "lists plugins", "" ) );

		settings->push_back( Setting( "", "importer", "importer plugin", "plugin name" ) );
		settings->push_back( Setting( "", "router", "router plugin", "plugin name" ) );
		settings->push_back( Setting( "", "gps-lookup", "gps lookup plugin", "plugin name" ) );
		settings->push_back( Setting( "", "renderer", "renderer plugin", "plugin name" ) );
		settings->push_back( Setting( "", "address-lookup", "address lookup plugin", "plugin name" ) );

		settings->push_back( Setting( "", "do-importing", "runs importer", "" ) );
		settings->push_back( Setting( "", "do-routing", "creates routing module", "module name" ) );
		settings->push_back( Setting( "", "do-rendering", "creates rendering module", "module name" ) );
		settings->push_back( Setting( "", "do-address-lookup", "creates address lookup module", "module name" ) );
		settings->push_back( Setting( "", "do-config", "writes main map config file", "" ) );
		settings->push_back( Setting( "", "do-del-tmp", "deletes temporary files", "" ) );

		settings->push_back( Setting( "", "log", "writes log to file", "filename" ) );

		settings->push_back( Setting( "", "settings", "use settings file", "settings filename" ) );

		settings->push_back( Setting( "v", "verbose", "verbose logging", "" ) );

		return true;
	}

	virtual bool SetSetting( int id, QVariant data )
	{
		switch ( id ) {
		case 0:
			input = data.toString();
			break;
		case 1:
			output = data.toString();
			break;
		case 2:
			name = data.toString();
			break;
		case 3:
			image = data.toString();
			break;
		case 4:
			listPlugins = true;
			break;
		case 5:
			importer = data.toString();
			break;
		case 6:
			router = data.toString();
			break;
		case 7:
			gpsLookup = data.toString();
			break;
		case 8:
			renderer = data.toString();
			break;
		case 9:
			addressLookup = data.toString();
			break;
		case 10:
			importing = true;
			break;
		case 11:
			routingModule = data.toString();
			break;
		case 12:
			renderingModule = data.toString();
			break;
		case 13:
			addressLookupModule = data.toString();
			break;
		case 14:
			config = true;
			break;
		case 15:
			del = true;
			break;
		case 16:
			Log::instance()->setLogFile( data.toString() );
			break;
		case 17:
			settings = data.toString();
			break;
		case 18:
			verbose = true;
			break;
		default:
			return false;
		}

		return true;
	}

	virtual ~Commands()
	{
	}

	QString input;
	QString output;
	QString name;
	QString image;
	bool listPlugins;
	QString importer;
	QString router;
	QString gpsLookup;
	QString renderer;
	QString addressLookup;
	bool importing;
	QString routingModule;
	QString renderingModule;
	QString addressLookupModule;
	bool config;
	bool del;
	QString settings;
	bool verbose;

};

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	CommandLineParser parser;
	Commands commands;

	parser.registerDataSink( &commands );
	if ( !parser.parse() ) {
		a.quit();
		return 0;
	}

	Log::instance()->setMessageTypeEnabled( QtDebugMsg, commands.verbose );
	PluginManager* pluginManager = PluginManager::instance();
	pluginManager->loadPlugins();
	if ( commands.listPlugins ) {
		printf( "%s\n\n", printStringTable( pluginManager->importerPlugins(), 1, "Importer Plugins" ).toUtf8().constData() );
		printf( "%s\n\n", printStringTable( pluginManager->routerPlugins(), 1, "Router Plugins" ).toUtf8().constData() );
		printf( "%s\n\n", printStringTable( pluginManager->gpsLookupPlugins(), 1, "GPS Lookup Plugins" ).toUtf8().constData() );
		printf( "%s\n\n", printStringTable( pluginManager->rendererPlugins(), 1, "Renderer Plugins" ).toUtf8().constData() );
		printf( "%s\n\n", printStringTable( pluginManager->addressLookupPlugins(), 1, "Address Lookup Plugins" ).toUtf8().constData() );
		a.quit();
		return 0;
	}

	if ( !commands.settings.isEmpty() ) {
		QSettings settings( commands.settings, QSettings::IniFormat );
		pluginManager->loadSettings( &settings );

		if ( commands.importer.isEmpty() )
			commands.importer = settings.value( "importer" ).toString();
		if ( commands.router.isEmpty() )
			commands.router = settings.value( "router" ).toString();
		if ( commands.gpsLookup.isEmpty() )
			commands.gpsLookup = settings.value( "gpsLookup" ).toString();
		if ( commands.renderer.isEmpty() )
			commands.renderer = settings.value( "renderer" ).toString();
		if ( commands.addressLookup.isEmpty() )
			commands.addressLookup = settings.value( "addressLookup" ).toString();
	}

	if ( !commands.input.isEmpty() )
		pluginManager->setInputFile( commands.input );
	if ( !commands.output.isEmpty() )
		pluginManager->setOutputDirectory( commands.output );
	if ( !commands.name.isEmpty() )
		pluginManager->setName( commands.name );
	if ( commands.image.isEmpty() )
		pluginManager->setImage( commands.image );

	if ( commands.config ) {
		if ( !pluginManager->writeConfig() ) {
			qCritical() << "Failed to write main map config";
			a.quit();
			return -1;
		}
	}

	if ( commands.importing ) {
		if ( !pluginManager->processImporter( commands.importer ) ) {
			qCritical() << "Failed to import";
			a.quit();
			return -1;
		}
	}

	if ( !commands.routingModule.isEmpty() ) {
		if ( !pluginManager->processRoutingModule( commands.routingModule, commands.importer, commands.router, commands.gpsLookup ) ) {
			qCritical() << "Failed to preprocess routing module";
			a.quit();
			return -1;
		}
	}

	if ( !commands.renderingModule.isEmpty() ) {
		if ( !pluginManager->processRenderingModule( commands.renderingModule, commands.importer, commands.renderer ) ) {
			qCritical() << "Failed to preprocess rendering module";
			a.quit();
			return -1;
		}
	}

	if ( !commands.addressLookupModule.isEmpty() ) {
		if ( !pluginManager->processAddressLookupModule( commands.addressLookupModule, commands.importer, commands.addressLookup ) ) {
			qCritical() << "Failed to preprocess address lookup module";
			a.quit();
			return -1;
		}
	}

	if ( commands.del ) {
		if ( !pluginManager->deleteTemporaryFiles() ) {
			qCritical() << "Failed to delete temporary files";
			a.quit();
			return -1;
		}
	}

	a.quit();
	return 0;
}

