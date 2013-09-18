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

#include "IndexCache.h"

#include <QCache>
#include <QDebug>

const int IndexCache::SIZE_OF_INDEX_BLOCK = IndexCache::INDEX_ENTRIES_PER_BLOCK * IndexCache::BYTES_PER_INDEX_ENTRY;

IndexCache::IndexCache(QIODevice *randomAccessFile, int capacity) :
	m_device(randomAccessFile)
{
	m_cache.setMaxCost(capacity);
}

IndexCache::~IndexCache()
{
	m_cache.clear();
}

quint64 IndexCache::getIndexEntry(const SubFileParameter &subFileParameter, quint64 blockNumber, quint64 numberOfBlocks)
{
	// check if the block number is out of bounds
	Q_ASSERT(blockNumber < numberOfBlocks && QString("invalid block number: %1").arg(blockNumber).toAscii().constData());
#if 0
	if (blockNumber >= numberOfBlocks) {
		throw new IOException("invalid block number: " + blockNumber);
	}
#endif

	// calculate the index block number
	const qint64 indexBlockNumber = blockNumber / INDEX_ENTRIES_PER_BLOCK;

	// create the cache entry key for this request
	const IndexCacheEntryKey indexCacheEntryKey(subFileParameter, indexBlockNumber);

	// check for cached index block
	QByteArray *indexBlock = m_cache.object(indexCacheEntryKey);
	if (indexBlock == 0) {
		// cache miss, seek to the correct index block in the file and read it
		const qint64 indexBlockPosition = subFileParameter.indexStartAddress() + indexBlockNumber * SIZE_OF_INDEX_BLOCK;
		const qint64 indexEndAddress = subFileParameter.indexStartAddress() + numberOfBlocks * BYTES_PER_INDEX_ENTRY;

		const int remainingIndexSize = (int) (indexEndAddress - indexBlockPosition);
		const int indexBlockSize = qMin<int>(SIZE_OF_INDEX_BLOCK, remainingIndexSize);
		QByteArray arr(indexBlockSize, Qt::Uninitialized);

		Q_ASSERT(arr.size() == indexBlockSize);

		m_device->seek(indexBlockPosition);
		if (m_device->read(arr.data(), arr.size()) != arr.size()) {
			qWarning() << "could not read index block with size:" << arr.size();
		}
		else {
			indexBlock = new QByteArray(arr);
			// put the index block in the map
			m_cache.insert(indexCacheEntryKey, indexBlock);
		}
	}

	// calculate the address of the index entry inside the index block
	const qint64 indexEntryInBlock = blockNumber % INDEX_ENTRIES_PER_BLOCK;
	const int addressInIndexBlock = (int) (indexEntryInBlock * BYTES_PER_INDEX_ENTRY);

	Q_ASSERT(indexBlock != 0 && "static assert");

	// return the real index entry
	return getFiveBytesLong(*indexBlock, addressInIndexBlock);
}

qint64 IndexCache::getFiveBytesLong(const QByteArray &buffer, int offset)
{
	qint64 result = 0;
	for ( int i = 0; i < 5; ++i ) {
		result = (result << 8) | (buffer[offset + i] & 0xff);
	}

	return result;
}

int qHash(const IndexCache::IndexCacheEntryKey &key)
{
	int result = 7;
	result = 31 * result + qHash(key.subFileParameter);
	result = 31 * result + qHash(key.indexBlockNumber);
	return result;
}
