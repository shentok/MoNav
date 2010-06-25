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

#ifndef CELL_H
#define CELL_H

#include "utils/config.h"
#include "utils/coordinates.h"
#include "utils/utils.h"
#include <vector>
#include <QHash>

namespace gg
{

	struct Cell
	{
		struct Edge {
			UnsignedCoordinate sourceCoord;
			UnsignedCoordinate targetCoord;
			NodeID source;
			NodeID target;
			bool bidirectional;
			bool operator==( const Edge& right ) const {
				return sourceCoord == right.sourceCoord && targetCoord == right.targetCoord && source == right.source && target == right.target;
			}
		};
		std::vector< Edge > edges;

		bool operator==( const Cell& right ) const
		{
			if ( edges.size() != right.edges.size() )
				return false;
			for ( int i = 0; i < ( int )edges.size(); i++ )
			{
				if ( !( edges[i] == right.edges[i] ) )
					return false;
			}
			return true;
		}

		size_t write( char* buffer, UnsignedCoordinate min, UnsignedCoordinate max ) {
			char* const oldBuffer = buffer;

			NodeID minID = std::numeric_limits< NodeID >::max();
			NodeID maxID = 0;

			unsigned numNodes = 2 * edges.size();
			for ( std::vector< Edge >::iterator i = edges.begin(), e = edges.end(); i != e; ++i ) {
				if ( i->source < minID )
					minID = i->source;
				if ( i->target < minID )
					minID = i->target;
				if ( i->source > maxID )
					maxID = i->source;
				if ( i->target > maxID )
					maxID = i->target;

				for ( std::vector< Edge >::iterator next = i + 1; next != e; ++next ) {
					if ( next->source == i->target ) {
						std::swap( *( i + 1 ), *next );
						numNodes--;
						break;
					}
					if ( next->target == i->target && next->bidirectional ) {
						std::swap( next->target, next->source );
						std::swap( next->targetCoord, next->sourceCoord );
						std::swap( *( i + 1 ), *next );
						numNodes--;
						break;
					}
				}
			}

			const int xBits = log2_rounded( max.x - min.x + 1 );
			const int yBits = log2_rounded( max.y - min.y + 1 );
			const int idBits = log2_rounded( maxID - minID + 1 );

			int offset = 0;
			*(( unsigned* ) buffer ) = numNodes;
			buffer += sizeof( unsigned );
			*(( unsigned* ) buffer ) = minID;
			buffer += sizeof( unsigned );
			*buffer = xBits;
			buffer++;
			*buffer = yBits;
			buffer++;
			*buffer = idBits;
			buffer++;

			QHash< NodeID, unsigned > nodes;
			for ( std::vector< Edge >::const_iterator i = edges.begin(), e = edges.end(); i != e; ) {
				write_unaligned_unsigned( &buffer, 1, 1, &offset );

				if ( !nodes.contains( i->source ) ) {
					write_unaligned_unsigned( &buffer, 1, 1, &offset );
					write_unaligned_unsigned( &buffer, i->source - minID, idBits, &offset );

					if ( i->sourceCoord.x < min.x || i->sourceCoord.x > max.x ) {
						write_unaligned_unsigned( &buffer, 1, 1, &offset );
						write_unaligned_unsigned( &buffer, i->sourceCoord.x, 32, &offset );
					}
					else {
						write_unaligned_unsigned( &buffer, 0, 1, &offset );
						write_unaligned_unsigned( &buffer, i->sourceCoord.x - min.x, xBits, &offset );
					}

					if ( i->sourceCoord.y < min.y || i->sourceCoord.y > max.y ) {
						write_unaligned_unsigned( &buffer, 1, 1, &offset );
						write_unaligned_unsigned( &buffer, i->sourceCoord.y, 32, &offset );
					}
					else {
						write_unaligned_unsigned( &buffer, 0, 1, &offset );
						write_unaligned_unsigned( &buffer, i->sourceCoord.y - min.y, yBits, &offset );
					}

					const unsigned num = nodes.size();
					nodes[i->source] = num;
				} else {
					write_unaligned_unsigned( &buffer, 0, 1, &offset );
					write_unaligned_unsigned( &buffer, nodes[i->source], log2_rounded( nodes.size() + 1 ), &offset );
				}

				NodeID lastNode = i->target;
				do {
					write_unaligned_unsigned( &buffer, 0, 1, &offset );

					write_unaligned_unsigned( &buffer, i->bidirectional ? 1 : 0, 1, &offset );

					if ( !nodes.contains( i->target ) ) {
						write_unaligned_unsigned( &buffer, 1, 1, &offset );
						write_unaligned_unsigned( &buffer, i->target - minID, idBits, &offset );

						if ( i->targetCoord.x < min.x || i->targetCoord.x > max.x ) {
							write_unaligned_unsigned( &buffer, 1, 1, &offset );
							write_unaligned_unsigned( &buffer, i->targetCoord.x, 32, &offset );
						}
						else {
							write_unaligned_unsigned( &buffer, 0, 1, &offset );
							write_unaligned_unsigned( &buffer, i->targetCoord.x - min.x, xBits, &offset );
						}

						if ( i->targetCoord.y < min.y || i->targetCoord.y > max.y ) {
							write_unaligned_unsigned( &buffer, 1, 1, &offset );
							write_unaligned_unsigned( &buffer, i->targetCoord.y, 32, &offset );
						}
						else {
							write_unaligned_unsigned( &buffer, 0, 1, &offset );
							write_unaligned_unsigned( &buffer, i->targetCoord.y - min.y, yBits, &offset );
						}

						const unsigned num = nodes.size();
						nodes[i->target] = num;
					} else {
						write_unaligned_unsigned( &buffer, 0, 1, &offset );
						write_unaligned_unsigned( &buffer, nodes[i->target], log2_rounded( nodes.size() + 1 ), &offset );
					}

					lastNode = i->target;
					i++;
				} while ( i != e && i->source == lastNode );
			}

			buffer += ( offset + 7 ) / 8;

			return buffer - oldBuffer;
		}

