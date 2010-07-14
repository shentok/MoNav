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

#ifndef COMPRESSEDCHGRAPH_H_INCLUDED
#define COMPRESSEDCHGRAPH_H_INCLUDED

#include <cassert>
#include <limits>
#include <vector>
#include <map>
#include <algorithm>
#include <QTime>
#include "blockcache.h"
#include "utils/coordinates.h"
#include "utils/utils.h"

//Path data includes shortcut bit & middle point
#define PATH_DATA true
//PATH_DATA has to be true unless PRE_UNPACK ist false
//unpacked data includes unpacked path blocks & reverse bit & pointers to the path instead of middle points for external shortcuts
#define PRE_UNPACK true
//dynamic data includes affected nodes & middle point for every shortcut
#define DYNAMIC_DATA false
#define HEADER_SIZE 1024
#define MAX_WITNESS 1

class block_graph {
private:

	class _logical_block;
	struct _str_edge_data;
	struct _packed_edge;
	struct _packed_first_edge_data;
	struct _packed_adj_block_data;
	struct _str_witness{
		unsigned affected;
		unsigned safety;
	};
	struct _str_global_settings {
		char internal_bits;
	};

	struct _str_block_settings {
		unsigned char block_bits;
		unsigned char adj_block_bits;
		unsigned char internal_bits;
		unsigned char first_edge_bits;
		unsigned char short_weight_bits;
		unsigned char long_weight_bits;
		unsigned char node_x_bits;
		unsigned char node_y_bits;
		unsigned node_min_x;
		unsigned node_min_y;
	};

public:

	class out_edge_iterator;

	//implements a descriptor class for nodes
	class vertex_descriptor {

	public:

		vertex_descriptor() {
		}

		explicit vertex_descriptor ( unsigned node ) {
			_node = node;
		}

		//castable to unsigned -> makes hashing possible
		operator unsigned () const {
			return _node;
		}

		//no real order, but makes it usable with maps
		bool operator< ( const vertex_descriptor& right ) const {
			return ( ( unsigned ) ( *this ) ) < ( ( unsigned ) right );
		}

		bool operator== ( const vertex_descriptor& right ) const {
			return _node == right._node;
		}

		bool operator!= ( const vertex_descriptor& right ) const {
			return _node != right._node;
		}

		unsigned get_block ( const _str_global_settings& settings ) const {
			return _node >> settings.internal_bits;
		}

		unsigned get_node ( const _str_global_settings& settings ) const {
			return _node & ( ( 1u << settings.internal_bits	) - 1 );
		}

		void set ( unsigned block, unsigned node, const _str_global_settings& settings ) {
			_node = ( block << settings.internal_bits ) | node;
		}

			private:

		unsigned _node;

	};

	//only present if path data is loaded
	UnsignedCoordinate get_location ( const vertex_descriptor& node ) {
		assert ( PATH_DATA );
		assert ( _blocks[node.get_block ( _settings ) ].loaded );
		UnsignedCoordinate coord;
		coord.x = _packed_node_x::unpack ( * ( _blocks + node.get_block ( _settings ) ), node.get_node ( _settings ) );
		coord.y = _packed_node_y::unpack ( * ( _blocks + node.get_block ( _settings ) ), node.get_node ( _settings ) );
		return coord;
	}

	//Implements a ReadablePropertyMap to extract an unpacked shotcut
	class shortcut_path_extractor {
	public:
		class path_iterator {
		public:
			path_iterator ( bool f, unsigned i, block_graph* g ) {
				index = 2 * i;
				reversed = f;
				graph = g;
				block_size = graph->_block_size / 4;
				load_block();
			}
			void operator++ () {
				unsigned old_index = index;
				if ( reversed )
					index -= 2;
				else
					index += 2;
				if ( ( index / block_size ) != ( old_index / block_size ) )
					load_block();
			}
			void operator++ ( int ) {
				operator++();
			}
			unsigned get_x () {
				const unsigned* data = graph->_path_blocks[index / block_size].data;
				const unsigned position = index & ( block_size - 1 );
				return data[position];
			}
			unsigned get_y () {
				const unsigned* data = graph->_path_blocks[index / block_size].data;
				const unsigned position = index & ( block_size - 1 );
				return data[position + 1];
			}
					private:
			void load_block() {
				const unsigned block_id = index / block_size;
				_path_block& block = graph->_path_blocks[block_id];
				if ( !graph->_preload ) {
					if ( !block.loaded ) {
						//load block
						unsigned unloaded_id = 0;
						bool unloaded = false;

						block.load ( graph->_cache.LoadBlock ( block_id + graph->_block_count, &block.cache_id, &unloaded, &unloaded_id ) );
						block.loaded = true;
						graph->_loads++;

						if ( unloaded ) {
							if ( unloaded_id < graph->_block_count )
								graph->_blocks[unloaded_id].loaded = false;
							else
								graph->_path_blocks[unloaded_id - graph->_block_count].loaded = false;
						}
					} else
						if ( !graph->_preload )
							graph->_cache.UseBlock ( block.cache_id );
				}
			}
			unsigned index;
			unsigned block_size: 31;
			bool reversed: 1;
			block_graph* graph;
		};
		block_graph* graph;
	};

	//Implements a ReadablePropertyMap to extract witness data from an edge
	class witness_extractor {
	public:
		class witness_iterator {
		public:
			witness_iterator ( const _str_witness* start, const _str_witness* end ) {
				data = start;
				data_end = end;
			}
			void operator++ () {
				data++;
			}
			void operator++ ( int ) {
				operator++();
			}
			_str_witness operator* () {
				return *data;
			}
			operator bool (){
				return data != data_end;
			}
			const _str_witness* data;
			const _str_witness* data_end;
		};
		typedef witness_iterator value_type;
		typedef unsigned key_type;
		typedef witness_extractor& reference;
		typedef unsigned category;
	};

	//needed to access _settings
	friend class shortcut_path_extractor::path_iterator;

	//Descriptor for weight
	struct weight_descriptor {
		static const unsigned MAX_VALUE = ( unsigned ) - 1;
		static const unsigned DEFAULT_VALUE = 0;
		static const unsigned MIN_VALUE = 0;
	};

private:

	//Internal/External Edge data structures
	struct _str_edge_data {
		unsigned weight;
		bool forward: 1;
		bool backward: 1;
		bool shortcut: 1;
		bool unpacked: 1;
		bool path_reversed: 1;
		vertex_descriptor middle;
		unsigned path;
		vertex_descriptor target;
		unsigned witness_count;
		_str_witness witness_affected[MAX_WITNESS];
	};

public:


	//Edge Iterator
	//Unpacks edge data while iterating
	class out_edge_iterator {

	public:

		out_edge_iterator ( unsigned node, unsigned edge, const _logical_block* block, _str_global_settings& settings, bool load_data = true )
			: _edge ( edge << 3 ), _node ( node ), _settings ( settings ), _block ( block ) {
			//unpack data if necessary
			if ( load_data )
				_size = _packed_edge::unpack ( _node, _edge, _data, _block, _settings );
		}

		out_edge_iterator() {
		}

		void operator++() {
			//weak condition, not more than 2^bits edge adresses
			assert ( ( _edge >> 3 ) < ( 1u << _block->settings.first_edge_bits ) );

			//next edge + unpacking
			_edge += _size;
			_size = _packed_edge::unpack ( _node, _edge, _data, _block, _settings );
		}

		void operator++ ( int ) {
			operator++();
		}

		bool operator!= ( const out_edge_iterator& b ) const {
			//only used to compare to the end edge iterator
			//the end edge iterator is always 8 Bit aligned
			return ( _edge + 7 ) / 8 != ( b._edge + 7 ) / 8;
		}

		bool operator== ( const out_edge_iterator& b ) const {
			return ! operator!= ( b );
		}

		//converts the edge into a unique unsigned long long
		operator unsigned long long() {
			return ( ( ( unsigned long long ) _node ) << 32 ) | _edge;
		}

		friend class block_graph;

			private:

		unsigned _edge;
		unsigned _size;
		unsigned _node;
		_str_edge_data _data;
		_str_global_settings _settings;
		const _logical_block* _block;
	};

