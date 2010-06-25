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

#include "gpsgrid.h"
#include "utils/utils.h"
#include <QDir>
#include <QFile>
#include <algorithm>
#include <cmath>

GPSGrid::GPSGrid()
{
	settingsDialog = NULL;
}

GPSGrid::~GPSGrid()
{
	if ( settingsDialog != NULL )
		delete settingsDialog;
}

QString GPSGrid::GetName()
{
	return "GPS Grid";
}

GPSGrid::Type GPSGrid::GetType()
{
	return GPSLookup;
}

void GPSGrid::SetOutputDirectory( const QString& dir )
{
	outputDirectory = dir;
}

void GPSGrid::ShowSettings()
{
	if ( settingsDialog == NULL )
		settingsDialog = new GGDialog();
	settingsDialog->exec();
}

bool GPSGrid::Preprocess( IImporter* importer )
{
	if ( settingsDialog == NULL )
		settingsDialog = new GGDialog();
	GGDialog::Settings settings;
	if ( !settingsDialog->getSettings( &settings ) )
		return false;

	IImporter::BoundingBox box;
	if ( !importer->GetBoundingBox( & box ) )
		return false;

	double proportion = ( double ) ( box.max.x - box.min.x ) / ( box.max.y - box.min.y );
	double sqrtProportion = sqrt( proportion );
	double sqrtCells = sqrt( settings.cells );
	unsigned cellsX = sqrtProportion * sqrtCells;
	unsigned cellsY = settings.cells / cellsX;

	QDir dir( outputDirectory );
	QString filename = dir.filePath( "GPSGrid" );

	QFile indexFile( filename + "_index" );
	QFile gridFile( filename + "_grid" );
	QFile configFile( filename + "_config" );

	indexFile.open( QIODevice::WriteOnly );
	gridFile.open( QIODevice::WriteOnly );
	configFile.open( QIODevice::WriteOnly );

	ProjectedCoordinate minCoord( std::numeric_limits< unsigned >::max(), std::numeric_limits< unsigned >::max() );
	ProjectedCoordinate maxCoord( 0, 0 );

	std::vector< IImporter::RoutingNode > inputNodes;
	std::vector< IImporter::RoutingEdge > inputEdges;

	if ( !importer->GetRoutingNodes( &inputNodes ) )
		return false;
	if ( !importer->GetRoutingEdges( &inputEdges ) )
		return false;

	for ( std::vector< IImporter::RoutingNode >::const_iterator i = inputNodes.begin(); i != inputNodes.end(); ++i ) {
		const UnsignedCoordinate& coordinate = i->coordinate;
		if ( coordinate.x < minCoord.x )
			minCoord.x = coordinate.x;
		if ( coordinate.y < minCoord.y )
			minCoord.y = coordinate.y;
		if ( coordinate.x > maxCoord.x )
			maxCoord.x = coordinate.x;
		if ( coordinate.y > maxCoord.y )
			maxCoord.y = coordinate.y;
	}

	double yWidth = ( maxCoord.y - minCoord.y ) + 2;
	double xWidth = ( maxCoord.x - minCoord.x ) + 2;
	minCoord.y--;
	minCoord.x--;

	std::vector< GridImportEdge > grid;

	qDebug( "Distributing Edges" );
	for ( std::vector< IImporter::RoutingEdge >::const_iterator i = inputEdges.begin(); i != inputEdges.end(); ++i ) {
		if ( i->source == i->target )
			continue;
		gg::Cell::Edge edgeData;
		edgeData.source = i->source;
		edgeData.target = i->target;
		edgeData.bidirectional = i->bidirectional;
		edgeData.sourceCoord = inputNodes[i->source].coordinate;
		edgeData.targetCoord = inputNodes[i->target].coordinate;

		NodeID minYGrid = floor((( double ) edgeData.sourceCoord.y - minCoord.y ) * cellsY / yWidth );
		NodeID minXGrid = floor((( double ) edgeData.sourceCoord.x - minCoord.x ) * cellsX / xWidth );
		NodeID maxYGrid = floor((( double ) edgeData.targetCoord.y - minCoord.y ) * cellsY / yWidth );
		NodeID maxXGrid = floor((( double ) edgeData.targetCoord.x - minCoord.x ) * cellsX / xWidth );

		if ( minYGrid > maxYGrid )
			std::swap( minYGrid, maxYGrid );
		if ( minXGrid > maxXGrid )
			std::swap( minXGrid, maxXGrid );

		for ( NodeID yGrid = minYGrid; yGrid <= maxYGrid; ++yGrid ) {
			for ( NodeID xGrid = minXGrid; xGrid <= maxXGrid; ++xGrid ) {
				GridImportEdge clippedEdge;
				clippedEdge.gridNumber = yGrid * cellsX + xGrid;
				assert( yGrid * cellsX + xGrid < cellsX * cellsY );
				clippedEdge.edge = edgeData;

				UnsignedCoordinate min;
				UnsignedCoordinate max;
				min.x = xGrid * xWidth / cellsX + minCoord.x;
				min.y = yGrid * yWidth / cellsY + minCoord.y;
				max.x = ( xGrid + 1 ) * xWidth / cellsX + minCoord.x;
				max.y = ( yGrid + 1 ) * yWidth / cellsY + minCoord.y;

				if ( !clipEdge( &clippedEdge.edge, min, max ) )
					continue;

				grid.push_back( clippedEdge );
			}
		}

	}
	qDebug( "Sorting" );
	std::sort( grid.begin(), grid.end() );

	qDebug( "Overhead: %d Duplicated Edges", ( int ) ( grid.size() - inputEdges.size() ) );
	qDebug( "Overhead: %d Percent Duplicated Edges", ( int ) ( ( grid.size() - inputEdges.size() ) * 100 / inputEdges.size() ) );
	qDebug( "Overhead: %dMB Index Size", ( int ) ( ( cellsX * cellsY + 1 ) * sizeof( NodeID ) / 1024 / 1024 ) );

	qDebug( "Writing File" );

	configFile.write( ( const char* ) &cellsX, sizeof( cellsX ) );
	configFile.write( ( const char* ) &cellsY, sizeof( cellsY ) );
	unsigned temp = minCoord.x;
	configFile.write( ( const char* ) &temp, sizeof( temp ) );
	temp = minCoord.y;
	configFile.write( ( const char* ) &temp, sizeof( temp ) );
	configFile.write( ( const char* ) &xWidth, sizeof( xWidth ) );
	configFile.write( ( const char* ) &yWidth, sizeof( yWidth ) );

	std::vector< unsigned > index;
	unsigned position = 0;
	NodeID emptyGridCells = 0;
	for ( std::vector< GridImportEdge >::iterator edge = grid.begin(); edge != grid.end(); ++edge ) {
		gg::Cell cell;
		NodeID gridNum = edge->gridNumber;
		cell.edges.push_back( edge->edge );
		while ( edge + 1 != grid.end() && ( edge + 1 )->gridNumber == gridNum ) {
			edge++;
			cell.edges.push_back( edge->edge );
		}

		UnsignedCoordinate min;
		UnsignedCoordinate max;
		min.x = ( gridNum % cellsX ) * xWidth / cellsX + minCoord.x;
		min.y = ( gridNum / cellsX ) * yWidth / cellsY + minCoord.y;
		max.x = (( gridNum % cellsX ) + 1 ) * xWidth / cellsX + minCoord.x;
		max.y = ( gridNum / cellsX + 1 ) * yWidth / cellsY + minCoord.y;

		char* buffer = new char[cell.edges.size() * sizeof( gg::Cell::Edge ) * 2 + 100];
		memset( buffer, 0, cell.edges.size() * sizeof( gg::Cell::Edge ) * 2 + 100 );
		unsigned size = cell.write( buffer, min, max );
		assert( size < cell.edges.size() * sizeof( gg::Cell::Edge ) * 2 + 100 );
#ifndef NDEBUG
		gg::Cell unpackCell;
		unpackCell.read( buffer, min, max );
		assert( unpackCell == cell );
#endif
		gridFile.write( buffer, size );
		delete[] buffer;
		emptyGridCells += gridNum - index.size();
		while ( index.size() <= gridNum ) {
			index.push_back( position );
		}
		position += size;
	}
	temp = 0;
	gridFile.write( ( const char* ) &temp, sizeof( temp ) );
	gridFile.write( ( const char* ) &temp, sizeof( temp ) );
	gridFile.write( ( const char* ) &temp, sizeof( temp ) );
	gridFile.write( ( const char* ) &temp, sizeof( temp ) );
	while ( index.size() <= cellsX * cellsY ) {
		index.push_back( position );
	}

	indexFile.write( ( const char* ) index.data(), index.size() * sizeof( unsigned ) );

	qDebug( "%d Grid Cells Empty", emptyGridCells );
	qDebug( "%d Percent Grid Cells Empty", emptyGridCells * 100 / ( cellsX * cellsY ) );

	return true;
	return false;
}

