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

using namespace MoNav;

bool processArguments( CommandType* type, UnpackCommand* unpack, RoutingCommand* routing, int argc, char** argv ) {
	if ( argc == 2 ) {
		type->value = CommandType::UnpackCommand;
		unpack->mapModuleFile = argv[1];
		return true;
	}
	if ( argc < 6 || ( argc & 1 ) == 1 )
		return false;
	type->value = CommandType::RoutingCommand;
	routing->dataDirectory = argv[1];
	for ( int i = 2; i < argc; i+=2 ) {
		bool ok;
		Node coordinate;
		coordinate.latitude = QString( argv[i] ).toDouble( &ok );
		if ( !ok )
			return false;
		coordinate.longitude = QString( argv[i + 1] ).toDouble( &ok );
		if ( !ok )
			return false;
		routing->waypoints.push_back( coordinate );
	}
	return true;
}

int main( int argc, char *argv[] ) {
	CommandType commandType;
	UnpackCommand unpackCommand;
	RoutingCommand routingCommand;
	routingCommand.lookupStrings = true;

	if ( !processArguments( &commandType, &unpackCommand, &routingCommand, argc, argv ) ) {
		qDebug() << "usage:" << argv[0] << "data-directory latitude1 longitude1 latitude2 longitude2 [...latitudeN longitudeN]";
		qDebug() << "\tcomputes a route using between the specified waypoints";
		qDebug() << "usage:" << argv[0] << "monav-map-module-file";
		qDebug() << "\tunpacks a map module";
		return 1;
	}

	QLocalSocket connection;
	connection.connectToServer( "MoNavD" );
	if ( !connection.waitForConnected() ) {
		qDebug() << "failed to connect to daemon:" << connection.error();
		return 2;
	}

	commandType.post( &connection );

	if ( commandType.value == CommandType::UnpackCommand ) {
		unpackCommand.post( &connection );
		connection.flush();
		UnpackResult reply;
		reply.type = UnpackResult::FailUnpacking;
		reply.read( &connection );
		qDebug() << connection.state();

		if ( reply.type == UnpackResult::FailUnpacking ) {
			qDebug() << "failed to unpack map file";
			return 3;
		}
		qDebug() << "finished unpacking map file";
		return 0;
	}

	routingCommand.post( &connection );
	connection.flush();
	RoutingResult reply;
	reply.read( &connection );
	qDebug() << connection.state();

	if ( reply.type == RoutingResult::LoadFailed ) {
		qDebug() << "failed to load data directory";
		return 3;
	} else if ( reply.type == RoutingResult::RouteFailed ) {
		qDebug() << "failed to compute route";
		return 3;
	} else if ( reply.type == RoutingResult::NameLookupFailed ) {
		qDebug() << "failed to compute route";
		return 3;
	} else if ( reply.type == RoutingResult::TypeLookupFailed ) {
		qDebug() << "failed to compute route";
		return 3;
	}else if ( reply.type == RoutingResult::Success ) {
		int seconds = reply.seconds;
		qDebug() << "distance:" << seconds / 60 / 60 << "h" << ( seconds / 60 ) % 60 << "m" << seconds % 60 << "s";
		qDebug() << "nodes:" << reply.pathNodes.size();
		qDebug() << "edges:" << reply.pathEdges.size();

		unsigned node = 0;
		for ( int i = 0; i < reply.pathEdges.size(); i++ ) {
			QString name = reply.nameStrings[reply.pathEdges[i].name];
			QString type = reply.typeStrings[reply.pathEdges[i].type];
			qDebug() << "name:" << name.toUtf8() << "type:" << type << "nodes:" << reply.pathEdges[i].length + 1 << "seconds:" << reply.pathEdges[i].seconds << "branching possible:" << reply.pathEdges[i].branchingPossible;
			for ( unsigned j = 0; j <= reply.pathEdges[i].length; j++ ) {
				QString latitude, longitude;
				latitude.setNum( reply.pathNodes[j + node].latitude, 'g', 10 );
				longitude.setNum( reply.pathNodes[j + node].longitude, 'g', 10 );
				qDebug() << latitude.toLatin1().data() << longitude.toLatin1().data();
			}
			node += reply.pathEdges[i].length;
		}
	} else {
		qDebug() << "return value not recognized";
		return 5;
	}
}
