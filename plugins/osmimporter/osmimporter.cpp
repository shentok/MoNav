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

#include "osmimporter.h"
#include "xmlreader.h"
#include "pbfreader.h"
#include "utils/qthelpers.h"
#include <algorithm>
#include <QtDebug>
#include <limits>

OSMImporter::OSMImporter()
{
	Q_INIT_RESOURCE(speedprofiles);
	m_settingsDialog = NULL;

	m_kmhStrings.push_back( "kmh" );
	m_kmhStrings.push_back( " kmh" );
	m_kmhStrings.push_back( "km/h" );
	m_kmhStrings.push_back( " km/h" );
	m_kmhStrings.push_back( "kph" );
	m_kmhStrings.push_back( " kph" );

	m_mphStrings.push_back( "mph" );
	m_mphStrings.push_back( " mph" );
}

void OSMImporter::setRequiredTags( IEntityReader *reader )
{
	QStringList list;
	// Place = 0, Population = 1, Highway = 2
	list.push_back( "place" );
	list.push_back( "population" );
	list.push_back( "highway" );
	for ( int i = 0; i < m_settings.languageSettings.size(); i++ )
		list.push_back( m_settings.languageSettings[i] );
	reader->setNodeTags( list );

	list.clear();
	//Oneway = 0, Junction = 1, Highway = 2, Ref = 3, PlaceName = 4, Place = 5, MaxSpeed = 6,
	list.push_back( "oneway" );
	list.push_back( "junction" );
	list.push_back( "highway" );
	list.push_back( "ref" );
	list.push_back( "place_name" );
	list.push_back( "place" );
	list.push_back( "maxspeed" );
	for ( int i = 0; i < m_settings.languageSettings.size(); i++ )
		list.push_back( m_settings.languageSettings[i] );
	for ( int i = 0; i < m_settings.accessList.size(); i++ )
		list.push_back( m_settings.accessList[i] );
	reader->setWayTags( list );

	list.clear();
	reader->setRelationTags( list );
}

OSMImporter::~OSMImporter()
{
	Q_CLEANUP_RESOURCE(speedprofiles);
	if ( m_settingsDialog != NULL )
		delete m_settingsDialog;
}

QString OSMImporter::GetName()
{
	return "OpenStreetMap Importer";
}

void OSMImporter::SetOutputDirectory( const QString& dir )
{
	m_outputDirectory = dir;
}

void OSMImporter::ShowSettings()
{
	if ( m_settingsDialog == NULL )
		m_settingsDialog = new OISettingsDialog;
	m_settingsDialog->exec();
}

bool OSMImporter::Preprocess()
{
	if ( m_settingsDialog == NULL )
		m_settingsDialog = new OISettingsDialog();

	if ( !m_settingsDialog->getSettings( &m_settings ) )
		return false;

	if ( m_settings.speedProfile.names.size() == 0 ) {
		qCritical( "no speed profile specified" );
		return false;
	}

	QString filename = fileInDirectory( m_outputDirectory, "OSM Importer" );
	FileStream typeData( filename + "_way_types" );
	if ( !typeData.open( QIODevice::WriteOnly ) )
		return false;

	for ( int type = 0; type < m_settings.speedProfile.names.size(); type++ )
		typeData << m_settings.speedProfile.names[type];
	typeData << QString( "roundabout" );

	std::vector< unsigned >().swap( m_usedNodes );
	std::vector< unsigned >().swap( m_outlineNodes );
	std::vector< unsigned >().swap( m_signalNodes );
	std::vector< unsigned >().swap( m_routingNodes );
	m_wayNames.clear();
	m_wayRefs.clear();

	m_statistics = Statistics();

	Timer time;

	if ( !read( m_settings.input, filename ) )
		return false;
	qDebug() << "OSM Importer: finished import pass 1:" << time.restart() << "ms";

	if ( m_usedNodes.size() == 0 ) {
		qCritical( "OSM Importer: no routing nodes found in the data set" );
		return false;
	}

	std::sort( m_usedNodes.begin(), m_usedNodes.end() );
	for ( unsigned i = 0; i < m_usedNodes.size(); ) {
		unsigned currentNode = m_usedNodes[i];
		int count = 1;
		for ( i++; i < m_usedNodes.size() && currentNode == m_usedNodes[i]; i++ )
			count++;

		if ( count > 1 )
			m_routingNodes.push_back( currentNode );
	}
	m_usedNodes.resize( std::unique( m_usedNodes.begin(), m_usedNodes.end() ) - m_usedNodes.begin() );
	std::sort( m_outlineNodes.begin(), m_outlineNodes.end() );
	m_outlineNodes.resize( std::unique( m_outlineNodes.begin(), m_outlineNodes.end() ) - m_outlineNodes.begin() );
	std::sort( m_signalNodes.begin(), m_signalNodes.end() );
	std::sort( m_routingNodes.begin(), m_routingNodes.end() );
	m_routingNodes.resize( std::unique( m_routingNodes.begin(), m_routingNodes.end() ) - m_routingNodes.begin() );

	if ( !preprocessData( filename ) )
		return false;
	qDebug() << "OSM Importer: finished import pass 2:" << time.restart() << "ms";

	qDebug() << "OSM Importer: nodes:" << m_statistics.numberOfNodes;
	qDebug() << "OSM Importer: ways:" << m_statistics.numberOfWays;
	qDebug() << "OSM Importer: places:" << m_statistics.numberOfPlaces;
	qDebug() << "OSM Importer: places outlines:" << m_statistics.numberOfOutlines;
	qDebug() << "OSM Importer: places outline nodes:" << ( int ) m_outlineNodes.size();
	qDebug() << "OSM Importer: routing edges:" << m_statistics.numberOfEdges;
	qDebug() << "OSM Importer: routing nodes:" << m_routingNodes.size();
	qDebug() << "OSM Importer: used nodes:" << ( int ) m_usedNodes.size();
	qDebug() << "OSM Importer: traffic signal nodes:" << ( int ) m_signalNodes.size();
	qDebug() << "OSM Importer: maxspeed:" << m_statistics.numberOfMaxspeed;
	qDebug() << "OSM Importer: zero speed ways:" << m_statistics.numberOfZeroSpeed;
	qDebug() << "OSM Importer: default city speed:" << m_statistics.numberOfDefaultCitySpeed;
	qDebug() << "OSM Importer: distinct way names:" << m_wayNames.size();
	qDebug() << "OSM Importer: distinct way refs:" << m_wayRefs.size();

	std::vector< unsigned >().swap( m_usedNodes );
	std::vector< unsigned >().swap( m_outlineNodes );
	std::vector< unsigned >().swap( m_signalNodes );
	std::vector< unsigned >().swap( m_routingNodes );
	m_wayNames.clear();
	m_wayRefs.clear();
	return true;
}

