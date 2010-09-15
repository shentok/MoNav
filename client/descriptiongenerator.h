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

#ifndef DESCRIPTIONGENERATOR_H
#define DESCRIPTIONGENERATOR_H

#include "interfaces/irouter.h"
#include <QVector>
#include <QStringList>
#include <QtDebug>

class DescriptionGenerator {

protected:

	struct Description {
		Description(){}
		Description( unsigned n, unsigned t, int d ) {
			name = n;
			type = t;
			distance = 0;
			direction = d;
		}

		unsigned name;
		unsigned type;
		double distance;
		int direction;
	};

public:

	static void descriptions( QStringList* icons, QStringList* labels, IRouter* router, QVector< IRouter::Node > pathNodes, QVector< IRouter::Edge > pathEdges, int maxDescriptions = std::numeric_limits< int >::max() )
	{
		icons->clear();
		labels->clear();

		if ( pathEdges.empty() || pathNodes.empty() ) {
			*icons = QStringList( "" );
			*labels = QStringList( "No Route" );
			return;
		}

		unsigned lastNameID = std::numeric_limits< unsigned >::max();
		QString lastName;
		unsigned lastTypeID = std::numeric_limits< unsigned >::max();
		QString lastType;

		QVector< Description > descriptions;
		Description lastDescription( pathEdges.first().name, pathEdges.first().type, 0 );

		int node = pathEdges.first().length;
		UnsignedCoordinate fromCoordinate = pathNodes[0].coordinate;
		UnsignedCoordinate toCoordinate = pathNodes[node].coordinate;
		GPSCoordinate fromGPS = fromCoordinate.ToGPSCoordinate();
		GPSCoordinate toGPS = toCoordinate.ToGPSCoordinate();
		lastDescription.distance += fromGPS.ApproximateDistance( toGPS );
		for ( int edge = 1; edge < pathEdges.size(); edge++ ) {
			node += pathEdges[edge].length;
			UnsignedCoordinate nextToCoordinate = pathNodes[node].coordinate;
			int direction = angle( fromCoordinate, toCoordinate, nextToCoordinate );
			if ( abs( direction ) > 1 || lastDescription.name != pathEdges[edge].name ) {
				if ( descriptions.size() + 1 >= maxDescriptions )
					break;
				descriptions.push_back( lastDescription );
				lastDescription = Description( pathEdges[edge].name, pathEdges[edge].type, direction );
			}

			fromCoordinate = toCoordinate;
			toCoordinate = nextToCoordinate;
			fromGPS = toGPS;
			toGPS = nextToCoordinate.ToGPSCoordinate();

			lastDescription.distance += fromGPS.ApproximateDistance( toGPS );
		}
		descriptions.push_back( lastDescription );

		for ( int edge = 0; edge < descriptions.size(); edge++ ) {
			if ( lastNameID != descriptions[edge].name ) {
				lastNameID = descriptions[edge].name;
				bool nameAvailable = router->GetName( &lastName, lastNameID );
				assert( nameAvailable );

				if ( lastName.isEmpty() )
					lastName = "Unnamed Road";
			}
			if ( lastTypeID != descriptions[edge].type ) {
				lastTypeID = descriptions[edge].type;
				bool typeAvailable = router->GetType( &lastType, lastTypeID );
				assert( typeAvailable );
			}

			switch ( descriptions[edge].direction ) {
			case 0:
				{
					icons->push_back( ":/images/directions/forward.png" );
					labels->push_back( ( "Continue on " + lastName + " for %1 m." ).arg( ( int ) descriptions[edge].distance ) );
					break;
				}
			case 1:
				{
					icons->push_back( ":/images/directions/slightlyright.png" );
					labels->push_back( ( "Keep slightly right and continue on " + lastName + " for %1 m." ).arg( ( int ) descriptions[edge].distance ) );
					break;
				}
			case 2:
				{
					icons->push_back( ":/images/directions/right.png" );
					labels->push_back( ( "Turn right and continue on " + lastName + " for %1 m." ).arg( ( int ) descriptions[edge].distance ) );
					break;
				}
			case 3:
				{
					icons->push_back( ":/images/directions/sharplyright.png" );
					labels->push_back( ( "Turn sharply right and continue on " + lastName + " for %1 m." ).arg( ( int ) descriptions[edge].distance ) );
					break;
				}
			case -1:
				{
					icons->push_back( ":/images/directions/slightlyleft.png" );
					labels->push_back( ( "Keep slightly left and continue on " + lastName + " for %1 m." ).arg( ( int ) descriptions[edge].distance ) );
					break;
				}
			case -2:
				{
					icons->push_back( ":/images/directions/left.png" );
					labels->push_back( ( "Turn left and continue on " + lastName + " for %1 m." ).arg( ( int ) descriptions[edge].distance ) );
					break;
				}
			case -3:
				{
					icons->push_back( ":/images/directions/sharplyleft.png" );
					labels->push_back( ( "Turn sharply left and continue on " + lastName + " for %1 m." ).arg( ( int ) descriptions[edge].distance ) );
					break;
				}
			}
		}
	}

protected:

	static int angle( UnsignedCoordinate first, UnsignedCoordinate second, UnsignedCoordinate third ) {
		double x1 = ( double ) second.x - first.x; // a = (x1,y1)
		double y1 = ( double ) second.y - first.y;
		double x2 = ( double ) third.x - second.x; // b = (x2, y2 )
		double y2 = ( double ) third.y - second.y;
		int angle = ( atan2( y1, x1 ) - atan2( y2, x2 ) ) * 180 / M_PI + 720;
		angle %= 360;
		static const int forward = 10;
		static const int sharp = 45;
		static const int slightly = 10;
		if ( angle > 180 ) {
			if ( angle > 360 - forward - slightly ) {
				if ( angle > 360 - forward )
					return 0;
				else
					return 1;
			} else {
				if ( angle > 180 + sharp )
					return 2;
				else
					return 3;
			}
		} else {
			if ( angle > forward + slightly ) {
				if ( angle > 180 - sharp )
					return -3;
				else
					return -2;
			} else {
				if ( angle > forward )
					return -1;
				else
					return 0;
			}
		}
	}

};

#endif // DESCRIPTIONGENERATOR_H
