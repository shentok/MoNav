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
#include "bz2input.h"
#include <algorithm>
#include <QtDebug>
#include "utils/qthelpers.h"

OSMImporter::OSMImporter()
{
	Q_INIT_RESOURCE(speedprofiles);
	m_settingsDialog = NULL;
	m_kmhStrings.push_back( "%.lf" );
	m_kmhStrings.push_back( "%.lf kmh" );
	m_kmhStrings.push_back( "%.lf km/h" );
	m_kmhStrings.push_back( "%.lfkmh" );
	m_kmhStrings.push_back( "%.lfkm/h" );
	m_kmhStrings.push_back( "%lf" );
	m_kmhStrings.push_back( "%lf kmh" );
	m_kmhStrings.push_back( "%lf km/h" );
	m_kmhStrings.push_back( "%lfkmh" );
	m_kmhStrings.push_back( "%lfkm/h" );

	m_mphStrings.push_back( "%.lf mph" );
	m_mphStrings.push_back( "%.lfmph" );
	m_mphStrings.push_back( "%lf mph" );
	m_mphStrings.push_back( "%lfmph" );
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

	std::vector< unsigned >().swap( m_usedNodes );
	std::vector< unsigned >().swap( m_outlineNodes );
	std::vector< unsigned >().swap( m_signalNodes );
	std::vector< unsigned >().swap( m_routingNodes );
	m_wayNames.clear();
	QString filename = fileInDirectory( m_outputDirectory, "OSM Importer" );

	m_statistics = Statistics();

	Timer time;

	if ( !readXML( m_settings.input, filename ) )
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

	std::vector< unsigned >().swap( m_usedNodes );
	std::vector< unsigned >().swap( m_outlineNodes );
	std::vector< unsigned >().swap( m_signalNodes );
	std::vector< unsigned >().swap( m_routingNodes );
	m_wayNames.clear();
	return true;
}

