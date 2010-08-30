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

#ifndef SIGNALS_H
#define SIGNALS_H

#include <QIODevice>
#include <QString>
#include <QVector>

class RoutingDaemonCommand {

public:

	enum CommandType {
		LoadData = 0, GetRoute = 1
	} type;

	struct Coordinate {
		double latitude;
		double longitude;
	};

	QVector< Coordinate > waypoints;
	QString dataDirectory;

	void post( QIODevice* out )
	{
		qint32 temp = type;
		out->write( ( const char* ) &temp, sizeof( qint32 ) );
		if ( type == LoadData ) {
			QByteArray data = dataDirectory.toUtf8();
			temp = data.size();
			out->write( ( const char* ) &temp, sizeof( qint32 ) );
			out->write( data );
		} else if ( type == GetRoute ) {
			temp = waypoints.size();
			out->write( ( const char* ) &temp, sizeof( qint32 ) );
			for ( int i = 0; i < waypoints.size(); i++ ) {
				out->write( ( const char* ) &waypoints[i].latitude, sizeof( double ) );
				out->write( ( const char* ) &waypoints[i].longitude, sizeof( double ) );
			}
		}
	}

	void read( QIODevice* in )
	{
		qint32 temp;
		while ( in->bytesAvailable() < ( int ) sizeof( qint32 ) )
			in->waitForReadyRead( 1000 );
		in->read( ( char* ) &temp, sizeof( qint32 ) );
		type = ( CommandType ) temp;
		if ( type == LoadData ) {
			while ( in->bytesAvailable() < ( int ) sizeof( qint32 ) )
				in->waitForReadyRead( 1000 );
			in->read( ( char* ) &temp, sizeof( qint32 ) );
			while ( in->bytesAvailable() < temp )
				in->waitForReadyRead( 1000 );
			QByteArray data = in->read( temp ).data();
			dataDirectory = QString::fromUtf8( data.data() );
		} else if ( type == GetRoute ) {
			while ( in->bytesAvailable() < ( int ) sizeof( qint32 ) )
				in->waitForReadyRead( 1000 );
			in->read( ( char* ) &temp, sizeof( qint32 ) );
			waypoints.resize( temp );
			while ( in->bytesAvailable() < temp * 2 * ( int ) sizeof( double ) )
				in->waitForReadyRead( 1000 );
			for ( int i = 0; i < waypoints.size(); i++ ) {
				in->read( ( char* ) &waypoints[i].latitude, sizeof( double ) );
				in->read( ( char* ) &waypoints[i].longitude, sizeof( double ) );
			}
		}
	}

};

class RoutingDaemonResult {

public:

	enum ResultType {
		LoadSuccess = 0, LoadFail = 1, RouteSuccess = 2, RouteFail = 3
	} type;

	struct Coordinate {
		double latitude;
		double longitude;
	};

	QVector< Coordinate > path;
	double seconds;

	void post( QIODevice* out )
	{
		qint32 temp = type;
		out->write( ( const char* ) &temp, sizeof( qint32 ) );
		if ( type == RouteSuccess ) {
			out->write( ( const char* ) &seconds, sizeof( double ) );
			temp = path.size();
			out->write( ( const char* ) &temp, sizeof( qint32 ) );
			for ( int i = 0; i < path.size(); i++ ) {
				out->write( ( const char* ) &path[i].latitude, sizeof( double ) );
				out->write( ( const char* ) &path[i].longitude, sizeof( double ) );
			}
		}
	}

	void read( QIODevice* in )
	{
		qint32 temp;
		while ( in->bytesAvailable() < ( int ) sizeof( qint32 ) )
			in->waitForReadyRead( 1000 );
		in->read( ( char* ) &temp, sizeof( qint32 ) );
		type = ( ResultType ) temp;
		if ( type == RouteSuccess ) {
			while ( in->bytesAvailable() < ( int ) sizeof( double ) + ( int ) sizeof( qint32 ) )
				in->waitForReadyRead( 1000 );
			in->read( ( char* ) &seconds, sizeof( double ) );
			in->read( ( char* ) &temp, sizeof( qint32 ) );
			path.resize( temp );
			while ( in->bytesAvailable() < temp * 2 * ( int ) sizeof( double ) )
				in->waitForReadyRead( 1000 );
			for ( int i = 0; i < path.size(); i++ ) {
				in->read( ( char* ) &path[i].latitude, sizeof( double ) );
				in->read( ( char* ) &path[i].longitude, sizeof( double ) );
			}
		}
	}

};

#endif // SIGNALS_H