bool OSMImporter::read( const QString& inputFilename, const QString& filename ) {
	FileStream edgesData( filename + "_edges" );
	FileStream placesData( filename + "_places" );
	FileStream boundingBoxData( filename + "_bounding_box" );
	FileStream allNodesData( filename + "_all_nodes" );
	FileStream cityOutlineData( filename + "_city_outlines" );
	FileStream wayNamesData( filename + "_way_names" );
	FileStream wayRefsData( filename + "_way_refs" );

	if ( !edgesData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !placesData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !boundingBoxData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !allNodesData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !cityOutlineData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !wayNamesData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !wayRefsData.open( QIODevice::WriteOnly ) )
		return false;

	m_wayNames[QString()] = 0;
	wayNamesData << QString();
	m_wayRefs[QString()] = 0;
	wayRefsData << QString();

	IEntityReader* reader = NULL;
	if ( inputFilename.endsWith( "osm.bz2" ) || inputFilename.endsWith( ".osm" ) )
		reader = new XMLReader();
	else if ( inputFilename.endsWith( ".pbf" ) )
		reader = new PBFReader();

	if ( reader == NULL ) {
		qCritical() << "file format not supporter";
		return false;
	}

	if ( !reader->open( inputFilename ) ) {
		qCritical() << "error opening file";
		return false;
	}

	try {
		GPSCoordinate min( std::numeric_limits< double >::max(), std::numeric_limits< double >::max() );
		GPSCoordinate max( std::numeric_limits< double >::min(), std::numeric_limits< double >::min() );

		setRequiredTags( reader );

		IEntityReader::Way inputWay;
		IEntityReader::Node inputNode;
		IEntityReader::Relation inputRelation;
		Node node;
		Way way;
		while ( true ) {
			IEntityReader::EntityType type = reader->getEntitiy( &inputNode, &inputWay, &inputRelation );

			if ( type == IEntityReader::EntityNone )
				break;

			if ( type == IEntityReader::EntityNode ) {
				m_statistics.numberOfNodes++;
				readNode( &node, inputNode );

				min.latitude = std::min( min.latitude, inputNode.coordinate.latitude );
				min.longitude = std::min( min.longitude, inputNode.coordinate.longitude );
				max.latitude = std::max( max.latitude, inputNode.coordinate.latitude );
				max.longitude = std::max( max.longitude, inputNode.coordinate.longitude );

				if ( node.trafficSignal )
					m_signalNodes.push_back( inputNode.id );

				UnsignedCoordinate coordinate( inputNode.coordinate );
				allNodesData << unsigned( inputNode.id ) << coordinate.x << coordinate.y;

				if ( node.type != Place::None && !node.name.isEmpty() ) {
					placesData << inputNode.coordinate.latitude << inputNode.coordinate.longitude << unsigned( node.type ) << node.population << node.name;
					m_statistics.numberOfPlaces++;
				}

				continue;
			}

			if ( type == IEntityReader::EntityWay ) {
				m_statistics.numberOfWays++;
				readWay( &way, inputWay );

				if ( way.usefull && way.access && inputWay.nodes.size() > 1 ) {
					for ( unsigned node = 0; node < inputWay.nodes.size(); ++node )
						m_usedNodes.push_back( inputWay.nodes[node] );

					m_routingNodes.push_back( inputWay.nodes.front() ); // first and last node are always considered routing nodes
					m_routingNodes.push_back( inputWay.nodes.back() );  // as ways are never merged

					QString name = way.name.simplified();

					if ( !m_wayNames.contains( name ) ) {
						wayNamesData << name;
						int id = m_wayNames.size();
						m_wayNames[name] = id;
					}

					QString ref = name; // fallback to name
					if ( !way.ref.isEmpty() )
						ref = way.ref.simplified();

					if ( !m_wayRefs.contains( ref ) ) {
						wayRefsData << ref;
						int id = m_wayRefs.size();
						m_wayRefs[ref] = id;
					}

					if ( m_settings.ignoreOneway )
						way.direction = Way::Bidirectional;
					if ( m_settings.ignoreMaxspeed )
						way.maximumSpeed = -1;

					if ( way.direction == Way::Opposite )
						std::reverse( inputWay.nodes.begin(), inputWay.nodes.end() );

					edgesData << m_wayNames[name];
					edgesData << m_wayRefs[ref];
					edgesData << way.type;
					edgesData << way.roundabout;
					edgesData << way.maximumSpeed;
					edgesData << !( way.direction == Way::Oneway || way.direction == Way::Opposite );
					edgesData << unsigned( inputWay.nodes.size() );
					for ( unsigned node = 0; node < inputWay.nodes.size(); ++node )
						edgesData << inputWay.nodes[node];

				}

				bool closedWay = inputWay.nodes.size() != 0 && inputWay.nodes.front() == inputWay.nodes.back();

				if ( closedWay && way.placeType != Place::None && !way.placeName.isEmpty() ) {
					QString name = way.placeName.simplified();

					cityOutlineData << unsigned( way.placeType ) << unsigned( inputWay.nodes.size() - 1 );
					cityOutlineData << name;
					for ( unsigned i = 1; i < inputWay.nodes.size(); ++i ) {
						m_outlineNodes.push_back( inputWay.nodes[i] );
						cityOutlineData << inputWay.nodes[i];
					}
					m_statistics.numberOfOutlines++;
				}

				continue;
			}

			if ( type == IEntityReader::EntityRelation ) {

				continue;
			}
		}

		boundingBoxData << min.latitude << min.longitude << max.latitude << max.longitude;

	} catch ( const std::exception& e ) {
		qCritical( "OSM Importer: caught execption: %s", e.what() );
		return false;
	}

	delete reader;
	return true;
}

