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

#ifndef ROUTINGDAEMON_H
#define ROUTINGDAEMON_H

#include "routingcommon.h"

#include "qtservice.h"

#include <QLocalServer>
#include <QLocalSocket>

using namespace MoNav;

class RoutingDaemon : public QObject, public QtService< QCoreApplication >, public RoutingCommon {

	Q_OBJECT

public:

	RoutingDaemon( int argc, char** argv ) : QtService< QCoreApplication >( argc, argv, "MoNav Routing Daemon" )
	{
		 setServiceDescription( "The MoNav Routing Daemon" );
		 m_server = new QLocalServer( this );
		 connect( m_server, SIGNAL(newConnection()), this, SLOT(newConnection()) );
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

		result = unpack( command );

		if ( connection->state() != QLocalSocket::ConnectedState )
			return;

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

		result = route( command );

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

	QLocalServer* m_server;
};

#endif // ROUTINGDAEMON_H
