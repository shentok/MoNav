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

#ifndef DYNAMICGRAPH_H_INCLUDED
#define DYNAMICGRAPH_H_INCLUDED

#include <vector>
#include <algorithm>

// returns the smallest power of two that is at least as large as x
static unsigned log2Rounded32( unsigned x ) {
	const unsigned bitPosition[32] = {
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};

	//round up
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	++x;
	//x is now a power of 2

	//each power of two is mapped to a unique 5 bit sequence with ( x * 0x077CB531U ) >> 27
	return bitPosition[( x * 0x077CB531U ) >> 27];
}

static unsigned log2Rounded64( unsigned long long x ) {
	int upperLog = log2Rounded32( x >> 32 );
	if ( upperLog > 0 )
		return upperLog;
	return log2Rounded32( x );
}


template< typename EdgeData>
class DynamicGraph {
	public:
		typedef NodeID NodeIterator;
		typedef NodeID EdgeIterator;

		class InputEdge {
			public:
				NodeIterator source;
				NodeIterator target;
				EdgeData data;
				bool operator<( const InputEdge& right ) const {
					if ( source != right.source )
						return source < right.source;
					return target < right.target;
				}
		};

		DynamicGraph( int nodes, const std::vector< InputEdge > &graph ) {
			_numNodes = nodes;
			_numEdges = ( EdgeIterator ) graph.size();
			_nodes.resize( _numNodes );
			EdgeIterator edge = 0;
			EdgeIterator position = 0;
			for ( NodeIterator node = 0; node < _numNodes; ++node ) {
				EdgeIterator lastEdge = edge;
				while ( edge < _numEdges && graph[edge].source == node ) {
					++edge;
				}
				_nodes[node].firstEdge = position;
				_nodes[node].edges = edge - lastEdge;
				_nodes[node].size = 1 << log2Rounded32( edge - lastEdge );
				position += _nodes[node].size;
			}
			_edges.resize( position );
			edge = 0;
			for ( NodeIterator node = 0; node < _numNodes; ++node ) {
				for ( EdgeIterator i = _nodes[node].firstEdge, e = _nodes[node].firstEdge + _nodes[node].edges; i != e; ++i ) {
					_edges[i].target = graph[edge].target;
					_edges[i].data = graph[edge].data;
					edge++;
				}
			}
		}

		unsigned GetNumberOfNodes() const {
			return _numNodes;
		}

		unsigned GetNumberOfEdges() const {
			return _numEdges;
		}

		unsigned GetOutDegree( const NodeIterator &n ) const {
			return _nodes[n].edges;
		}

		NodeIterator GetTarget( const EdgeIterator &e ) const {
			return NodeIterator( _edges[e].target );
		}

		EdgeData &GetEdgeData( const EdgeIterator &e ) {
			return _edges[e].data;
		}

		const EdgeData &GetEdgeData( const EdgeIterator &e ) const {
			return _edges[e].data;
		}

		EdgeIterator BeginEdges( const NodeIterator &n ) const {
			//assert( EndEdges( n ) - EdgeIterator( _nodes[n].firstEdge ) <= 100 );
			return EdgeIterator( _nodes[n].firstEdge );
		}

		EdgeIterator EndEdges( const NodeIterator &n ) const {
			return EdgeIterator( _nodes[n].firstEdge + _nodes[n].edges );
		}

		//adds an edge. Invalidates edge iterators for the source node
		EdgeIterator InsertEdge( const NodeIterator &from, const NodeIterator &to, const EdgeData &data ) {
			_StrNode &node = _nodes[from];
			if ( node.edges + 1 >= node.size ) {
				node.size *= 2;
				EdgeIterator newFirstEdge = ( EdgeIterator ) _edges.size();
				_edges.resize( _edges.size() + node.size );
				for ( unsigned i = 0; i < node.edges; ++i ) {
					_edges[newFirstEdge + i ] = _edges[node.firstEdge + i];
				}
				node.firstEdge = newFirstEdge;
			}
			_StrEdge &edge = _edges[node.firstEdge + node.edges];
			edge.target = to;
			edge.data = data;
			_numEdges++;
			node.edges++;
			return EdgeIterator( node.firstEdge + node.edges );
		}

		//removes an edge. Invalidates edge iterators for the source node
		/*void DeleteEdge( const NodeIterator source, const EdgeIterator &e ) {
			_StrNode &node = _nodes[source];
			--_numEdges;
			--node.edges;
			const unsigned last = node.firstEdge + node.edges;
			//swap with last edge
			_edges[e] = _edges[last];
		}*/

		//removes all edges (source,target)
		int DeleteEdgesTo( const NodeIterator source, const NodeIterator target ) {
			int deleted = 0;
			for ( EdgeIterator i = BeginEdges( source ), iend = EndEdges( source ); i < iend - deleted; ++i ) {
				if ( _edges[i].target == target ) {
					do {
						deleted++;
						_edges[i] = _edges[iend - deleted];
					} while ( i < iend - deleted && _edges[i].target == target );
				}
			}

			#pragma omp atomic
			_numEdges -= deleted;
			_nodes[source].edges -= deleted;

			return deleted;
		}

		//searches for a specific edge
		EdgeIterator FindEdge( const NodeIterator &from, const NodeIterator &to ) const {
			for ( EdgeIterator i = BeginEdges( from ), iend = EndEdges( from ); i != iend; ++i ) {
				if ( _edges[i].target == to ) {
					return i;
				}
			}
			return EndEdges( from );
		}

	private:

		struct _StrNode {
			//index of the first edge
			EdgeIterator firstEdge;
			//amount of edges
			unsigned edges;
			unsigned size;
		};

		struct _StrEdge {
			NodeIterator target;
			EdgeData data;
		};

		NodeIterator _numNodes;
		EdgeIterator _numEdges;

		std::vector< _StrNode > _nodes;
		std::vector< _StrEdge > _edges;
};

#endif // DYNAMICGRAPH_H_INCLUDED