bool OSMImporter::preprocessData( const QString& filename ) {
	std::vector< UnsignedCoordinate > nodeCoordinates( m_usedNodes.size() );
	std::vector< UnsignedCoordinate > outlineCoordinates( m_outlineNodes.size() );
	std::vector< UnsignedCoordinate > routingCoordinates( m_routingNodes.size() );

	FileStream allNodesData( filename + "_all_nodes" );

	if ( !allNodesData.open( QIODevice::ReadOnly ) )
		return false;

	FileStream routingCoordinatesData( filename + "_routing_coordinates" );

	if ( !routingCoordinatesData.open( QIODevice::WriteOnly ) )
		return false;

	Timer time;

	while ( true ) {
		unsigned node;
		UnsignedCoordinate coordinate;
		allNodesData >> node >> coordinate.x >> coordinate.y;
		if ( allNodesData.status() == QDataStream::ReadPastEnd )
			break;
		std::vector< NodeID >::const_iterator element = std::lower_bound( m_usedNodes.begin(), m_usedNodes.end(), node );
		if ( element != m_usedNodes.end() && *element == node )
			nodeCoordinates[element - m_usedNodes.begin()] = coordinate;
		element = std::lower_bound( m_outlineNodes.begin(), m_outlineNodes.end(), node );
		if ( element != m_outlineNodes.end() && *element == node )
			outlineCoordinates[element - m_outlineNodes.begin()] = coordinate;
		element = std::lower_bound( m_routingNodes.begin(), m_routingNodes.end(), node );
		if ( element != m_routingNodes.end() && *element == node )
			routingCoordinates[element - m_routingNodes.begin()] = coordinate;
	}

	qDebug() << "OSM Importer: filtered node coordinates:" << time.restart() << "ms";

	for ( std::vector< UnsignedCoordinate >::const_iterator i = routingCoordinates.begin(); i != routingCoordinates.end(); ++i )
		routingCoordinatesData << i->x << i->y;

	qDebug() << "OSM Importer: wrote routing node coordinates:" << time.restart() << "ms";

	std::vector< NodeLocation > nodeLocation( m_usedNodes.size() );

	if ( !computeInCityFlags( filename, &nodeLocation, nodeCoordinates, outlineCoordinates ) )
		return false;
	std::vector< UnsignedCoordinate >().swap( outlineCoordinates );

	if ( !remapEdges( filename, nodeCoordinates, nodeLocation ) )
		return false;

	return true;
}

