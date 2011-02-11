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

#include "gpsgridclient.h"
#include <QtDebug>
#include <QHash>
#include <algorithm>
#include "utils/qthelpers.h"
#ifndef NOGUI
	#include <QInputDialog>
#endif
#include <QSettings>

GPSGridClient::GPSGridClient()
{
	index = NULL;
	gridFile = NULL;
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "GPS Grid" );
	cacheSize = settings.value( "cacheSize", 1 ).toInt();
	cache.setMaxCost( 1024 * 1024 * 3 * cacheSize / 4 );
}

GPSGridClient::~GPSGridClient()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "GPS Grid" );
	settings.setValue( "cacheSize", cacheSize );
	UnloadData();
}

QString GPSGridClient::GetName()
{
	return "GPS Grid";
}

void GPSGridClient::SetInputDirectory( const QString& dir )
{
	directory = dir;
}

void GPSGridClient::ShowSettings()
{
#ifndef NOGUI
	bool ok = false;
	int result = QInputDialog::getInt( NULL, "Settings", "Enter Cache Size [MB]", cacheSize, 1, 1024, 1, &ok );
	if ( !ok )
		return;
	cacheSize = result;
	if ( index != NULL )
		index->SetCacheSize( 1024 * 1024 * cacheSize / 4 );
	cache.setMaxCost( 1024 * 1024 * 3 * cacheSize / 4 );
#endif
}

bool GPSGridClient::IsCompatible( int fileFormatVersion )
{
	if ( fileFormatVersion == 1 )
		return true;
	return false;
}

bool GPSGridClient::LoadData()
{
	UnloadData();
	QString filename = fileInDirectory( directory, "GPSGrid" );
	QFile configFile( filename + "_config" );
	if ( !openQFile( &configFile, QIODevice::ReadOnly ) )
		return false;

	index = new gg::Index( filename + "_index" );
	index->SetCacheSize( 1024 * 1024 * cacheSize / 4 );

	gridFile = new QFile( filename + "_grid" );
	if ( !gridFile->open( QIODevice::ReadOnly ) ) {
		qCritical() << "failed to open file: " << gridFile->fileName();
		return false;
	}

	return true;
}

bool GPSGridClient::UnloadData()
{
	if ( index != NULL )
		delete index;
	index = NULL;
	if ( gridFile != NULL )
		delete gridFile;
	gridFile = NULL;
	cache.clear();

	return true;
}

bool GPSGridClient::GetNearestEdge( Result* result, const UnsignedCoordinate& coordinate, double radius, bool headingPenalty, double heading )
{
	const GPSCoordinate gps = coordinate.ToProjectedCoordinate().ToGPSCoordinate();
	const GPSCoordinate gpsMoved( gps.latitude, gps.longitude + 1 );

	const double meter = gps.ApproximateDistance( gpsMoved );
	double gridRadius = (( double ) UnsignedCoordinate( ProjectedCoordinate( gpsMoved ) ).x - coordinate.x ) / meter * radius;
	gridRadius *= gridRadius;
	heading = fmod( ( heading + 270 ) * 2.0 * M_PI / 360.0, 2 * M_PI );

	static const int width = 32 * 32 * 32;

	ProjectedCoordinate position = coordinate.ToProjectedCoordinate();
	NodeID yGrid = floor( position.y * width );
	NodeID xGrid = floor( position.x * width );

	result->distance = gridRadius;
	QVector< UnsignedCoordinate > path;

	checkCell( result, &path, xGrid - 1, yGrid - 1, coordinate, heading, headingPenalty );
	checkCell( result, &path, xGrid - 1, yGrid, coordinate, heading, headingPenalty );
	checkCell( result, &path, xGrid - 1, yGrid + 1, coordinate, heading, headingPenalty );

	checkCell( result, &path, xGrid, yGrid - 1, coordinate, heading, headingPenalty );
	checkCell( result, &path, xGrid, yGrid, coordinate, heading, headingPenalty );
	checkCell( result, &path, xGrid, yGrid + 1, coordinate, heading, headingPenalty );

	checkCell( result, &path, xGrid + 1, yGrid - 1, coordinate, heading, headingPenalty );
	checkCell( result, &path, xGrid + 1, yGrid, coordinate, heading, headingPenalty );
	checkCell( result, &path, xGrid + 1, yGrid + 1, coordinate, heading, headingPenalty );

	if ( path.empty() )
		return false;

	double length = 0;
	double lengthToNearest = 0;
	for ( int pathID = 1; pathID < path.size(); pathID++ ) {
		UnsignedCoordinate sourceCoord = path[pathID - 1];
		UnsignedCoordinate targetCoord = path[pathID];
		double xDiff = ( double ) sourceCoord.x - targetCoord.x;
		double yDiff = ( double ) sourceCoord.y - targetCoord.y;

		double distance = sqrt( xDiff * xDiff + yDiff * yDiff );
		length += distance;
		if ( pathID < ( int ) result->previousWayCoordinates )
			lengthToNearest += distance;
		else if ( pathID == ( int ) result->previousWayCoordinates )
			lengthToNearest += result->percentage * distance;
	}
	if ( length == 0 )
		result->percentage = 0;
	else
		result->percentage = lengthToNearest / length;
	return true;
}

