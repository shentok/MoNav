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

#ifndef COMPRESSEDGRAPHBUILDER_H
#define COMPRESSEDGRAPHBUILDER_H

#include "compressedgraph.h"
#include <limits>
#include <QHash>

class CompressedGraphBuilder : public CompressedGraph {

public:

	CompressedGraphBuilder( QString filename, unsigned blockSize, std::vector< Edge >& inputEdges, std::vector< Node >& inputNodes, std::vector< unsigned >* remap )
	{
		m_edges.swap( inputEdges );
		m_settings.blockSize = blockSize;
		m_nodes.swap( inputNodes );
		for ( std::vector< Node >::iterator i = m_edges.begin(), iend = m_edges.end(); i != iend; i++ )
			i->data.unpacked = false;
	}

	bool run()
	{
		createGraph( filename, remap );
	}

private:

	// STRUCTS

	struct BlockBuilder: public Block {
		unsigned blockID;
		unsigned maxSize;
		unsigned baseSize;
		unsigned maxX;
		unsigned maxY;
		unsigned firstNode;

		unsigned internalShortcutCount;
		unsigned shortcutCount;
		unsigned internalEdgeCount;
		unsigned edgeCount;
		std::vector< unsigned > weightDistribution;
		QHash< unsigned, int > internalShortcutTargets;
		QHash< unsigned, unsigned > adjacentBlocks;
	};

	struct PathBlockBuilder: public PathBlock {

	};

	// FUNCTIONS

	// Adds a new builder block starting at node
	initBlock( unsigned node )
	{
		m_block.blockID = m_blocks.size();
		m_block.maxSize = 1u << m_settings.blockSize;
		// Settings + #nodes + +adjacent blocks + 64Bit buffer
		m_block.baseSize = sizeof( Block::Settings ) + 2 * sizeof( unsigned ) + 4;
		m_block.maxX = 0;
		m_block.maxY = 0;
		m_block.firstNode = node;
		m_block.settings.blockBits = bits_needed( m_blocks.size() );
		m_block.settings.adjacentBlockBits = 1;
		// set internal bits to the maximum encountered so far
		m_block.settings.internalBits = m_settings.internalBits;
		m_block.settings.firstEdgeBits = 1;
		m_block.settings.shortWeightBits = 0;
		m_block.settings.longWeightBits = 1;
		m_block.settings.xBits = 1;
		m_block.settings.yBits = 1;
		m_block.settings.minX = std::numeric_limits< unsigned >::max();
		m_block.settings.minY = std::numeric_limits< unsigned >::max();

		m_block.internalShortcutCount = 0;
		m_block.shortcutCount = 0;
		m_block.internalEdgeCount = 0;
		m_block.edgeCount = 0;
		m_block.weightDistribution.clear();
		m_block.weightDistribution.resize( 33, 0 );
		m_block.internalShortcutTargets.clear();
	}

	// recalculates size of the edges part of the last block in m_blocks
	unsigned reevaluateEdges()
	{
		assert( m_blocks.size() > 0 );
		BlockBuilder& block = m_blocks.back();
		// best weight distribution found so far
		unsigned bestSize = std::numeric_limits< unsigned >::max();

		for ( unsigned shortWeightBits = 0; shortWeightBits < block.settings.longWeightBits; shortWeightBits++ ) {
			unsigned size = 0;
			for ( unsigned node = 0; node < block->nodeCount; node++ ) {
				const unsigned edgesBegin = m_firstEdges[node];
				const unsigned edgesEnd = m_firstEdges[node + 1];

				for ( unsigned edge = edgesBegin; edge < edgesEnd; edge++ )
					size += edgeSize( node, edge, shortWeightBits );
			}

			if ( size < bestSize ) {
				bestSize = size;
				block.settings.shortWeightBits = shortWeightBits;
			}
		}

		return bestSize;
	}

	// calculates the size of an edge in the last block in m_blocks
	unsigned edgeSize( unsigned node, unsigned edge, unsigned shortWeightBits )
	{
		BlockBuilder& block = m_blocks.back();
		unsigned size = 0;
		size++; // forward flag
		size++; // backward flag
		size++; // target flag ( internal vs external )
		if ( shortWeightBits != 0 )
			size++; // distance flag ( short vs long );
		size++; // shortcut flag
		// target ( log2( node ) due to DAG property vs adjBlock + internalID
		size += m_edges[edge].target >= block.firstNode ? bits_needed ( node ) : block.settings.internalBits + block.settings.adj_block_bits;
		// distance ( short bits vs long bits )
		size += bits_needed( m_edges[edge].data.distance ) > shortWeightBits ? settings.long_weight_bits : short_weight_bits;

		if ( m_edges[edge].shortcut ) {
			if ( m_edges[edge].middle < block.firstNode + block.nodeCount ) {
				size += 1 + 1 + settings.internal_bits;
			} else
				size += 32;
		}
	}

