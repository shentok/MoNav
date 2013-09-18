/*
 * Copyright 2010, 2011, 2012, 2013 mapsforge.org
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

#ifndef MAPSFORGE_SUBFILEPARAMETER_H
#define MAPSFORGE_SUBFILEPARAMETER_H

#include "../BoundingBox.h"
#include "../MercatorProjection.h"
#include "../ReadBuffer.h"

#include <limits>

typedef char byte;

/**
 * Holds all parameters of a sub-file.
 */
class SubFileParameter
{
public:
	SubFileParameter();

	bool isNull() const { return m_zoomLevelMin > m_zoomLevelMax; }

	static SubFileParameter fromReadBuffer(ReadBuffer &readBuffer, bool isDebugFile);

	qint64 indexStartAddress() const { return m_indexStartAddress; }

	byte physicalZoomLevel() const { return m_physicalZoomLevel; }

	qint64 startAddress() const { return m_startAddress; }

	qint64 subFileSize() const { return m_subFileSize; }

	/**
	 * Maximum zoom level for which the block entries tables are made.
	 */
	byte zoomLevelMax() const { return m_zoomLevelMax; }

	/**
	 * Minimum zoom level for which the block entries tables are made.
	 */
	byte zoomLevelMin() const { return m_zoomLevelMin; }

	bool operator==(const SubFileParameter &other) const;

	bool operator!=(const SubFileParameter &other) const
	{
		return !(*this == other);
	}

private:
	friend int qHash(const SubFileParameter &value);

	/**
	 * Maximum valid base zoom level of a sub-file.
	 */
	static const qint32 PHYSICAL_ZOOM_LEVEL_MAX = 20;

	/**
	 * Minimum size of the file header in bytes.
	 */
	static const qint32 HEADER_SIZE_MIN = 70;

	/**
	 * Length of the debug signature at the beginning of the index.
	 */
	static const byte SIGNATURE_LENGTH_INDEX = 16;

	qint64 m_startAddress;
	qint64 m_indexStartAddress;
	qint64 m_subFileSize;
	byte m_physicalZoomLevel;
	byte m_zoomLevelMin;
	byte m_zoomLevelMax;
};

int qHash( const SubFileParameter &value );

#endif