bool GPSGridClient::checkCell( Result* result, QVector< UnsignedCoordinate >* path, NodeID gridX, NodeID gridY, const UnsignedCoordinate& coordinate, double heading, double headingPenalty ) {
	static const int width = 32 * 32 * 32;
	ProjectedCoordinate minPos( ( double ) gridX / width, ( double ) gridY / width );
	ProjectedCoordinate maxPos( ( double ) ( gridX + 1 ) / width, ( double ) ( gridY + 1 ) / width );
	UnsignedCoordinate min( minPos );
	UnsignedCoordinate max( maxPos );
	if ( distance( min, max, coordinate ) >= result->distance )
		return false;

	qint64 cellNumber = ( qint64( gridX ) << 32 ) + gridY;
	if ( !cache.contains( cellNumber ) ) {
		qint64 position = index->GetIndex( gridX, gridY );
		if ( position == -1 )
			return true;
		gridFile->seek( position );
		int size;
		gridFile->read( (char* ) &size, sizeof( size ) );
		unsigned char* buffer = new unsigned char[size + 8]; // reading buffer + 4 bytes

		gridFile->read( ( char* ) buffer, size );
		gg::Cell* cell = new gg::Cell();
		cell->read( buffer, min, max );
		cache.insert( cellNumber, cell, cell->edges.size() * sizeof( gg::Cell::Edge ) );
		delete[] buffer;
	}
	gg::Cell* cell = cache.object( cellNumber );
	if ( cell == NULL )
		return true;

	UnsignedCoordinate nearestPoint;
	for ( std::vector< gg::Cell::Edge >::const_iterator i = cell->edges.begin(), e = cell->edges.end(); i != e; ++i ) {
		bool found = false;

		for ( int pathID = 1; pathID < i->pathLength; pathID++ ) {
			UnsignedCoordinate sourceCoord = cell->coordinates[pathID + i->pathID - 1];
			UnsignedCoordinate targetCoord = cell->coordinates[pathID + i->pathID];
			double percentage = 0;

			double d = distance( &nearestPoint, &percentage, sourceCoord, targetCoord, coordinate );
			if ( d + headingPenalty > result->distance )
				continue;

			double xDiff = ( double ) targetCoord.x - sourceCoord.x;
			double yDiff = ( double ) targetCoord.y - sourceCoord.y;
			double direction = 0;
			if ( xDiff != 0 || yDiff != 0 )
				direction = fmod( atan2( yDiff, xDiff ), 2 * M_PI );
			else
				headingPenalty = 0;
			double penalty = fabs( direction - heading );
			if ( penalty > M_PI )
				penalty = 2 * M_PI - penalty;
			if ( i->bidirectional && penalty > M_PI / 2 )
				penalty = M_PI - penalty;
			penalty = penalty / M_PI * headingPenalty;
			d += penalty;

			if ( d < result->distance ) {
				result->nearestPoint = nearestPoint;
				result->distance = d;
				result->previousWayCoordinates = pathID;
				result->percentage = percentage;
				found = true;
			}
		}

		if ( found ) {
			result->source = i->source;
			result->target = i->target;
			result->edgeID = i->edgeID;
			path->clear();
			for ( int pathID = 0; pathID < i->pathLength; pathID++ )
				path->push_back( cell->coordinates[pathID + i->pathID] );
		}
	}

	return true;
}

double GPSGridClient::distance( UnsignedCoordinate* nearestPoint, double* percentage, const UnsignedCoordinate source, const UnsignedCoordinate target, const UnsignedCoordinate& coordinate ) {
	const double vY = ( double ) target.y - source.y;
	const double vX = ( double ) target.x - source.x;
	const double wY = ( double ) coordinate.y - source.y;
	const double wX = ( double ) coordinate.x - source.x;
	const double vLengthSquared = vY * vY + vX * vX;

	double r = 0;
	if ( vLengthSquared != 0 )
		r = ( vX * wX + vY * wY ) / vLengthSquared;
	*percentage = r;

	if ( r <= 0 ) {
		*nearestPoint = source;
		*percentage = 0;
		return wY * wY + wX * wX;
	} else if ( r >= 1 ) {
		*nearestPoint = target;
		*percentage = 1;
		const double dY = ( double ) coordinate.y - target.y;
		const double dX = ( double ) coordinate.x - target.x;
		return dY * dY + dX * dX;
	}

	nearestPoint->x = source.x + r * vX;
	nearestPoint->y = source.y + r * vY;

	const double dX = ( double ) nearestPoint->x - coordinate.x;
	const double dY = ( double ) nearestPoint->y - coordinate.y;

	return dY * dY + dX * dX;
}

double GPSGridClient::distance( const UnsignedCoordinate& min, const UnsignedCoordinate& max, const UnsignedCoordinate& coordinate ) {
	UnsignedCoordinate nearest = coordinate;

	if ( coordinate.x <= min.x )
		nearest.x = min.x;
	else if ( coordinate.x >= max.x )
		nearest.x = max.x;

	if ( coordinate.y <= min.y )
		nearest.y = min.y;
	else if ( coordinate.y >= max.y )
		nearest.y = max.y;

	double xDiff = ( double ) coordinate.x - nearest.x;
	double yDiff = ( double ) coordinate.y - nearest.y;

	return xDiff * xDiff + yDiff * yDiff;
}

Q_EXPORT_PLUGIN2(gpsgridclient, GPSGridClient)