bool OSMImporter::computeInCityFlags( QString filename, std::vector< NodeLocation >* nodeLocation, const std::vector< UnsignedCoordinate >& nodeCoordinates, const std::vector< UnsignedCoordinate >& outlineCoordinates )
{
	FileStream cityOutlinesData( filename + "_city_outlines" );
	FileStream placesData( filename + "_places" );

	if ( !cityOutlinesData.open( QIODevice::ReadOnly ) )
		return false;
	if ( !placesData.open( QIODevice::ReadOnly ) )
		return false;

	Timer time;

	std::vector< Outline > cityOutlines;
	while ( true ) {
		Outline outline;
		unsigned type, numberOfPathNodes;
		cityOutlinesData >> type >> numberOfPathNodes >> outline.name;
		if ( cityOutlinesData.status() == QDataStream::ReadPastEnd )
			break;

		bool valid = true;
		for ( int i = 0; i < ( int ) numberOfPathNodes; ++i ) {
			unsigned node;
			cityOutlinesData >> node;
			NodeID mappedNode = std::lower_bound( m_outlineNodes.begin(), m_outlineNodes.end(), node ) - m_outlineNodes.begin();
			if ( !outlineCoordinates[mappedNode].IsValid() ) {
				qDebug( "OSM Importer: inconsistent OSM data: missing outline node coordinate %d", ( int ) mappedNode );
				valid = false;
			}
			DoublePoint point( outlineCoordinates[mappedNode].x, outlineCoordinates[mappedNode].y );
			outline.way.push_back( point );
		}
		if ( valid )
			cityOutlines.push_back( outline );
	}
	std::sort( cityOutlines.begin(), cityOutlines.end() );

	qDebug() << "OSM Importer: read city outlines:" << time.restart() << "ms";

	std::vector< Location > places;
	while ( true ) {
		Location place;
		unsigned type;
		int population;
		placesData >> place.coordinate.latitude >> place.coordinate.longitude >> type >> population >> place.name;

		if ( placesData.status() == QDataStream::ReadPastEnd )
			break;

		place.type = ( Place::Type ) type;
		places.push_back( place );
	}

	qDebug() << "OSM Importer: read places:" << time.restart() << "ms";

	typedef GPSTree::InputPoint InputPoint;
	std::vector< InputPoint > kdPoints;
	kdPoints.reserve( m_usedNodes.size() );
	for ( std::vector< UnsignedCoordinate >::const_iterator node = nodeCoordinates.begin(), endNode = nodeCoordinates.end(); node != endNode; ++node ) {
		InputPoint point;
		point.data = node - nodeCoordinates.begin();
		point.coordinates[0] = node->x;
		point.coordinates[1] = node->y;
		kdPoints.push_back( point );
		nodeLocation->at( point.data ).isInPlace = false;
		nodeLocation->at( point.data ).distance = std::numeric_limits< double >::max();
	}
	GPSTree kdTree( kdPoints );

	qDebug() << "OSM Importer: build kd-tree:" << time.restart() << "ms";

	for ( std::vector< Location >::const_iterator place = places.begin(), endPlace = places.end(); place != endPlace; ++place ) {
		InputPoint point;
		UnsignedCoordinate placeCoordinate( place->coordinate );
		point.coordinates[0] = placeCoordinate.x;
		point.coordinates[1] = placeCoordinate.y;
		std::vector< InputPoint > result;

		const Outline* placeOutline = NULL;
		double radius = 0;
		Outline searchOutline;
		searchOutline.name = place->name;
		for ( std::vector< Outline >::const_iterator outline = std::lower_bound( cityOutlines.begin(), cityOutlines.end(), searchOutline ), outlineEnd = std::upper_bound( cityOutlines.begin(), cityOutlines.end(), searchOutline ); outline != outlineEnd; ++outline ) {
			UnsignedCoordinate cityCoordinate = UnsignedCoordinate( place->coordinate );
			DoublePoint cityPoint( cityCoordinate.x, cityCoordinate.y );
			if ( pointInPolygon( outline->way.size(), &outline->way[0], cityPoint ) ) {
				placeOutline = &( *outline );
				for ( std::vector< DoublePoint >::const_iterator way = outline->way.begin(), wayEnd = outline->way.end(); way != wayEnd; ++way ) {
					UnsignedCoordinate coordinate( way->x, way->y );
					double distance = coordinate.ToGPSCoordinate().ApproximateDistance( place->coordinate );
					radius = std::max( radius, distance );
				}
				break;
			}
		}

		if ( placeOutline != NULL ) {
			kdTree.NearNeighbors( &result, point, radius );
			for ( std::vector< InputPoint >::const_iterator i = result.begin(), e = result.end(); i != e; ++i ) {
				UnsignedCoordinate coordinate( i->coordinates[0], i->coordinates[1] );
				DoublePoint nodePoint;
				nodePoint.x = coordinate.x;
				nodePoint.y = coordinate.y;
				if ( !pointInPolygon( placeOutline->way.size(), &placeOutline->way[0], nodePoint ) )
					continue;
				nodeLocation->at( i->data ).isInPlace = true;
				nodeLocation->at( i->data ).place = place - places.begin();
				nodeLocation->at( i->data ).distance = 0;
			}
		} else {
			switch ( place->type ) {
			case Place::None:
				continue;
			case Place::Suburb:
				continue;
			case Place::Hamlet:
				kdTree.NearNeighbors( &result, point, 300 );
				break;
			case Place::Village:
				kdTree.NearNeighbors( &result, point, 1000 );
				break;
			case Place::Town:
				kdTree.NearNeighbors( &result, point, 5000 );
				break;
			case Place::City:
				kdTree.NearNeighbors( &result, point, 10000 );
				break;
			}

			for ( std::vector< InputPoint >::const_iterator i = result.begin(), e = result.end(); i != e; ++i ) {
				UnsignedCoordinate coordinate( i->coordinates[0], i->coordinates[1] );
				double distance =  coordinate.ToGPSCoordinate().ApproximateDistance( place->coordinate );
				if ( distance >= nodeLocation->at( i->data ).distance )
					continue;
				nodeLocation->at( i->data ).isInPlace = true;
				nodeLocation->at( i->data ).place = place - places.begin();
				nodeLocation->at( i->data ).distance = distance;
			}
		}
	}

	qDebug() << "OSM Importer: assigned 'in-city' flags:" << time.restart() << "ms";

	return true;
}

