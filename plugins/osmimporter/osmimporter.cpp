#include "osmimporter.h"
#include <QMessageBox>
#include <algorithm>
#include <map>
#include <QtDebug>
#include <QDir>
#include <QDataStream>

OSMImporter::OSMImporter()
{
	settingsDialog = NULL;
}

OSMImporter::~OSMImporter()
{
	if ( settingsDialog != NULL )
		delete settingsDialog;
}

QString OSMImporter::GetName()
{
	return "OpenStreetMap Importer";
}

void OSMImporter::SetOutputDirectory( const QString& dir )
{
	outputDirectory = dir;
}

void OSMImporter::ShowSettings()
{
	if ( settingsDialog == NULL )
		settingsDialog = new OISettingsDialog;
	settingsDialog->exec();
}

bool OSMImporter::Preprocess()
{
	if ( settingsDialog == NULL )
		settingsDialog = new OISettingsDialog();

	std::vector< NodeID > usedNodes;
	std::vector< NodeID > outlineNodes;
	std::vector< NodeID > signalNodes;
	QDir directory( outputDirectory );
	QString inputFilename = settingsDialog->getInput();
	settingsDialog->getSpeedProfile( &speedProfile );
	QString filename = directory.filePath( "OSM Importer" );

	stats.numberOfNodes = 0;
	stats.numberOfEdges = 0;
	stats.numberOfWays = 0;
	stats.numberOfPlaces = 0;
	stats.numberOfOutlines = 0;
	stats.numberOfMaxspeed = 0;
	stats.numberOfZeroSpeed = 0;
	stats.numberOfDefaultCitySpeed = 0;
	stats.numberOfCityEdges = 0;

	qDebug( "Starting Import Pass 1" );
	if ( !_ReadXML( inputFilename, filename, usedNodes, outlineNodes, signalNodes ) )
		return false;

	std::sort( usedNodes.begin(), usedNodes.end() );
	usedNodes.resize( std::unique( usedNodes.begin(), usedNodes.end() ) - usedNodes.begin() );
	std::sort( outlineNodes.begin(), outlineNodes.end() );
	outlineNodes.resize( std::unique( outlineNodes.begin(), outlineNodes.end() ) - outlineNodes.begin() );
	std::sort( signalNodes.begin(), signalNodes.end() );

	qDebug( "Starting Import Pass 2" );
	if ( !_PreprocessData( filename, usedNodes, outlineNodes, signalNodes ) )
		return false;

	qDebug( "OSM Importer: Nodes: %d", stats.numberOfNodes );
	qDebug( "OSM Importer: Ways: %d", stats.numberOfWays );
	qDebug( "OSM Importer: Places: %d", stats.numberOfPlaces );
	qDebug( "OSM Importer: Places Outlines: %d ", stats.numberOfOutlines );
	qDebug( "OSM Importer: Places Outline Nodes: %d " , ( int ) outlineNodes.size() );
	qDebug( "OSM Importer: Edges: %d" , stats.numberOfEdges );
	qDebug( "OSM Importer: Routing Nodes: %d" , ( int ) usedNodes.size() );
	qDebug( "OSM Importer: Traffic Signal Nodes: %d" , ( int ) signalNodes.size() );
	qDebug( "OSM Importer: #Maxspeed Specified: %d" , stats.numberOfMaxspeed );
	qDebug( "OSM Importer: Number Of Zero Speed Ways: %d" , stats.numberOfZeroSpeed );
	qDebug( "OSM Importer: Number Of Edges with Default City Speed: %d" , stats.numberOfDefaultCitySpeed );

	qDebug( "Import Finished" );

	return true;
	return false;
}

