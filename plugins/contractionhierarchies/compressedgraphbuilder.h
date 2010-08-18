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
#include <QSet>
#include <QTime>
#include <QtDebug>

class CompressedGraphBuilder : public CompressedGraph {

public:

	CompressedGraphBuilder( unsigned blockSize, std::vector< Node >& inputNodes, std::vector< Edge >& inputEdges )
	{
		m_edges.swap( inputEdges );
		m_settings.blockSize = blockSize;
		m_nodes.swap( inputNodes );
		for ( std::vector< Edge >::iterator i = m_edges.begin(), iend = m_edges.end(); i != iend; i++ )
			i->data.unpacked = false;
	}

	bool run( QString filename, std::vector< unsigned >* remap)
	{
		return createGraph( filename, remap );
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
		unsigned externalEdgeCount;
		unsigned internalEdgeTargetSize;
		unsigned edgeCount;
		std::vector< unsigned > weightDistribution;
		QHash< unsigned, int > internalShortcutTargets;
		QSet< unsigned > adjacentBlocks;
	};

	struct PathBlockBuilder: public PathBlock {

	};

	// FUNCTIONS

	// Adds a new builder block starting at node
	void initBlock( unsigned blockID, unsigned firstNode )
	{
		m_block.id = blockID;
		m_block.maxSize = m_settings.blockSize;
		// Settings + 64Bit buffer
		m_block.baseSize = sizeof( Block::Settings ) + 4;
		m_block.maxX = 0;
		m_block.maxY = 0;
		m_block.firstNode = firstNode;

		m_block.settings.blockBits = bits_needed( blockID - 1 );
		m_block.settings.externalBits = 0;
		m_block.settings.firstEdgeBits = 0;
		m_block.settings.shortWeightBits = 0;
		m_block.settings.longWeightBits = 0;
		m_block.settings.xBits = 0;
		m_block.settings.yBits = 0;
		m_block.settings.minX = std::numeric_limits< unsigned >::max();
		m_block.settings.minY = std::numeric_limits< unsigned >::max();
		m_block.settings.adjacentBlockCount = 0;
		m_block.settings.nodeCount = 0;

		m_block.internalShortcutCount = 0;
		m_block.shortcutCount = 0;
		m_block.externalEdgeCount = 0;
		m_block.internalEdgeTargetSize = 0;
		m_block.edgeCount = 0;
		m_block.weightDistribution.clear();
		m_block.weightDistribution.resize( 33, 0 );
		m_block.internalShortcutTargets.clear();
	}

