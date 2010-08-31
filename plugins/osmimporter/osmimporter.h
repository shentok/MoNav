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

#include <vector>
#include <libxml/xmlreader.h>
#include <QObject>
#include "interfaces/iimporter.h"
#include "oisettingsdialog.h"
#include "statickdtree.h"
#include "utils/intersection.h"

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
	virtual bool GetRoutingEdges( std::vector< RoutingEdge >* data );
	virtual bool GetRoutingNodes( std::vector< RoutingNode >* data );
	virtual bool GetAddressData( std::vector< Place >* dataPlaces, std::vector< Address >* dataAddresses, std::vector< UnsignedCoordinate >* dataWayBuffer );
	virtual bool GetBoundingBox( BoundingBox* box );
	virtual void DeleteTemporaryFiles();
	virtual ~OSMImporter();

protected:
	QString outputDirectory;
	OISettingsDialog* settingsDialog;

	OISettingsDialog::Settings settings;
	std::vector< const char* > kmhStrings;
	std::vector< const char* > mphStrings;

	struct {
		NodeID numberOfNodes;
		NodeID numberOfEdges;
		NodeID numberOfWays;
		NodeID numberOfPlaces;
		NodeID numberOfOutlines;
		NodeID numberOfMaxspeed;
		NodeID numberOfZeroSpeed;
		NodeID numberOfDefaultCitySpeed;
		NodeID numberOfCityEdges;
	} stats;

	struct _Way {
		std::vector< NodeID > path;
		enum {
			notSure = 0, oneway, bidirectional, opposite
				  } direction;
		double maximumSpeed;
		bool usefull;
		bool access;
		int accessPriority;
		xmlChar* name;
		xmlChar* placeName;
		enum {
			none = 0, suburb, hamlet, village, town, city
			   } placeType;
		int type;
	};

	struct _Node {
		double latitude;
		double longitude;
		unsigned id;
		xmlChar* name;
		unsigned population;
		bool trafficSignal;
		Place::Type type;
	};

	struct _NodeLocation {
		// City / Town
		NodeID place;
		double distance;
		bool isInPlace: 1;
	};

	struct _Place {
		QString name;
		Place::Type type;
		GPSCoordinate coordinate;
	};

	struct _Outline {
		QString name;
		std::vector< DoublePoint > way;

		bool operator<( const _Outline& right ) const {
			return name < right.name;
		}
	};

	class GPSMetric {
	public:
		double operator() ( const double left[2], const double right[2] ) {
			GPSCoordinate leftGPS( left[0], left[1] );
			GPSCoordinate rightGPS( right[0], right[1] );
			double result = leftGPS.ApproximateDistance( rightGPS );
			return result * result;
		}

		double operator() ( const KDTree::BoundingBox< 2, double > &box, const double point[2] ) {
			double nearest[2];
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

		bool operator() ( const KDTree::BoundingBox< 2, double > &box, const double point[2], const double radiusSquared ) {
			double farthest[2];
			for ( unsigned dim = 0; dim < 2; ++dim ) {
				if ( point[dim] < ( box.min[dim] + box.max[dim] ) / 2 )
					farthest[dim] = box.max[dim];
				else
					farthest[dim] = box.min[dim];
			}
			return operator() ( point, farthest ) <= radiusSquared;
		}
	};

	bool _ReadXML( const QString& inputFilename, const QString& filename, std::vector< NodeID >& usedNodes, std::vector< NodeID >& outlineNodes, std::vector< NodeID >& signalNodes );
	bool _PreprocessData( const QString& filename, const std::vector< NodeID >& usedNodes, const std::vector< NodeID >& outlineNodes, const std::vector< NodeID >& signalNodes );
	_Way _ReadXMLWay( xmlTextReaderPtr& inputReader );
	_Node _ReadXMLNode( xmlTextReaderPtr& inputReader );

	xmlTextReaderPtr inputReader;

	typedef KDTree::StaticKDTree< 2, double, NodeID, GPSMetric > GPSTree;
};

#endif // OSMIMPORTER_H