	bool addNode ( unsigned node ) {
		m_block.nodeCount++;
		m_block.settings.internalBits = bits_needed( m_block.nodeCount );

		m_block.settings.minX = std::min( m_nodes[node].x, m_block.settings.minX );
		m_block.settings.minY = std::min( m_nodes[node].x, m_block.settings.minY );
		m_block.settings.maxX = std::max( m_nodes[node].x, m_block.settings.maxX );
		m_block.settings.maxX = std::max( m_nodes[node].x, m_block.settings.maxY );

		m_block.settings.xBits = bits_needed( m_block.maxX - m_block.settings.minX );
		m_block.settings.yBits = bits_needed( m_block.maxY - m_block.settings.minY );

		for ( unsigned i = m_firstEdges[node]; i < m_firstEdges[node + 1]; i++ ) {
			const Edge& edge = m_edges[i];

			unsigned weightBits = bits_needed( edge.data.distance );
			m_weightDistribution[weightBits]++;
			m_block.settings.longWeightBits = std::max( weightBits, m_block.settings.longWeightBits );

			if ( edge.target >= m_block.firstNode )
				m_block.internalEdgeCount++;
			else
				m_block.adjacentBlocks.contains[m_nodeIDs[node].block]++;

			if ( edge.data.shortcut ) {
				m_block.shortcutCount++;
				m_block.internalShortcutTargets[edge.data.middle]++;
			}
		}
		m_block.edgeCount += m_firstEdges[node + 1] - m_firstEdges[node];
		m_block.internalShortcutCount += m_block.internalShortcutTargets[node];
		m_block.settings.adjacentBlockBits = bits_needed( m_block.adjacentBlocks.size() );

		int size = 0;
		size += m_block.settings.xBits * m_block.nodeCount; // x coordinate
		size += m_block.settings.yBits * m_block.nodeCount; // y coordinate

		while ( newEdgeSize + 1 >= ( 1u << new_settings.first_edge_bits ) )
			new_settings.first_edge_bits++;

		if ( size + m_block.baseSize > m_block.maxSize ) {
			if ( m_block.nodeCount == 1 ) {
				qCritical( "ERROR: a node requires more space than a single block can suffice: node %d", node );
				qCritical( "try increasing the block size" );
				exit( -1 );
			}
			return false;
		}

		for ( std::map<unsigned, unsigned>::const_iterator i = new_adj_blocks.begin(); i != new_adj_blocks.end(); i++ ) {
			const unsigned ID = ( unsigned ) _adj_blocks.size();
			_adj_blocks[i->first] = ID;
		}

		//remap nodes
		_block_map[node].block = _block_id ;
		_block_map[node].node = _node_count;

		_first_edge_size = new_first_edge_size;
		_adj_blocks_size = new_adj_blocks_size;
		_edge_size = newEdgeSize;
		if ( PATH_DATA ) {
			_node_x_size = new_node_x_size;
			_node_y_size = new_node_y_size;
			_max_x = newMaxX;
			_max_y = newMaxY;
		}
		_settings = new_settings;
		global_settings.internal_bits = new_settings.internal_bits;
		_node_count++;
		return true;
	}

	// build edge index
	void buildIndex()
	{
		for ( unsigned i = 0; i < m_edges.size(); i++ ) {
			//edges belong to a new node? -> update index
			while ( m_firstEdges.size() <= m_edges[i].source )
				m_firstEdges.push_back( i );
		}
		while ( m_firstEdges.size() <= m_nodes.size() )
			m_firstEdges.push_back( m_edges.size() );
	}