bool OSMImporter::_ReadXML( const QString& inputFilename, const QString& filename, std::vector< NodeID >& usedNodes, std::vector< NodeID >& outlineNodes, std::vector< NodeID >& signalNodes ) {
	QFile edgesFile( filename + "_edges" );
	QFile placesFile( filename + "_places" );
	QFile boundBoxFile( filename + "_bounding_box" );
	QFile allNodesFile( filename + "_all_nodes" );
	QFile cityOutlineFile( filename + "_city_outlines" );
	edgesFile.open( QIODevice::WriteOnly );
	placesFile.open( QIODevice::WriteOnly );
	boundBoxFile.open( QIODevice::WriteOnly );
	allNodesFile.open( QIODevice::WriteOnly );
	cityOutlineFile.open( QIODevice::WriteOnly );
	QDataStream edgesData( &edgesFile );
	QDataStream placesData( &placesFile );
	QDataStream boundingBoxData( &boundBoxFile );
	QDataStream allNodesData( &allNodesFile );
	QDataStream cityOutlineData( &cityOutlineFile );

	xmlTextReaderPtr inputReader = xmlNewTextReaderFilename( ( const char* ) inputFilename.toLocal8Bit() );
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
				stats.numberOfNodes++;
				_Node node = _ReadXMLNode( inputReader );

				if ( node.trafficSignal )
					signalNodes.push_back( node.id );

				allNodesData << quint32( node.id ) << node.latitude << node.longitude;

				if ( node.type != Place::None && node.name != NULL ) {
					placesData << node.latitude << node.longitude << quint32( node.type ) << quint32( node.population ) << QString::fromUtf8( ( const char* ) node.name );
					stats.numberOfPlaces++;
				}
				if ( node.name != NULL )
					xmlFree( node.name );
			}


			else if ( xmlStrEqual( currentName, ( const xmlChar* ) "way" ) == 1 ) {
				stats.numberOfWays++;
				_Way way = _ReadXMLWay( inputReader );

				if ( way.usefull && way.access && way.path.size() > 0 ) {
					for ( unsigned i = 0; i < way.path.size(); ++i ) {
						usedNodes.push_back( way.path[i] );
					}

					if ( way.name != NULL )
						edgesData << QString::fromUtf8( ( const char* ) way.name );
					else
						edgesData << QString( "" );

					edgesData << qint32( way.type );
					edgesData << way.maximumSpeed;
					edgesData << qint32(( way.direction == _Way::oneway || way.direction == _Way::opposite ) ? 0 : 1 );
					edgesData << qint32( way.path.size() );

					if ( way.direction == _Way::opposite )
						std::reverse( way.path.begin(), way.path.end() );

					for ( int i = 0; i < ( int ) way.path.size(); ++i )
						edgesData << quint32( way.path[i] );

					stats.numberOfEdges += ( int ) way.path.size() - 1;
				}

				if ( way.placeType != _Way::none && way.path.size() > 1 && way.path[0] == way.path[way.path.size() - 1] && way.placeName != NULL ) {
					cityOutlineData << quint32( way.placeType ) << quint32( way.path.size() - 1 );
					if ( way.placeName != NULL )
						cityOutlineData << QString::fromUtf8( ( const char* ) way.placeName );
					else
						cityOutlineData << QString( "" );
					for ( unsigned i = 1; i < way.path.size(); ++i ) {
						outlineNodes.push_back( way.path[i] );
						cityOutlineData << quint32( way.path[i] );
					}
					stats.numberOfOutlines++;
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
						qCritical( "Bounding Box not valid!" );
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

			xmlFree( currentName );
		}

	} catch ( const std::exception& e ) {
		qCritical( "Caught Execption: %s", e.what() );
		return false;
	}
	return true;
}