	block_graph ( const QString& filename, bool preload = true , unsigned cache_blocks = 100 )
		: _preload ( preload ) {
		//needs at least 1 cache block
		assert ( cache_blocks > 0 );

		qDebug( "Blockgraph parameters:" );
		qDebug( "\tpreload: %d", preload );
		qDebug( "\tcache_blocks: %d", cache_blocks );
		qDebug( "\tPATH_DATA: %d", PATH_DATA );
		qDebug( "\tPRE_UNPACK: %d", PRE_UNPACK );
		qDebug( "\tDYNAMIC_DATA: %d", DYNAMIC_DATA );

		QFile inputFile ( filename );
		inputFile.open( QIODevice::ReadOnly );

		//read file settings
		unsigned* header = new unsigned[HEADER_SIZE / sizeof ( unsigned ) + 1];

		inputFile.read ( ( char* ) header, HEADER_SIZE );

		if ( header[0] != FILE_TYPE ) {
			qCritical( "Wrong File Format" );
			exit ( -1 );
		}

		_block_size = header[1];
		_node_count = header[2];
		_edge_count = header[3];
		_block_count = header[4];
		_path_block_count = header[5];
		_settings = * ( ( _str_global_settings* ) ( header + 6 ) );

		qDebug( "\tblock_size: %d" , _block_size );
		qDebug( "\tnode_count: %d", _node_count );
		qDebug( "\tedge_count: %d", _edge_count );
		qDebug( "\tblock_count: %d", _block_count );
		qDebug( "\tpath block_count: %d", _path_block_count );

		delete[] header;

		//data structur for block managment
		_blocks = new _logical_block[_block_count];
		if ( PRE_UNPACK )
			_path_blocks = new _path_block[_path_block_count];

		//load the whole file
		if ( _preload ) {
			if ( PRE_UNPACK )
				_data = new unsigned char[ ( _block_count + _path_block_count ) * _block_size];
			else
				_data = new unsigned char[_block_count * _block_size];
			//_remap = new unsigned[_node_count];

			inputFile.seek ( _block_size );
			if ( PRE_UNPACK )
				inputFile.read ( ( char* ) _data, ( _block_count + _path_block_count ) * _block_size );
			else
				inputFile.read ( ( char* ) _data, _block_count * _block_size );
			//map_in.Read ( ( char* ) _remap, sizeof ( unsigned ) * _node_count );

			for ( unsigned i = 0; i < _block_count; i++ ) {
				_blocks[i].load ( _data + i * _block_size );
				_blocks[i].id = i;
			}

			if ( PRE_UNPACK ) {
				for ( unsigned i = 0; i < _path_block_count; i++ )
					_path_blocks[i].load ( _data + ( i + _block_count ) * _block_size );
			}
		} else {
			inputFile.close();

			//_remap_file = new MemoryMappedFile ( map_filename.mb_str(), sizeof ( unsigned ) * _node_count );
			//_remap = ( unsigned* ) _remap_file->getAddress();

			_cache.Init ( filename, cache_blocks, _block_size );
			_loads = 0;

			if ( PRE_UNPACK ) {
				for ( unsigned i = 0; i < _path_block_count; i++ )
					_path_blocks[i].loaded = false;
			}

			for ( unsigned i = 0; i < _block_count; i++ ) {
				_blocks[i].id = i;
				_blocks[i].loaded = false;
			}
		}
	}

	unsigned loads() {
		return _loads;
	}

	unsigned number_of_nodes() {
		return _node_count;
	}

	std::pair<out_edge_iterator, out_edge_iterator> find_edge ( const vertex_descriptor& source, const vertex_descriptor& target ) {
		if ( target < source ) {
			std::pair<out_edge_iterator, out_edge_iterator> edges = out_edges( source );
			while ( edges.first != edges.second && get_target( edges.first ) != target && get_shortcut( edges.first ) )
				++edges.first;

			assert( edges.first != edges.second );

			return edges;
		}
		else if ( source < target ) {
			std::pair<out_edge_iterator, out_edge_iterator> edges = out_edges( target );
			while ( edges.first != edges.second && get_target( edges.first ) != source && get_shortcut( edges.first ) )
				++edges.first;

			assert( edges.first != edges.second );

			return edges;
		}
		else {
			assert( false );
		}
		return out_edges( source );
	}

	//mapping external node id to internal id
	vertex_descriptor map_ext_to_int ( unsigned node ) const {
		return ( vertex_descriptor ) node;
	}

	std::pair<out_edge_iterator, out_edge_iterator> out_edges ( const vertex_descriptor& node ) {
		const unsigned block_id = node.get_block ( _settings );
		_logical_block& block = _blocks[block_id];

		if ( !_preload ) {
			if ( !block.loaded ) {
				//load block
				unsigned unloaded_id = 0;
				bool unloaded = false;

				block.load ( _cache.LoadBlock ( block_id, &block.cache_id, &unloaded, &unloaded_id ) );
				block.loaded = true;
				_loads++;

				if ( unloaded ) {
					if ( unloaded_id < _block_count )
						_blocks[unloaded_id].unload();
					else {
						assert ( PRE_UNPACK );
						_path_blocks[unloaded_id - _block_count].unload();
					}
				}
			} else
				if ( !_preload )
					_cache.UseBlock ( block.cache_id );
		}

		const unsigned node_id = node.get_node ( _settings );

		return std::pair<out_edge_iterator, out_edge_iterator> (
				out_edge_iterator ( node_id, _packed_first_edge_data::unpack ( block, node_id ), _blocks + block_id, _settings ),
				out_edge_iterator ( node_id, _packed_first_edge_data::unpack ( block, node_id + 1 ), NULL, _settings , false ) );
	}

	//returns the target property of an edge
	vertex_descriptor get_target ( const out_edge_iterator& edge ) {
		return edge._data.target;
	}

	//get the weight Property of an edge
	unsigned int get_weight ( const out_edge_iterator& edge ) {
		return edge._data.weight;
	}

	//get the direction Property of an edge
	bool get_forward ( const out_edge_iterator& edge ) {
		return edge._data.forward;
	}

	//get the direction Property of an edge
	bool get_backward ( const out_edge_iterator& edge ) {
		return edge._data.backward;
	}

	//get the shortcut property of an edge
	bool get_shortcut ( const out_edge_iterator& edge ) {
		assert ( PATH_DATA );
		return edge._data.shortcut;
	}

	bool get_unpacked ( const out_edge_iterator& edge ) {
		if ( PRE_UNPACK )
			return edge._data.unpacked;
		else
			return false;
	}

	void get_unpacked_path ( const out_edge_iterator& edge, std::vector< UnsignedCoordinate >* buffer, bool forward ) {
		assert ( PRE_UNPACK );
		assert ( edge._data.unpacked );
		shortcut_path_extractor::path_iterator i( edge._data.path_reversed, edge._data.path, this );

		vertex_descriptor target = get_target( edge );
		const size_t size = buffer->size();
		assert ( _blocks[target.get_block ( _settings ) ].loaded );
		unsigned x = _packed_node_x::unpack ( * ( _blocks + target.get_block ( _settings ) ), target.get_node ( _settings ) );
		unsigned y = _packed_node_y::unpack ( * ( _blocks + target.get_block ( _settings ) ), target.get_node ( _settings ) );
		if ( !forward ) {
			UnsignedCoordinate coord;
			coord.x = i.get_x();
			coord.y = i.get_y();
			buffer->push_back( coord );
		}
		++i;
		while ( i.get_x() != x || i.get_y() != y ) {
			UnsignedCoordinate coord;
			coord.x = i.get_x();
			coord.y = i.get_y();
			buffer->push_back( coord );
			++i;
		}

		if ( forward ) {
			UnsignedCoordinate coord;
			coord.x = i.get_x();
			coord.y = i.get_y();
			buffer->push_back( coord );
		}
		else {
			//buffer->pop_back();
			std::reverse( buffer->begin() + size, buffer->end() );
		}
	}

	//get the middle property of a shortcut edge
	//only possible if path data is present
	vertex_descriptor get_middle ( const out_edge_iterator& edge ) {
		assert ( PATH_DATA );
		assert ( edge._data.shortcut );
		return edge._data.middle;
	}

	witness_extractor::value_type get_witness ( const out_edge_iterator& edge ) {
		assert ( DYNAMIC_DATA );
		return witness_extractor::value_type ( &edge._data.witness_affected[0], &edge._data.witness_affected[edge._data.witness_count] );
	}

	~block_graph() {
		delete[] _blocks;
		if ( _preload ) {
			delete[] _data;
			//delete[] _remap;
		} else {
			_cache.Destroy ( );
			//delete _remap_file;
		}
		if ( PRE_UNPACK ) {
			delete[] _path_blocks;
		}
	}

private:
	
	class _path_block {

	public:
		void load ( const unsigned char* d ) {
			data = ( const unsigned* ) d;
			loaded = true;
		}

		void unload() {
			loaded = false;
		}

		const unsigned* data;
		unsigned cache_id;
		bool loaded: 1;
	};

	class _logical_block {

	public:

		void load ( const unsigned char* data ) {
			node_count = * ( ( unsigned* ) data );
			data += sizeof ( unsigned );
			const unsigned adj_blocks_count = * ( ( unsigned* ) data );
			data += sizeof ( unsigned );
			settings = * ( ( _str_block_settings* ) data );
			data += sizeof ( _str_block_settings );
			assert ( node_count <= ( 1u << settings.internal_bits ) );

			first_edges = data;
			data += ( ( node_count + 1 ) * settings.first_edge_bits + 7 ) / 8;
			adj_blocks = data;
			data += ( adj_blocks_count * settings.block_bits + 7 ) / 8;
			if ( PATH_DATA ) {
				node_x = data;
				data += ( node_count * settings.node_x_bits + 7 ) / 8;
				node_y = data;
				data += ( node_count * settings.node_y_bits + 7 ) / 8;
			}
			edges = data;
			loaded = true;
		}

		void unload() {
			loaded = false;
		}

		const unsigned char* edges;
		const unsigned char* adj_blocks;
		const unsigned char* first_edges;
		const unsigned char* node_x;
		const unsigned char* node_y;
		unsigned id;
		unsigned node_count;
		_str_block_settings settings;

		bool loaded:1;
		unsigned cache_id;
	};