bool OSMImporter::remapEdges( QString filename, const std::vector< UnsignedCoordinate >& nodeCoordinates, const std::vector< NodeLocation >& nodeLocation )
{
	FileStream edgesData( filename + "_edges" );

	if ( !edgesData.open( QIODevice::ReadOnly ) )
		return false;

	FileStream mappedEdgesData( filename + "_mapped_edges" );
	FileStream edgeAddressData( filename + "_address" );
	FileStream edgePathsData( filename + "_paths" );

	if ( !mappedEdgesData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !edgeAddressData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !edgePathsData.open( QIODevice::WriteOnly ) )
		return false;

	Timer time;

	unsigned pathID = 0;
	unsigned addressID = 0;
	while ( true ) {
		double speed;
		unsigned numberOfPathNodes, type, nameID, refID;
		bool bidirectional, roundabout;
		std::vector< unsigned > way;

		edgesData >> nameID >> refID >> type >> roundabout >> speed >> bidirectional >> numberOfPathNodes;
		if ( edgesData.status() == QDataStream::ReadPastEnd )
			break;

		assert( ( int ) type < m_settings.speedProfile.names.size() );
		if ( speed <= 0 )
			speed = std::numeric_limits< double >::max();

		bool valid = true;

		for ( int i = 0; i < ( int ) numberOfPathNodes; ++i ) {
			unsigned node;
			edgesData >> node;
			if ( !valid )
				continue;

			NodeID mappedNode = std::lower_bound( m_usedNodes.begin(), m_usedNodes.end(), node ) - m_usedNodes.begin();
			if ( !nodeCoordinates[mappedNode].IsValid() ) {
				qDebug() << "OSM Importer: inconsistent OSM data: skipping way with missing node coordinate";
				valid = false;
			}
			way.push_back( mappedNode );
		}
		if ( !valid )
			continue;

		for ( unsigned pathNode = 0; pathNode + 1 < way.size(); ) {
			unsigned source = std::lower_bound( m_routingNodes.begin(), m_routingNodes.end(), m_usedNodes[way[pathNode]] ) - m_routingNodes.begin();
			assert( source < m_routingNodes.size() && m_routingNodes[source] == m_usedNodes[way[pathNode]] );
			NodeID target = 0;
			double seconds = 0;

			unsigned nextRoutingNode = pathNode + 1;
			while ( true ) {
				NodeID from = way[nextRoutingNode - 1];
				NodeID to = way[nextRoutingNode];
				GPSCoordinate fromCoordinate = nodeCoordinates[from].ToGPSCoordinate();
				GPSCoordinate toCoordinate = nodeCoordinates[to].ToGPSCoordinate();
				double distance = fromCoordinate.Distance( toCoordinate );

				double segmentSpeed = speed;
				if ( m_settings.defaultCitySpeed && ( nodeLocation[from].isInPlace || nodeLocation[to].isInPlace ) ) {
					if ( segmentSpeed == std::numeric_limits< double >::max() ) {
						segmentSpeed = m_settings.speedProfile.speedInCity[type];
						m_statistics.numberOfDefaultCitySpeed++;
					}
				}

				segmentSpeed = std::min( m_settings.speedProfile.speed[type], segmentSpeed );

				segmentSpeed *= m_settings.speedProfile.averagePercentage[type] / 100.0;

				seconds += distance * 3.6 / segmentSpeed;

				if ( std::binary_search( m_signalNodes.begin(), m_signalNodes.end(), m_usedNodes[from] ) )
					seconds += m_settings.trafficLightPenalty / 2.0;
				if ( std::binary_search( m_signalNodes.begin(), m_signalNodes.end(), m_usedNodes[to] ) )
					seconds += m_settings.trafficLightPenalty / 2.0;

				target = std::lower_bound( m_routingNodes.begin(), m_routingNodes.end(), m_usedNodes[to] ) - m_routingNodes.begin();
				if ( target < m_routingNodes.size() && m_routingNodes[target] == m_usedNodes[to] )
					break;

				edgePathsData << nodeCoordinates[to].x << nodeCoordinates[to].y;

				nextRoutingNode++;
			}

			std::vector< unsigned > wayPlaces;
			for ( unsigned i = pathNode; i < nextRoutingNode; i++ ) {
				if ( nodeLocation[way[i]].isInPlace )
					wayPlaces.push_back( nodeLocation[way[i]].place );
			}
			std::sort( wayPlaces.begin(), wayPlaces.end() );
			wayPlaces.resize( std::unique( wayPlaces.begin(), wayPlaces.end() ) - wayPlaces.begin() );
			for ( unsigned i = 0; i < wayPlaces.size(); i++ )
				edgeAddressData << wayPlaces[i];

			mappedEdgesData << source << target << bidirectional << seconds;
			mappedEdgesData << nameID << refID;
			if ( roundabout )
				mappedEdgesData << unsigned( m_settings.speedProfile.names.size() );
			else
				mappedEdgesData << type;
			mappedEdgesData << pathID << nextRoutingNode - pathNode - 1;
			mappedEdgesData << addressID << unsigned( wayPlaces.size() );

			pathID += nextRoutingNode - pathNode - 1;
			addressID += wayPlaces.size();
			pathNode = nextRoutingNode;

			m_statistics.numberOfEdges++;
		}
	}

	qDebug() << "OSM Importer: remapped edges" << time.restart() << "ms";

	return true;
}

