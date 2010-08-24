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
#include <QDir>
#include <QtDebug>
#include <QHash>
#include <algorithm>
#include <QInputDialog>
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
	unload();
}

void GPSGridClient::unload()
{
	if ( index != NULL )
		delete index;
	index = NULL;
	if ( gridFile != NULL )
		delete gridFile;
	gridFile = NULL;
	cache.clear();
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
	bool ok = false;
	int result = QInputDialog::getInt( NULL, "Settings", "Enter Cache Size [MB]", cacheSize, 1, 1024, 1, &ok );
	if ( !ok )
		return;
	cacheSize = result;
	if ( index != NULL )
		index->SetCacheSize( 1024 * 1024 * cacheSize / 4 );
	cache.setMaxCost( 1024 * 1024 * 3 * cacheSize / 4 );
}

bool GPSGridClient::LoadData()
{
	unload();
	QDir dir( directory );
	QString filename = dir.filePath( "GPSGrid" );
	QFile configFile( filename + "_config" );
	if ( !configFile.open( QIODevice::ReadOnly ) )
	{
		qCritical() << "failed top open file: " << configFile.fileName();
		return false;
	}

	index = new gg::Index( filename + "_index" );
	index->SetCacheSize( 1024 * 1024 * cacheSize / 4 );

	gridFile = new QFile( filename + "_grid" );
	if ( !gridFile->open( QIODevice::ReadOnly ) )
	{
		qCritical() << "failed to open file: " << gridFile->fileName();
		return false;
	}

	return true;
}

bool GPSGridClient::GetNearEdges( QVector< Result >* result, const UnsignedCoordinate& coordinate, double radius, bool headingPenalty, double heading )
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

	checkCell( result, gridRadius, xGrid - 1, yGrid - 1, coordinate, heading, headingPenalty );
	checkCell( result, gridRadius, xGrid - 1, yGrid, coordinate, heading, headingPenalty );
	checkCell( result, gridRadius, xGrid - 1, yGrid + 1, coordinate, heading, headingPenalty );

	checkCell( result, gridRadius, xGrid, yGrid - 1, coordinate, heading, headingPenalty );
	checkCell( result, gridRadius, xGrid, yGrid, coordinate, heading, headingPenalty );
	checkCell( result, gridRadius, xGrid, yGrid + 1, coordinate, heading, headingPenalty );

	checkCell( result, gridRadius, xGrid + 1, yGrid - 1, coordinate, heading, headingPenalty );
	checkCell( result, gridRadius, xGrid + 1, yGrid, coordinate, heading, headingPenalty );
	checkCell( result, gridRadius, xGrid + 1, yGrid + 1, coordinate, heading, headingPenalty );

	std::sort( result->begin(), result->end() );
	return result->size() != 0;
}

bool GPSGridClient::checkCell( QVector< Result >* result, double radius, NodeID gridX, NodeID gridY, const UnsignedCoordinate& coordinate, double heading, double headingPenalty ) {
	static const int width = 32 * 32 * 32;
	ProjectedCoordinate minPos( ( double ) gridX / width, ( double ) gridY / width );
	ProjectedCoordinate maxPos( ( double ) ( gridX + 1 ) / width, ( double ) ( gridY + 1 ) / width );
	UnsignedCoordinate min( minPos );
	UnsignedCoordinate max( maxPos );
	UnsignedCoordinate nearestPoint;
	//qDebug() << "Try Cell (" << gridX << ", " << gridY << ")";
	if ( distance( min, max, coordinate ) >= radius )
		return false;

	qint64 cellNumber = ( qint64( gridX ) << 32 ) + gridY;
	//qDebug() << "Cell number: " << cellNumber;
	if ( !cache.contains( cellNumber ) ) {
		qint64 position = index->GetIndex( gridX, gridY );
		//qDebug() << "Load position: " << position;
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

	//qDebug() << "Checking cell, " << cell->edges.size() << " edges";

	for ( std::vector< gg::Cell::Edge >::const_iterator i = cell->edges.begin(), e = cell->edges.end(); i != e; ++i ) {
		double percentage = 0;

		double d = distance( &nearestPoint, &percentage, *i, coordinate );
		double xDiff = ( double ) i->targetCoord.x - i->sourceCoord.x;
		double yDiff = ( double ) i->targetCoord.y - i->sourceCoord.y;
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
		if ( d < radius ) {
			Result resultEdge;
			resultEdge.source = i->source;
			resultEdge.target = i->target;
			resultEdge.nearestPoint = nearestPoint;
			resultEdge.percentage = percentage;
			//resultEdge.distance = resultEdge.nearestPoint.ToProjectedCoordinate().ToGPSCoordinate().ApproximateDistance( coordinate.ToProjectedCoordinate().ToGPSCoordinate() ) + penalty;
			resultEdge.distance = d;
			resultEdge.bidirectional = i->bidirectional;
			result->push_back( resultEdge );
		}
	}

	return true;
}

double GPSGridClient::distance( UnsignedCoordinate* nearestPoint, double* percentage, const gg::Cell::Edge& edge, const UnsignedCoordinate& coordinate ) {
	const double vY = ( double ) edge.targetCoord.y - edge.sourceCoord.y;
	const double vX = ( double ) edge.targetCoord.x - edge.sourceCoord.x;
	const double wY = ( double ) coordinate.y - edge.sourceCoord.y;
	const double wX = ( double ) coordinate.x - edge.sourceCoord.x;
	const double vLengthSquared = vY * vY + vX * vX;

	double r = 0;
	if ( vLengthSquared != 0 )
		r = ( vX * wX + vY * wY ) / vLengthSquared;
	*percentage = r;

	if ( r <= 0 ) {
		*nearestPoint = edge.sourceCoord;
		*percentage = 0;
		return wY * wY + wX * wX;
	} else if ( r >= 1 ) {
		*nearestPoint = edge.targetCoord;
		*percentage = 1;
		const double dY = ( double ) coordinate.y - edge.targetCoord.y;
		const double dX = ( double ) coordinate.x - edge.targetCoord.x;
		return dY * dY + dX * dX;
	}

	nearestPoint->x = edge.sourceCoord.x + r * vX;
	nearestPoint->y = edge.sourceCoord.y + r * vY;

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
