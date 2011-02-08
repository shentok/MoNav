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

#include "interfaces/irouter.h"
#include "interfaces/igpslookup.h"
#include "signals.h"
#include "qtservice.h"
#include "utils/directoryunpacker.h"

#include <QtCore>
#include <QSettings>
#include <QFile>
#include <QtDebug>
#include <QLocalServer>
#include <QLocalSocket>

using namespace MoNav;

class RoutingDaemon : public QObject, public QtService< QCoreApplication > {

	Q_OBJECT

public:

	RoutingDaemon( int argc, char** argv ) : QtService< QCoreApplication >( argc, argv, "MoNav Routing Daemon" )
	{
		 setServiceDescription( "The MoNav Routing Daemon" );
		 m_loaded = false;
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
		CommandType type;

		if ( !type.read( connection ) )
			return;

		if ( type.value == CommandType::UnpackCommand ) {
			unpackCommand( connection );
		} else if ( type.value == CommandType::RoutingCommand ) {
			routingCommand( connection );
		}

	}

protected:

	void unpackCommand( QLocalSocket* connection )
	{
		UnpackCommand command;
		UnpackResult result;

		if ( !command.read( connection ) )
			return;

		result.type = UnpackResult::Success;

		DirectoryUnpacker unpacker( command.mapModuleFile );
		if ( !unpacker.decompress( command.deleteFile ) )
			result.type = UnpackResult::FailUnpacking;

		result.post( connection );
		connection->flush();
		connection->disconnectFromServer();
	}

	void routingCommand( QLocalSocket* connection )
	{
		RoutingCommand command;
		RoutingResult result;

		if ( !command.read( connection ) )
			return;

		result.type = RoutingResult::Success;

		if ( !m_loaded || command.dataDirectory != m_dataDirectory ) {
			unloadPlugins();
			m_loaded = loadPlugins( command.dataDirectory );
			m_dataDirectory = command.dataDirectory;
		}

		if ( m_loaded ) {
			QVector< IRouter::Node > pathNodes;
			QVector< IRouter::Edge > pathEdges;
			double distance = 0;
			bool success = true;
			for ( int i = 1; i < command.waypoints.size(); i++ ) {
				if ( i != 1 )
					result.pathNodes.pop_back();
				double segmentDistance;
				GPSCoordinate source( command.waypoints[i - 1].latitude, command.waypoints[i - 1].longitude );
				GPSCoordinate target( command.waypoints[i].latitude, command.waypoints[i].longitude );
				pathNodes.clear();
				pathEdges.clear();
				if ( !computeRoute( &segmentDistance, &pathNodes, &pathEdges, source, target, command.lookupRadius ) ) {
					success = false;
					break;
				}
				distance += segmentDistance;

				for ( int j = 0; j < pathNodes.size(); j++ ) {
					Node node;
					GPSCoordinate gps = pathNodes[j].coordinate.ToGPSCoordinate();
					node.latitude = gps.latitude;
					node.longitude = gps.longitude;
					result.pathNodes.push_back( node );
				}

				for ( int j = 0; j < pathEdges.size(); j++ ) {
					Edge edge;
					edge.length = pathEdges[j].length;
					edge.name = pathEdges[j].name;
					edge.type = pathEdges[j].type;
					edge.seconds = pathEdges[j].seconds;
					edge.branchingPossible = pathEdges[j].branchingPossible;
					result.pathEdges.push_back( edge );
				}
			}
			result.seconds = distance;

			if ( success ) {
				if ( command.lookupStrings ) {
					unsigned lastNameID = std::numeric_limits< unsigned >::max();
					QString lastName;
					unsigned lastTypeID = std::numeric_limits< unsigned >::max();
					QString lastType;
					for ( int j = 0; j < result.pathEdges.size(); j++ ) {
						if ( lastNameID != result.pathEdges[j].name ) {
							lastNameID = result.pathEdges[j].name;
							if ( !m_router->GetName( &lastName, lastNameID ) )
								result.type = RoutingResult::NameLookupFailed;
							result.nameStrings.push_back( lastName );
						}

						if ( lastTypeID != result.pathEdges[j].type ) {
							lastTypeID = result.pathEdges[j].type;
							if ( !m_router->GetType( &lastType, lastTypeID ) )
								result.type = RoutingResult::TypeLookupFailed;
							result.typeStrings.push_back( lastType );
						}

						result.pathEdges[j].name = result.nameStrings.size() - 1;
						result.pathEdges[j].type = result.typeStrings.size() - 1;
					}
				}
			} else {
				result.type = RoutingResult::RouteFailed;
			}
		} else {
			result.type = RoutingResult::LoadFailed;
		}

		if ( connection->state() != QLocalSocket::ConnectedState )
			return;

		result.post( connection );
		connection->flush();
		connection->disconnectFromServer();
	}

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

	bool computeRoute( double* resultDistance, QVector< IRouter::Node >* resultNodes, QVector< IRouter::Edge >* resultEdge, GPSCoordinate source, GPSCoordinate target, double lookupRadius )
	{
		if ( m_gpsLookup == NULL || m_router == NULL ) {
			qCritical() << "tried to query route before setting valid data directory";
			return false;
		}
		UnsignedCoordinate sourceCoordinate( source );
		UnsignedCoordinate targetCoordinate( target );
		IGPSLookup::Result sourcePosition;
		QTime time;
		time.start();
		bool found = m_gpsLookup->GetNearestEdge( &sourcePosition, sourceCoordinate, lookupRadius );
		qDebug() << "GPS Lookup:" << time.restart() << "ms";
		if ( !found ) {
			qDebug() << "no edge near source found";
			return false;
		}
		IGPSLookup::Result targetPosition;
		found = m_gpsLookup->GetNearestEdge( &targetPosition, targetCoordinate, lookupRadius );
		qDebug() << "GPS Lookup:" << time.restart() << "ms";
		if ( !found ) {
			qDebug() << "no edge near target found";
			return false;
		}
		found = m_router->GetRoute( resultDistance, resultNodes, resultEdge, sourcePosition, targetPosition );
		qDebug() << "Routing:" << time.restart() << "ms";
		return found;
	}

	bool loadPlugins( QString dataDirectory )
	{
		QDir dir( dataDirectory );
		QString configFilename = dir.filePath( "Module.ini" );
		if ( !QFile::exists( configFilename ) ) {
			qCritical() << "Not a valid routing module directory: Missing Module.ini";
			return false;
		}
		QSettings pluginSettings( configFilename, QSettings::IniFormat );
		int iniVersion = pluginSettings.value( "configVersion" ).toInt();
		if ( iniVersion != 2 ) {
			qCritical() << "Config File not compatible";
			return false;
		}
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
			int gpsLookupFileFormatVersion = pluginSettings.value( "gpsLookupFileFormatVersion" ).toInt();
			if ( !m_gpsLookup->IsCompatible( gpsLookupFileFormatVersion ) ) {
				qCritical() << "GPS Lookup file format not compatible";
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
			int routerFileFormatVersion = pluginSettings.value( "routerFileFormatVersion" ).toInt();
			if ( !m_gpsLookup->IsCompatible( routerFileFormatVersion ) ) {
				qCritical() << "Router file format not compatible";
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

		qDebug() << "loaded:" << pluginSettings.value( "name" ).toString() << pluginSettings.value( "description" ).toString();

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

	bool m_loaded;
	QString m_dataDirectory;
	IGPSLookup* m_gpsLookup;
	IRouter* m_router;
	QLocalServer* m_server;

};

#endif // ROUTINDDAEMON_H
