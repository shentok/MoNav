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

#ifndef TABLE_H
#define TABLE_H

#include <QtGlobal>
#include <QCache>
#include <QFile>
#include <string.h>

namespace gg {

	template< class T, int size >
	struct IndexTable {
		T index[size * size];
		IndexTable()
		{
			for ( int i = 0; i < size * size; i++ )
				index[i] = -1;
		}
		IndexTable( const char* buffer )
		{
			Read( buffer );
		}

		T GetIndex( int x, int y ) {
			return index[x + y * size];
		}

		static size_t Size()
		{
			return size * size * sizeof( T );
		}

		void Write( char* buffer )
		{
			memcpy( buffer, index, size * size * sizeof( T ) );
		}

		void Read( const char* buffer )
		{
			memcpy( index, buffer, size * size * sizeof( T ) );
		}
	};

	class Index {

	public:
		Index( QString filename ) :
				file2( filename + "_2" ), file3( filename + "_3" )
		{
			QFile file1( filename + "_1" );
			file1.open( QIODevice::ReadOnly );
			top.Read( file1.read( top.Size() ).constData() );
			file2.open( QIODevice::ReadOnly );
			file3.open( QIODevice::ReadOnly );
		}

		qint64 GetIndex( int x, int y ) {
			int topx = x / 32 / 32;
			int topy = y / 32 / 32;
			int middle = top.GetIndex( topx, topy );
			if ( middle == -1 )
				return -1;

			int middlex = ( x / 32 ) % 32;
			int middley = ( y / 32 ) % 32;
			if ( !cache2.contains( middle ) ) {
				IndexTable< int, 32 >* newEntry;
				file2.seek( middle * newEntry->Size() );
				newEntry = new IndexTable< int, 32 >( file2.read( newEntry->Size() ) );
				cache2.insert( middle, newEntry );
			}
			if ( !cache2.contains( middle ) )
				return -1;
			IndexTable< int, 32 >* middleTable = cache2.object( middle );
			int bottom = middleTable->GetIndex( middlex, middley );
			if ( bottom == -1 )
				return -1;

			int bottomx = x % 32;
			int bottomy = y % 32;
			if ( !cache3.contains( bottom ) ) {
				IndexTable< qint64, 32 >* newEntry;
				file3.seek( bottom * newEntry->Size() );
				newEntry = new IndexTable< qint64, 32 >( file3.read( newEntry->Size() ) );
				cache3.insert( bottom, newEntry );
			}
			if ( !cache3.contains( bottom ) )
				return -1;
			IndexTable< qint64, 32 >* bottomTable = cache3.object( bottom );
			qint64 position = bottomTable->GetIndex( bottomx, bottomy );
			return position;
		}

		void SetCacheSize( int size )
		{
			assert( size > 0 );
			cache2.setMaxCost( size );
			cache3.setMaxCost( size );
		}

	private:
		QFile file2;
		QFile file3;
		IndexTable< int, 32 > top;
		QCache< int, IndexTable< int, 32 > > cache2;
		QCache< int, IndexTable< qint64, 32 > > cache3;
	};
}

#endif // TABLE_H