bool OSMImporter::_PreprocessData( const QString& filename, const std::vector< NodeID >& usedNodes, const std::vector< NodeID >& outlineNodes, const std::vector< NodeID >& signalNodes ) {
	std::vector< GPSCoordinate > nodeCoordinates( usedNodes.size(), GPSCoordinate( -1, -1 ) );
	std::vector< GPSCoordinate > outlineCoordinates( outlineNodes.size(), GPSCoordinate( -1, -1 ) );

	QFile allNodesFile( filename + "_all_nodes" );
	QFile edgesFile( filename + "_edges" );
	QFile cityOutlinesFile( filename + "_city_outlines" );
	QFile placesFile( filename + "_places" );

	allNodesFile.open( QIODevice::ReadOnly );
	edgesFile.open( QIODevice::ReadOnly );
	cityOutlinesFile.open( QIODevice::ReadOnly );
	placesFile.open( QIODevice::ReadOnly );

	QDataStream allNodesData( &allNodesFile );
	QDataStream edgesData( &edgesFile );
	QDataStream cityOutlineData( &cityOutlinesFile );
	QDataStream placesData( &placesFile );

	QFile nodeCoordinatesFile( filename + "_node_coordinates" );
	QFile mappedEdgesFile( filename + "_mapped_edges" );
	QFile locationFile( filename + "_location" );

	nodeCoordinatesFile.open( QIODevice::WriteOnly );
	mappedEdgesFile.open( QIODevice::WriteOnly );
	locationFile.open( QIODevice::WriteOnly );

	QDataStream nodeCoordinatesData( &nodeCoordinatesFile );
	QDataStream mappedEdgesData( &mappedEdgesFile );
	QDataStream locationData( &locationFile );

	while ( true ) {
		quint32 node;
		GPSCoordinate gps;
		allNodesData >> node >> gps.latitude >> gps.longitude;
		if ( allNodesData.status() == QDataStream::ReadPastEnd )
			break;
		std::vector< NodeID >::const_iterator element = std::lower_bound( usedNodes.begin(), usedNodes.end(), node );
		if ( element != usedNodes.end() && *element == node ) {
			nodeCoordinates[element - usedNodes.begin()] = gps;
		}
		element = std::lower_bound( outlineNodes.begin(), outlineNodes.end(), node );
		if ( element != outlineNodes.end() && *element == node ) {
			outlineCoordinates[element - outlineNodes.begin()] = gps;
		}
	}

	for ( std::vector< NodeID >::const_iterator i = usedNodes.begin(); i != usedNodes.end(); ++i ) {
		NodeID node = i - usedNodes.begin();
		nodeCoordinatesData << nodeCoordinates[node].latitude << nodeCoordinates[node].longitude;
		if ( nodeCoordinates[node].latitude == -1 && nodeCoordinates[node].longitude == -1 )
			qDebug( "Inconsistent OSM Data: Missing Way Node Coordinate %d" , ( int ) node );
	}

	std::vector< _Outline > cityOutlines;
	while ( true ) {
		_Outline outline;
		quint32 type, numberOfPathNodes;
		cityOutlineData >> type >> numberOfPathNodes >> outline.name;
		if ( cityOutlineData.status() == QDataStream::ReadPastEnd )
			break;

		bool valid = true;
		for ( int i = 0; i < ( int ) numberOfPathNodes; ++i ) {
			quint32 node;
			cityOutlineData >> node;
			NodeID mappedNode = std::lower_bound( outlineNodes.begin(), outlineNodes.end(), node ) - outlineNodes.begin();
			UnsignedCoordinate coordinate( outlineCoordinates[mappedNode] );
			if ( outlineCoordinates[mappedNode].latitude == -1 && outlineCoordinates[mappedNode].longitude == -1 ) {
				qDebug( "Inconsistent OSM Data: Missing Outline Node Coordinate %d", ( int ) mappedNode );
				valid = false;
			}
			DoublePoint point( coordinate.x, coordinate.y );
			outline.way.push_back( point );
		}
		if ( valid )
			cityOutlines.push_back( outline );
	}
	outlineCoordinates.clear();
	std::sort( cityOutlines.begin(), cityOutlines.end() );

	std::vector< _Place > places;
	while ( true ) {
		_Place place;
		quint32 type;
		quint32 population;
		placesData >> place.coordinate.latitude >> place.coordinate.longitude >> type >> population >> place.name;

		if ( placesData.status() == QDataStream::ReadPastEnd )
			break;

		place.type = type;
		places.push_back( place );
	}

	typedef GPSTree::InputPoint InputPoint;
	std::vector< InputPoint > kdPoints;
	kdPoints.reserve( usedNodes.size() );
	std::vector< _NodeLocation > nodeLocation( usedNodes.size() );
	for ( std::vector< GPSCoordinate >::const_iterator node = nodeCoordinates.begin(), endNode = nodeCoordinates.end(); node != endNode; ++node ) {
		InputPoint point;
		point.data = node - nodeCoordinates.begin();
		point.coordinates[0] = node->latitude;
		point.coordinates[1] = node->longitude;
		kdPoints.push_back( point );
		nodeLocation[point.data].isInPlace = false;
		nodeLocation[point.data].distance = std::numeric_limits< double >::max();
	}
	GPSTree* kdTree = new GPSTree( kdPoints );
	kdPoints.clear();

	for ( std::vector< _Place >::const_iterator place = places.begin(), endPlace = places.end(); place != endPlace; ++place ) {
		InputPoint point;
		point.coordinates[0] = place->coordinate.latitude;
		point.coordinates[1] = place->coordinate.longitude;
		std::vector< InputPoint > result;

		const _Outline* placeOutline = NULL;
		double radius = 0;
		_Outline searchOutline;
		searchOutline.name = place->name;
		for ( std::vector< _Outline >::const_iterator outline = std::lower_bound( cityOutlines.begin(), cityOutlines.end(), searchOutline ), outlineEnd = std::upper_bound( cityOutlines.begin(), cityOutlines.end(), searchOutline ); outline != outlineEnd; ++outline ) {
			UnsignedCoordinate cityCoordinate = UnsignedCoordinate( place->coordinate );
			DoublePoint cityPoint( cityCoordinate.x, cityCoordinate.y );
			//qDebug( place->name );
			if ( pointInPolygon( outline->way.size(), &outline->way[0], cityPoint ) ) {
				//qDebug( "!!!!!" ) + place->name );
				placeOutline = &( *outline );
				for ( std::vector< DoublePoint >::const_iterator way = outline->way.begin(), wayEnd = outline->way.end(); way != wayEnd; ++way ) {
					UnsignedCoordinate coordinate;
					coordinate.x = way->x;
					coordinate.y = way->y;
					double distance = coordinate.ToGPSCoordinate().ApproximateDistance( place->coordinate );
					radius = std::max( radius, distance );
				}
				break;
			}
		}

		if ( placeOutline != NULL ) {
			kdTree->NearNeighbors( &result, point, radius );
			for ( std::vector< InputPoint >::const_iterator i = result.begin(), e = result.end(); i != e; ++i ) {
				GPSCoordinate gps;
				gps.latitude = i->coordinates[0];
				gps.longitude = i->coordinates[1];
				UnsignedCoordinate coordinate = UnsignedCoordinate( gps );
				DoublePoint nodePoint;
				nodePoint.x = coordinate.x;
				nodePoint.y = coordinate.y;
				if ( !pointInPolygon( placeOutline->way.size(), &placeOutline->way[0], nodePoint ) )
					continue;
				nodeLocation[i->data].isInPlace = true;
				nodeLocation[i->data].place = place - places.begin();
				nodeLocation[i->data].distance = 0;
			}
		} else {
			switch ( place->type ) {
				case 1: {
					//Suburb
						continue;
					}
				case 2: {
					//Hamlet
						kdTree->NearNeighbors( &result, point, 300 );
						break;
					}
				case 3: {
					//Village
						kdTree->NearNeighbors( &result, point, 1000 );
						break;
					}
				case 4: {
					//Town
						kdTree->NearNeighbors( &result, point, 5000 );
						break;
					}
				case 5: {
					//City
						kdTree->NearNeighbors( &result, point, 10000 );
						break;
					}
			}

			for ( std::vector< InputPoint >::const_iterator i = result.begin(), e = result.end(); i != e; ++i ) {
				GPSCoordinate gps;
				gps.latitude = i->coordinates[0];
				gps.longitude = i->coordinates[1];
				double distance =  gps.ApproximateDistance( place->coordinate );
				if ( distance >= nodeLocation[i->data].distance )
					continue;
				nodeLocation[i->data].isInPlace = true;
				nodeLocation[i->data].place = place - places.begin();
				nodeLocation[i->data].distance = distance;
			}
		}
	}

	delete kdTree;
	places.clear();
	cityOutlines.clear();

	for ( std::vector< _NodeLocation >::const_iterator i = nodeLocation.begin(), e = nodeLocation.end(); i != e; ++i ) {
		locationData << quint32( i->isInPlace ? 1 : 0 ) << quint32( i->place );
	}

	while ( true ) {
		QString name;
		double speed;
		qint32 bidirectional, numberOfPathNodes, type;
		std::vector< NodeID > way;
		edgesData >> name >> type >> speed >> bidirectional >> numberOfPathNodes;
		if ( edgesData.status() == QDataStream::ReadPastEnd )
			break;

		bool valid = true;
		if ( speed == 0 )
			valid = false;
		for ( int i = 0; i < ( int ) numberOfPathNodes; ++i ) {
			quint32 node;
			edgesData >> node;
			if ( !valid )
				continue;

			NodeID mappedNode = std::lower_bound( usedNodes.begin(), usedNodes.end(), node ) - usedNodes.begin();
			way.push_back( mappedNode );
			if ( nodeCoordinates[mappedNode].latitude == -1 && nodeCoordinates[mappedNode].longitude == -1 ) {
				qDebug( "Inconsistent OSM Data: Skipping Way With Missing Node Coordinate %d", ( int ) mappedNode );
				valid = false;
			}
		}
		if ( !valid )
			continue;

		mappedEdgesData << name << bidirectional << numberOfPathNodes;

		for ( int i = 0; i < ( int ) numberOfPathNodes; i++ ) {
			mappedEdgesData << way[i];
		}

		if ( speed == 0 || ( speed < 0 && type < 0 ) ) {
			stats.numberOfZeroSpeed++;
			continue;
		}
		if ( type < 0 )
			type = speedProfile.names.size();

		for ( int i = 1; i < ( int ) numberOfPathNodes; ++i ) {
			GPSCoordinate fromCoordinate = nodeCoordinates[way[i - 1]];
			GPSCoordinate toCoordinate = nodeCoordinates[way[i]];
			double distance = fromCoordinate.Distance( toCoordinate );
			double tempSpeed = speed;
			if ( tempSpeed < 0 ) {
				assert( type < ( int ) speedProfile.names.size() );
				if ( config.enforceCityLimits && ( nodeLocation[way[i - 1]].isInPlace || nodeLocation[way[i]].isInPlace ) ) {
					stats.numberOfDefaultCitySpeed++;
					tempSpeed = speedProfile.speedInCity[type];
				}
				else {
					tempSpeed = speedProfile.speed[type];
				}
			}

			if ( type < ( int ) speedProfile.names.size()  ) {
				if ( nodeLocation[way[i - 1]].isInPlace || nodeLocation[way[i]].isInPlace ) {
					stats.numberOfCityEdges++;
				}
				tempSpeed *= speedProfile.averagePercentage[type] / 100.0;
			}
			double seconds = distance * 36 / tempSpeed;

			if ( seconds < 0 )
				qCritical( "Distance less than Zero: %lf", seconds );
			if ( seconds > 24 * 60 * 60 ) {
				qDebug( "Found very large edge: %lf seconds", distance * 36 / tempSpeed );
				qDebug( "Found very large edge: %d to %d", way[i-1], way[i] );
				qDebug( "Found very large edge: lat %lf, lon %lf to lat %lf, lon %lf", fromCoordinate.latitude, fromCoordinate.longitude, toCoordinate.latitude, toCoordinate.longitude );
				qDebug( "Found very large edge: %lf speed", tempSpeed );
			}

			std::vector< NodeID >::const_iterator sourceNode = std::lower_bound( signalNodes.begin(), signalNodes.end(), way[i - 1] );
			std::vector< NodeID >::const_iterator targetNode = std::lower_bound( signalNodes.begin(), signalNodes.end(), way[i] );
			if ( sourceNode != signalNodes.end() && *sourceNode == way[i - 1] )
				seconds += config.trafficLightsPenalty / 2.0;
			if ( targetNode != signalNodes.end() && *targetNode == way[i] )
				seconds += config.trafficLightsPenalty / 2.0;

			mappedEdgesData << seconds;
		}

	}

	return true;
}