		size_t  read( const char* buffer, UnsignedCoordinate min, UnsignedCoordinate /*max*/ ) {
			const char* oldBuffer = buffer;

			int offset = 0;
			const unsigned numNodes = *(( unsigned* ) buffer );
			buffer += sizeof( unsigned );
			const NodeID minID = *(( unsigned* ) buffer );
			buffer += sizeof( unsigned );
			const int xBits = *buffer;
			buffer++;
			const int yBits = *buffer;
			buffer++;
			const int idBits = *buffer;
			buffer++;

			std::vector< NodeID > nodes;
			std::vector< UnsignedCoordinate > coordinates;
			Edge edge;
			edge.source = 666;
			for ( unsigned i = 0; i < numNodes; ++i ) {
				bool newWay = read_unaligned_unsigned( &buffer, 1, &offset ) == 1;

				if ( !newWay )
					edge.bidirectional = read_unaligned_unsigned( &buffer, 1, &offset ) == 1;

				bool newNode = read_unaligned_unsigned( &buffer, 1, &offset ) == 1;

				if ( newNode ) {
					edge.target = read_unaligned_unsigned( &buffer, idBits, &offset ) + minID;

					bool xOutside = read_unaligned_unsigned( &buffer, 1, &offset ) == 1;
					if ( xOutside )
						edge.targetCoord.x = read_unaligned_unsigned( &buffer, 32, &offset );
					else
						edge.targetCoord.x = read_unaligned_unsigned( &buffer, xBits, &offset ) + min.x;

					bool yOutside = read_unaligned_unsigned( &buffer, 1, &offset ) == 1;
					if ( yOutside )
						edge.targetCoord.y = read_unaligned_unsigned( &buffer, 32, &offset );
					else
						edge.targetCoord.y = read_unaligned_unsigned( &buffer, yBits, &offset ) + min.y;

					nodes.push_back( edge.target );
					coordinates.push_back( edge.targetCoord );
				} else {
					const unsigned position = read_unaligned_unsigned( &buffer, log2_rounded( nodes.size() + 1 ), &offset );
					edge.target = nodes[position];
					edge.targetCoord = coordinates[position];
				}

				if ( !newWay )
					edges.push_back( edge );

				edge.source = edge.target;
				edge.sourceCoord.x = edge.targetCoord.x;
				edge.sourceCoord.y = edge.targetCoord.y;
			}

			buffer += ( offset + 7 ) / 8;

			return buffer - oldBuffer;
		}

	};
}

#endif // CELL_H