bool OSMImporter::readXML( const QString& inputFilename, const QString& filename ) {
	FileStream edgesData( filename + "_edges" );
	FileStream placesData( filename + "_places" );
	FileStream boundingBoxData( filename + "_bounding_box" );
	FileStream allNodesData( filename + "_all_nodes" );
	FileStream cityOutlineData( filename + "_city_outlines" );
	FileStream wayNamesData( filename + "_way_names" );

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

	m_wayNames[QString()] = 0;
	wayNamesData << QString();

	xmlTextReaderPtr inputReader;
	if ( inputFilename.endsWith( ".bz2" ) )
		inputReader = getBz2Reader( inputFilename.toLocal8Bit().constData() );
	else
		inputReader = xmlNewTextReaderFilename( inputFilename.toLocal8Bit().constData() );

	if ( inputReader == NULL ) {
		qCritical() << "failed to open XML reader";
		return false;
	}

	try {
		while ( xmlTextReaderRead( inputReader ) == 1 ) {
			const int type = xmlTextReaderNodeType( inputReader );

			//1 is Element
			if ( type != 1 )
				continue;

			xmlChar* currentName = xmlTextReaderName( inputReader );
			if ( currentName == NULL )
				continue;

			if ( xmlStrEqual( currentName, ( const xmlChar* ) "node" ) == 1 ) {
				m_statistics.numberOfNodes++;
				Node node = readXMLNode( inputReader );

				if ( node.trafficSignal )
					m_signalNodes.push_back( node.id );

				GPSCoordinate gps( node.latitude, node.longitude );
				UnsignedCoordinate coordinate( gps );
				allNodesData << node.id << coordinate.x << coordinate.y;

				if ( node.type != Place::None && node.name != NULL ) {
					placesData << node.latitude << node.longitude << unsigned( node.type ) << node.population << QString::fromUtf8( ( const char* ) node.name );
					m_statistics.numberOfPlaces++;
				}
				if ( node.name != NULL )
					xmlFree( node.name );
			}


			else if ( xmlStrEqual( currentName, ( const xmlChar* ) "way" ) == 1 ) {
				m_statistics.numberOfWays++;
				Way way = readXMLWay( inputReader );

				if ( way.usefull && way.access && way.path.size() > 1 ) {
					for ( unsigned i = 0; i < way.path.size(); ++i )
						m_usedNodes.push_back( way.path[i] );
					m_routingNodes.push_back( way.path.front() );
					m_routingNodes.push_back( way.path.back() );

					QString name;
					if ( way.name != NULL )
						name = QString::fromUtf8( ( const char* ) way.name ).simplified();
					if ( !m_wayNames.contains( name ) ) {
						wayNamesData << name;
						int id = m_wayNames.size();
						m_wayNames[name] = id;
					}
					edgesData << m_wayNames[name];

					if ( m_settings.ignoreOneway )
						way.direction = Way::Bidirectional;
					if ( m_settings.ignoreMaxspeed )
						way.maximumSpeed = -1;

					edgesData << way.type;
					edgesData << way.maximumSpeed;
					edgesData << !( way.direction == Way::Oneway || way.direction == Way::Opposite );
					edgesData << unsigned( way.path.size() );

					if ( way.direction == Way::Opposite )
						std::reverse( way.path.begin(), way.path.end() );

					for ( int i = 0; i < ( int ) way.path.size(); ++i )
						edgesData << way.path[i];

				}

				if ( way.placeType != Place::None && way.path.size() > 1 && way.path[0] == way.path[way.path.size() - 1] && way.placeName != NULL ) {
					cityOutlineData << unsigned( way.placeType ) << unsigned( way.path.size() - 1 );
					QString name;
					if ( way.placeName != NULL )
						name = QString::fromUtf8( ( const char* ) way.placeName ).simplified();

					cityOutlineData << name;
					for ( unsigned i = 1; i < way.path.size(); ++i ) {
						m_outlineNodes.push_back( way.path[i] );
						cityOutlineData << way.path[i];
					}
					m_statistics.numberOfOutlines++;
				}

				if ( way.name != NULL )
					xmlFree( way.name );
				if ( way.placeName != NULL )
					xmlFree( way.placeName );
			}


			else if ( xmlStrEqual( currentName, ( const xmlChar* ) "bound" ) == 1 ) {
				xmlChar* box = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "box" );
				if ( box != NULL ) {
					QString boxString = QString::fromUtf8( ( const char* ) box );
					QStringList coordinateList = boxString.split( ',' );
					if ( coordinateList.size() != 4 )
					{
						qCritical( "OSM Importer: bounding box not valid!" );
						return false;
					}
					double temp;
					temp = coordinateList[0].toDouble();
					boundingBoxData << temp;
					temp = coordinateList[1].toDouble();
					boundingBoxData << temp;
					temp = coordinateList[2].toDouble();
					boundingBoxData << temp;
					temp = coordinateList[3].toDouble();
					boundingBoxData << temp;
					xmlFree( box );
				}
			}

			else if ( xmlStrEqual( currentName, ( const xmlChar* ) "bounds" ) == 1 ) {
				xmlChar* minLat = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "minlat" );
				xmlChar* maxLat = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "maxlat" );
				xmlChar* minLon = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "minlon" );
				xmlChar* maxLon = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "maxlon" );
				if ( minLat != NULL && maxLat != NULL && minLon != NULL && maxLon != NULL ) {
					double temp;
					temp = atof( ( const char* ) minLat );
					boundingBoxData << temp;
					temp = atof( ( const char* ) minLon );
					boundingBoxData << temp;
					temp = atof( ( const char* ) maxLat );
					boundingBoxData << temp;
					temp = atof( ( const char* ) maxLon );
					boundingBoxData << temp;
				}
				if ( minLat != NULL )
					xmlFree( minLat );
					xmlFree( maxLat );
					xmlFree( minLon );
					xmlFree( maxLon );
			}

			xmlFree( currentName );
		}

	} catch ( const std::exception& e ) {
		qCritical( "OSM Importer: caught execption: %s", e.what() );
		return false;
	}
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
		unsigned numberOfPathNodes, type, nameID;
		bool bidirectional;
		std::vector< unsigned > way;

		edgesData >> nameID >> type >> speed >> bidirectional >> numberOfPathNodes;
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
			mappedEdgesData << nameID << type;
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

