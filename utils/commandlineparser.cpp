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

#include "commandlineparser.h"

#include <cassert>
#include <QVector>
#include <QMultiHash>
#include <QtDebug>
#include <QCoreApplication>
#include <QStringList>

struct Location {
	IConsoleSettings* dataSink;
	int id;
	QVariant::Type type;

	Location( IConsoleSettings* dataSink, int id, QVariant::Type type )
		: dataSink( dataSink ), id ( id ), type( type )
	{
	}
};

struct CommandLineParser::PrivateImplementation {

	QVector< IConsoleSettings* > dataSinks;
	QMultiHash< QString, Location > shortIDs;
	QMultiHash< QString, Location > longIDs;
};

CommandLineParser::CommandLineParser()
{
	d = new PrivateImplementation;
}

CommandLineParser::~CommandLineParser()
{
	delete d;
}

void CommandLineParser::registerDataSink( IConsoleSettings *dataSink )
{
	assert( dataSink != NULL );

	QVector< IConsoleSettings::Setting > settings;
	if ( !dataSink->GetSettingsList( &settings ) ) {
		qWarning() << "Failed to get settings list from data sink";
		return;
	}
	QString name = dataSink->GetModuleName();

	// add settings
	for ( int id = 0; id < settings.size(); id++ ) {
		if ( !settings[id].shortID.isEmpty() )
			d->shortIDs.insert( settings[id].shortID, Location( dataSink, id, settings[id].type ) );
		d->longIDs.insert( settings[id].longID, Location( dataSink, id, settings[id].type ) );
	}
}

void CommandLineParser::clean()
{
	d->dataSinks.clear();
	d->shortIDs.clear();
	d->longIDs.clear();
}

bool CommandLineParser::parse()
{
	QStringList args = QCoreApplication::arguments();
	for ( int i = 0; i < args.size(); i++ ) {

		QString argument = args[i];
		QString data = "yes";
		int separatorIndex = argument.indexOf( '=' );
		if ( separatorIndex != -1 ) {
			data = argument.mid( separatorIndex + 1 );
			argument = argument.left( separatorIndex );
		}

		// long or short setting?
		QList< Location > settings;
		if ( argument.startsWith( "--" ) ) {
			argument = argument.mid( 2 );
			settings = d->longIDs.values( argument );
		} else if ( argument.startsWith( '-' ) ) {
			argument = argument.mid( 2 );
			settings = d->shortIDs.values( argument );
		} else {
			qWarning() << "Invalid command line option:" << args[i];
			return false;
		}

		// setting available and uniquely identifiable?
		if ( settings.empty() ) {
			qWarning() << "Invalid command line option:" << args[i];
			return false;
		}
		if ( settings.size() > 1 ) {
			qWarning() << "Ambigious command line option not available:" << args[i];
			return false;
		}

		// set setting
		settings.front().dataSink->SetSetting( settings.front().id, data );
	}
	return true;
}