void OSMImporter::readWay( OSMImporter::Way* way, const IEntityReader::Way& inputWay ) {
	way->direction = Way::NotSure;
	way->maximumSpeed = -1;
	way->type = -1;
	way->roundabout = false;
	way->name.clear();
	way->namePriority = m_settings.languageSettings.size();
	way->ref.clear();
	way->placeType = Place::None;
	way->placeName.clear();
	way->usefull = false;
	way->access = true;
	way->accessPriority = m_settings.accessList.size();

	for ( unsigned tag = 0; tag < inputWay.tags.size(); tag++ ) {
		int key = inputWay.tags[tag].key;
		QString value = inputWay.tags[tag].value;

		if ( key < WayTags::MaxTag ) {
			switch ( WayTags::Key( key ) ) {
			case WayTags::Oneway:
				{
					if ( value== "no" || value == "false" || value == "0" )
						way->direction = Way::Bidirectional;
					else if ( value == "yes" || value == "true" || value == "1" )
						way->direction = Way::Oneway;
					else if ( value == "-1" )
						way->direction = Way::Opposite;
					break;
				}
			case WayTags::Junction:
				{
					if ( value == "roundabout" ) {
						if ( way->direction == Way::NotSure ) {
							way->direction = Way::Oneway;
							way->roundabout = true;
						}
					}
					break;
				}
			case WayTags::Highway:
				{
					if ( value == "motorway" ) {
						if ( way->direction == Way::NotSure )
							way->direction = Way::Oneway;
					} else if ( value =="motorway_link" ) {
						if ( way->direction == Way::NotSure )
							way->direction = Way::Oneway;
					}

					int index = m_settings.speedProfile.names.indexOf( value );
					if ( index != -1 ) {
						way->type = index;
						way->usefull = true;
					}
					break;
				}
			case WayTags::Ref:
				{
					way->ref = value;
					break;
				}
			case WayTags::PlaceName:
				{
					way->placeName = value;
					break;
				}
			case WayTags::Place:
				{
					way->placeType = parsePlaceType( value );
					break;
				}
			case WayTags::MaxSpeed:
				{
					int index = -1;
					double factor = 1.609344;
					for ( unsigned i = 0; index == -1 && i < m_mphStrings.size(); i++ )
						index = value.lastIndexOf( m_mphStrings[i] );

					if ( index == -1 )
						factor = 1;

					for ( unsigned i = 0; index == -1 && i < m_kmhStrings.size(); i++ )
						index = value.lastIndexOf( m_kmhStrings[i] );

					if( index == -1 )
						index = value.size();
					bool ok = true;
					double maxspeed = value.left( index ).toDouble( &ok );
					if ( ok ) {
						way->maximumSpeed = maxspeed * factor;
						//qDebug() << value << index << value.left( index ) << way->maximumSpeed;
						m_statistics.numberOfMaxspeed++;
					}
					break;
				}
			case WayTags::MaxTag:
				assert( false );
			}

			continue;
		}

		key -= WayTags::MaxTag;
		if ( key < m_settings.languageSettings.size() ) {
			if ( key < way->namePriority ) {
				way->namePriority = key;
				way->name = value;
			}

			continue;
		}

		key -= m_settings.languageSettings.size();
		if ( key < m_settings.accessList.size() ) {
				if ( key < way->accessPriority ) {
					if ( value == "private" || value == "no" || value == "agricultural" || value == "forestry" || value == "delivery" ) {
						way->access = false;
						way->accessPriority = key;
					} else if ( value == "yes" || value == "designated" || value == "official" || value == "permissive" ) {
						way->access = true;
						way->accessPriority = key;
					}
				}

			continue;
		}
	}
}

void OSMImporter::readNode( OSMImporter::Node* node, const IEntityReader::Node& inputNode ) {
	node->name.clear();
	node->namePriority = m_settings.languageSettings.size();
	node->type = Place::None;
	node->population = -1;
	node->trafficSignal = false;

	for ( unsigned tag = 0; tag < inputNode.tags.size(); tag++ ) {
		int key = inputNode.tags[tag].key;
		QString value = inputNode.tags[tag].value;

		if ( key < NodeTags::MaxTag ) {
			switch ( NodeTags::Key( key ) ) {
			case NodeTags::Place:
				{
					node->type = parsePlaceType( value );
					break;
				}
			case NodeTags::Population:
				{
					bool ok;
					int population = value.toInt( &ok );
					if ( ok )
						node->population = population;
					break;
				}
			case NodeTags::Highway:
				{
					if ( value == "traffic_signals" )
						node->trafficSignal = true;
					break;
				}
			case NodeTags::MaxTag:
				assert( false );
			}

			continue;
		}

		key -= NodeTags::MaxTag;
		if ( key < m_settings.languageSettings.size() ) {
			if ( key < node->namePriority ) {
				node->namePriority = key;
				node->name = value;
			}

			continue;
		}
	}
}

OSMImporter::Place::Type OSMImporter::parsePlaceType( const QString& type )
{
	if ( type == "city" )
		return Place::City;
	if ( type == "town" )
		return Place::Town;
	if ( type == "village" )
		return Place::Village;
	if ( type == "hamlet" )
		return Place::Hamlet;
	if ( type == "suburb" )
		return Place::Suburb;
	return Place::None;
}

bool OSMImporter::SetIDMap( const std::vector< NodeID >& idMap )
{
	FileStream idMapData( fileInDirectory( m_outputDirectory, "OSM Importer" ) + "_id_map" );

	if ( !idMapData.open( QIODevice::WriteOnly ) )
		return false;

	idMapData << unsigned( idMap.size() );
	for ( NodeID i = 0; i < ( NodeID ) idMap.size(); i++ )
		idMapData << idMap[i];

	return true;
}

bool OSMImporter::GetIDMap( std::vector< NodeID >* idMap )
{
	FileStream idMapData( fileInDirectory( m_outputDirectory, "OSM Importer" ) + "_id_map" );

	if ( !idMapData.open( QIODevice::ReadOnly ) )
		return false;

	unsigned numNodes;

	idMapData >> numNodes;
	idMap->resize( numNodes );

	for ( NodeID i = 0; i < ( NodeID ) numNodes; i++ ) {
		unsigned temp;
		idMapData >> temp;
		( *idMap )[i] = temp;
	}

	if ( idMapData.status() == QDataStream::ReadPastEnd )
		return false;

	return true;
}

bool OSMImporter::SetEdgeIDMap( const std::vector< NodeID >& idMap )
{
	FileStream idMapData( fileInDirectory( m_outputDirectory, "OSM Importer" ) + "_edge_id_map" );

	if ( !idMapData.open( QIODevice::WriteOnly ) )
		return false;

	idMapData << unsigned( idMap.size() );
	for ( NodeID i = 0; i < ( NodeID ) idMap.size(); i++ )
		idMapData << idMap[i];

	return true;
}

