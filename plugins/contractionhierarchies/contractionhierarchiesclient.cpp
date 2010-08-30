/*
Copyright 2010  Christian Vetter veaac.fdirct@gmail.com

This file is part of MoNav.

MoNav is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MoNav is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY
{
}
 without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MoNav.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "contractionhierarchiesclient.h"
#include <QDir>
#include <QtDebug>
#include <stack>
#ifndef NOGUI
	#include <QMessageBox>
#endif

ContractionHierarchiesClient::ContractionHierarchiesClient()
{
	heapForward = NULL;
	heapBackward = NULL;
}

ContractionHierarchiesClient::~ContractionHierarchiesClient()
{
	unload();
}


QString ContractionHierarchiesClient::GetName()
{
	return "Contraction Hierarchies";
}

void ContractionHierarchiesClient::SetInputDirectory( const QString& dir )
{
	directory = dir;
}

void ContractionHierarchiesClient::ShowSettings()
{
#ifndef NOGUI
	QMessageBox::information( NULL, "Settings", "No settings available" );
#endif
}

void ContractionHierarchiesClient::unload()
{
	if ( heapForward != NULL )
		delete heapForward;
	heapForward = NULL;
	if ( heapBackward != NULL )
		delete heapBackward;
	heapBackward = NULL;
}

bool ContractionHierarchiesClient::LoadData()
{
	unload();
	QDir dir( directory );
	QString filename = dir.filePath( "Contraction Hierarchies" );
	if ( !graph.loadGraph( filename, 1024 * 1024 * 4 ) )
		return false;
	heapForward = new _Heap( graph.numberOfNodes() );
	heapBackward = new _Heap( graph.numberOfNodes() );
	return true;
}

bool ContractionHierarchiesClient::GetRoute( double* distance, QVector< UnsignedCoordinate>* path, const IGPSLookup::Result& source, const IGPSLookup::Result& target )
{
	heapForward->Clear();
	heapBackward->Clear();
	if ( target.source == source.source && target.target == source.target && ( source.bidirectional || source.percentage < target.percentage ) ) {
		path->push_back( source.nearestPoint );
		path->push_back( target.nearestPoint );
		if ( source.source != source.target ) {
			EdgeIterator targetEdge = graph.findEdge( source.source, source.target );
			const unsigned targetWeight = targetEdge.distance();
			*distance = fabs( target.percentage - source.percentage ) * targetWeight;
		}
		else
			*distance = 0;
		return true;
	}
	*distance = computeRoute( source, target, path );
	if ( *distance == std::numeric_limits< unsigned >::max() )
		return false;
	*distance /= 10;
	return true;
}

template< class EdgeAllowed, class StallEdgeAllowed >
void ContractionHierarchiesClient::computeStep( _Heap* heapForward, _Heap* heapBackward, const EdgeAllowed& edgeAllowed, const StallEdgeAllowed& stallEdgeAllowed, Node* middle, int* targetDistance ) {

	const Node node = heapForward->DeleteMin();
	const int distance = heapForward->GetKey( node );

	if ( heapForward->GetData( node ).stalled )
		return;

	if ( heapBackward->WasInserted( node ) && !heapBackward->GetData( node ).stalled ) {
		const int newDistance = heapBackward->GetKey( node ) + distance;
		if ( newDistance < *targetDistance ) {
			*middle = node;
			*targetDistance = newDistance;
		}
	}

	if ( distance > *targetDistance ) {
		heapForward->DeleteAll();
		return;
	}
	for ( EdgeIterator edge = graph.edges( node ); edge.hasEdgesLeft(); ) {
		graph.unpackNextEdge( &edge );
		const Node to = edge.target();
		const int edgeWeight = edge.distance();
		assert( edgeWeight > 0 );
		const int toDistance = distance + edgeWeight;

		if ( stallEdgeAllowed( edge.forward(), edge.backward() ) && heapForward->WasInserted( to ) ) {
			const int shorterDistance = heapForward->GetKey( to ) + edgeWeight;
			if ( shorterDistance < distance ) {
				//perform a bfs starting at node
				//only insert nodes when a sub-optimal path can be proven
				//insert node into the stall queue
				heapForward->GetKey( node ) = shorterDistance;
				heapForward->GetData( node ).stalled = true;
				stallQueue.push( node );

				while ( !stallQueue.empty() ) {
					//get node from the queue
					const Node stallNode = stallQueue.front();
					stallQueue.pop();
					const int stallDistance = heapForward->GetKey( stallNode );

					//iterate over outgoing edges
					for ( EdgeIterator stallEdge = graph.edges( stallNode ); stallEdge.hasEdgesLeft(); ) {
						graph.unpackNextEdge( &stallEdge );
						//is edge outgoing/reached/stalled?
						if ( !edgeAllowed( stallEdge.forward(), stallEdge.backward() ) )
							continue;
						const Node stallTo = stallEdge.target();
						if ( !heapForward->WasInserted( stallTo ) )
							continue;
						if ( heapForward->GetData( stallTo ).stalled == true )
							continue;

						const int stallToDistance = stallDistance + stallEdge.distance();
						//sub-optimal path found -> insert stallTo
						if ( stallToDistance < heapForward->GetKey( stallTo ) ) {
							if ( heapForward->WasRemoved( stallTo ) )
								heapForward->GetKey( stallTo ) = stallToDistance;
							else
								heapForward->DecreaseKey( stallTo, stallToDistance );

							stallQueue.push( stallTo );
							heapForward->GetData( stallTo ).stalled = true;
						}
					}
				}
				break;
			}
		}

		if ( edgeAllowed( edge.forward(), edge.backward() ) ) {
			//New Node discovered -> Add to Heap + Node Info Storage
			if ( !heapForward->WasInserted( to ) )
				heapForward->Insert( to, toDistance, node );

			//Found a shorter Path -> Update distance
			else if ( toDistance <= heapForward->GetKey( to ) ) {
				heapForward->DecreaseKey( to, toDistance );
				//new parent + unstall
				heapForward->GetData( to ).parent = node;
				heapForward->GetData( to ).stalled = false;
			}
		}
	}
}

int ContractionHierarchiesClient::computeRoute( const IGPSLookup::Result& inputSource, const IGPSLookup::Result& inputTarget, QVector< UnsignedCoordinate >* path ) {
	struct {
		Node source;
		Node target;
	} source, target;
	source.source = inputSource.source;
	source.target = inputSource.target;
	target.source = inputTarget.source;
	target.target = inputTarget.target;

	//insert source into heap
	unsigned sourceWeight = 0;
	unsigned targetWeight = 0;
	if ( source.source != source.target ) {
		EdgeIterator sourceEdge = graph.findEdge( source.source, source.target );
		sourceWeight = sourceEdge.distance();
	}
	if ( target.source != target.target ) {
		EdgeIterator targetEdge = graph.findEdge( target.source, target.target );
		targetWeight = targetEdge.distance();
	}

	heapForward->Insert( source.target, sourceWeight - sourceWeight * inputSource.percentage, source.target );
	if ( inputSource.bidirectional )
		heapForward->Insert( source.source, sourceWeight * inputSource.percentage, source.source );

	heapBackward->Insert( target.source, targetWeight * inputTarget.percentage, target.source );
	if ( inputTarget.bidirectional )
		heapBackward->Insert( target.target, targetWeight - targetWeight * inputTarget.percentage, target.target );

	int targetDistance = std::numeric_limits< int >::max();
	Node middle = ( Node ) 0;
	AllowForwardEdge forward;
	AllowBackwardEdge backward;

	while ( heapForward->Size() + heapBackward->Size() > 0 ) {

		if ( heapForward->Size() > 0 ) {
			computeStep( heapForward, heapBackward, forward, backward, &middle, &targetDistance );
		}

		if ( heapBackward->Size() > 0 ) {
			computeStep( heapBackward, heapForward, backward, forward, &middle, &targetDistance );
		}

	}

	if ( targetDistance == std::numeric_limits< int >::max() )
		return std::numeric_limits< unsigned >::max();

	Node pathNode = middle;
	Node source1 = source.target;
	Node source2 = source.target;
	if ( inputSource.bidirectional )
		source2 = source.source;

	std::stack< Node > stack;

	while ( pathNode != source1 && pathNode != source2 ) {
		Node parent = heapForward->GetData( pathNode ).parent;
		stack.push( pathNode );
		pathNode = parent;
	}
	stack.push( pathNode );
	path->push_back( inputSource.nearestPoint );
	path->push_back( graph.node( pathNode ).coordinate );

	while ( stack.size() > 1 ) {
		const Node node = stack.top();
		stack.pop();
		unpackEdge( node, stack.top(), true, path );
	}

	pathNode = middle;
	source1 = target.source;
	source2 = target.source;
	if ( inputTarget.bidirectional )
		source2 = target.target;
	while ( pathNode != source1 && pathNode != source2 ) {
		Node parent = heapBackward->GetData( pathNode ).parent;
		unpackEdge( parent, pathNode, false, path );
		pathNode = parent;
	}
	path->push_back( inputTarget.nearestPoint );

	return targetDistance / 10;
}

bool ContractionHierarchiesClient::unpackEdge( const Node source, const Node target, bool forward, QVector< UnsignedCoordinate >* path ) {
	EdgeIterator edge = graph.edges( source );

	for ( ; edge.hasEdgesLeft(); ) {
		graph.unpackNextEdge( &edge );
		if ( edge.target() == target ) {
			if ( forward && edge.forward() )
				break;
			if ( ( !forward ) && edge.backward() )
				break;
		}
	}

	if ( !edge.shortcut() ) {
		if ( forward )
			path->push_back( graph.node( target ).coordinate );
		else
			path->push_back( graph.node( source ).coordinate );
		return true;
	}

	if ( edge.unpacked() ) {
		graph.path( edge, path, forward );
		return true;
	}

	const Node middle = edge.middle();

	if ( forward ) {
		unpackEdge( middle, source, false, path );
		unpackEdge( middle, target, true, path );
		return true;
	} else {
		unpackEdge( middle, target, false, path );
		unpackEdge( middle, source, true, path );
		return true;
	}
}

Q_EXPORT_PLUGIN2( contractionhierarchiesclient, ContractionHierarchiesClient )