	bool addNode ( unsigned node ) {
		m_block.settings.nodeCount++;

		m_block.settings.minX = std::min( m_nodes[node].coordinate.x, m_block.settings.minX );
		m_block.settings.minY = std::min( m_nodes[node].coordinate.y, m_block.settings.minY );
		m_block.maxX = std::max( m_nodes[node].coordinate.x, m_block.maxX );
		m_block.maxY = std::max( m_nodes[node].coordinate.y, m_block.maxY );

		m_block.settings.xBits = bits_needed( m_block.maxX - m_block.settings.minX );
		m_block.settings.yBits = bits_needed( m_block.maxY - m_block.settings.minY );

		unsigned internalTargetBits = bits_needed( node - m_block.firstNode - 1 ); // no loops, only previous nodes, (0 - 1) will never be used
		for ( unsigned i = m_firstEdges[node]; i < m_firstEdges[node + 1]; i++ ) {
			const Edge& edge = m_edges[i];

			unsigned char weightBits = bits_needed( edge.data.distance );
			m_block.weightDistribution[weightBits]++;
			m_block.settings.longWeightBits = std::max( weightBits, m_block.settings.longWeightBits );

			if ( edge.target >= m_block.firstNode ) {
				m_block.internalEdgeTargetSize += internalTargetBits; // we only need to address previous nodes due to DAG property
			} else {
				m_block.externalEdgeCount++;
				int block = m_nodeIDs[node].block;
				if ( !m_block.adjacentBlocks.contains( block ) ) {
					m_block.adjacentBlocks.insert( block );
					m_block.settings.externalBits = std::max( m_externalBits[block], m_block.settings.externalBits );
				}
			}

			if ( edge.data.shortcut ) {
				m_block.shortcutCount++;
				m_block.internalShortcutTargets[edge.data.middle]++;
			}
		}

		m_block.edgeCount += m_firstEdges[node + 1] - m_firstEdges[node];
		m_block.internalShortcutCount += m_block.internalShortcutTargets[node];
		m_block.settings.adjacentBlockCount = m_block.adjacentBlocks.size();

		int size = 0;
		size += m_block.edgeCount * ( 1 + 1 + 1 + 1 ); // forward + backward + shortcut + target ( external vs internal ) flags
		size += m_block.externalEdgeCount * ( bits_needed( m_block.settings.adjacentBlockCount - 1 ) + m_block.settings.externalBits ); // external targets
		size += m_block.internalEdgeTargetSize; // internal targets
		size += m_block.shortcutCount * ( 1 ); // middle ( external vs internal ) flag
		size += ( m_block.shortcutCount - m_block.internalShortcutCount ) * ( 32 ); // external middle => unpacked
		size += m_block.internalShortcutCount * bits_needed( node - m_block.firstNode ); // internal middle
		unsigned shortWeightsCount = 0;
		unsigned minimumWeightSize = std::numeric_limits< unsigned >::max();
		for ( int bits = 0; bits <= m_block.settings.longWeightBits; bits++ ) {
			shortWeightsCount += m_block.weightDistribution[bits];
			unsigned weightSize = shortWeightsCount * bits + ( m_block.edgeCount - shortWeightsCount ) * m_block.settings.longWeightBits;
			if ( bits == m_block.settings.longWeightBits )
				weightSize++; // long vs short flag
			if ( weightSize < minimumWeightSize ) {
				weightSize = minimumWeightSize;
				m_block.settings.shortWeightBits = bits;
			}
		}
		// size == edge block size => compute firstEdgeBits
		m_block.settings.firstEdgeBits = bits_needed( size );

		size = ( size + 7 ) / 8; // round to full bytes;

		size += ( m_block.settings.xBits * m_block.settings.nodeCount + 7 ) / 8; // x coordinate
		size += ( m_block.settings.yBits * m_block.settings.nodeCount + 7 ) / 8; // y coordinate
		size += ( m_block.settings.firstEdgeBits * ( m_block.settings.nodeCount + 1 ) + 7 ) / 8; // first edge entries
		size += ( m_block.settings.blockBits * m_block.settings.adjacentBlockCount + 7 ) / 8; // adjacent blocks entries

		if ( size + m_block.baseSize > m_block.maxSize ) {
			if ( m_block.settings.nodeCount == 1 ) {
				qCritical() << "ERROR: a node requires more space than a single block can suffice\n"
					<< "block:" << m_block.id << "node:" << node << "size:" << size << "maxSize:" << m_block.maxSize << "\n"
					<< "try increasing the block size";
				exit( -1 );
			}
			return false;
		}

		//remap nodes
		m_nodeIDs[node].block = m_block.blockID;
		m_nodeIDs[node].node = node - m_block.firstNode;

		return true;
	}