bool OSMImporter::GetEdgeIDMap( std::vector< NodeID >* idMap )
{
	FileStream idMapData( fileInDirectory( m_outputDirectory, "OSM Importer" ) + "_edge_id_map" );

	if ( !idMapData.open( QIODevice::ReadOnly ) )
		return false;

	unsigned numEdges;

	idMapData >> numEdges;
	idMap->resize( numEdges );

	for ( NodeID i = 0; i < ( NodeID ) numEdges; i++ ) {
		unsigned temp;
		idMapData >> temp;
		( *idMap )[i] = temp;
	}

	if ( idMapData.status() == QDataStream::ReadPastEnd )
		return false;

	return true;
}

bool OSMImporter::GetRoutingEdges( std::vector< RoutingEdge >* data )
{
	FileStream mappedEdgesData( fileInDirectory( m_outputDirectory, "OSM Importer" ) + "_mapped_edges" );

	if ( !mappedEdgesData.open( QIODevice::ReadOnly ) )
		return false;

	std::vector< int > nodeOutDegree;
	while ( true ) {
		unsigned source, target, nameID, refID, type;
		unsigned pathID, pathLength;
		unsigned addressID, addressLength;
		bool bidirectional;
		double seconds;

		mappedEdgesData >> source >> target >> bidirectional >> seconds;
		mappedEdgesData >> nameID >> refID >> type;
		mappedEdgesData >> pathID >> pathLength;
		mappedEdgesData >> addressID >> addressLength;

		if ( mappedEdgesData.status() == QDataStream::ReadPastEnd )
			break;

		RoutingEdge edge;
		edge.source = source;
		edge.target = target;
		edge.bidirectional = bidirectional;
		edge.distance = seconds;
		edge.nameID = refID;
		edge.type = type;
		edge.pathID = pathID;
		edge.pathLength = pathLength;

		data->push_back( edge );

		if ( source >= nodeOutDegree.size() )
			nodeOutDegree.resize( source + 1, 0 );
		if ( target >= nodeOutDegree.size() )
			nodeOutDegree.resize( target + 1, 0 );
		nodeOutDegree[source]++;
		if ( bidirectional )
			nodeOutDegree[target]++;
	}

	for ( unsigned edge = 0; edge < data->size(); edge++ ) {
		int branches = nodeOutDegree[data->at( edge ).target];
		branches -= data->at( edge ).bidirectional ? 1 : 0;
		( *data )[edge].branchingPossible = branches > 1;
	}

	return true;
}

bool OSMImporter::GetRoutingNodes( std::vector< RoutingNode >* data )
{
	FileStream routingCoordinatesData( fileInDirectory( m_outputDirectory, "OSM Importer" ) + "_routing_coordinates" );

	if ( !routingCoordinatesData.open( QIODevice::ReadOnly ) )
		return false;

	while ( true ) {
		UnsignedCoordinate coordinate;
		routingCoordinatesData >> coordinate.x >> coordinate.y;
		if ( routingCoordinatesData.status() == QDataStream::ReadPastEnd )
			break;
		RoutingNode node;
		node.coordinate = coordinate;
		data->push_back( node );
	}

	return true;
}

bool OSMImporter::GetRoutingEdgePaths( std::vector< RoutingNode >* data )
{
	FileStream edgePathsData( fileInDirectory( m_outputDirectory, "OSM Importer" ) + "_paths" );

	if ( !edgePathsData.open( QIODevice::ReadOnly ) )
		return false;

	while ( true ) {
		UnsignedCoordinate coordinate;
		edgePathsData >> coordinate.x >> coordinate.y;
		if ( edgePathsData.status() == QDataStream::ReadPastEnd )
			break;
		RoutingNode node;
		node.coordinate = coordinate;
		data->push_back( node );
	}
	return true;
}

bool OSMImporter::GetRoutingWayNames( std::vector< QString >* data )
{
	FileStream wayRefsData( fileInDirectory( m_outputDirectory, "OSM Importer" ) + "_way_refs" );

	if ( !wayRefsData.open( QIODevice::ReadOnly ) )
		return false;

	while ( true ) {
		QString ref;
		wayRefsData >> ref;
		if ( wayRefsData.status() == QDataStream::ReadPastEnd )
			break;
		data->push_back( ref );
	}
	return true;
}

bool OSMImporter::GetRoutingWayTypes( std::vector< QString >* data )
{
	FileStream wayTypesData( fileInDirectory( m_outputDirectory, "OSM Importer" ) + "_way_types" );

	if ( !wayTypesData.open( QIODevice::ReadOnly ) )
		return false;

	while ( true ) {
		QString type;
		wayTypesData >> type;
		if ( wayTypesData.status() == QDataStream::ReadPastEnd )
			break;
		data->push_back( type );
	}
	return true;
}