	struct _packed_edge {
#ifndef MOBILE_VERSION
		static unsigned pack ( unsigned node, unsigned char* buffer, unsigned offset, bool external, bool external_shortcut, const _str_edge_data& data, const _str_global_settings& global_settings, const _str_block_settings& settings ) {
			const unsigned internal_edge_bits = log2_rounded ( node );
			buffer += offset >> 3;
			offset &= 7;
			unsigned used_bits = 0;
			unsigned temp = external ? 1 : 0;
			temp |= data.forward ? 2 : 0;
			temp |= data.backward ? 4 : 0;

			assert ( settings.internal_bits + settings.adj_block_bits <= sizeof ( unsigned ) * 8 - 3 );

			if ( external ) {
				temp |= ( ( ( data.target.get_block ( global_settings ) << settings.internal_bits ) | data.target.get_node ( global_settings ) ) << 3 );
				( ( unsigned* ) buffer ) [0] |= temp << offset;
				( ( unsigned* ) ( buffer + 1 ) ) [0] |= temp >> ( 8 - offset );
				used_bits += settings.internal_bits + settings.adj_block_bits + 3;
				offset += settings.internal_bits + settings.adj_block_bits + 3;
			} else {
				temp |= ( data.target.get_node ( global_settings ) << 3 );
				( ( unsigned* ) buffer ) [0] |= temp << offset;
				( ( unsigned* ) ( buffer + 1 ) ) [0] |= temp >> ( 8 - offset );
				used_bits += internal_edge_bits + 3;
				offset += internal_edge_bits + 3;
			}
			buffer += offset >> 3;
			offset &= 7;

			if ( PATH_DATA ) {
				if ( data.shortcut ) {
					if ( external_shortcut ) {
						if ( PRE_UNPACK ) {
							temp = ( unsigned ) data.path;
							temp = ( temp << 1 ) | ( data.path_reversed ? 1 : 0 );
							temp = ( temp << 2 ) | 3;
							( ( unsigned* ) buffer ) [0] |= temp << offset;
							( ( unsigned* ) ( buffer + 1 ) ) [0] |= temp >> ( 8 - offset );
							used_bits += 32;
							offset += 32;
							buffer += offset >> 3;
							offset &= 7;
						}
						if ( DYNAMIC_DATA || !PRE_UNPACK ) {
							temp = ( unsigned ) data.middle;
							temp = ( temp << 2 ) | 3;
							( ( unsigned* ) buffer ) [0] |= temp << offset;
							( ( unsigned* ) ( buffer + 1 ) ) [0] |= temp >> ( 8 - offset );
							used_bits += 32;
							offset += 32;
						}
					} else {
						temp = ( data.middle.get_node ( global_settings ) << 2 ) | 1;
						( ( unsigned* ) buffer ) [0] |= temp << offset;
						( ( unsigned* ) ( buffer + 1 ) ) [0] |= temp >> ( 8 - offset );
						used_bits += 1 + 1 + settings.internal_bits;
						offset += 1 + 1 + settings.internal_bits;
					}
				} else {
					//memory is zeroed -> zero bit
					used_bits++;
					offset++;
				}
				buffer += offset >> 3;
				offset &= 7;
			}

			if ( data.weight >= ( 1u << ( settings.short_weight_bits - 1 ) ) ) {
				temp = 1 | ( data.weight << 1 );
				( ( unsigned* ) buffer ) [0] |= temp << offset;
				( ( unsigned* ) ( buffer + 1 ) ) [0] |= temp >> ( 8 - offset );
				used_bits += settings.long_weight_bits;
				offset += settings.long_weight_bits;
			} else {
				temp = data.weight << 1;
				( ( unsigned* ) buffer ) [0] |= temp << offset;
				( ( unsigned* ) ( buffer + 1 ) ) [0] |= temp >> ( 8 - offset );
				used_bits += settings.short_weight_bits;
				offset += settings.short_weight_bits;
			}
			buffer += offset >> 3;
			offset &= 7;

			if ( DYNAMIC_DATA ) {
				if ( data.witness_count == 0 ) {
					//memory is zeroed -> zero bit
					used_bits++;
					offset++;
					buffer += offset >> 3;
					offset &= 7;
				} else {
					( ( unsigned* ) buffer ) [0] |= 1 << offset;
					used_bits++;
					offset++;
					buffer += offset >> 3;
					offset &= 7;
					for ( unsigned i = 0; i < data.witness_count; i++ ) {
						if ( i != data.witness_count - 1 )
							temp = 1;
						else
							temp = 0;
						temp |= data.witness_affected[i].affected << 1;
						( ( unsigned* ) buffer ) [0] |= temp << offset;
						( ( unsigned* ) ( buffer + 1 ) ) [0] |= temp >> ( 8 - offset );
						used_bits += 32;
						offset += 32;
						buffer += offset >> 3;
						offset &= 7;
						if ( data.witness_affected[i].safety >= ( 1u << ( settings.short_weight_bits - 1 ) ) ) {
							temp = 1 | ( data.witness_affected[i].safety << 1 );
							( ( unsigned* ) buffer ) [0] |= temp << offset;
							( ( unsigned* ) ( buffer + 1 ) ) [0] |= temp >> ( 8 - offset );
							used_bits += settings.long_weight_bits;
							offset += settings.long_weight_bits;
						} else {
							temp = data.witness_affected[i].safety << 1;
							( ( unsigned* ) buffer ) [0] |= temp << offset;
							( ( unsigned* ) ( buffer + 1 ) ) [0] |= temp >> ( 8 - offset );
							used_bits += settings.short_weight_bits;
							offset += settings.short_weight_bits;
						}
						buffer += offset >> 3;
						offset &= 7;
					}
				}
			}

			assert ( used_bits > 7 );

			return used_bits;
		}
#endif

		static unsigned unpack ( unsigned node, unsigned offset, _str_edge_data& data, const _logical_block* block, const _str_global_settings& global_settings ) {
			const _str_block_settings& settings = block->settings;
			const unsigned char* buffer = block->edges + ( offset >> 3 );
			offset &= 7;

			unsigned temp = read_unaligned_unsigned ( buffer, offset );
			unsigned used_bits = 0;
			const bool external = ( temp & 1 ) != 0;
			data.forward = ( temp & 2 ) != 0;
			data.backward = ( temp & 4 ) != 0;
			temp >>= 3;
			offset += 3;
			used_bits += 3;

			// read 31 bits for internal + adja + forward + backward + external bit
			assert ( settings.internal_bits + settings.adj_block_bits <= 31 - 3 );

			if ( external ) {
				const unsigned target_node = read_bits ( temp, settings.internal_bits );
				temp >>= settings.internal_bits;
				const unsigned adj_block = read_bits ( temp, settings.adj_block_bits );
				const unsigned target_block = _packed_adj_block_data::unpack ( *block, adj_block );

				data.target.set ( target_block, target_node, global_settings );

				used_bits += settings.internal_bits + settings.adj_block_bits;
				offset += settings.internal_bits + settings.adj_block_bits;
			} else {
				const unsigned internal_edge_bits = log2_rounded ( node );
				data.target.set ( block->id, read_bits ( temp, internal_edge_bits ), global_settings );
				used_bits += internal_edge_bits;
				offset += internal_edge_bits;
			}

			buffer += offset >> 3;
			offset &= 7;

			if ( PATH_DATA ) {
				temp = read_unaligned_unsigned ( buffer, offset );
				if ( ( temp & 1 ) != 0 ) {
					data.shortcut = true;
					const bool external_shortcut = ( ( temp & 2 ) != 0 );
					temp >>= 2;
					if ( external_shortcut ) {
						if ( PRE_UNPACK ) {
							data.path_reversed = ( ( temp & 1 ) != 0 );
							data.unpacked = true;
							temp >>= 1;
							data.path = temp;
							used_bits += 32;
							offset += 32;
							if ( DYNAMIC_DATA ) {
								buffer += offset >> 3;
								offset &= 7;
								temp = read_unaligned_unsigned ( buffer, offset ) >> 2;
							}
						}
						if ( DYNAMIC_DATA | !PRE_UNPACK ) {
							data.middle = ( vertex_descriptor ) temp;
							used_bits += 32;
							offset += 32;
						}
					} else {
						data.unpacked = false;
						data.middle.set ( block->id, read_bits ( temp, settings.internal_bits ), global_settings );
						used_bits += 1 + 1 + settings.internal_bits;
						offset += 1 + 1 + settings.internal_bits;
					}
				} else {
					data.shortcut = false;
					data.unpacked = false;
					data.middle = ( vertex_descriptor ) ( ( unsigned ) - 1 );
					used_bits++;
					offset++;
				}
				buffer += offset >> 3;
				offset &= 7;
			}

			temp = read_unaligned_unsigned ( buffer, offset );

			if ( ( temp & 1 ) == 0 ) {
				data.weight = read_bits ( temp, settings.short_weight_bits ) >> 1;
				used_bits += settings.short_weight_bits;
				offset += settings.short_weight_bits;
			} else {
				data.weight = read_bits ( temp, settings.long_weight_bits ) >> 1;
				used_bits += settings.long_weight_bits;
				offset += settings.long_weight_bits;
			}
			buffer += offset >> 3;
			offset &= 7;

			if ( DYNAMIC_DATA ) {
				temp = read_unaligned_unsigned ( buffer, offset );
				data.witness_count = 0;
				used_bits++;
				offset++;
				buffer += offset >> 3;
				offset &= 7;

				if ( ( temp & 1 ) != 0 )
					do {
					temp = read_unaligned_unsigned ( buffer, offset );
					data.witness_affected[data.witness_count].affected = temp >> 1;
					assert ( data.witness_count < MAX_WITNESS );
					used_bits += 32;
					offset += 32;
					buffer += offset >> 3;
					offset &= 7;
					unsigned temp2 = read_unaligned_unsigned ( buffer, offset );
					if ( ( temp2 & 1 ) == 0 ) {
						data.witness_affected[data.witness_count++].safety = read_bits ( temp2, settings.short_weight_bits ) >> 1;
						used_bits += settings.short_weight_bits;
						offset += settings.short_weight_bits;
					} else {
						data.witness_affected[data.witness_count++].safety = read_bits ( temp2, settings.long_weight_bits ) >> 1;
						used_bits += settings.long_weight_bits;
						offset += settings.long_weight_bits;
					}
					buffer += offset >> 3;
					offset &= 7;
				} while ( ( temp & 1 ) == 1 );
			}

			assert ( used_bits > 7 );

			return used_bits;
		}

	};

#ifndef MOBILE_VERSION
	static unsigned _pack_array ( unsigned char* buffer, const unsigned* data, unsigned data_count, unsigned bits ) {
		unsigned used_bits = 0;
		assert ( bits <= 31 );

		for ( unsigned i = 0, e = ( bits * data_count + 7 ) / 8; i <= e; i++ )
			buffer[i] = 0;

		for ( unsigned i = 0; i < data_count; i++ ) {
			unsigned* destination = ( unsigned* ) ( buffer + ( used_bits >> 3 ) );
			destination[0] |= data[i] << ( used_bits & 7 );
			destination = ( unsigned* ) ( buffer + ( used_bits >> 3 ) + 1 );
			destination[0] |= data[i] >> ( 8 - ( used_bits & 7 ) );
			used_bits += bits;
		}

		return ( bits * data_count + 7 ) / 8;
	}
#endif

