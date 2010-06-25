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

GPSGridClient::GPSGridClient()
{
}

GPSGridClient::~GPSGridClient()
{
	unload();
}

void GPSGridClient::unload()
{
	if ( indexFile != NULL )
		delete indexFile;
	indexFile = NULL;
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

	configFile.read( ( char* ) &cellsX, sizeof( cellsX ) );
	configFile.read( ( char* ) &cellsY, sizeof( cellsY ) );
	configFile.read( ( char* ) &minX, sizeof( minX ) );
	configFile.read( ( char* ) &minY, sizeof( minY ) );
	configFile.read( ( char* ) &xWidth, sizeof( xWidth ) );
	configFile.read( ( char* ) &yWidth, sizeof( yWidth ) );


	indexFile = new QFile( filename + "_index" );
	if ( !indexFile->open( QIODevice::ReadOnly ) )
	{
		qCritical() << "failed to open file: " << indexFile->fileName();
		return false;
	}
	indexData = ( unsigned* ) indexFile->map( 0, indexFile->size() );
	if ( indexData == NULL )
	{
		qCritical() << "failed to memory map: " << indexFile->fileName();
		return false;
	}

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
	double gridHeadingPenalty = (( double ) UnsignedCoordinate( ProjectedCoordinate( gpsMoved ) ).x - coordinate.x ) / meter * headingPenalty;
	gridHeadingPenalty *= gridHeadingPenalty;
	heading = fmod( heading, 2 * M_PI );

	NodeID yGrid = floor( ( ( double ) coordinate.y - minY ) * cellsY / yWidth );
	NodeID xGrid = floor( ( ( double ) coordinate.x - minX ) * cellsX / xWidth );
	if ( coordinate.y < minY )
		yGrid = 0;
	else if ( coordinate.y > minY + yWidth )
		yGrid = cellsY - 1;
	if ( coordinate.x < minX )
		xGrid = 0;
	else if ( coordinate.x > minX + xWidth )
		xGrid = cellsX - 1;

	checkCell( result, gridRadius, xGrid, yGrid, coordinate, heading, gridHeadingPenalty );
	if ( xGrid > 0 ) {
		checkCell( result, gridRadius, xGrid - 1, yGrid, coordinate, heading, gridHeadingPenalty );
		if ( yGrid > 0 )
			checkCell( result, gridRadius, xGrid - 1, yGrid - 1, coordinate, heading, gridHeadingPenalty );
		if ( yGrid < cellsY - 1 )
			checkCell( result, gridRadius, xGrid - 1, yGrid + 1, coordinate, heading, gridHeadingPenalty );
	}
	if ( xGrid < cellsX - 1 ) {
		checkCell( result, gridRadius, xGrid + 1, yGrid, coordinate, heading, gridHeadingPenalty );
		if ( yGrid > 0 )
			checkCell( result, gridRadius, xGrid + 1, yGrid - 1, coordinate, heading, gridHeadingPenalty );
		if ( yGrid < cellsY - 1 )
			checkCell( result, gridRadius, xGrid + 1, yGrid + 1, coordinate, heading, gridHeadingPenalty );
	}
	if ( yGrid > 0 )
		checkCell( result, gridRadius, xGrid, yGrid - 1, coordinate, heading, gridHeadingPenalty );
	if ( yGrid < cellsY - 1 )
		checkCell( result, gridRadius, xGrid, yGrid + 1, coordinate, heading, gridHeadingPenalty );

	std::sort( result->begin(), result->end() );
	return result->size() != 0;
}

bool GPSGridClient::checkCell( QVector< Result >* result, double radius, NodeID gridX, NodeID gridY, const UnsignedCoordinate& coordinate, double heading, double headingPenalty ) {
	UnsignedCoordinate min;
	UnsignedCoordinate max;
	UnsignedCoordinate nearestPoint;
	min.x = gridX * xWidth / cellsX + minX;
	min.y = gridY * yWidth / cellsY + minY;
	max.x = min.x + xWidth / cellsX;
	max.y = min.y + yWidth / cellsY;

	if ( distance( min, max, coordinate ) >= radius )
		return false;

	const unsigned cellNumber = gridY * cellsX + gridX;

	if ( !cache.contains( cellNumber ) ) {
		const size_t size = indexData[cellNumber + 1] - indexData[cellNumber];
		if ( size == 0 )
			return true;
		char* buffer = new char[size];
		gridFile->seek( indexData[cellNumber] );
		gridFile->read( buffer, size );
		gg::Cell* cell = new gg::Cell();
		cell->read( buffer, min, max );
		cache.insert( cellNumber, cell, 1 );
		delete[] buffer;
	}
	gg::Cell* cell = cache.object( cellNumber );
	if ( cell == NULL )
		return true;

	for ( std::vector< gg::Cell::Edge >::const_iterator i = cell->edges.begin(), e = cell->edges.end(); i != e; ++i ) {
		double percentage = 0;

		double d = distance( &nearestPoint, &percentage, *i, coordinate );
		double xDiff = ( double ) i->targetCoord.x - i->sourceCoord.x;
		double yDiff = ( double ) i->targetCoord.y - i->sourceCoord.y;
		double direction = 0;
		if ( xDiff != 0 || yDiff != 0 )
			direction = fmod( atan2( yDiff, xDiff ) + M_PI / 2, 2 * M_PI );
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
			resultEdge.distance = resultEdge.nearestPoint.ToProjectedCoordinate().ToGPSCoordinate().ApproximateDistance( coordinate.ToProjectedCoordinate().ToGPSCoordinate() ) + penalty;
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