bool OSMImporter::GetAddressData( std::vector< Place >* dataPlaces, std::vector< Address >* dataAddresses, std::vector< UnsignedCoordinate >* dataWayBuffer, std::vector< QString >* addressNames )
{
	QString filename = fileInDirectory( m_outputDirectory, "OSM Importer" );

	FileStream mappedEdgesData( filename + "_mapped_edges" );
	FileStream routingCoordinatesData( filename + "_routing_coordinates" );
	FileStream placesData( filename + "_places" );
	FileStream edgeAddressData( filename + "_address" );
	FileStream edgePathsData( filename + "_paths" );

	if ( !mappedEdgesData.open( QIODevice::ReadOnly ) )
		return false;
	if ( !routingCoordinatesData.open( QIODevice::ReadOnly ) )
		return false;
	if ( !placesData.open( QIODevice::ReadOnly ) )
		return false;
	if ( !edgeAddressData.open( QIODevice::ReadOnly ) )
		return false;
	if ( !edgePathsData.open( QIODevice::ReadOnly ) )
		return false;

	std::vector< UnsignedCoordinate > coordinates;
	while ( true ) {
		UnsignedCoordinate node;
		routingCoordinatesData >> node.x >> node.y;
		if ( routingCoordinatesData.status() == QDataStream::ReadPastEnd )
			break;
		coordinates.push_back( node );
	}

	while ( true ) {
		GPSCoordinate gps;
		unsigned type;
		QString name;
		unsigned population;
		placesData >> gps.latitude >> gps.longitude >> type >> population >> name;

		if ( placesData.status() == QDataStream::ReadPastEnd )
			break;

		Place place;
		place.name = name;
		place.population = population;
		place.coordinate = UnsignedCoordinate( gps );
		place.type = ( Place::Type ) type;
		dataPlaces->push_back( place );
	}

	std::vector< unsigned > edgeAddress;
	while ( true ) {
		unsigned place;
		edgeAddressData >> place;
		if ( edgeAddressData.status() == QDataStream::ReadPastEnd )
			break;
		edgeAddress.push_back( place );
	}

	std::vector< UnsignedCoordinate > edgePaths;
	while ( true ) {
		UnsignedCoordinate coordinate;
		edgePathsData >> coordinate.x >> coordinate.y;
		if ( edgePathsData.status() == QDataStream::ReadPastEnd )
			break;
		edgePaths.push_back( coordinate );
	}

	long long numberOfEdges = 0;
	long long numberOfAddressPlaces = 0;

	while ( true ) {
		unsigned source, target, nameID, refID, type;
		unsigned pathID, pathLength;
		unsigned addressID, addressLength;
		bool bidirectional;
		double seconds;

		mappedEdgesData >> source >> target >> bidirectional >> seconds;
		mappedEdgesData >> nameID >> refID >> type;
		mappedEdgesData >> pathID >> pathLength;
		mappedEdgesData >> addressID >> addressLength;
		if ( mappedEdgesData.status() == QDataStream::ReadPastEnd )
			break;

		if ( nameID == 0 || addressLength == 0 )
			continue;

		Address newAddress;
		newAddress.name = nameID;
		newAddress.pathID = dataWayBuffer->size();

		dataWayBuffer->push_back( coordinates[source] );
		for ( unsigned i = 0; i < pathLength; i++ )
			dataWayBuffer->push_back( edgePaths[i + pathID] );
		dataWayBuffer->push_back( coordinates[target] );

		newAddress.pathLength = pathLength + 2;
		numberOfEdges++;

		for ( unsigned i = 0; i < addressLength; i++ ) {
			newAddress.nearPlace = edgeAddress[i + addressID];
			dataAddresses->push_back( newAddress );

			numberOfAddressPlaces++;
		}

	}

	FileStream wayNamesData( fileInDirectory( m_outputDirectory, "OSM Importer" ) + "_way_names" );

	if ( !wayNamesData.open( QIODevice::ReadOnly ) )
		return false;

	while ( true ) {
		QString name;
		wayNamesData >> name;
		if ( wayNamesData.status() == QDataStream::ReadPastEnd )
			break;
		addressNames->push_back( name );
	}

	qDebug() << "OSM Importer: edges:" << numberOfEdges;
	qDebug() << "OSM Importer: address entries:" << numberOfAddressPlaces;
	qDebug() << "OSM Importer: address entries per way:" << ( double ) numberOfAddressPlaces / numberOfEdges;
	qDebug() << "OSM Importer: coordinates:" << dataWayBuffer->size();
	return true;
}

bool OSMImporter::GetBoundingBox( BoundingBox* box )
{
	FileStream boundingBoxData( fileInDirectory( m_outputDirectory, "OSM Importer" ) + "_bounding_box" );

	if ( !boundingBoxData.open( QIODevice::ReadOnly ) )
		return false;

	GPSCoordinate minGPS, maxGPS;

	boundingBoxData >> minGPS.latitude >> minGPS.longitude >> maxGPS.latitude >> maxGPS.longitude;

	if ( boundingBoxData.status() == QDataStream::ReadPastEnd ) {
		qCritical() << "error reading bounding box";
		return false;
	}

	box->min = UnsignedCoordinate( minGPS );
	box->max = UnsignedCoordinate( maxGPS );
	if ( box->min.x > box->max.x )
		std::swap( box->min.x, box->max.x );
	if ( box->min.y > box->max.y )
		std::swap( box->min.y, box->max.y );

	return true;
}

void OSMImporter::DeleteTemporaryFiles()
{
	QString filename = fileInDirectory( m_outputDirectory, "OSM Importer" );
	QFile::remove( filename + "_address" );
	QFile::remove( filename + "_all_nodes" );
	QFile::remove( filename + "_bounding_box" );
	QFile::remove( filename + "_city_outlines" );
	QFile::remove( filename + "_edge_id_map" );
	QFile::remove( filename + "_edges" );
	QFile::remove( filename + "_id_map" );
	QFile::remove( filename + "_mapped_edges" );
	QFile::remove( filename + "_paths" );
	QFile::remove( filename + "_places" );
	QFile::remove( filename + "_routing_coordinates" );
	QFile::remove( filename + "_way_names" );
	QFile::remove( filename + "_way_refs" );
	QFile::remove( filename + "_way_types" );
}

Q_EXPORT_PLUGIN2( osmimporter, OSMImporter )
