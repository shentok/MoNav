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
#include "utils/qthelpers.h"

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

	QString filename = fileInDirectory( outputDirectory, "Contraction Hierarchies" );

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

	unsigned numEdges = inputEdges.size();
	unsigned numNodes = inputNodes.size();

	Contractor* contractor = new Contractor( numNodes, inputEdges );
	std::vector< IImporter::RoutingEdge >().swap( inputEdges );
	contractor->Run();

	std::vector< Contractor::Witness > witnessList;
	contractor->GetWitnessList( witnessList );

	std::vector< ContractionCleanup::Edge > contractedEdges;
	std::vector< ContractionCleanup::Edge > contractedLoops;
	contractor->GetEdges( &contractedEdges );
	contractor->GetLoops( &contractedLoops );
	delete contractor;

	ContractionCleanup* cleanup = new ContractionCleanup( inputNodes.size(), contractedEdges, contractedLoops, witnessList );
	std::vector< ContractionCleanup::Edge >().swap( contractedEdges );
	std::vector< ContractionCleanup::Edge >().swap( contractedLoops );
	std::vector< Contractor::Witness >().swap( witnessList );
	cleanup->Run();

	std::vector< CompressedGraph::Edge > edges;
	std::vector< NodeID > map;
	cleanup->GetData( &edges, &map );
	delete cleanup;

	{
		std::vector< unsigned > edgeIDs( numEdges );
		for ( unsigned edge = 0; edge < edges.size(); edge++ ) {
			if ( edges[edge].data.shortcut )
				continue;
			unsigned id = 0;
			unsigned otherEdge = edge;
			while ( true ) {
				if ( otherEdge == 0 )
					break;
				otherEdge--;
				if ( edges[otherEdge].source != edges[edge].source )
					break;
				if ( edges[otherEdge].target != edges[edge].target )
					continue;
				if ( edges[otherEdge].data.shortcut )
					continue;
				id++;
			}
			edgeIDs[edges[edge].data.id] = id;
		}
		importer->SetEdgeIDMap( edgeIDs );
	}

	std::vector< CompressedGraph::Node > nodes( numNodes );
	for ( std::vector< IImporter::RoutingNode >::const_iterator i = inputNodes.begin(), iend = inputNodes.end(); i != iend; i++ )
		nodes[map[i - inputNodes.begin()]].coordinate = i->coordinate;
	std::vector< IImporter::RoutingNode >().swap( inputNodes );

	std::vector< CompressedGraph::Node > pathNodes;
	{
		std::vector< IImporter::RoutingNode > edgePaths;
		if ( !importer->GetRoutingEdgePaths( &edgePaths ) )
			return false;
		pathNodes.resize( edgePaths.size() );
		for ( unsigned i = 0; i < edgePaths.size(); i++ )
			pathNodes[i].coordinate = edgePaths[i].coordinate;
	}

	if ( !importer->GetRoutingEdges( &inputEdges ) )
		return false;

	for ( std::vector< IImporter::RoutingEdge >::iterator i = inputEdges.begin(), iend = inputEdges.end(); i != iend; i++ ) {
		i->source = map[i->source];
		i->target = map[i->target];
	}

	CompressedGraphBuilder* builder = new CompressedGraphBuilder( 1u << settings.blockSize, nodes, edges, inputEdges, pathNodes );
	if ( !builder->run( filename, &map ) )
		return false;
	delete builder;

	importer->SetIDMap( map );

	return true;
}

Q_EXPORT_PLUGIN2( contractionhierarchies, ContractionHierarchies )
