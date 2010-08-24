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

#include "contractionhierarchies.h"
#include "compressedgraphbuilder.h"
#include "contractor.h"
#include "contractioncleanup.h"
#include <cassert>
#include <limits>
#include <stack>
#include <omp.h>
#include <QDir>
#include <QTextStream>

ContractionHierarchies::ContractionHierarchies()
{
	settingsDialog = NULL;
}

ContractionHierarchies::~ContractionHierarchies()
{
}

QString ContractionHierarchies::GetName()
{
	return "Contraction Hierarchies";
}

ContractionHierarchies::Type ContractionHierarchies::GetType()
{
	return Router;
}

void ContractionHierarchies::SetOutputDirectory( const QString& dir )
{
	outputDirectory = dir;
}

void ContractionHierarchies::ShowSettings()
{
	if ( settingsDialog == NULL )
		settingsDialog = new CHSettingsDialog();
	settingsDialog->exec();
}

bool ContractionHierarchies::Preprocess( IImporter* importer )
{
	if ( settingsDialog == NULL )
		settingsDialog = new CHSettingsDialog();
	settingsDialog->getSettings( &settings );

	QDir directory( outputDirectory );
	QString filename = directory.filePath( "Contraction Hierarchies" );

	if ( settings.threads == 0 )
		omp_set_num_threads( omp_get_max_threads() );
	else
		omp_set_num_threads( settings.threads );

	std::vector< IImporter::RoutingNode > inputNodes;
	std::vector< IImporter::RoutingEdge > inputEdges;

	if ( !importer->GetRoutingNodes( &inputNodes ) )
		return false;
	if ( !importer->GetRoutingEdges( &inputEdges ) )
		return false;

	/*QFile textFile( filename + ".dtmp" );
	textFile.open( QIODevice::WriteOnly );
	QTextStream textOut( &textFile );
	textOut << inputNodes.size() << " " << inputEdges.size() << "\n";
	for ( int i = 0; i < ( int ) inputNodes.size(); i++ )
		textOut << i << " " << inputNodes[i].coordinate.x << " " << inputNodes[i].coordinate.y << "\n";
	for ( int i = 0; i < ( int ) inputEdges.size(); i++ )
		textOut << inputEdges[i].source << " " << inputEdges[i].target << "\n";
	return false;
	*/

	Contractor* contractor = new Contractor( inputNodes.size(), inputEdges );
	std::vector< IImporter::RoutingEdge >().swap( inputEdges );
	contractor->Run();

	std::vector< Contractor::Witness > witnessList;
	contractor->GetWitnessList( witnessList );

	std::vector< ContractionCleanup::Edge > contractedEdges;
	contractor->GetEdges( contractedEdges );
	delete contractor;

	ContractionCleanup* cleanup = new ContractionCleanup( inputNodes.size(), contractedEdges, witnessList );
	std::vector< ContractionCleanup::Edge >().swap( contractedEdges );
	std::vector< Contractor::Witness >().swap( witnessList );
	cleanup->Run();

	std::vector< CompressedGraph::Edge > edges;
	std::vector< NodeID > map;
	cleanup->GetData( edges, map );
	delete cleanup;

	std::vector< CompressedGraph::Node > nodes( inputNodes.size() );
	for ( std::vector< IImporter::RoutingNode >::const_iterator i = inputNodes.begin(), iend = inputNodes.end(); i != iend; i++ )
		nodes[map[i - inputNodes.begin()]].coordinate = i->coordinate;
	std::vector< IImporter::RoutingNode >().swap( inputNodes );

	//block_graph::build( edges, nodes, &map, filename, 1u << settings.blockSize );
	CompressedGraphBuilder builder( 1u << settings.blockSize, nodes, edges );
	if ( !builder.run( filename, &map ) )
		return false;

	importer->SetIDMap( map );

	return true;
}

Q_EXPORT_PLUGIN2( contractionhierarchies, ContractionHierarchies )
