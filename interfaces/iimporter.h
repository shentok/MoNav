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

#ifndef IIMPORTER_H
#define IIMPORTER_H

#include <vector>
#include <QString>
#include <QtPlugin>
#include "utils/config.h"
#include "utils/coordinates.h"

class IImporter
{
public:

	struct BoundingBox
	{
		UnsignedCoordinate min;
		UnsignedCoordinate max;
	};
	struct RoutingEdge
	{
		NodeID source;
		NodeID target;
		double distance;
		bool bidirectional;
	};
	struct RoutingNode
	{
		UnsignedCoordinate coordinate;
	};
	struct Place
	{
		QString name;
		int population;
		UnsignedCoordinate coordinate;
		enum Type
		{
			City = 0, Town = 1, Hamlet = 2, Suburb = 3, Village = 4, None = 5
		} type;
		bool operator<( const Place& right ) const {
			if ( name != right.name )
				return name < right.name;
			if ( type != right.type )
				return type < right.type;
			return population < right.population;
		}
	};
	struct Address
	{
		QString name;
		unsigned wayStart;
		unsigned wayEnd;
		unsigned nearPlace;
		bool operator<( const Address& right ) const {
			if ( nearPlace != right.nearPlace )
				return nearPlace < right.nearPlace;
			return name < right.name;
		}
	};

	virtual QString GetName() = 0;
	virtual void SetOutputDirectory( const QString& dir ) = 0;
	virtual void ShowSettings() = 0;
	virtual bool Preprocess() = 0;
	virtual bool SetIDMap( const std::vector< NodeID >& idMap ) = 0;
	virtual bool GetIDMap( std::vector< NodeID >* idMap ) = 0;
	virtual bool GetRoutingEdges( std::vector< RoutingEdge >* data ) = 0;
	virtual bool GetRoutingNodes( std::vector< RoutingNode >* data ) = 0;
	virtual bool GetAddressData( std::vector< Place >* dataPlaces, std::vector< Address >* dataAddresses, std::vector< UnsignedCoordinate >* dataWayBuffer ) = 0;
	virtual bool GetBoundingBox( BoundingBox* box ) = 0;
	virtual ~IImporter() {};
};

Q_DECLARE_INTERFACE( IImporter, "monav.IImporter/1.0" )

#endif // IIMPORTER_H