OSMImporter::Way OSMImporter::readXMLWay( xmlTextReaderPtr& inputReader ) {
	Way way;
	way.direction = Way::NotSure;
	way.maximumSpeed = -1;
	way.type = -1;
	way.name = NULL;
	way.placeType = Place::None;
	way.placeName = NULL;
	way.usefull = false;
	way.access = true;
	way.accessPriority = m_settings.accessList.size();

	if ( xmlTextReaderIsEmptyElement( inputReader ) != 1 ) {
		const int depth = xmlTextReaderDepth( inputReader );
		while ( xmlTextReaderRead( inputReader ) == 1 ) {
			const int childType = xmlTextReaderNodeType( inputReader );
			if ( childType != 1 && childType != 15 )
				continue;
			const int childDepth = xmlTextReaderDepth( inputReader );
			xmlChar* childName = xmlTextReaderName( inputReader );
			if ( childName == NULL )
				continue;

			if ( depth == childDepth && childType == 15 && xmlStrEqual( childName, ( const xmlChar* ) "way" ) == 1 ) {
				xmlFree( childName );
				break;
			}
			if ( childType != 1 ) {
				xmlFree( childName );
				continue;
			}

			if ( xmlStrEqual( childName, ( const xmlChar* ) "tag" ) == 1 ) {
				xmlChar* k = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "k" );
				xmlChar* value = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "v" );
				if ( k != NULL && value != NULL ) {
					if ( xmlStrEqual( k, ( const xmlChar* ) "oneway" ) == 1 ) {
						if ( xmlStrEqual( value, ( const xmlChar* ) "no" ) == 1 || xmlStrEqual( value, ( const xmlChar* ) "false" ) == 1 || xmlStrEqual( value, ( const xmlChar* ) "0" ) == 1 )
							way.direction = Way::Bidirectional;
						else if ( xmlStrEqual( value, ( const xmlChar* ) "yes" ) == 1 || xmlStrEqual( value, ( const xmlChar* ) "true" ) == 1 || xmlStrEqual( value, ( const xmlChar* ) "1" ) == 1 )
							way.direction = Way::Oneway;
						else if ( xmlStrEqual( value, ( const xmlChar* ) "-1" ) == 1 )
							way.direction = Way::Opposite;
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "junction" ) == 1 ) {
						if ( xmlStrEqual( value, ( const xmlChar* ) "roundabout" ) == 1 ) {
							if ( way.direction == Way::NotSure ) {
								way.direction = Way::Oneway;
							}

						}
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "highway" ) == 1 ) {
						if ( xmlStrEqual( value, ( const xmlChar* ) "motorway" ) == 1 ) {
							if ( way.direction == Way::NotSure ) {
								way.direction = Way::Oneway;
							}
						} else if ( xmlStrEqual( value, ( const xmlChar* ) "motorway_link" ) == 1 ) {
							if ( way.direction == Way::NotSure ) {
								way.direction = Way::Oneway;
							}
						}
						{
							QString name = QString::fromUtf8( ( const char* ) value );
							for ( int i = 0; i < m_settings.speedProfile.names.size(); i++ ) {
								if ( name == m_settings.speedProfile.names[i] ) {
									way.type = i;
									way.usefull = true;
									break;
								}
							}
						}
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "name" ) == 1 ) {
						way.name = xmlStrdup( value );
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "place_name" ) ) {
						way.placeName = xmlStrdup( value );
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "place" ) ) {
						way.placeType = parsePlaceType( value );
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "maxspeed" ) == 1 ) {
						double maxspeed = atof(( const char* ) value );

						xmlChar buffer[100];
						for ( int i = 0; i < ( int ) m_kmhStrings.size(); i++ ) {
							xmlStrPrintf( buffer, 100, ( const xmlChar* ) m_kmhStrings[i], maxspeed );
							if ( xmlStrEqual( value, buffer ) == 1 ) {
								way.maximumSpeed = maxspeed;
								m_statistics.numberOfMaxspeed++;
								break;
							}
						}
						for ( int i = 0; i < ( int ) m_mphStrings.size(); i++ ) {
							xmlStrPrintf( buffer, 100, ( const xmlChar* ) m_mphStrings[i], maxspeed );
							if ( xmlStrEqual( value, buffer ) == 1 ) {
								way.maximumSpeed = maxspeed * 1.609344;
								m_statistics.numberOfMaxspeed++;
								break;
							}
						}
					} else {
						QString key = QString::fromUtf8( ( const char* ) k );
						int index = m_settings.accessList.indexOf( key );
						if ( index != -1 && index < way.accessPriority ) {
							if ( xmlStrEqual( value, ( const xmlChar* ) "private" ) == 1
									|| xmlStrEqual( value, ( const xmlChar* ) "no" ) == 1
									|| xmlStrEqual( value, ( const xmlChar* ) "agricultural" ) == 1
									|| xmlStrEqual( value, ( const xmlChar* ) "forestry" ) == 1
									|| xmlStrEqual( value, ( const xmlChar* ) "delivery" ) == 1
									) {
								way.access = false;
								way.accessPriority = index;
							}
							else if ( xmlStrEqual( value, ( const xmlChar* ) "yes" ) == 1
									|| xmlStrEqual( value, ( const xmlChar* ) "designated" ) == 1
									|| xmlStrEqual( value, ( const xmlChar* ) "official" ) == 1
									|| xmlStrEqual( value, ( const xmlChar* ) "permissive" ) == 1
									) {
								way.access = true;
								way.accessPriority = index;
							}
						}
					}

					if ( k != NULL )
						xmlFree( k );
					if ( value != NULL )
						xmlFree( value );
				}
			} else if ( xmlStrEqual( childName, ( const xmlChar* ) "nd" ) == 1 ) {
				xmlChar* ref = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "ref" );
				if ( ref != NULL ) {
					way.path.push_back( atoi(( const char* ) ref ) );
					xmlFree( ref );
				}
			}

			xmlFree( childName );
		}

	}
	return way;
}