	struct _packed_first_edge_data {

#ifndef MOBILE_VERSION
		static unsigned pack ( unsigned char* buffer, const unsigned* data, unsigned data_count, const _str_block_settings& settings ) {
			return _pack_array ( buffer, data, data_count, settings.first_edge_bits );
		}
#endif

		static unsigned unpack ( const _logical_block& block, unsigned node ) {
			const unsigned position = node * block.settings.first_edge_bits;
			return read_unaligned_unsigned ( block.first_edges + ( position >> 3 ), ( position & 7 ) ) & ( ( 1u << block.settings.first_edge_bits ) - 1 );;
		}

	};

	struct _packed_adj_block_data {

#ifndef MOBILE_VERSION
		static unsigned pack ( unsigned char* buffer, const unsigned* data, unsigned data_count, const _str_block_settings& settings ) {
			return _pack_array ( buffer, data, data_count, settings.block_bits );
		}
#endif

		static unsigned unpack ( const _logical_block& block, unsigned block_id ) {
			const unsigned position = block_id * block.settings.block_bits;
			return read_unaligned_unsigned ( block.adj_blocks + ( position >> 3 ), ( position & 7 ) ) & ( ( 1u << block.settings.block_bits ) - 1 );;
		}

	};

	struct _packed_node_x {

#ifndef MOBILE_VERSION
		static unsigned pack ( unsigned char* buffer, const unsigned* data, unsigned data_count, const _str_block_settings& settings ) {
			return _pack_array ( buffer, data, data_count, settings.node_x_bits );
		}
#endif

		static unsigned unpack ( const _logical_block& block, unsigned node ) {
			const unsigned position = node * block.settings.node_x_bits;
			return block.settings.node_min_x + ( read_unaligned_unsigned ( block.node_x + ( position >> 3 ), ( position & 7 ) ) & ( ( 1u << block.settings.node_x_bits ) - 1 ) );
		}

	};

	struct _packed_node_y {

#ifndef MOBILE_VERSION
		static unsigned pack ( unsigned char* buffer, const unsigned* data, unsigned data_count, const _str_block_settings& settings ) {
			return _pack_array ( buffer, data, data_count, settings.node_y_bits );
		}
#endif

		static unsigned unpack ( const _logical_block& block, unsigned node ) {
			const unsigned position = node * block.settings.node_y_bits;
			return block.settings.node_min_y + ( read_unaligned_unsigned ( block.node_y + ( position >> 3 ), ( position & 7 ) ) & ( ( 1u << block.settings.node_y_bits ) - 1 ) );
		}

	};

	struct _str_builder_edge {
		unsigned target;
		unsigned weight;
		unsigned middle;
		unsigned path;
		bool forward: 1;
		bool backward: 1;
		bool shortcut: 1;
		bool unpacked: 1;
		bool internal_shortcut: 1;
		bool path_included: 1;
		bool path_reversed: 1;
	};

	struct _str_builder_node {
		unsigned x;
		unsigned y;
		unsigned first_edge;
	};

	struct _str_builder_map {
		unsigned block;
		unsigned node;
	};

	_logical_block* _blocks;
	_path_block* _path_blocks;
	_str_global_settings _settings;
	//unsigned* _remap;
	//MemoryMappedFile* _remap_file;
	unsigned _block_count, _path_block_count;
	unsigned _node_count;
	unsigned _edge_count;
	unsigned _block_size;
	static const unsigned FILE_TYPE = 4869534;

	//cache
	unsigned char* _data;
	unsigned _loads;
	BlockCache _cache;
	bool _preload: 1;

public:
	
