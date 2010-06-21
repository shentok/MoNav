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

#ifndef BLOCKCACHE_H_INCLUDED
#define BLOCKCACHE_H_INCLUDED

#include <QFile>

class BlockCache{

	public:

		void Init ( const QString& filename, unsigned cacheBlocks, unsigned blockSize ){
			_cacheBlocks = cacheBlocks;
			_blockSize = blockSize;
			_inputFile.setFileName( filename );
			_inputFile.open( QIODevice::ReadOnly | QIODevice::Unbuffered );

			//get an page aligned adress
			_cache = new char[( _cacheBlocks + 1 ) * _blockSize];
			_blocks = new _CacheBlock[_cacheBlocks];

			_firstLoaded = NONE;
			_lastLoaded = NONE;
			_loadedCount = 0;
		}

		void Destroy ( ){
			_inputFile.close();
			delete[] _cache;
			delete[] _blocks;
		}

		const unsigned char* LoadBlock ( unsigned block, unsigned* cacheID, bool* unloaded, unsigned* unloaded_id ){
			unsigned free_block = _loadedCount;
			if ( _loadedCount == _cacheBlocks ) {
				assert ( _lastLoaded != NONE );

				free_block = _lastLoaded;

				*unloaded = true;
				*unloaded_id = _blocks[_lastLoaded].id;

			}
			else{
				*unloaded = false;
				//insert into the front of the list
				_blocks[_loadedCount].previousLoaded = NONE;
				_blocks[_loadedCount].nextLoaded = _firstLoaded;
				if ( _firstLoaded != NONE )
					_blocks[_firstLoaded].previousLoaded = free_block;
				if ( _lastLoaded == NONE )
					_lastLoaded = free_block;
				_firstLoaded = free_block;
				_loadedCount++;
			}

			//load block (skip first block -> header )
			_inputFile.seek( ( block + 1 ) * _blockSize );
			_inputFile.read(_cache + free_block * _blockSize, _blockSize );
			_blocks[free_block].id = block;

			*cacheID = free_block;

			UseBlock( free_block );

			return ( unsigned char* ) ( _cache + free_block * _blockSize );
		}

		void UseBlock ( unsigned cacheID ){
			if ( _firstLoaded == cacheID )
				return;
			
			_CacheBlock& block = _blocks[cacheID];

			//remove block from the list to put it into the front
			if ( block.nextLoaded != NONE )
				_blocks[block.nextLoaded].previousLoaded = block.previousLoaded;
			else
				_lastLoaded = block.previousLoaded;

			_blocks[block.previousLoaded].nextLoaded = block.nextLoaded;

			// insert block into the front
			if ( _firstLoaded != NONE ) {
				_blocks[_firstLoaded].previousLoaded = cacheID;
				block.nextLoaded = _firstLoaded;
			}
			else {
				_lastLoaded = cacheID;
				block.nextLoaded = NONE;
			}
			block.previousLoaded = NONE;
			_firstLoaded = cacheID;

			return;
		}

	private:

		struct _CacheBlock{
			unsigned nextLoaded;
			unsigned previousLoaded;
			unsigned id;
		};

		_CacheBlock* _blocks;
		char* _cache;
		static const unsigned NONE = ( unsigned ) - 1;
		unsigned _firstLoaded;
		unsigned _lastLoaded;
		unsigned _loadedCount;
		unsigned _cacheBlocks;
		unsigned _blockSize;
		QFile _inputFile;

};

#endif // BLOCKCACHE_H_INCLUDED
