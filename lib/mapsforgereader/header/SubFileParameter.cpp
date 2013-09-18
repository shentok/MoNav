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

#include "SubFileParameter.h"

#include "../ReadBuffer.h"

#include <limits>

SubFileParameter::SubFileParameter() :
	m_startAddress(),
	m_indexStartAddress(),
	m_subFileSize(),
	m_physicalZoomLevel(),
	m_zoomLevelMin(std::numeric_limits<byte>::max()),
	m_zoomLevelMax(std::numeric_limits<byte>::min())
{
}

SubFileParameter SubFileParameter::fromReadBuffer(ReadBuffer &readBuffer, bool isDebugFile)
{
	// get and check the base zoom level (1 byte)
	const byte physicalZoomLevel = readBuffer.readByte();
	if (physicalZoomLevel < 0 || physicalZoomLevel > PHYSICAL_ZOOM_LEVEL_MAX) {
		readBuffer.setError(QString("invalid base zooom level: %1").arg(physicalZoomLevel));
	}

	// get and check the minimum zoom level (1 byte)
	const byte zoomLevelMin = readBuffer.readByte();
	if (zoomLevelMin < 0 || zoomLevelMin > 22) {
		readBuffer.setError(QString("invalid minimum zoom level: %1").arg(zoomLevelMin));
	}

	// get and check the maximum zoom level (1 byte)
	const byte zoomLevelMax = readBuffer.readByte();
	if (zoomLevelMax < 0 || zoomLevelMax > 22) {
		readBuffer.setError(QString("invalid maximum zoom level: %1").arg(zoomLevelMax));
	}

	// check for valid zoom level range
	if (zoomLevelMin > zoomLevelMax) {
		readBuffer.setError(QString("invalid zoom level range: %1 .. %2").arg(zoomLevelMin).arg(zoomLevelMax));
	}

	// get and check the start address of the sub-file (8 bytes)
	const qint64 startAddress = readBuffer.readLong();
	if (startAddress < HEADER_SIZE_MIN || startAddress >= readBuffer.fileSize()) {
		readBuffer.setError(QString("invalid start address: %1").arg(startAddress));
	}

	const qint64 indexStartAddress = startAddress + (isDebugFile ? SIGNATURE_LENGTH_INDEX : 0);

	// get and check the size of the sub-file (8 bytes)
	const qint64 subFileSize = readBuffer.readLong();
	if (subFileSize < 1) {
		readBuffer.setError(QString("invalid sub-file size: %1").arg(subFileSize));
	}

	if (readBuffer.hasError()) {
		return SubFileParameter();
	}

	SubFileParameter subFileParameter;
	subFileParameter.m_physicalZoomLevel = physicalZoomLevel;
	subFileParameter.m_zoomLevelMin = zoomLevelMin;
	subFileParameter.m_zoomLevelMax = zoomLevelMax;
	subFileParameter.m_startAddress = startAddress;
	subFileParameter.m_indexStartAddress = indexStartAddress;
	subFileParameter.m_subFileSize = subFileSize;

	return subFileParameter;
}

bool SubFileParameter::operator==(const SubFileParameter &other) const
{
	if (m_startAddress != other.m_startAddress) {
		return false;
	} else if (m_subFileSize != other.m_subFileSize) {
		return false;
	} else if (m_physicalZoomLevel != other.m_physicalZoomLevel) {
		return false;
	}
	return true;
}

int qHash(const SubFileParameter &value) {
	int result = 7;
	result = 31 * result + (int) (value.m_startAddress ^ (value.m_startAddress >> 32));
	result = 31 * result + (int) (value.m_subFileSize ^ (value.m_subFileSize >> 32));
	result = 31 * result + value.m_physicalZoomLevel;
	return result;
}
