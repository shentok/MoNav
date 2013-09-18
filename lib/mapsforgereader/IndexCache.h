/*
 * Copyright 2010, 2011, 2012 mapsforge.org
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAPSFORGE_INDEXCACHE_H
#define MAPSFORGE_INDEXCACHE_H

#include "header/SubFileParameter.h"

#include <QCache>

/**
 * A cache for database index blocks with a fixed size and LRU policy.
 */
class IndexCache
{
	/**
	 * An immutable container class which is the key for the index cache.
	 */
	class IndexCacheEntryKey
	{
	public:
		/**
		 * Creates an immutable key to be stored in a map.
		 * 
		 * @param subFileParameter
		 *            the parameters of the map file.
		 * @param indexBlockNumber
		 *            the number of the index block.
		 */
		IndexCacheEntryKey(const SubFileParameter &subFileParameter, long indexBlockNumber) :
			indexBlockNumber(indexBlockNumber),
			subFileParameter(subFileParameter)
		{
		}

		bool operator==(const IndexCacheEntryKey &other) const
		{
			if (subFileParameter != other.subFileParameter) {
				return false;
			} else if (indexBlockNumber != other.indexBlockNumber) {
				return false;
			}
			return true;
		}

	private:
		friend int qHash(const IndexCacheEntryKey &);
		long indexBlockNumber;
		SubFileParameter subFileParameter;
	};

public:
	/**
	 * @param randomAccessFile
	 *            the map file from which the index should be read and cached.
	 * @param capacity
	 *            the maximum number of entries in the cache.
	 * @throws IllegalArgumentException
	 *             if the capacity is negative.
	 */
	IndexCache(QIODevice *randomAccessFile, int capacity);

	/**
	 * Destroy the cache at the end of its lifetime.
	 */
	~IndexCache();

	/**
	 * Returns the index entry of a block in the given map file. If the required index entry is not cached, it will be
	 * read from the map file index and put in the cache.
	 * 
	 * @param subFileParameter
	 *            the parameters of the map file for which the index entry is needed.
	 * @param blockNumber
	 *            the number of the block in the map file.
	 * @return the index entry.
	 * @throws IOException
	 *             if an I/O error occurs during reading.
	 */
	quint64 getIndexEntry(const SubFileParameter &subFileParameter, quint64 blockNumber, quint64 numberOfBlocks);

private:
	/**
	 * Converts five bytes of a byte array to a quint64.
	 * <p>
	 * The byte order is big-endian.
	 *
	 * @param buffer
	 *            the byte array.
	 * @param offset
	 *            the offset in the array.
	 * @return the long value.
	 */
	static qint64 getFiveBytesLong(const QByteArray &buffer, int offset);

private:
	friend int qHash(const IndexCacheEntryKey &);

	/**
	 * Number of index entries that one index block consists of.
	 */
	static const int INDEX_ENTRIES_PER_BLOCK = 128;

	/**
	 * Number of bytes a single index entry consists of.
	 */
	static const byte BYTES_PER_INDEX_ENTRY = 5;

	/**
	 * Maximum size in bytes of one index block.
	 */
	static const int SIZE_OF_INDEX_BLOCK;

	QIODevice *const m_device;
	QCache<IndexCacheEntryKey, QByteArray> m_cache;
};

int qHash(const IndexCache::IndexCacheEntryKey &key);

#endif