	// unpack all external shortcuts
	void unpackShortcuts( QFile& pathFile )
	{
		unsigned numberOfShortcuts = 0;
		unsigned numberOfUnpacked = 0;

		qDebug( "Computing path data" );
		for ( unsigned i = 0; i < inputEdges.size(); i++ ) {
			if ( !inputEdges[i].data.shortcut )
				continue;

			numberOfShortcuts++;

			//do not unpack internal shortcuts
			if ( m_nodeIDs[inputEdges[i].source].block == m_nodeIDs[inputEdges[i].target].block )
				continue;

			//path already fully unpacked ( covered by some higher level shortcut )?
			if ( inputEdges[i].data.unpacked )
				continue;

			//get unpacked path
			if ( m_edges[i].forward ) {
				pathBuffer.push_back( inputNodes[inputEdges[i].source] );
				unpackPath ( inputEdges[i].source, inputNodes[i].target, true );
			} else {
				pathBuffer.push_back( inputNodes[inputEdges[i].target] );
				unpackPath ( inputEdges[i].source, inputNodes[i].target, false );
			}
			numberOfUnpacked++;
		}

		qDebug( "unpacked total shortcuts: %lf %%", 100.0f * numberOfUnpacked / numberOfShortcuts );

		//TODO: STORE PATH BLOCKS
	}

	// unpacks a shortcut and updates the pointers for all shortcuts encountered
	void unpackPath( unsigned source, unsigned target, bool forward ) {
		unsigned edge = m_firstEdges[source];
		assert ( edge != m_firstEdges[source + 1] );
		for ( ;m_edges[edge].target != target || ( forward && !m_edges[edge].forward ) || ( !forward && !m_edges[edge].backward ); edge++ )
			assert ( edge <= m_nodes[source + 1].first_edge );

		if ( !m_edges[edge].shortcut ) {
			m_unpackBuffer.push_back ( m_nodes[forward ? target : source] );
			return;
		}

		unsigned middle = m_edges[edge].middle;
		m_edges[edge].data.reversed = !forward;
		m_edges[edge].data.unpacked = true;

		//unpack the nodes in the right order
		if ( forward ) {
			//point at first node between source and destination
			m_edges[edge].path = m_unpackBuffer.size() - 1;

			unpack_path ( middle, source, false );
			unpack_path ( middle, target, true );
		} else {
			unpack_path ( middle, target, false );
			unpack_path ( middle, source, true );

			//point at last node between source and destination
			m_edges[edge].path = m_unpackBuffer.size() - 1;
		}
	}

	void createGraph( QString filename, std::vector< unsigned >* remap )
	{
		assert( inputNodes.size() == remap->size() );
		qDebug( "creating compressed graph with %d nodes and %d edges", inputNodes.size(), inputEdges.size() );

		buildIndex();
		m_settings.internalBits = 1;
		m_nodeIDs.resize( inputNodes.size() );
		// TODO: BUILD

		QFile pathFile( filename + "_path" );
		if ( !pathFile.open( QIODevice::WriteOnly ) ) {
			qCritical() << "failed to open path file:" << pathFile.fileName();
			return false;
		}
		unpackShortcuts( pathFile );

		unsigned blocks = 0;

		// write config
		QFile configFile( filename + "_config" );
		if ( !configFile.open( QIODevice::WriteOnly ) ) {
			qCritical() << "failed to open config file:" << configFile.fileName();
			return false;
		}
		m_settings.write( configFile );

		qDebug( "Used Settings:" );
		qDebug( "\tblock size: %d", m_settings.blockSize );
		qDebug( "\tinternal bits: %d", m_settings.internalBits );
		qDebug( "\tblocks: %d", blocks );
		qDebug( "\tpath blocks: %d", pathBlocks );
		qDebug( "\tblock space: %lld Mb" , ( long long ) blocks * block_size / 1024 / 1024 );
		qDebug( "\tpath block space: %lld Mb" , pathFile.size() / 1024 / 1024 );
		qDebug( "\tmax internal ID: %ud", nodeFromDescriptor( m_nodeIDs.back() ) );

		m_settings.internalBits = 1;
		//TODO: STORE & BUILD?

		for ( unsigned i = 0; i < node_count; i++ )
			( *remap )[i] = nodeFromDescriptor( m_nodeIDs[( *remap )[i]] );
	}

	//VARIABLES
	std::vector< Node > m_nodes;
	std::vector< unsigned > m_firstEdges;
	std::vector< Edge > m_edges;
	std::vector< nodeDescriptor > m_nodeIDs;
	std::vector< Node > m_unpackBuffer;
	std::vector< PathBlockBuilder > m_pathBlocks;
	BlockBuilder m_block;
};

#endif // COMPRESSEDGRAPHBUILDER_H
