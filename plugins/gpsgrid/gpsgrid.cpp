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
#include "utils/intersection.h"
#include "utils/qthelpers.h"
#include <QFile>
#include <algorithm>

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

	QString filename = fileInDirectory( outputDirectory, "GPSGrid" );

	QFile gridFile( filename + "_grid" );
	QFile configFile( filename + "_config" );

	if ( !openQFile( &gridFile, QIODevice::WriteOnly ) )
		return false;
	if ( !openQFile( &configFile, QIODevice::WriteOnly ) )
		return false;

	std::vector< IImporter::RoutingNode > inputNodes;
	std::vector< IImporter::RoutingEdge > inputEdges;
	std::vector< unsigned > idmap;

	if ( !importer->GetRoutingNodes( &inputNodes ) )
		return false;
	if ( !importer->GetRoutingEdges( &inputEdges ) )
		return false;
	if ( !importer->GetIDMap( &idmap ) )
		return false;

	static const int width = 32 * 32 * 32;

	std::vector< GridImportEdge > grid;

	Timer time;
	for ( std::vector< IImporter::RoutingEdge >::const_iterator i = inputEdges.begin(); i != inputEdges.end(); ++i ) {
		if ( i->source == i->target )
			continue;

		ProjectedCoordinate sourceCoordinate = inputNodes[i->source].coordinate.ToProjectedCoordinate();
		ProjectedCoordinate targetCoordinate = inputNodes[i->target].coordinate.ToProjectedCoordinate();
		sourceCoordinate.x *= width;
		sourceCoordinate.y *= width;
		targetCoordinate.x *= width;
		targetCoordinate.y *= width;

		NodeID minYGrid = floor( sourceCoordinate.y );
		NodeID minXGrid = floor( sourceCoordinate.x );
		NodeID maxYGrid = floor( targetCoordinate.y );
		NodeID maxXGrid = floor( targetCoordinate.x );

		if ( minYGrid > maxYGrid )
			std::swap( minYGrid, maxYGrid );
		if ( minXGrid > maxXGrid )
			std::swap( minXGrid, maxXGrid );

		for ( NodeID yGrid = minYGrid; yGrid <= maxYGrid; ++yGrid ) {
			for ( NodeID xGrid = minXGrid; xGrid <= maxXGrid; ++xGrid ) {
				if ( !clipEdge( sourceCoordinate, targetCoordinate, ProjectedCoordinate( xGrid, yGrid ), ProjectedCoordinate( xGrid + 1, yGrid + 1 ) ) )
					continue;

				GridImportEdge clippedEdge;
				clippedEdge.x = xGrid;
				clippedEdge.y = yGrid;
				clippedEdge.source = i->source;
				clippedEdge.target = i->target;
				clippedEdge.bidirectional = i->bidirectional;

				grid.push_back( clippedEdge );
			}
		}
	}
	qDebug() << "GPS Grid: distributed edges:" << time.restart() << "s";
	qDebug() << "GPS Grid: overhead:" << grid.size() - inputEdges.size() << "duplicated edges";
	qDebug() << "GPS Grid: overhead:" << ( grid.size() - inputEdges.size() ) * 100 / inputEdges.size() << "% duplicated edges";;
	std::vector< IImporter::RoutingEdge >().swap( inputEdges );

	std::sort( grid.begin(), grid.end() );
	qDebug() << "GPS Grid: sorted edges:" << time.restart() << "s";

	std::vector< gg::GridIndex > tempIndex;
	qint64 position = 0;
	for ( std::vector< GridImportEdge >::const_iterator edge = grid.begin(); edge != grid.end(); ) {
		gg::Cell cell;
		gg::GridIndex entry;
		entry.x = edge->x;
		entry.y = edge->y;
		entry.position = position;

		tempIndex.push_back( entry );
		do {
			gg::Cell::Edge newEdge;
			newEdge.source = idmap[edge->source];
			newEdge.target = idmap[edge->target];
			newEdge.bidirectional = edge->bidirectional;
			newEdge.sourceCoord = inputNodes[edge->source].coordinate;
			newEdge.targetCoord = inputNodes[edge->target].coordinate;
			cell.edges.push_back( newEdge );
			edge++;
		} while ( edge != grid.end() && edge->x == entry.x && edge->y == entry.y );

		ProjectedCoordinate min( ( double ) entry.x / width, ( double ) entry.y / width );
		ProjectedCoordinate max( ( double ) ( entry.x + 1 ) / width, ( double ) ( entry.y + 1 ) / width );

		unsigned char* buffer = new unsigned char[cell.edges.size() * sizeof( gg::Cell::Edge ) * 2 + 100];
		memset( buffer, 0, cell.edges.size() * sizeof( gg::Cell::Edge ) * 2 + 100 );
		int size = cell.write( buffer, UnsignedCoordinate( min ), UnsignedCoordinate( max ) );
		assert( size < ( int ) ( cell.edges.size() * sizeof( gg::Cell::Edge ) * 2 + 100 ) );

#ifndef NDEBUG
		gg::Cell unpackCell;
		unpackCell.read( buffer, UnsignedCoordinate( min ), UnsignedCoordinate( max ) );
		assert( unpackCell == cell );
#endif

		gridFile.write( ( const char* ) &size, sizeof( size ) );
		gridFile.write( ( const char* ) buffer, size );

		delete[] buffer;
		position += size + sizeof( size );
	}
	qDebug() << "GPS Grid: wrote cells:" << time.restart() << "s";

	gg::Index::Create( filename + "_index", tempIndex );
	qDebug() << "GPS Grid: created index:" << time.restart() << "s";

	return true;
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

bool GPSGrid::clipEdge( ProjectedCoordinate source, ProjectedCoordinate target, ProjectedCoordinate min, ProjectedCoordinate max ) {
	const double xDiff = target.x - source.x;
	const double yDiff = target.y - source.y;
	double tMinimum = 0, tMaximum = 1;

	if ( clipHelper( -xDiff, source.x - min.x, &tMinimum, &tMaximum ) )
		if ( clipHelper( xDiff, max.x - source.x, &tMinimum, &tMaximum ) )
			if ( clipHelper( -yDiff, source.y - min.y, &tMinimum, &tMaximum ) )
				if ( clipHelper( yDiff, max.y - source.y, &tMinimum, &tMaximum ) )
					return true;
	return false;

}

Q_EXPORT_PLUGIN2(gpsgrid, GPSGrid)