	struct InputEdge {
		unsigned source;
		unsigned target;
		struct Data {
			int distance;
			bool shortcut : 1;
			bool forward : 1;
			bool backward : 1;
			unsigned middle;
		} data;
		bool operator<( const InputEdge& right ) const {
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

#ifndef MOBILE_VERSION
	static void build ( std::vector< InputEdge >& inputEdges, std::vector< UnsignedCoordinate >& inputNodes, std::vector< unsigned >* remap, const QString& destination_filename, unsigned block_size ) {
		QFile outputFile( destination_filename );
		outputFile.open( QIODevice::WriteOnly );
		unsigned node_count, edge_count;
		std::vector< unsigned > unpacked_path_buffer;
		unsigned unpacked_path_blocks = 0;

		//Read Edge and Node count
		node_count = inputNodes.size();
		assert( node_count == remap->size() );
		edge_count = inputEdges.size();

		qDebug( "Reading %d nodes and %d edges", node_count, edge_count );


		//allocate temporary memory for edges and first_edges
		_str_builder_node* nodes = new _str_builder_node[node_count + 1];
		_str_builder_edge* edges = new _str_builder_edge[edge_count];

		//inputFile.read ( ( char* ) remap, sizeof ( unsigned ) * node_count );
		for ( unsigned i = 0; i < node_count; i++ ) {
			nodes[( *remap )[i]].x = inputNodes[i].x;
			nodes[( *remap )[i]].y = inputNodes[i].y;
		}
		std::vector< UnsignedCoordinate >().swap( inputNodes );

		//Read all edges, edges are already sorted by (from,direction,to)
		unsigned node = 0;
		for ( unsigned i = 0; i < edge_count; i++ ) {
			unsigned from;

			from = inputEdges[i].source;
			edges[i].target = inputEdges[i].target;
			edges[i].weight = inputEdges[i].data.distance;
			edges[i].forward = inputEdges[i].data.forward;
			edges[i].backward = inputEdges[i].data.backward;
			edges[i].shortcut = inputEdges[i].data.shortcut;
			edges[i].internal_shortcut = true;
			edges[i].unpacked = false;
			edges[i].path_included = false;

			if ( edges[i].shortcut )
				edges[i].middle = inputEdges[i].data.middle;

			//edges belong to a new node? -> update index
			for ( ; node <= from; node++ )
				nodes[node].first_edge = i;

		}
		std::vector< InputEdge >().swap( inputEdges );

		//read witness data
		unsigned witness_count;
		witness_count = 0;
		unsigned* witness_index = new unsigned[edge_count + 1];
		_str_witness* witness_edges = new _str_witness[witness_count];
		witness_index[0] = 0;
		unsigned max_witnesses = 0;
		for ( unsigned i = 0, edge = 0; i < witness_count; i++ ) {
			unsigned temp = 0;
			//inputFile.read ( ( char* ) &temp, sizeof ( unsigned ) );
			while ( temp != edge ) {
				witness_index[++edge] = i;
				if ( witness_index[edge] - witness_index[edge - 1] > max_witnesses )
					max_witnesses = witness_index[edge] - witness_index[edge - 1];
			}
			//inputFile.read ( ( char* ) &witness_edges[i].affected, sizeof ( unsigned ) );
			//inputFile.read ( ( char* ) &witness_edges[i].safety, sizeof ( unsigned ) );
			if ( i == witness_count - 1 ) {
				while ( edge <= edge_count )
					witness_index[++edge] = i;
				if ( witness_index[edge] - witness_index[edge - 1] > max_witnesses )
					max_witnesses = witness_index[edge] - witness_index[edge - 1];
			}
		}
		qDebug( "Max Witness per edge: %d", max_witnesses );
		assert ( max_witnesses < MAX_WITNESS );

		//conserve NodeCount
		for ( ;node <= node_count; node++ )
			nodes[node].first_edge = edge_count;

		_str_global_settings settings;
		settings.internal_bits = 1;

		double time = _Timestamp();

		_BlockBuilder builder ( node_count, edges, nodes, witness_index, witness_edges, block_size );

		int blocks = builder.build ( NULL, settings );

		//pre-unpack path
		if ( PRE_UNPACK ) {
			unsigned stats_shortcuts = 0;
			unsigned stats_unpacked_shortcuts = 0;
			double unpack_time = _Timestamp();

			//Step 1: choose shortcuts to unpack
			qDebug( "Computing path data" );
			for ( unsigned node = 0; node < node_count; node++ ) {
				for ( unsigned i = nodes[node].first_edge, e = nodes[node + 1].first_edge; i < e; i++ ) {
					if ( edges[i].shortcut ) {
						stats_shortcuts++;
						//do not unpack internal shortcuts
						if ( edges[i].internal_shortcut )
							continue;

						//path already fully coverey by previous?
						if ( edges[i].path_included )
							continue;

						//get unpacked path
						if ( edges[i].forward ) {
							unpacked_path_buffer.push_back ( nodes[node].x );
							unpacked_path_buffer.push_back ( nodes[node].y );
							_unpack_path ( edges, nodes, unpacked_path_buffer, node, edges[i].target, true );
						} else {
							unpacked_path_buffer.push_back ( nodes[edges[i].target].x );
							unpacked_path_buffer.push_back ( nodes[edges[i].target].y );
							_unpack_path ( edges, nodes, unpacked_path_buffer, node, edges[i].target, false );
						}
						stats_unpacked_shortcuts++;
					}
				}
			}

			for ( unsigned node = 0; node < node_count; node++ ) {
				for ( unsigned i = nodes[node].first_edge, e = nodes[node + 1].first_edge; i < e; i++ ) {
					if ( !edges[i].shortcut )
						continue;
					assert ( edges[i].path_included == !edges[i].internal_shortcut );
					if ( edges[i].internal_shortcut )
						continue;

					assert ( edges[i].path < unpacked_path_buffer.size() );
					assert ( unpacked_path_buffer[edges[i].path * 2] == nodes[node].x );
					assert ( unpacked_path_buffer[edges[i].path * 2 + 1] == nodes[node].y );
				}
			}

			qDebug( "Merged external unpacked shortcuts: %d Mb", ( int ) unpacked_path_buffer.size() * 4 / 1024 / 1024 );
			qDebug( "Unpacked %d of total shortcuts: %lf Percent", stats_unpacked_shortcuts, 100.0f * stats_unpacked_shortcuts / stats_shortcuts );

			unpacked_path_blocks = ( unpacked_path_buffer.size() * sizeof ( unsigned ) + block_size - 1 ) / block_size;

			unpack_time = _Timestamp() - unpack_time;
			qDebug( "Unpacking time: %lf s", unpack_time );
			time += unpack_time;
		}


		unsigned char* buffer = new unsigned char[ ( blocks + 10 ) * block_size];
		unsigned char* header_buffer = new unsigned char[block_size];
		unsigned* header = ( unsigned* ) header_buffer;
		memset ( buffer, 0, ( blocks + 10 ) * block_size );
		memset ( header_buffer, 0, block_size );

		header[0] = FILE_TYPE;
		header[1] = block_size;
		header[2] = node_count;
		header[3] = edge_count;
		header[4] = blocks;
		header[5] = unpacked_path_blocks;
		* ( ( _str_global_settings* ) ( header + 6 ) ) = settings;

		qDebug( "Used Settings:" );
		qDebug( "\tblock_size: %d", block_size );
		qDebug( "\tnode_count: %d", node_count );
		qDebug( "\tedge_count: %d", edge_count );
		qDebug( "\tblocks: %d", blocks );
		if ( PRE_UNPACK )
			qDebug( "\tpath blocks: %d", unpacked_path_blocks );
		qDebug( "\tspace: %d Mb" , ( blocks + unpacked_path_blocks ) * block_size / 1024 / 1024 );
		vertex_descriptor max_internal ( 0 );
		max_internal.set ( builder._block_map[( *remap )[node_count - 1]].block, builder._block_map[( *remap )[node_count - 1]].node, settings );
		qDebug( "\tmax internal ID: %d",( unsigned ) max_internal );

		settings.internal_bits = 1;
		builder.build ( buffer, settings );

		if ( PRE_UNPACK ) {
			for ( unsigned node = 0; node < node_count; node++ ) {
				for ( unsigned i = nodes[node].first_edge, e = nodes[node + 1].first_edge; i < e; i++ ) {
					if ( edges[i].shortcut ) {
						assert ( edges[i].path_included == !edges[i].internal_shortcut );
						if ( !edges[i].internal_shortcut ) {
							assert ( unpacked_path_buffer[edges[i].path * 2] == nodes[node].x );
							assert ( unpacked_path_buffer[edges[i].path * 2 + 1] == nodes[node].y );
						}
					}
				}
			}
		}

		time = _Timestamp() - time;
		qDebug( "Time to import: %lf s", time );

		outputFile.write ( ( char* ) header_buffer, block_size );
		outputFile.write ( ( char* ) buffer, blocks * block_size );
		if ( PRE_UNPACK )
			outputFile.write ( ( char* ) &unpacked_path_buffer[0], unpacked_path_blocks * block_size );

		delete[] buffer;
		delete[] header_buffer;
		if ( PRE_UNPACK ) {
			std::vector< unsigned >().swap( unpacked_path_buffer );
		}

		for ( unsigned i = 0; i < node_count; i++ ) {
			vertex_descriptor temp ( 0 );
			temp.set ( builder._block_map[( *remap )[i]].block, builder._block_map[( *remap )[i]].node, settings );
			( *remap )[i] = ( unsigned ) temp;
		}

		//free memory
		delete[] nodes;
		delete[] edges;
		delete[] witness_index;
		delete[] witness_edges;
	}
#endif

private:
	
	static double _Timestamp() {
		static QTime timer;
		static bool first = true;
		if ( first )
		{
			first = false;
			timer.start();
		}
		return ( double ) timer.elapsed() / 1000;
	}

	//unpacks a shortcut
	static void _unpack_path ( _str_builder_edge* edges, const _str_builder_node* nodes , std::vector< unsigned >& buffer,
										unsigned source, unsigned destination, bool forward ) {
		unsigned j = nodes[source].first_edge;
		assert ( j != nodes[source + 1].first_edge );
		for ( ;edges[j].target != destination || ( forward && !edges[j].forward ) || ( !forward && !edges[j].backward ); j++ )
			assert ( j != nodes[source + 1].first_edge );

		if ( !edges[j].shortcut ) {
			if ( forward ) {
				buffer.push_back ( nodes[destination].x );
				buffer.push_back ( nodes[destination].y );
			}
			else {
				buffer.push_back ( nodes[source].x );
				buffer.push_back ( nodes[source].y );
			}
			return;
		}

		const unsigned middle = edges[j].middle;
		if ( !edges[j].internal_shortcut ) {
			edges[j].path_reversed = !forward;
			edges[j].path_included = true;
		}

		//unpack the nodes in the right order
		if ( forward ) {
			//point at first node between source and destination
			edges[j].path = buffer.size() / 2 - 1;

			_unpack_path ( edges, nodes, buffer, middle, source, false );
			_unpack_path ( edges, nodes, buffer, middle, destination, true );
		} else {
			_unpack_path ( edges, nodes, buffer, middle, destination, false );
			_unpack_path ( edges, nodes, buffer, middle, source, true );

			//point at first node between source and destination
			edges[j].path = buffer.size() / 2 - 1;
		}

		return;
	}

	struct _str_statistics {

		_str_statistics() {
			memset ( this, 0, sizeof ( _str_statistics ) );
		}

		unsigned max_nodes;
		unsigned max_edges;
		unsigned internal_edges;
		unsigned max_adj_blocks;
		unsigned adj_blocks;
		unsigned internal_shortcuts;
		unsigned external_shortcuts;
		unsigned max_x_diff;
		unsigned max_y_diff;
		unsigned first_edge_space;
		unsigned adj_blocks_space;
		unsigned weight_space_bits;
		unsigned internal_space_bits;
		unsigned external_space_bits;
		unsigned internal_shortcut_space_bits;
		unsigned external_shortcut_space_bits;
		unsigned node_x_space;
		unsigned node_y_space;

	};

#ifndef MOBILE_VERSION
	class _logical_block_builder {

	public:

		_logical_block_builder ( unsigned block_id, unsigned size, _str_builder_map* block_map, _str_builder_edge* edges, const _str_builder_node* nodes, const unsigned* witness_index, const _str_witness* witness_edges, const _str_global_settings& global_settings ) {
			_block_id = block_id;
			_max_size = size - /*reading/writing buffer*/ 2 * sizeof ( unsigned );
			_block_map = block_map;
			_edges = edges;
			_nodes = nodes;
			_witness_index = witness_index;
			_witness_edges = witness_edges;
			_node_count = _edge_size = _node_x_size = _node_y_size = _adj_blocks_size = 0;
			_first_edge_size = 1;
			_max_x = _max_y = 0;
			_base_size = sizeof ( _str_block_settings ) + sizeof ( unsigned ) + sizeof ( unsigned );
			_settings.adj_block_bits = 1;
			_settings.node_x_bits = 2;
			_settings.node_y_bits = 2;
			_settings.block_bits = log2_rounded ( block_id );
			_settings.first_edge_bits = 1;
			_settings.internal_bits = global_settings.internal_bits;
			_settings.long_weight_bits = 4;
			_settings.short_weight_bits = 3;
			_settings.node_min_x = ( unsigned ) ( -1 );
			_settings.node_min_y = ( unsigned ) ( -1 );
		}

		unsigned reevalueate_edges ( _str_block_settings& settings, unsigned node_count ) {
			unsigned edge_size = ( unsigned ) - 1;

			for ( unsigned short_weight_bits = 3; short_weight_bits <= settings.long_weight_bits; short_weight_bits++ ) {
				unsigned new_edge_size = 0;
				for ( unsigned node = 0; node < node_count; node++ ) {
					const unsigned edges_begin = _nodes[node].first_edge;
					const unsigned edges_end = _nodes[node + 1].first_edge;
					unsigned bits_used = 0;

					for ( unsigned i = edges_begin; i < edges_end; i++ ) {
						const unsigned weight_bits = _edges[i].weight >= ( 1u << ( short_weight_bits - 1 ) ) ? settings.long_weight_bits : short_weight_bits;
						if ( _edges[i].target >= _first_node )
							bits_used += log2_rounded ( node ) + 1 + 1 + 1 + weight_bits;
						else
							bits_used += settings.internal_bits + settings.adj_block_bits + 1 + 1 + 1 + weight_bits;
						if ( PATH_DATA ) {
							if ( _edges[i].shortcut )
								if ( _edges[i].middle - _first_node < node_count && _edges[i].internal_shortcut ) {
								bits_used += 1 + 1 + settings.internal_bits;
							} else {
								bits_used += 32;
								if ( PRE_UNPACK && DYNAMIC_DATA )
									bits_used += 32;
							}
							else
								bits_used++;
						}
						if ( DYNAMIC_DATA ){
							bits_used += 1 + ( _witness_index[i + 1] - _witness_index[i] ) * 32;
							for ( unsigned j = _witness_index[i]; j < _witness_index[i + 1]; j++ )
								bits_used+= _witness_edges[j].safety >= ( 1u << ( short_weight_bits - 1 ) ) ? settings.long_weight_bits : short_weight_bits;
						}
					}

					new_edge_size += ( bits_used + 7 ) / 8;
				}

				if ( new_edge_size < edge_size ) {
					edge_size = new_edge_size;
					settings.short_weight_bits = short_weight_bits;
				}
			}

			return edge_size;
		}

		bool add_node ( unsigned node, _str_global_settings& global_settings ) {
			const unsigned edges_begin = _nodes[_node_count].first_edge;
			const unsigned edges_end = _nodes[_node_count + 1].first_edge;
			bool bit_increase = false;
			unsigned new_edge_size = _edge_size;
			unsigned new_max_x = _max_x;
			unsigned new_max_y = _max_y;
			_str_block_settings new_settings = _settings;
			std::map<unsigned, unsigned> new_adj_blocks;

			if ( _node_count == 0 )
				_first_node = node;

			if ( _node_count == ( 1u << new_settings.internal_bits ) ) {
				new_settings.internal_bits++;
				bit_increase = true;
			}

			if ( PATH_DATA ) {
				if ( _nodes[_node_count].x < new_settings.node_min_x )
					new_settings.node_min_x = _nodes[_node_count].x;
				if ( _nodes[_node_count].x > new_max_x )
					new_max_x = _nodes[_node_count].x;
				if ( new_max_x - new_settings.node_min_x >= ( 1u << new_settings.node_x_bits ) )
					new_settings.node_x_bits = log2_rounded ( new_max_x - new_settings.node_min_x + 1 );
				if ( _nodes[_node_count].y < new_settings.node_min_y )
					new_settings.node_min_y = _nodes[_node_count].y;
				if ( _nodes[_node_count].y > new_max_y )
					new_max_y = _nodes[_node_count].y;
				if ( new_max_y - new_settings.node_min_y >= ( 1u << new_settings.node_y_bits ) )
					new_settings.node_y_bits = log2_rounded ( new_max_y - new_settings.node_min_y + 1 );
			}

			unsigned edge_bits = 0;
			for ( unsigned i = edges_begin; i < edges_end; i++ ) {
				while ( _edges[i].weight >= ( 1u << ( new_settings.long_weight_bits - 1 ) ) ) {
					new_settings.long_weight_bits++;
					bit_increase = true;
				}
				const unsigned weight_bits = _edges[i].weight >= ( 1u << ( new_settings.short_weight_bits - 1 ) ) ? new_settings.long_weight_bits : new_settings.short_weight_bits;
				if ( _edges[i].target >= _first_node )
					edge_bits += log2_rounded ( _node_count ) + 1 + 1 + 1 + weight_bits ;
				else {
					edge_bits += new_settings.internal_bits + new_settings.adj_block_bits + 1 + 1 + 1 + weight_bits;
					const unsigned target_block = _block_map[_edges[i].target].block;
					if ( _adj_blocks.find ( target_block ) == _adj_blocks.end() && new_adj_blocks.find ( target_block ) == new_adj_blocks.end() ) {
						new_adj_blocks[target_block] = 0;
						if ( new_adj_blocks.size() + _adj_blocks.size() == ( 1u << new_settings.adj_block_bits ) ) {
							new_settings.adj_block_bits++;
							bit_increase = true;
						}
					}
				}
				if ( PATH_DATA ) {
					if ( _edges[i].shortcut ) {
						edge_bits += 32;
						if ( PRE_UNPACK && DYNAMIC_DATA )
							edge_bits += 32;
					} else
						edge_bits++;
				}
				if ( DYNAMIC_DATA ) {
					edge_bits += 1 + ( _witness_index[i + 1] - _witness_index[i] ) * 32;
					for ( unsigned j = _witness_index[i]; j < _witness_index[i + 1]; j++ ){
						const unsigned safety_bits = _witness_edges[j].safety >= ( 1u << ( new_settings.short_weight_bits - 1 ) ) ? new_settings.long_weight_bits : new_settings.short_weight_bits;
						edge_bits+= safety_bits;
						while ( _witness_edges[j].safety >= ( 1u << ( new_settings.long_weight_bits - 1 ) ) ) {
							new_settings.long_weight_bits++;
							bit_increase = true;
						}
					}
				}
			}
			new_edge_size += ( edge_bits + 7 ) / 8;

			if ( bit_increase )
				new_edge_size = reevalueate_edges ( new_settings, _node_count + 1 );

			while ( new_edge_size + 1 >= ( 1u << new_settings.first_edge_bits ) )
				new_settings.first_edge_bits++;

			unsigned new_first_edge_size = ( ( _node_count + 2 ) * new_settings.first_edge_bits + 7 ) / 8;
			const unsigned new_adj_blocks_size = ( ( unsigned ) ( new_adj_blocks.size() + _adj_blocks.size() ) * new_settings.block_bits + 7 ) / 8;
			unsigned new_node_x_size = 0;
			unsigned new_node_y_size = 0;
			if ( PATH_DATA ) {
				new_node_x_size = ( ( _node_count + 1 ) * new_settings.node_x_bits + 7 ) / 8;
				new_node_y_size = ( ( _node_count + 1 ) * new_settings.node_y_bits + 7 ) / 8;
			}


			if ( _base_size + new_edge_size + new_adj_blocks_size + new_first_edge_size + new_node_x_size + new_node_y_size > _max_size ) {
				if ( !bit_increase )
					new_edge_size = reevalueate_edges ( new_settings, _node_count + 1 );
				while ( new_edge_size + 1 >= ( 1u << new_settings.first_edge_bits ) )
					new_settings.first_edge_bits++;

				new_first_edge_size = ( ( _node_count + 2 ) * new_settings.first_edge_bits + 7 ) / 8;
			}


			if ( _base_size + new_edge_size + new_adj_blocks_size + new_first_edge_size + new_node_x_size + new_node_y_size > _max_size ) {
				if ( _node_count == 0 ) {
					qCritical( "ERROR: A node requires more space than a single block can suffice (block %d, node %d )", _block_id, node );
					qCritical( "_base_size: %d", _base_size );
					qCritical( "new_edge_size: %d", new_edge_size );
					qCritical( "new_adj_blocks_size: %d", new_adj_blocks_size );
					qCritical( "new_first_edge_size: %d", new_first_edge_size );
					qCritical( "_max_size: %d", _max_size );
					exit ( -1 );
				}
				const unsigned edges_begin = _nodes[0].first_edge;
				const unsigned edges_end = _nodes[_node_count].first_edge;
				for ( unsigned i = edges_begin; i < edges_end; i++ )
					if ( _edges[i].shortcut ) {
					if ( _edges[i].internal_shortcut ) {
						if ( _edges[i].middle - _first_node < _node_count )
							_edges[i].internal_shortcut = true;
						else
							_edges[i].internal_shortcut = false;
					}
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
			_edge_size = new_edge_size;
			if ( PATH_DATA ) {
				_node_x_size = new_node_x_size;
				_node_y_size = new_node_y_size;
				_max_x = new_max_x;
				_max_y = new_max_y;
			}
			_settings = new_settings;
			global_settings.internal_bits = new_settings.internal_bits;
			_node_count++;
			return true;
		}

		void store ( unsigned char* buffer, const _str_global_settings& global_settings, _str_statistics& stats ) {
			const unsigned edge_count = _nodes[_node_count].first_edge - _nodes[0].first_edge;
			const unsigned char* old_buffer = buffer;

			//#nodes + #adjBlocks
			* ( ( unsigned* ) buffer ) = _node_count;
			buffer += sizeof ( unsigned );
			* ( ( unsigned* ) buffer ) = ( unsigned ) _adj_blocks.size();
			buffer += sizeof ( unsigned );
			* ( ( _str_block_settings* ) buffer ) = _settings;
			buffer += sizeof ( _str_block_settings );

			assert ( buffer - old_buffer == _base_size );

			//#nodes+1 firstEdges
			unsigned* temp_first_edges = new unsigned[_node_count + 10];
			unsigned edge_pointer = 0;
			for ( unsigned i = 0, edge = _nodes[0].first_edge; i <= _node_count; i++ ) {
				unsigned bits_used = 0;
				while ( edge != _nodes[i].first_edge ) {
					const unsigned weight_bits = _edges[edge].weight >= ( 1u << ( _settings.short_weight_bits - 1 ) ) ? _settings.long_weight_bits : _settings.short_weight_bits;
					stats.weight_space_bits += weight_bits;
					if ( _edges[edge].target >= _first_node ) {
						bits_used += log2_rounded ( i - 1 ) + 1 + 1 + 1 + weight_bits;
						stats.internal_space_bits += log2_rounded ( i - 1 ) + 1 + 1 + 1;
					} else {
						bits_used += _settings.internal_bits + _settings.adj_block_bits + 1 + 1 + 1 + weight_bits;
						stats.external_space_bits += _settings.internal_bits + _settings.adj_block_bits + 1 + 1 + 1;
					}
					if ( PATH_DATA ) {
						if ( _edges[edge].shortcut ) {
							if ( _edges[edge].internal_shortcut ) {
								bits_used += 1 + 1 + _settings.internal_bits;
								stats.internal_shortcut_space_bits += 1 + 1 + _settings.internal_bits;
								stats.internal_shortcuts++;
							} else {
								bits_used += 32;
								stats.external_shortcut_space_bits += 32;
								stats.external_shortcuts++;
								if ( PRE_UNPACK && DYNAMIC_DATA ) {
									bits_used += 32;
									stats.external_shortcut_space_bits += 32;
								}
							}
						} else
							bits_used++;
					}
					if ( DYNAMIC_DATA ) {
						bits_used += 1 + ( _witness_index[edge + 1] - _witness_index[edge] ) * 32;
						for ( unsigned j = _witness_index[edge]; j < _witness_index[edge + 1]; j++ )
							bits_used += _witness_edges[j].safety >= ( 1u << ( _settings.short_weight_bits - 1 ) ) ? _settings.long_weight_bits : _settings.short_weight_bits;
					}
					edge++;
				}

				edge_pointer += ( bits_used + 7 ) / 8;

				temp_first_edges[i] = edge_pointer;
				assert ( edge_pointer <= _edge_size );
			}
			temp_first_edges[_node_count] = edge_pointer;
			buffer += _packed_first_edge_data::pack ( buffer, temp_first_edges, _node_count + 1, _settings );

			assert ( buffer - old_buffer == _base_size + _first_edge_size );

			//adjBlocks
			unsigned* temp_adj_blocks = new unsigned[_adj_blocks.size() + 1];
			for ( std::map<unsigned, unsigned>::const_iterator i = _adj_blocks.begin(), e = _adj_blocks.end() ; i != e; i++ )
				temp_adj_blocks[i->second] = i->first;
			std::sort ( temp_adj_blocks, temp_adj_blocks + _adj_blocks.size() );
			for ( unsigned i = 0, e = _adj_blocks.size(); i < e; i++ )
				_adj_blocks[temp_adj_blocks[i]] = i;
			buffer += _packed_adj_block_data::pack ( buffer, temp_adj_blocks, ( unsigned ) _adj_blocks.size(), _settings );

			assert ( buffer - old_buffer == _base_size + _first_edge_size + _adj_blocks_size );

			//node_coordinates
			if ( PATH_DATA ) {
				unsigned* temp_node_x = new unsigned[_node_count];
				for ( unsigned i = 0; i < _node_count; i++ )
					temp_node_x[i] = _nodes[i].x - _settings.node_min_x;
				buffer += _packed_node_x::pack ( buffer, temp_node_x, _node_count, _settings );
				delete[] temp_node_x;

				unsigned* temp_node_y = new unsigned[_node_count];
				for ( unsigned i = 0; i < _node_count; i++ )
					temp_node_y[i] = _nodes[i].y - _settings.node_min_y;
				buffer += _packed_node_y::pack ( buffer, temp_node_y, _node_count, _settings );
				delete[] temp_node_y;
			}

			assert ( buffer - old_buffer == _base_size + _first_edge_size + _adj_blocks_size + _node_x_size + _node_y_size );
			//block for asserting data integrity
			_logical_block test_block;
			test_block.load ( old_buffer );
			test_block.id = _block_id;

			//firstEdge[#nodes] edges
			for ( unsigned i = 0; i < _node_count; i++ ) {
				unsigned edge_position = temp_first_edges[i] << 3;
				for ( unsigned j = _nodes[i].first_edge; j < _nodes[i + 1].first_edge; j++ ) {
					const _str_builder_edge& edge = _edges[j];
					unsigned size;
					_str_edge_data data;
					data.path = ( vertex_descriptor ) 0;
					data.middle = ( vertex_descriptor ) 0;
					data.weight = edge.weight;
					data.forward = edge.forward;
					data.backward = edge.backward;
					data.shortcut = edge.shortcut;
					data.path_reversed = edge.path_reversed;
					if ( data.shortcut ) {
						if ( PRE_UNPACK && !edge.internal_shortcut )
							data.path = ( vertex_descriptor ) edge.path;
						else
							data.middle.set ( _block_map[edge.middle].block, _block_map[edge.middle].node, global_settings );
						if ( DYNAMIC_DATA && edge.shortcut )
							data.middle.set ( _block_map[edge.middle].block, _block_map[edge.middle].node, global_settings );
					}
					if ( DYNAMIC_DATA ) {
						data.witness_count = _witness_index[j + 1] - _witness_index[j];
						for ( unsigned k = _witness_index[j], position = 0; k < _witness_index[j + 1]; k++ ) {
							vertex_descriptor temp;
							temp.set ( _block_map[_witness_edges[k].affected].block, _block_map[_witness_edges[k].affected].node, global_settings );
							data.witness_affected[position].affected = ( unsigned ) temp;
							data.witness_affected[position++].safety = _witness_edges[k].safety;
						}
					}
					if ( edge.target >= _first_node ) {
						data.target.set ( _block_id, _block_map[edge.target].node, global_settings );
						size = _packed_edge::pack ( i, buffer, edge_position, false, !edge.internal_shortcut,  data, global_settings, _settings );
						stats.internal_edges++;
					} else {
						data.target.set ( _adj_blocks[_block_map[edge.target].block], _block_map[edge.target].node, global_settings );
						size = _packed_edge::pack ( i, buffer, edge_position, true, !edge.internal_shortcut, data, global_settings, _settings );
					}
					_str_edge_data temp;
					assert ( size == _packed_edge::unpack ( i, edge_position, data, &test_block, global_settings ) );
					edge_position += size;
				}
				assert ( ( edge_position + 7 ) / 8 == temp_first_edges[i + 1] );
			}
			buffer += temp_first_edges[_node_count];

			assert ( buffer - old_buffer <= _base_size + _first_edge_size + _edge_size + _adj_blocks_size + _node_x_size + _node_y_size );

			stats.first_edge_space += _first_edge_size;
			stats.adj_blocks_space += _adj_blocks_size;
			stats.node_x_space += _node_x_size;
			stats.node_y_space += _node_y_size;
			if ( _node_count > stats.max_nodes )
				stats.max_nodes = _node_count;
			if ( edge_count > stats.max_edges )
				stats.max_edges = edge_count;
			if ( _adj_blocks.size() > stats.max_adj_blocks )
				stats.max_adj_blocks = _adj_blocks.size();
			stats.adj_blocks += _adj_blocks.size();

			unsigned min_x = _nodes[0].x, max_x = _nodes[0].x;
			unsigned min_y = _nodes[0].y, max_y = _nodes[0].y;
			for ( unsigned i = 1; i < _node_count; i++ ) {
				if ( _nodes[i].x > max_x )
					max_x = _nodes[i].x;
				if ( _nodes[i].x < min_x )
					min_x = _nodes[i].x;
				if ( _nodes[i].y > max_y )
					max_y = _nodes[i].y;
				if ( _nodes[i].y < min_y )
					min_y = _nodes[i].y;
			}
			if ( max_x - min_x > stats.max_x_diff )
				stats.max_x_diff = max_x - min_x;
			if ( max_y - min_y > stats.max_y_diff )
				stats.max_y_diff = max_y - min_y;

#ifndef NDEBUG
			//unpack block to test data integrity
			assert ( _settings.adj_block_bits == test_block.settings.adj_block_bits );
			assert ( _settings.block_bits == test_block.settings.block_bits );
			assert ( _settings.first_edge_bits == test_block.settings.first_edge_bits );
			assert ( _settings.internal_bits == test_block.settings.internal_bits );
			assert ( _settings.long_weight_bits == test_block.settings.long_weight_bits );
			assert ( _settings.short_weight_bits == test_block.settings.short_weight_bits );
			assert ( _base_size == test_block.first_edges - old_buffer );
			assert ( _base_size + _first_edge_size == test_block.adj_blocks - old_buffer );
			assert ( _base_size + _first_edge_size + _adj_blocks_size  + _node_x_size + _node_y_size == test_block.edges - old_buffer );
			for ( unsigned i = 0; i <= _node_count; i++ ) {
				assert ( _packed_first_edge_data::unpack ( test_block, i ) == temp_first_edges[i] );
			}
			for ( unsigned i = 0, e = ( unsigned ) _adj_blocks.size(); i < e; i++ ) {
				assert ( _packed_adj_block_data::unpack ( test_block, i ) == temp_adj_blocks[i] );
			}

			for ( unsigned i = 0; i < _node_count; i++ ) {
				if ( PATH_DATA ) {
					assert ( _nodes[i].x == _packed_node_x::unpack ( test_block, i ) );
					assert ( _nodes[i].y == _packed_node_y::unpack ( test_block, i ) );
				}
				unsigned edge_position = temp_first_edges[i] << 3;
				for ( unsigned j = _nodes[i].first_edge; j < _nodes[i + 1].first_edge; j++ ) {
					_str_edge_data data;
					data.path = ( vertex_descriptor ) 0;
					data.middle = ( vertex_descriptor ) 0;
					if ( PRE_UNPACK ) {
						data.unpacked = false;
						data.path_reversed = false;
					}
					const _str_builder_edge& edge = _edges[j];
					const unsigned size = _packed_edge::unpack ( i, edge_position, data, &test_block, global_settings );
					assert ( data.forward == edge.forward );
					assert ( data.backward == edge.backward );
					assert ( data.target.get_block ( global_settings ) == _block_map[edge.target].block );
					assert ( data.target.get_node ( global_settings ) == _block_map[edge.target].node );
					if ( PATH_DATA ) {
						assert ( data.shortcut == edge.shortcut );
						if ( data.shortcut ) {
							if ( PRE_UNPACK ) {
								assert ( data.unpacked == edge.path_included );
								if ( data.unpacked ) {
									assert ( data.path_reversed == edge.path_reversed );
									assert ( data.path == edge.path );
								} else {
									assert ( data.middle.get_block ( global_settings ) == _block_map[edge.middle].block );
									assert ( data.middle.get_node ( global_settings ) == _block_map[edge.middle].node );
								}
							}
							if ( DYNAMIC_DATA || !PRE_UNPACK ) {
								assert ( data.middle.get_block ( global_settings ) == _block_map[edge.middle].block );
								assert ( data.middle.get_node ( global_settings ) == _block_map[edge.middle].node );
							}
						}
					}
					if ( DYNAMIC_DATA ) {
						assert ( data.witness_count == _witness_index[j + 1] - _witness_index[j] );
						for ( unsigned k = _witness_index[j], position = 0; k < _witness_index[j + 1]; k++ ) {
							vertex_descriptor temp;
							temp.set ( _block_map[_witness_edges[k].affected].block, _block_map[_witness_edges[k].affected].node, global_settings );
							assert ( ( unsigned ) temp == data.witness_affected[position].affected );
							assert ( _witness_edges[k].safety == data.witness_affected[position++].safety );
						}
					}
					assert ( data.weight == edge.weight );
					edge_position += size;
				}
				assert ( ( edge_position + 7 ) / 8 == temp_first_edges[i + 1] );
			}
#endif

			delete[] temp_first_edges;
			delete[] temp_adj_blocks;
			std::map<unsigned, unsigned>().swap( _adj_blocks );

		}

			private:

		unsigned _block_id;
		_str_block_settings _settings;
		unsigned _max_size, _base_size, _edge_size, _first_edge_size, _adj_blocks_size, _node_x_size, _node_y_size;
		unsigned _max_x, _max_y;
		_str_builder_map* _block_map;
		const _str_builder_node* _nodes;
		const unsigned* _witness_index;
		const _str_witness* _witness_edges;
		_str_builder_edge* _edges;
		unsigned _first_node, _node_count;
		std::map< unsigned, unsigned > _adj_blocks;
	};
#endif

#ifndef MOBILE_VERSION
	class _BlockBuilder {
	public:

		_BlockBuilder ( unsigned node_count, _str_builder_edge* edges, const _str_builder_node* nodes, const unsigned* witness_index, const _str_witness* witness_edges, unsigned block_size )
			: _node_count ( node_count ),
			_nodes ( nodes ),
			_edges ( edges ),
			_witness_index ( witness_index ),
			_witness_edges ( witness_edges ),
			_block_map ( new _str_builder_map[node_count] ),
			_block_size ( block_size ) {
		}

		~_BlockBuilder() {
			delete[] _block_map;
		}

		int build ( unsigned char* buffer, _str_global_settings& settings ) {

			std::vector<_logical_block_builder> blocks;

			blocks.push_back ( _logical_block_builder ( 0, _block_size, _block_map, _edges, _nodes, _witness_index, _witness_edges, settings ) );

			if ( buffer != NULL )
				qDebug( "importing: " );
			else
				qDebug( "calculating size: " );
			for ( unsigned i = 0; i < _node_count; i++ ) {
				if ( !blocks.back().add_node ( i, settings ) ) {
					blocks.push_back ( _logical_block_builder ( ( unsigned ) blocks.size(), _block_size, _block_map, _edges, _nodes + i, _witness_index, _witness_edges,  settings ) );
					i--;
				}
			}

			if ( buffer == NULL )
				return ( int ) blocks.size();

			qDebug( "packing: " );
			_str_statistics stats;
			for ( unsigned i = 0, e = ( unsigned ) blocks.size(); i < e; i++ ) {
				blocks[i].store ( buffer, settings, stats );
				buffer += _block_size;
			}

			qDebug( "Maximum nodes per block: %d", stats.max_nodes );
			qDebug( "Maximum edges per block: %d", stats.max_edges );
			qDebug( "Maximum adjacent blocks per block: %d", stats.max_adj_blocks );
			qDebug( "Nodes per Block: %lf", 1.0f * _node_count / ( double ) blocks.size() );
			qDebug( "Adjacent blocks per Block: %lf", 1.0f *stats.adj_blocks / ( double ) blocks.size() );
			qDebug( "Internal edges: %lf Percent", 100.0f * stats.internal_edges / _nodes[_node_count].first_edge );
			qDebug( "Internal Shortcuts: %lf Percent", 100.0f * stats.internal_shortcuts / _nodes[_node_count].first_edge );
			qDebug( "External Shortcuts: %lf Percent", 100.0f * stats.external_shortcuts / _nodes[_node_count].first_edge );
			qDebug( "Space (Weight): %d Mb / %d Mb", stats.weight_space_bits / 8 / 1024 / 1024, _nodes[_node_count].first_edge * 4 / 1024 / 1024 );
			qDebug( "Space (First Edge): %d Mb / %d Mb", stats.first_edge_space / 1024 / 1024, _node_count * 4 / 1024 / 1024 );
			qDebug( "Space (External Edges + Adj. Blocks): %d Mb / %d Mb", ( stats.external_space_bits + stats.adj_blocks_space ) / 8 / 1024 / 1024, ( _nodes[_node_count].first_edge - stats.internal_edges ) * 4 / 1024 / 1024 );
			qDebug( "Space (Internal Edges): %d Mb / %d Mb", stats.internal_space_bits / 8 / 1024 / 1024, stats.internal_edges * 4 / 1024 / 1024 );
			if ( PATH_DATA ) {
				qDebug( "Max x Diff: %d", stats.max_x_diff );
				qDebug( "Max y Diff: %d", stats.max_y_diff );
				qDebug( "Space (X Coordinate): %d Mb / %d Mb", stats.node_x_space / 1024 / 1024, _node_count * 8 / 1024 / 1024 );
				qDebug( "Space (Y Coordinate): %d Mb / %d Mb", stats.node_y_space / 1024 / 1024, _node_count * 8 / 1024 / 1024 );
				qDebug( "Space (External Shortcuts): %d Mb / %d Mb", stats.external_shortcut_space_bits / 8 / 1024 / 1024, stats.external_shortcuts * 4 / 1024 / 1024 );
				qDebug( "Space (Internal Shortcuts): %d Mb / %d Mb", stats.internal_shortcut_space_bits / 8 / 1024 / 1024, stats.internal_shortcuts * 4 / 1024 / 1024 );
			}

			return ( int ) blocks.size();
		}

		unsigned _node_count;
		const _str_builder_node* _nodes;
		_str_builder_edge* _edges;
		const unsigned* _witness_index;
		const _str_witness* _witness_edges;
		_str_builder_map* _block_map;
		unsigned _block_size;
	};
#endif

};

#endif // COMPRESSEDCHGRAPH_H_INCLUDED
