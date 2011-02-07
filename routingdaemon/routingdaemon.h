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
#include "signals.pb.h"
#include "signals.h"

#include "qtservice.h"

#include <QLocalServer>
#include <QLocalSocket>


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

		MoNav::CommandType type;

		if ( !MoNav::MessageWrapper<MoNav::CommandType, QLocalSocket>::read( connection, type ) ) {
			qDebug() << "Could not read CommandType.";
			return;
		}

		if ( type.value() == MoNav::CommandType::UNPACK_COMMAND ) {
			unpackCommand( connection );
		} else if ( type.value() == MoNav::CommandType::ROUTING_COMMAND ) {
			routingCommand( connection );
		}
	}

protected:

	void unpackCommand( QLocalSocket* connection )
	{
		MoNav::UnpackCommand command;
		MoNav::UnpackResult result;

		if ( !MoNav::MessageWrapper<MoNav::UnpackCommand, QLocalSocket>::read( connection, command ) ) {
			qDebug() << "Could not read UnpackCommand.";
			return;
		}

		result = unpack( command );

		if ( connection->state() != QLocalSocket::ConnectedState ) {
			qDebug() << "Client has disconnected unexpectedly.";
			return;
		}

		MoNav::MessageWrapper<MoNav::UnpackResult, QLocalSocket>::post( connection, result );

		connection->flush();
		connection->disconnectFromServer();
	}

	void routingCommand( QLocalSocket* connection )
	{
		MoNav::RoutingCommand command;
		MoNav::RoutingResult result;

		if ( !MoNav::MessageWrapper<MoNav::RoutingCommand, QLocalSocket>::read( connection, command ) ) {
			qDebug() << "Could not read RoutingCommand.";
			return;
		}

		result = route( command );

		if ( connection->state() != QLocalSocket::ConnectedState ) {
			qDebug() << "Client has disconnected unexpectedly.";
			return;
		}

		MoNav::MessageWrapper<MoNav::RoutingResult, QLocalSocket>::post( connection, result );

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