OSMImporter::Node OSMImporter::readXMLNode( xmlTextReaderPtr& inputReader ) {
	Node node;
	node.name = NULL;
	node.type = Place::None;
	node.population = -1;
	node.trafficSignal = false;

	xmlChar* attribute = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "lat" );
	if ( attribute != NULL ) {
		node.latitude =  atof(( const char* ) attribute );
		xmlFree( attribute );
	}
	attribute = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "lon" );
	if ( attribute != NULL ) {
		node.longitude =  atof(( const char* ) attribute );
		xmlFree( attribute );
	}
	attribute = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "id" );
	if ( attribute != NULL ) {
		node.id =  atoi(( const char* ) attribute );
		xmlFree( attribute );
	}

	if ( xmlTextReaderIsEmptyElement( inputReader ) != 1 ) {
		const int depth = xmlTextReaderDepth( inputReader );
		while ( xmlTextReaderRead( inputReader ) == 1 ) {
			const int childType = xmlTextReaderNodeType( inputReader );
			// 1 = Element, 15 = EndElement
			if ( childType != 1 && childType != 15 )
				continue;
			const int childDepth = xmlTextReaderDepth( inputReader );
			xmlChar* childName = xmlTextReaderName( inputReader );
			if ( childName == NULL )
				continue;

			if ( depth == childDepth && childType == 15 && xmlStrEqual( childName, ( const xmlChar* ) "node" ) == 1 ) {
				xmlFree( childName );
				break;
			}
			if ( childType != 1 ) {
				xmlFree( childName );
				continue;
			}

			if ( xmlStrEqual( childName, ( const xmlChar* ) "tag" ) == 1 ) {
				xmlChar* k = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "k" );
				xmlChar* value = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "v" );
				if ( k != NULL && value != NULL ) {
					if ( xmlStrEqual( k, ( const xmlChar* ) "place" ) == 1 ) {
						node.type = parsePlaceType( value );
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "name" ) == 1 ) {
						node.name = xmlStrdup( value );
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "population" ) == 1 ) {
						node.population = atoi(( const char* ) value );
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "highway" ) == 1 ) {
						if ( xmlStrEqual( value, ( const xmlChar* ) "traffic_signals" ) == 1 )
							node.trafficSignal = true;
					}
				}
				if ( k != NULL )
					xmlFree( k );
				if ( value != NULL )
					xmlFree( value );
			}

			xmlFree( childName );
		}
	}
	return node;
}