OSMImporter::_Way OSMImporter::_ReadXMLWay( xmlTextReaderPtr& inputReader ) {
	_Way way;
	way.direction = _Way::notSure;
	way.maximumSpeed = -1;
	way.type = -1;
	way.name = NULL;
	way.placeType = _Way::none;
	way.placeName = NULL;
	way.usefull = false;
	way.access = true;

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
							way.direction = _Way::bidirectional;
						else if ( xmlStrEqual( value, ( const xmlChar* ) "yes" ) == 1 || xmlStrEqual( value, ( const xmlChar* ) "true" ) == 1 || xmlStrEqual( value, ( const xmlChar* ) "1" ) == 1 )
							way.direction = _Way::oneway;
						else if ( xmlStrEqual( value, ( const xmlChar* ) "-1" ) == 1 )
							way.direction = _Way::opposite;
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "junction" ) == 1 ) {
						if ( xmlStrEqual( value, ( const xmlChar* ) "roundabout" ) == 1 ) {
							if ( way.direction != _Way::notSure ) {
								way.direction = _Way::oneway;
							}
							if ( way.maximumSpeed == -1 )
								way.maximumSpeed = 10;
							way.usefull = true;
						}
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "highway" ) == 1 ) {
						if ( xmlStrEqual( value, ( const xmlChar* ) "motorway" ) == 1 ) {
							if ( way.direction != _Way::notSure ) {
								way.direction = _Way::oneway;
							}
						} else if ( xmlStrEqual( value, ( const xmlChar* ) "motorway_link" ) == 1 ) {
							if ( way.direction != _Way::notSure ) {
								way.direction = _Way::oneway;
							}
						}
						for ( unsigned i = 0; i < speedProfile.names.size(); i++ ) {
							QString name = QString::fromUtf8( ( const char* ) value );
							if ( name == speedProfile.names[i] ) {
								way.type = i;
								way.usefull = true;
								break;
							}
						}
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "name" ) == 1 ) {
						way.name = xmlStrdup( value );
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "place_name" ) ) {
						way.placeName = xmlStrdup( value );
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "place" ) ) {
						if ( xmlStrEqual( value, ( const xmlChar* ) "city" ) == 1 )
							way.placeType = _Way::city;
						else if ( xmlStrEqual( value, ( const xmlChar* ) "town" ) == 1 )
							way.placeType = _Way::town;
						else if ( xmlStrEqual( value, ( const xmlChar* ) "village" ) == 1 )
							way.placeType = _Way::village;
						else if ( xmlStrEqual( value, ( const xmlChar* ) "hamlet" ) == 1 )
							way.placeType = _Way::hamlet;
						else if ( xmlStrEqual( value, ( const xmlChar* ) "suburb" ) == 1 )
							way.placeType = _Way::suburb;
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "maxspeed" ) == 1 ) {
						double maxspeed = atof(( const char* ) value );

						xmlChar buffer[100];
						xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lf", maxspeed );
						if ( xmlStrEqual( value, buffer ) == 1 ) {
							way.maximumSpeed = maxspeed;
							stats.numberOfMaxspeed++;
						} else {
							xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lf kmh", maxspeed );
							if ( xmlStrEqual( value, buffer ) == 1 ) {
								way.maximumSpeed = maxspeed;
								stats.numberOfMaxspeed++;
							} else {
								xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lfkmh", maxspeed );
								if ( xmlStrEqual( value, buffer ) == 1 ) {
									way.maximumSpeed = maxspeed;
									stats.numberOfMaxspeed++;
								} else {
									xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lf km/h", maxspeed );
									if ( xmlStrEqual( value, buffer ) == 1 ) {
										way.maximumSpeed = maxspeed;
										stats.numberOfMaxspeed++;
									} else {
										xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lfkm/h", maxspeed );
										if ( xmlStrEqual( value, buffer ) == 1 ) {
											way.maximumSpeed = maxspeed;
											stats.numberOfMaxspeed++;
										}
									}
								}
							}
						}
					} else if ( xmlStrEqual( k, ( const xmlChar* ) "access" ) == 1 ) {
						if ( xmlStrEqual( value, ( const xmlChar* ) "private" ) == 1
								|| xmlStrEqual( value, ( const xmlChar* ) "no" ) == 1
								|| xmlStrEqual( value, ( const xmlChar* ) "agricultural" ) == 1
								|| xmlStrEqual( value, ( const xmlChar* ) "forestry" ) == 1 )
							way.access = false;
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

OSMImporter::_Node OSMImporter::_ReadXMLNode( xmlTextReaderPtr& inputReader ) {
	_Node node;
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
						if ( xmlStrEqual( value, ( const xmlChar* ) "city" ) == 1 )
							node.type = Place::City;
						else if ( xmlStrEqual( value, ( const xmlChar* ) "town" ) == 1 )
							node.type = Place::Town;
						else if ( xmlStrEqual( value, ( const xmlChar* ) "village" ) == 1 )
							node.type = Place::Village;
						else if ( xmlStrEqual( value, ( const xmlChar* ) "hamlet" ) == 1 )
							node.type = Place::Hamlet;
						else if ( xmlStrEqual( value, ( const xmlChar* ) "suburb" ) == 1 )
							node.type = Place::Suburb;
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

bool OSMImporter::SetIDMap( const std::vector< NodeID >& idMap )
{
	QDir directory( outputDirectory );
	QString filename = directory.filePath( "OSM Importer" );
	QFile idMapFile( filename + "_id_map" );

	idMapFile.open( QIODevice::WriteOnly );
	QDataStream idMapData( &idMapFile );

	idMapData << quint32( idMap.size() );
	for ( NodeID i = 0; i < ( NodeID ) idMap.size(); i++ )
	{
		idMapData << quint32( idMap[i] );
	}

	return true;
}

bool OSMImporter::GetIDMap( std::vector< NodeID >* idMap )
{
	QDir directory( outputDirectory );
	QString filename = directory.filePath( "OSM Importer" );
	QFile idMapFile( filename + "_id_map" );

	if ( !idMapFile.exists() ) {
		qCritical( "File Not Found: _node_coordinates" );
		return false;
	}

	idMapFile.open( QIODevice::WriteOnly );
	QDataStream idMapData( &idMapFile );
	quint32 numNodes;

	idMapData >> numNodes;
	idMap->resize( numNodes );

	for ( NodeID i = 0; i < ( NodeID ) numNodes; i++ )
	{
		quint32 temp;
		idMapData >> temp;
		( *idMap )[i] = temp;
	}

	if ( idMapData.status() == QDataStream::ReadPastEnd )
		return false;

	return true;
}

bool OSMImporter::GetRoutingEdges( std::vector< RoutingEdge >* data )
{
	QDir directory( outputDirectory );
	QString filename = directory.filePath( "OSM Importer" );
	QFile mappedEdgesFile( filename + "_mapped_edges" );

	if ( !mappedEdgesFile.exists() ) {
		qCritical( "File Not Found: _mapped_edges" );
		return false;
	}

	mappedEdgesFile.open( QIODevice::ReadOnly );
	QDataStream mappedEdgesData( &mappedEdgesFile );

	unsigned wayID = 0;
	std::vector< NodeID > way;
	QString name;
	while ( true ) {
		quint32 bidirectional, numberOfPathNodes;
		mappedEdgesData >> name >> bidirectional >> numberOfPathNodes;
		if ( mappedEdgesData.status() == QDataStream::ReadPastEnd )
			break;

		way.clear();
		for ( int i = 0; i < ( int ) numberOfPathNodes; ++i ) {
			NodeID node;
			mappedEdgesData >> node;
			way.push_back( node );
		}
		for ( int i = 1; i < ( int ) numberOfPathNodes; ++i ) {
			RoutingEdge edge;
			edge.source = way[i - 1];
			edge.target = way[i];
			edge.bidirectional = bidirectional == 1;
			double seconds;
			mappedEdgesData >> seconds;
			edge.distance = seconds;

			data->push_back( edge );
		}
		wayID++;
	}

	return true;
}

bool OSMImporter::GetRoutingNodes( std::vector< RoutingNode >* data )
{
	QDir directory( outputDirectory );
	QString filename = directory.filePath( "OSM Importer" );
	QFile nodeCoordinatesFile( filename + "_node_coordinates" );

	if ( !nodeCoordinatesFile.exists() ) {
		qCritical( "File Not Found: _node_coordinates" );
		return false;
	}

	nodeCoordinatesFile.open( QIODevice::ReadOnly );
	QDataStream nodeCoordinatesData( &nodeCoordinatesFile );

	while ( true ) {
		GPSCoordinate gps;
		nodeCoordinatesData >> gps.latitude >> gps.longitude;
		if ( nodeCoordinatesData.status() == QDataStream::ReadPastEnd )
			break;
		RoutingNode node;
		node.coordinate = UnsignedCoordinate( gps );
		data->push_back( node );
	}

	return true;
}

bool OSMImporter::GetAddressData( std::vector< Place >* dataPlaces, std::vector< Address >* dataAddresses, std::vector< UnsignedCoordinate >* dataWayBuffer )
{
	QDir directory( outputDirectory );
	QString filename = directory.filePath( "OSM Importer" );

	QFile mappedEdgesFile( filename + "_mapped_edges" );
	QFile nodeCoordinatesFile( filename + "_node_coordinates" );
	QFile placesFile( filename + "_places" );
	QFile locationFile( filename + "_location" );

	if ( !mappedEdgesFile.exists() ) {
		qCritical( "File Not Found: _mapped_edges" );
		return false;
	}
	if ( !nodeCoordinatesFile.exists() ) {
		qCritical( "File Not Found: _node_coordinates" );
		return false;
	}
	if ( !placesFile.exists() ) {
		qCritical( "File Not Found: _places" );
		return false;
	}
	if ( !locationFile.exists() ) {
		qCritical( "File Not Found: _location" );
		return false;
	}

	QDataStream mappedEdgesData( &mappedEdgesFile );
	QDataStream nodeCoordinatesData( &nodeCoordinatesFile );
	QDataStream placesData( &placesFile );
	QDataStream locationData( &locationFile );

	std::vector< GPSCoordinate >  coordinates;

	while ( true ) {
		GPSCoordinate gps;
		nodeCoordinatesData >> gps.latitude >> gps.longitude;
		if ( nodeCoordinatesData.status() == QDataStream::ReadPastEnd )
			break;
		coordinates.push_back( gps );
	}

	std::vector< _NodeLocation > nodeLocation;
	while( true ) {
		quint32 placeID, isInPlace;
		_NodeLocation location;
		locationData >> isInPlace >> placeID;

		if ( locationData.status() == QDataStream::ReadPastEnd )
			break;

		location.isInPlace = isInPlace == 1;
		location.place = placeID;
		nodeLocation.push_back( location );
	}

	while ( true ) {
		GPSCoordinate gps;
		quint32 type;
		QString name;
		quint32 population;
		placesData >> gps.latitude >> gps.longitude >> type >> population >> name;

		if ( placesData.status() == QDataStream::ReadPastEnd )
			break;

		Place place;
		place.name = name;
		place.population = population;
		place.coordinate = UnsignedCoordinate( gps );
		switch ( type ) {
			case 1: {
					place.type = Place::Suburb;
					break;
				}
			case 2: {
					place.type = Place::Hamlet;
					break;
				}
			case 3: {
					place.type = Place::Village;
					break;
				}
			case 4: {
					place.type = Place::Town;
					break;
				}
			case 5: {
					place.type = Place::City;
					break;
				}
		}
		dataPlaces->push_back( place );
	}

	long long numberOfWays = 0;
	long long numberOfAddressPlaces = 0;
	std::vector< NodeID > wayBuffer;

	while ( true ) {
		std::vector< NodeID > addressPlaces;
		Address newAddress;
		QString name;
		quint32 bidirectional, numberOfPathNodes;
		mappedEdgesData >> name >> bidirectional >> numberOfPathNodes;
		if ( mappedEdgesData.status() == QDataStream::ReadPastEnd )
			break;

		name = name.simplified();
		newAddress.name = name;
		newAddress.wayStart = wayBuffer.size();

		for ( int i = 0; i < ( int ) numberOfPathNodes; ++i ) {
			NodeID node;
			mappedEdgesData >> node;
			if ( name.length() > 0 ) {
				wayBuffer.push_back( node );
				if ( nodeLocation[node].isInPlace )
					addressPlaces.push_back( nodeLocation[node].place );
			}
		}
		for ( int i = 1; i < ( int ) numberOfPathNodes; ++i ) {
			double seconds;
			mappedEdgesData >> seconds;
		}

		newAddress.wayEnd = wayBuffer.size();
		numberOfWays++;

		if ( addressPlaces.size() == 0 ) {
			wayBuffer.resize( newAddress.wayStart );
			continue;
		}

		std::sort( addressPlaces.begin(), addressPlaces.end() );
		addressPlaces.resize( std::unique( addressPlaces.begin(), addressPlaces.end() ) - addressPlaces.begin() );

		if ( name.length() > 0 && addressPlaces.size() > 0 ) {
			for ( std::vector< NodeID >::const_iterator i = addressPlaces.begin(), e = addressPlaces.end(); i != e; ++i ) {
				newAddress.nearPlace = *i;
				dataAddresses->push_back( newAddress );

				numberOfAddressPlaces++;
			}
		}

	}

	dataWayBuffer->reserve( wayBuffer.size() );
	for ( std::vector< NodeID >::const_iterator i = wayBuffer.begin(), e = wayBuffer.end(); i != e; ++i ) {
		dataWayBuffer->push_back( UnsignedCoordinate( coordinates[*i] ) );
	}
	wayBuffer.clear();

	qDebug( "Number Of Ways: %lld", numberOfWays );
	qDebug( "Number Of Address Entries: %lld", numberOfAddressPlaces );
	qDebug( "Average Address Entries per _Way: %lf", ( double ) numberOfAddressPlaces / numberOfWays );
	qDebug( "Number Of _Way Nodes: %d", ( int ) dataWayBuffer->size() );
	return true;
}

bool OSMImporter::GetBoundingBox( BoundingBox* box )
{
	QDir directory( outputDirectory );
	QString filename = directory.filePath( "OSM Importer" );
	QFile boundingBoxFile( filename + "_bounding_box" );

	if ( !boundingBoxFile.exists() ) {
		qCritical( "File Not Found: _bounding_box" );
		return false;
	}

	boundingBoxFile.open( QIODevice::ReadOnly );
	QDataStream boundingBoxData( &boundingBoxFile );

	GPSCoordinate minGPS, maxGPS;

	boundingBoxData >> minGPS.latitude >> minGPS.longitude >> maxGPS.latitude >> maxGPS.longitude;

	if ( boundingBoxData.status() == QDataStream::ReadPastEnd )
		return false;

	box->min = UnsignedCoordinate( minGPS );
	box->max = UnsignedCoordinate( maxGPS );
	if ( box->min.x > box->max.x )
		std::swap( box->min.x, box->max.x );
	if ( box->min.y > box->max.y )
		std::swap( box->min.y, box->max.y );

	return true;
}

Q_EXPORT_PLUGIN2( osmimporter, OSMImporter )
