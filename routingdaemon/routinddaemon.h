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

#ifndef ROUTINDDAEMON_H
#define ROUTINDDAEMON_H

#include <QtCore>
#include <QSettings>
#include <QFile>
#include <QtDebug>
#include <QLocalServer>
#include <QLocalSocket>

#include "qtservice.h"

#include "interfaces/irouter.h"
#include "interfaces/igpslookup.h"
#include "signals.h"

class RoutingDaemon : public QObject, public QtService< QCoreApplication > {
	Q_OBJECT
public:
	RoutingDaemon( int argc, char** argv ) : QtService< QCoreApplication >( argc, argv, "MoNav Routing Daemon" )
	{
		 setServiceDescription( "The MoNav Routing Daemon" );
		 m_gpsLookup = NULL,
		 m_router = NULL;
		 m_server = new QLocalServer( this );
		 connect( m_server, SIGNAL(newConnection()), this, SLOT(newConnection()) );
	}

	virtual ~RoutingDaemon()
	{
		unloadPlugins();
	}

public slots:

	void newConnection()
	{
		QLocalSocket* connection = m_server->nextPendingConnection();
		connect( connection, SIGNAL(disconnected()), connection, SLOT(deleteLater()) );
		RoutingDaemonCommand command;
		RoutingDaemonResult result;

		command.read( connection );
		if ( command.type == RoutingDaemonCommand::LoadData ) {
			unloadPlugins();
			bool loaded = loadPlugins( command.dataDirectory );
			if ( loaded )
				result.type = RoutingDaemonResult::LoadSuccess;
			else
				result.type = RoutingDaemonResult::LoadFail;
		} else if ( command.type == RoutingDaemonCommand::GetRoute ) {
			QVector< UnsignedCoordinate > path;
			double distance = 0;
			bool success = true;
			for ( int i = 1; i < command.waypoints.size(); i++ ) {
				if ( i != 1 )
					result.path.pop_back();
				double segmentDistance;
				GPSCoordinate source( command.waypoints[i - 1].latitude, command.waypoints[i - 1].longitude );
				GPSCoordinate target( command.waypoints[i].latitude, command.waypoints[i].longitude );
				path.clear();
				if ( !computeRoute( &segmentDistance, &path, source, target ) ) {
					success = false;
					break;
				}
				distance += segmentDistance;
				for ( int j = 0; j < path.size(); j++ ) {
					RoutingDaemonResult::Coordinate coordinate;
					GPSCoordinate gps = path[j].ToGPSCoordinate();
					coordinate.latitude = gps.latitude;
					coordinate.longitude = gps.longitude;
					result.path.push_back( coordinate );
				}
			}

			if ( success )
				result.type = RoutingDaemonResult::RouteSuccess;
			else
				result.type = RoutingDaemonResult::RouteFail;

			result.seconds = distance;
		} else {
			qDebug() << "command not recognized:" << command.type;
			return;
		}
		result.post( connection );
		connection->flush();
		connection->disconnectFromServer();
	}

protected:

	virtual void start()
	{
		if ( !m_server->listen( "MoNavD" ) ) {
			// try to clean up after possible crash
			m_server->removeServer( "MoNavD" );
			if ( !m_server->listen( "MoNavD" ) ) {
				qCritical() << "unable to start server";
				exit( -1 );
			}
		}
	}

	virtual void stop()
	{
		m_server->close();
	}

	bool computeRoute( double* resultDistance, QVector< UnsignedCoordinate >* resultPath, GPSCoordinate source, GPSCoordinate target )
	{
		if ( m_gpsLookup == NULL || m_router == NULL ) {
			qCritical() << "tried to query route before setting valid data directory";
			return false;
		}
		UnsignedCoordinate sourceCoordinate( source );
		UnsignedCoordinate targetCoordinate( target );
		QVector< IGPSLookup::Result > gpsList;
		QTime time;
		time.start();
		bool found = m_gpsLookup->GetNearEdges( &gpsList, sourceCoordinate, 100 );
		qDebug() << "GPS Lookup:" << time.restart() << "ms";
		if ( !found ) {
			qDebug() << "no edge near source found";
			return false;
		}
		IGPSLookup::Result sourcePosition = gpsList.first();
		gpsList.clear();
		found = m_gpsLookup->GetNearEdges( &gpsList, targetCoordinate, 100 );
		qDebug() << "GPS Lookup:" << time.restart() << "ms";
		if ( !found ) {
			qDebug() << "no edge near target found";
			return false;
		}
		IGPSLookup::Result targetPosition = gpsList.first();
		found = m_router->GetRoute( resultDistance, resultPath, sourcePosition, targetPosition );
		qDebug() << "Routing:" << time.restart() << "ms";
		return found;
	}

	bool loadPlugins( QString dataDirectory )
	{
		QDir dir( dataDirectory );
		QString configFilename = dir.filePath( "plugins.ini" );
		if ( !QFile::exists( configFilename ) ) {
			qCritical() << "Not a valid data directory: Missing plugins.ini";
			return false;
		}
		QSettings pluginSettings( configFilename, QSettings::IniFormat );
		QString routerName = pluginSettings.value( "router" ).toString();
		QString gpsLookupName = pluginSettings.value( "gpsLookup" ).toString();

		foreach ( QObject *plugin, QPluginLoader::staticInstances() )
			testPlugin( plugin, routerName, gpsLookupName );

		try
		{
			if ( m_gpsLookup == NULL ) {
				qCritical() << "GPSLookup plugin not found:" << gpsLookupName;
				return false;
			}
			m_gpsLookup->SetInputDirectory( dataDirectory );
			if ( !m_gpsLookup->LoadData() ) {
				qCritical() << "could not load GPSLookup data";
				return false;
			}

			if ( m_router == NULL ) {
				qCritical() << "router plugin not found:" << routerName;
				return false;
			}
			m_router->SetInputDirectory( dataDirectory );
			if ( !m_router->LoadData() ) {
				qCritical() << "could not load router data";
				return false;
			}
		}
		catch ( ... )
		{
			qCritical() << "caught exception while loading plugins";
			return false;
		}

		return true;
	}

	void testPlugin( QObject* plugin, QString routerName, QString gpsLookupName )
	{
		if ( IGPSLookup *interface = qobject_cast< IGPSLookup* >( plugin ) ) {
			qDebug() << "found plugin:" << interface->GetName();
			if ( interface->GetName() == gpsLookupName )
				m_gpsLookup = interface;
		}
		if ( IRouter *interface = qobject_cast< IRouter* >( plugin ) ) {
			qDebug() << "found plugin:" << interface->GetName();
			if ( interface->GetName() == routerName )
				m_router = interface;
		}
	}

	void unloadPlugins()
	{
		m_router = NULL;
		m_gpsLookup = NULL;
	}

	IGPSLookup* m_gpsLookup;
	IRouter* m_router;
	QLocalServer* m_server;

};

#endif // ROUTINDDAEMON_H