OSMImporter::Place::Type OSMImporter::parsePlaceType( const xmlChar* type )
{
	if ( xmlStrEqual( type, ( const xmlChar* ) "city" ) == 1 )
		return Place::City;
	if ( xmlStrEqual( type, ( const xmlChar* ) "town" ) == 1 )
		return Place::Town;
	if ( xmlStrEqual( type, ( const xmlChar* ) "village" ) == 1 )
		return Place::Village;
	if ( xmlStrEqual( type, ( const xmlChar* ) "hamlet" ) == 1 )
		return Place::Hamlet;
	if ( xmlStrEqual( type, ( const xmlChar* ) "suburb" ) == 1 )
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

	std::vector< NodeID > way;
	while ( true ) {
		unsigned source, target, nameID, type;
		unsigned pathID, pathLength;
		unsigned addressID, addressLength;
		bool bidirectional;
		double seconds;

		mappedEdgesData >> source >> target >> bidirectional >> seconds;
		mappedEdgesData >> nameID >> type;
		mappedEdgesData >> pathID >> pathLength;
		mappedEdgesData >> addressID >> addressLength;

		if ( mappedEdgesData.status() == QDataStream::ReadPastEnd )
			break;

		RoutingEdge edge;
		edge.source = source;
		edge.target = target;
		edge.bidirectional = bidirectional;
		edge.distance = seconds;
		edge.nameID = nameID;
		edge.type = type;
		edge.pathID = pathID;
		edge.pathLength = pathLength;

		data->push_back( edge );
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
	FileStream wayNamesData( fileInDirectory( m_outputDirectory, "OSM Importer" ) + "_way_names" );

	if ( !wayNamesData.open( QIODevice::ReadOnly ) )
		return false;

	while ( true ) {
		QString name;
		wayNamesData >> name;
		if ( wayNamesData.status() == QDataStream::ReadPastEnd )
			break;
		data->push_back( name );
	}
	return true;
}

bool OSMImporter::GetAddressData( std::vector< Place >* dataPlaces, std::vector< Address >* dataAddresses, std::vector< UnsignedCoordinate >* dataWayBuffer )
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
		unsigned source, target, nameID, type;
		unsigned pathID, pathLength;
		unsigned addressID, addressLength;
		bool bidirectional;
		double seconds;

		mappedEdgesData >> source >> target >> bidirectional >> seconds;
		mappedEdgesData >> nameID >> type;
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
	QFile::remove( filename + "_all_nodes" );
	QFile::remove( filename + "_bounding_box" );
	QFile::remove( filename + "_city_outlines" );
	QFile::remove( filename + "_edges" );
	QFile::remove( filename + "_id_map" );
	QFile::remove( filename + "_location" );
	QFile::remove( filename + "_mapped_edges" );
	QFile::remove( filename + "_node_coordinates" );
	QFile::remove( filename + "_places" );
}

Q_EXPORT_PLUGIN2( osmimporter, OSMImporter )
