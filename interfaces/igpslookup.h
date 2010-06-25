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

#ifndef IGPSLOOKUP_H
#define IGPSLOOKUP_H

#include "utils/config.h"
#include "utils/coordinates.h"
#include <QtPlugin>
#include <QVector>

class IGPSLookup
{
public:

	struct Result {
		bool bidirectional;
		NodeID source;
		NodeID target;
		UnsignedCoordinate nearestPoint;
		double distance;
		double percentage;
		bool operator<( const Result& right ) const {
			return distance < right.distance;
		}
	};

	virtual ~IGPSLookup() {}

	virtual QString GetName() = 0;
	virtual void SetInputDirectory( const QString& dir ) = 0;
	virtual void ShowSettings() = 0;
	virtual bool LoadData() = 0;
	virtual bool GetNearEdges( QVector< Result >* result, const UnsignedCoordinate& coordinate, double radius, bool headingPenalty = 0, double heading = 0 ) = 0;
};

Q_DECLARE_INTERFACE( IGPSLookup, "monav.IGPSLookup/1.1" )

#endif // IGPSLOOKUP_H
