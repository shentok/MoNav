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

	public:

	private:

		Edge m_data;
	};

	CompressedGraph( QString filename, unsigned blockSize, std::vector< Edge >& inputEdges, std::vector< UnsignedCoordinate >& inputNodes, std::vector< unsigned >* remap )
	{
		createGraph( filename, inputEdges, inputNodes, remap );
		loadGraph( filename );
	}

	CompressedGraph( const QString& filename )
	{
		unloadGraph();
		loadGraph( filename );
	}

	~CompressedGraph()
	{
		unloadGraph();
	}

protected:

	// TYPES

	struct globalSettings {
		char internalBits;
		char blockSize;
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

	void createGraph( QString filename, unsigned blockSize, std::vector< Edge >& inputEdges, std::vector< Node >& inputNodes, std::vector< unsigned >* remap )
	{
		assert( inputNodes.size() == remap->size() );
		qDebug( "creating compressed graph with %d nodes and %d edges", inputNodes.size(), inputEdges.size() );

		m_settings.blockSize = blockSize;
		m_settings.internalBits = 1;

		// build edge index
		std::vector< unsigned > firstEdge;
		for ( unsigned i = 0; i < inputEdges.size(); i++ ) {
			inputEdges[i].unpacked = false;

			//edges belong to a new node? -> update index
			while ( firstEdge.size() <= inputEdges[i].source )
				firstEdge.push_back( i );
		}
		while ( firstEdge.size() < inputNodes.size() )
			firstEdge.push_back( inputEdges.size() );

		std::vector< nodeDescriptor > nodeID( firstEdge.size() );
		// TODO: BUILD

		std::vector< Node > pathBuffer;
		unsigned pathBlocks = 0;

		//pre-unpack paths
		QFile pathFile( filename + "_path" );
		pathFile.open( QIODevice::WriteOnly );
		unsigned numberOfShortcuts = 0;
		unsigned numberOfUnpacked = 0;

		qDebug( "Computing path data" );
		for ( unsigned i = 0; i < inputEdges.size(); i++ ) {
			if ( !inputEdges[i].data.shortcut )
				continue;

			numberOfShortcuts++;

			//do not unpack internal shortcuts
			if ( nodeID[inputEdges[i].source].block == nodeID[inputEdges[i].target].block )
				continue;

			//path already fully unpacked ( covered by some higher level shortcut )?
			if ( inputEdges[i].data.unpacked )
				continue;

			//get unpacked path
			if ( edges[i].forward ) {
				pathBuffer.push_back( inputNodes[inputEdges[i].source] );
				unpackPath ( inputNodes, firstEdge, inputEdge, &pathBuffer, inputEdges[i].source, inputNodes[i].target, true );
			} else {
				pathBuffer.push_back( inputNodes[inputEdges[i].target] );
				unpackPath ( inputNodes, firstEdge, inputEdge, &pathBuffer, inputEdges[i].source, inputNodes[i].target, false );
			}
			numberOfUnpacked++;
		}

		qDebug( "unpacked total shortcuts: %lf %%", 100.0f * numberOfUnpacked / numberOfShortcuts );

		//TODO: STORE PATH BLOCKS
		QFile pathFile( filename + "_path" );
		pathFile.open( QIODevice::WriteOnly );

		unsigned blocks = 0;
		unsigned pathBlocks = 0;

		// write config
		QFile configFile( filename + "_config" );
		configFile.open( QIODevice::WriteOnly );
		unsigned temp = m_settings.blockSize;
		configFile.write( ( const char* ) &temp, sizeof( temp ) );
		temp = m_settings.blockSize;
		configFile.write( ( const char* ) &temp, sizeof( temp ) );
		temp = m_settings.internalBits;
		configFile.write( ( const char* ) &temp, sizeof( temp ) );
		temp = blocks; //BLOCKS
		configFile.write( ( const char* ) &temp, sizeof( temp ) );
		temp = pathBlocks; //PATH BLOCKS
		configFile.write( ( const char* ) &temp, sizeof( temp ) );


		qDebug( "Used Settings:" );
		qDebug( "\tblock size: %d", m_settings.blockSize );
		qDebug( "\tinternal bits: %d", m_settings.internalBits );
		qDebug( "\tblocks: %d", 0 );
		qDebug( "\tpath blocks: %d", 0 );
		qDebug( "\tblock space: %lld Mb" , ( long long ) blocks * block_size / 1024 / 1024 );
		qDebug( "\tpath block space: %lld Mb" , ( long long ) pathBlocks * block_size / 1024 / 1024 );
		qDebug( "\tmax internal ID: %ud", nodeFromDescriptor( nodeID.back() ) );

		m_settings.internalBits = 1;
		//TODO: STORE & BUILD?

		for ( unsigned i = 0; i < node_count; i++ )
			( *remap )[i] = nodeFromDescriptor( nodeID[( *remap )[i]] );
	}

	void loadGraph()
	{

	}

	void unloadGraph()
	{

	}

	// VARIABLES

	globalSettings m_settings;
};

#endif // COMPRESSEDGRAPH_H