bool GPSGrid::clipHelper( double directedProjection, double directedDistance, double* tMinimum, double* tMaximum ) {
	if ( directedProjection == 0 ) {
		if ( directedDistance < 0 )
			return false;
	} else {
		double amount = directedDistance / directedProjection;
		if ( directedProjection < 0 ) {
			if ( amount > *tMaximum )
				return false;
			else if ( amount > *tMinimum )
				*tMinimum = amount;
		} else {
			if ( amount < *tMinimum )
				return false;
			else if ( amount < *tMaximum )
				*tMaximum = amount;
		}
	}
	return true;
}

bool GPSGrid::clipEdge( gg::Cell::Edge* edge, UnsignedCoordinate min, UnsignedCoordinate max ) {
	const double xDiff = ( double ) edge->targetCoord.x - edge->sourceCoord.x;
	const double yDiff = ( double ) edge->targetCoord.y - edge->sourceCoord.y;
	double tMinimum = 0, tMaximum = 1;

	if ( clipHelper( -xDiff, ( double ) edge->sourceCoord.x - min.x, &tMinimum, &tMaximum ) )
		if ( clipHelper( xDiff, ( double ) max.x - edge->sourceCoord.x, &tMinimum, &tMaximum ) )
			if ( clipHelper( -yDiff, ( double ) edge->sourceCoord.y - min.y, &tMinimum, &tMaximum ) )
				if ( clipHelper( yDiff, ( double ) max.y - edge->sourceCoord.y, &tMinimum, &tMaximum ) )
					return true;
	return false;

}

Q_EXPORT_PLUGIN2(gpsgrid, GPSGrid)
