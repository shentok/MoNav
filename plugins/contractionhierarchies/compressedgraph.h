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

#ifndef COMPRESSEDGRAPH_H
#define COMPRESSEDGRAPH_H

#include "utils/coordinates.h"
#include "utils/utils.h"
#include <QString>
#include <QFile>
#include <vector>

class CompressedGraph {

public:

	typedef unsigned NodeIterator;

	struct Node {
		UnsignedCoordinate coordinate;
	};

	struct Edge {
		unsigned source;
		unsigned target;
		struct Data {
			int distance;
			bool shortcut : 1;
			bool forward : 1;
			bool backward : 1;
			bool unpacked : 1;
			bool reversed : 1;
			unsigned middle;
			unsigned path;
		} data;

		bool operator<( const Edge& right ) const {
			if ( source != right.source )
				return source < right.source;
			int l = ( data.forward ? -1 : 0 ) + ( data.backward ? 1 : 0 );
			int r = ( right.data.forward ? -1 : 0 ) + ( right.data.backward ? 1 : 0 );
			if ( l != r )
				return l < r;
			if ( target != right.target )
				return target < right.target;
			return data.distance < right.data.distance;
		}

	};

	class EdgeIterator {

		friend class CompressedGraph;

	public:

		bool hasEdgesLeft()
		{
			return m_position < m_end;
		}

		// can only be invoked when hasEdgesLeft return true
		// furthermore the edge has to be unpacked by the graph
		bool operator++()
		{
			assert( m_position + size <= m_end );
			assert( m_size != 0 );
			m_position += m_size;
#ifndef NDEBUG
			m_size = 0;
#endif
		}

	private:

		EdgeIterator( unsigned block, unsigned position, unsigned end )
		{
			m_block = block;
			m_position = position;
			m_end = end;
			m_size = 0;
		}

		unsigned m_block;
		unsigned m_position;
		unsigned m_end;
		unsigned m_size;
		Edge m_edge;
	};

	CompressedGraph()
	{
	}

	CompressedGraph( const QString& filename )
	{
		loadGraph( filename );
	}

	~CompressedGraph()
	{
		unloadGraph();
	}

	EdgeIterator getEdges( NodeIterator node )
	{

	}

	EdgeIterator findEdge( NodeIterator source, NodeIterator target )
	{

	}

	Edge unpackEdge( const EdgeIterator& edge )
	{
		edge.size = 1;
	}

	Node getNode( NodeIterator )
	{

	}

protected:

	// TYPES

	struct Block {
		struct Settings {
			unsigned char blockBits;
			unsigned char adjacentBlockBits;
			unsigned char internalBits;
			unsigned char firstEdgeBits;
			unsigned char shortWeightBits;
			unsigned char longWeightBits;
			unsigned char xBits;
			unsigned char yBits;
			unsigned minX;
			unsigned minY;
		} settings;

		unsigned edges;
		unsigned adjacentBlocks;
		unsigned firstEdges;
		unsigned nodeX;
		unsigned nodeY;
		unsigned id;
		unsigned nodeCount;
		unsigned cacheID;
	};

	struct PathBlock {

	};

	struct GlobalSettings {
		char internalBits;
		char blockSize;

		bool read( QFile& in )
		{
			in.read( ( const char* ) &blockSize, sizeof( blockSize ) );
			in.read( ( const char* ) &internalBits, sizeof( internalBits ) );
		}

		void write( QFile& out )
		{
			out.write( ( const char* ) &blockSize, sizeof( blockSize ) );
			out.write( ( const char* ) &internalBits, sizeof( internalBits ) );
		}
	};

	struct nodeDescriptor {
		unsigned block;
		unsigned id;
	};

	// FUNCTIONS

	unsigned nodeToBlock( NodeIterator node )
	{
		return node >> m_settings.internalBits;
	}

	unsigned nodeToInternal( NodeIterator node )
	{
		return read_bits( node, m_settings.internalBits );
	}

	NodeIterator nodeFromDescriptor( nodeDescriptor node )
	{
		NodeIterator result = ( node.block << m_settings.internalBits ) | node.id;
		assert( nodeToBlock( result ) == node.block );
		assert( nodeToInternal( result ) == node.id );
		return result;
	}

	void loadGraph()
	{

	}

	void unloadGraph()
	{

	}

	// VARIABLES

	GlobalSettings m_settings;
};

#endif // COMPRESSEDGRAPH_H
