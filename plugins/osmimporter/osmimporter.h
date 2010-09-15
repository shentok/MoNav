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

#ifndef OSMIMPORTER_H
#define OSMIMPORTER_H

#include "interfaces/iimporter.h"
#include "oisettingsdialog.h"
#include "statickdtree.h"
#include "utils/intersection.h"
#include <libxml/xmlreader.h>
#include <QObject>
#include <QHash>
#include <cstring>

class OSMImporter : public QObject, public IImporter
{
	Q_OBJECT
	Q_INTERFACES( IImporter )

public:

	OSMImporter();
	virtual QString GetName();
	virtual void SetOutputDirectory( const QString& dir );
	virtual void ShowSettings();
	virtual bool Preprocess();
	virtual bool SetIDMap( const std::vector< NodeID >& idMap );
	virtual bool GetIDMap( std::vector< NodeID >* idMap );
	virtual bool SetEdgeIDMap( const std::vector< NodeID >& idMap );
	virtual bool GetEdgeIDMap( std::vector< NodeID >* idMap );
	virtual bool GetRoutingEdges( std::vector< RoutingEdge >* data );
	virtual bool GetRoutingEdgePaths( std::vector< RoutingNode >* data );
	virtual bool GetRoutingNodes( std::vector< RoutingNode >* data );
	virtual bool GetRoutingWayNames( std::vector< QString >* data );
	virtual bool GetRoutingWayTypes( std::vector< QString >* data );
	virtual bool GetAddressData( std::vector< Place >* dataPlaces, std::vector< Address >* dataAddresses, std::vector< UnsignedCoordinate >* dataWayBuffer, std::vector< QString >* addressNames );
	virtual bool GetBoundingBox( BoundingBox* box );
	virtual void DeleteTemporaryFiles();
	virtual ~OSMImporter();

protected:

	struct Statistics{
		NodeID numberOfNodes;
		NodeID numberOfEdges;
		NodeID numberOfWays;
		NodeID numberOfPlaces;
		NodeID numberOfOutlines;
		NodeID numberOfMaxspeed;
		NodeID numberOfZeroSpeed;
		NodeID numberOfDefaultCitySpeed;
		NodeID numberOfCityEdges;

		Statistics() {
			memset( this, 0, sizeof( Statistics ) );
		}
	};

	struct Way {
		std::vector< unsigned > path;
		enum {
			NotSure = 0, Oneway, Bidirectional, Opposite
		} direction;
		double maximumSpeed;
		bool usefull;
		bool access;
		int accessPriority;
		xmlChar* name;
		xmlChar* ref;
		xmlChar* placeName;
		Place::Type placeType;
		unsigned type;
	};

	struct Node {
		double latitude;
		double longitude;
		unsigned id;
		xmlChar* name;
		unsigned population;
		bool trafficSignal;
		Place::Type type;
	};

	struct NodeLocation {
		// City / Town
		unsigned place;
		double distance;
		bool isInPlace: 1;
	};

	struct Location {
		QString name;
		Place::Type type;
		GPSCoordinate coordinate;
	};

	struct Outline {
		QString name;
		std::vector< DoublePoint > way;

		bool operator<( const Outline& right ) const {
			return name < right.name;
		}
	};

	class GPSMetric {
	public:
		double operator() ( const unsigned left[2], const unsigned right[2] ) {
			UnsignedCoordinate leftGPS( left[0], left[1] );
			UnsignedCoordinate rightGPS( right[0], right[1] );
			double result = leftGPS.ToGPSCoordinate().ApproximateDistance( rightGPS.ToGPSCoordinate() );
			return result * result;
		}

		double operator() ( const KDTree::BoundingBox< 2, unsigned > &box, const unsigned point[2] ) {
			unsigned nearest[2];
			for ( unsigned dim = 0; dim < 2; ++dim ) {
				if ( point[dim] < box.min[dim] )
					nearest[dim] = box.min[dim];
				else if ( point[dim] > box.max[dim] )
					nearest[dim] = box.max[dim];
				else
					nearest[dim] = point[dim];
			}
			return operator() ( point, nearest );
		}

		bool operator() ( const KDTree::BoundingBox< 2, unsigned > &box, const unsigned point[2], const double radiusSquared ) {
			unsigned farthest[2];
			for ( unsigned dim = 0; dim < 2; ++dim ) {
				if ( point[dim] < ( box.min[dim] + box.max[dim] ) / 2 )
					farthest[dim] = box.max[dim];
				else
					farthest[dim] = box.min[dim];
			}
			return operator() ( point, farthest ) <= radiusSquared;
		}
	};

	typedef KDTree::StaticKDTree< 2, unsigned, unsigned, GPSMetric > GPSTree;

	bool readXML( const QString& inputFilename, const QString& filename );
	bool preprocessData( const QString& filename );
	bool computeInCityFlags( QString filename, std::vector< NodeLocation >* nodeLocation, const std::vector< UnsignedCoordinate >& nodeCoordinates, const std::vector< UnsignedCoordinate >& outlineCoordinates );
	bool remapEdges( QString filename, const std::vector< UnsignedCoordinate >& nodeCoordinates, const std::vector< NodeLocation >& nodeLocation );
	Way readXMLWay( xmlTextReaderPtr& inputReader );
	Node readXMLNode( xmlTextReaderPtr& inputReader );
	Place::Type parsePlaceType( const xmlChar* type );

	Statistics m_statistics;
	QString m_outputDirectory;
	OISettingsDialog* m_settingsDialog;

	OISettingsDialog::Settings m_settings;
	std::vector< const char* > m_kmhStrings;
	std::vector< const char* > m_mphStrings;
	std::vector< unsigned > m_usedNodes;
	std::vector< unsigned > m_routingNodes;
	std::vector< unsigned > m_outlineNodes;
	std::vector< unsigned > m_signalNodes;
	QHash< QString, unsigned > m_wayNames;
	QHash< QString, unsigned > m_wayRefs;
};

#endif // OSMIMPORTER_H
