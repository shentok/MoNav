/*
Copyright 2010  Christian Vetter veaac.fdirct@gmail.com

This file is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This file is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this file. If not, see <http://www.gnu.org/licenses/>.
*/

#include "signals.h"
#include <QtCore>
#include <QLocalSocket>

bool processArguments( RoutingDaemonCommand* command, int argc, char** argv ) {
	if ( argc < 6 || ( argc & 1 ) == 1 )
		return false;
	command->dataDirectory = argv[1];
	for ( int i = 2; i < argc; i+=2 ) {
		bool ok;
		RoutingDaemonCoordinate coordinate;
		coordinate.latitude = QString( argv[i] ).toDouble( &ok );
		if ( !ok )
			return false;
		coordinate.longitude = QString( argv[i + 1] ).toDouble( &ok );
		if ( !ok )
			return false;
		command->waypoints.push_back( coordinate );
	}
	return true;
}

int main( int argc, char *argv[] ) {
	RoutingDaemonCommand command;
	if ( !processArguments( &command, argc, argv ) ) {
		qDebug() << "usage:" << argv[0] << "data-directory latitude1 longitude1 latitude2 longitude2 [...latitudeN longitudeN]";
		qDebug() << "\tcomputes a route using between the specified waypoints";
		return 1;
	}
	command.lookupRadius = 200;

	QLocalSocket connection;
	connection.connectToServer( "MoNavD" );
	if ( !connection.waitForConnected() ) {
		qDebug() << "failed to connect to daemon:" << connection.error();
		return 2;
	}

	command.post( &connection );
	connection.flush();
	RoutingDaemonResult reply;
	reply.read( &connection );
	qDebug() << connection.state();

	if ( reply.type == RoutingDaemonResult::LoadFail ) {
		qDebug() << "failed to load data directory";
		return 3;
	} else if ( reply.type == RoutingDaemonResult::RouteFail ) {
		qDebug() << "failed to compute route";
		return 4;
	} else if ( reply.type == RoutingDaemonResult::Success ) {
		int seconds = reply.seconds;
		qDebug() << "distance:" << seconds / 60 / 60 << "h" << ( seconds / 60 ) % 60 << "m" << seconds % 60 << "s";
		qDebug() << "nodes:" << reply.path.size();
		for ( int i = 0; i < reply.path.size(); i++ ) {
			QString latitude, longitude;
			latitude.setNum( reply.path[i].latitude, 'g', 10 );
			longitude.setNum( reply.path[i].longitude, 'g', 10 );
			qDebug() << latitude.toLatin1().data() << longitude.toLatin1().data();
		}
	} else {
		qDebug() << "return value not recognized";
		return 5;
	}
}