	void writeBlock( QFile& blockFile )
	{

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
		for ( unsigned i = 0; i < m_edges.size(); i++ ) {
			if ( !m_edges[i].data.shortcut )
				continue;

			numberOfShortcuts++;

			//do not unpack internal shortcuts
			if ( m_nodeIDs[m_edges[i].source].block == m_nodeIDs[m_edges[i].target].block )
				continue;

			//path already fully unpacked ( covered by some higher level shortcut )?
			if ( m_edges[i].data.unpacked )
				continue;

			//get unpacked path
			if ( m_edges[i].data.forward ) {
				m_unpackBuffer.push_back( m_nodes[m_edges[i].source] );
				unpackPath ( m_edges[i].source, m_edges[i].target, true );
			} else {
				m_unpackBuffer.push_back( m_nodes[m_edges[i].target] );
				unpackPath ( m_edges[i].source, m_edges[i].target, false );
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
		for ( ;m_edges[edge].target != target || ( forward && !m_edges[edge].data.forward ) || ( !forward && !m_edges[edge].data.backward ); edge++ )
			assert ( edge <= m_firstEdges[source + 1] );

		if ( !m_edges[edge].data.shortcut ) {
			m_unpackBuffer.push_back ( m_nodes[forward ? target : source] );
			return;
		}

		unsigned middle = m_edges[edge].data.middle;
		m_edges[edge].data.reversed = !forward;
		m_edges[edge].data.unpacked = true;

		//unpack the nodes in the right order
		if ( forward ) {
			//point at first node between source and destination
			m_edges[edge].data.path = m_unpackBuffer.size() - 1;

			unpackPath ( middle, source, false );
			unpackPath ( middle, target, true );
		} else {
			unpackPath ( middle, target, false );
			unpackPath ( middle, source, true );

			//point at last node between source and destination
			m_edges[edge].data.path = m_unpackBuffer.size() - 1;
		}
	}

	bool createGraph( QString filename, std::vector< unsigned >* remap )
	{
		assert( m_nodes.size() == remap->size() );
		qDebug( "creating compressed graph with %d nodes and %d edges", m_nodes.size(), m_edges.size() );
		QTime time;
		time.start();

		buildIndex();
		m_nodeIDs.resize( m_nodes.size() );
		qDebug() << "build node index:" << time.restart() << "ms";

		// compute mapping nodes -> blocks
		unsigned blocks = 0;
		initBlock( 0, 0 );
		for ( unsigned node = 0; node < m_nodes.size(); node++ ) {
			if ( !addNode( node ) ) {
				m_externalBits.push_back( bits_needed( node - 1 - m_block.firstNode ) ); // never negative <= at least one node
				initBlock( blocks++, node );
				addNode( node ); // cannot fail -> exit( -1 ) in addNode otherwise
			}
		}
		assert( blocks == m_externalBits.size() );
		// account for the last block
		blocks++;
		m_externalBits.push_back( bits_needed( m_nodes.size() - 1 - m_block.firstNode ) );
		qDebug() << "computed block layout:" << time.restart() << "ms";

		// unpack shortcuts
		QFile pathFile( filename + "_paths" );
		if ( !pathFile.open( QIODevice::WriteOnly ) ) {
			qCritical() << "failed to open path file:" << pathFile.fileName();
			return false;
		}
		unpackShortcuts( pathFile );
		qDebug() << "unpacked shortcuts:" << time.restart() << "ms";

		// recompute block settings and write
		QFile blockFile( filename + "_edges" );
		if ( !blockFile.open( QIODevice::WriteOnly ) ) {
			qCritical() << "failed to open path file:" << blockFile.fileName();
			return false;
		}
		for ( unsigned block = 0, node = 0; block < blocks; block++ ) {
			initBlock( block, node );
			while ( node < m_nodes.size() && m_nodeIDs[node].block == block )
				addNode( node++ );  // should never fail
			writeBlock( blockFile );
			qDebug() << block;
		}
		qDebug() << "wrote blocks" << time.restart() << "ms";

		m_settings.internalBits = 0;
		for ( unsigned block = 0; block < blocks; block++ )
			m_settings.internalBits = std::max( m_externalBits[block], m_settings.internalBits );

		// write config
		QFile configFile( filename + "_config" );
		if ( !configFile.open( QIODevice::WriteOnly ) ) {
			qCritical() << "failed to open config file:" << configFile.fileName();
			return false;
		}
		m_settings.write( configFile );

		qDebug( "used Settings:" );
		qDebug( "\tblock size: %d", m_settings.blockSize );
		qDebug( "\tinternal bits: %d", m_settings.internalBits );
		qDebug( "\tblocks: %d", blocks );
		qDebug( "\tpath blocks: %d", m_unpackBuffer.size() / m_settings.blockSize );
		qDebug( "\tblock space: %lld Mb" , ( long long ) blocks * m_settings.blockSize / 1024 / 1024 );
		qDebug( "\tpath block space: %lld Mb" , pathFile.size() / 1024 / 1024 );
		qDebug( "\tmax internal ID: %u", nodeFromDescriptor( m_nodeIDs.back() ) );

		for ( unsigned i = 0; i < m_nodes.size(); i++ )
			( *remap )[i] = nodeFromDescriptor( m_nodeIDs[( *remap )[i]] );

		return true;
	}

	//VARIABLES
	std::vector< Node > m_nodes;
	std::vector< unsigned > m_firstEdges;
	std::vector< Edge > m_edges;
	std::vector< nodeDescriptor > m_nodeIDs;
	std::vector< unsigned char > m_externalBits;
	std::vector< Node > m_unpackBuffer;
	std::vector< PathBlockBuilder > m_pathBlocks;
	BlockBuilder m_block;
};

#endif // COMPRESSEDGRAPHBUILDER_H
