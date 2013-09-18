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

#include "MapFileHeader.h"

#include "MapFileInfo.h"
#include "SubFileParameter.h"

MapFileHeader::MapFileHeader() :
    m_mapFileInfo(),
    m_subFileParameters(),
	m_zoomLevelMaximum(std::numeric_limits<byte>::min()),
	m_zoomLevelMinimum(std::numeric_limits<byte>::max())
{
}

byte MapFileHeader::getQueryZoomLevel(byte zoomLevel) const
{
	if (zoomLevel > m_zoomLevelMaximum) {
		return m_zoomLevelMaximum;
	} else if (zoomLevel < m_zoomLevelMinimum) {
		return m_zoomLevelMinimum;
	}
	return zoomLevel;
}

SubFileParameter MapFileHeader::getSubFileParameter(int queryZoomLevel)
{
	return m_subFileParameters[queryZoomLevel];
}

ReadBuffer &operator>>(ReadBuffer &readBuffer, MapFileHeader &header)
{
	static const QString BINARY_OSM_MAGIC_BYTE = "mapsforge binary OSM";
	static const int HEADER_SIZE_MIN = 70;
	static const int HEADER_SIZE_MAX = 1000000;

	// read the the magic byte and the file header size into the buffer
	const int magicByteLength = BINARY_OSM_MAGIC_BYTE.length();
	if (!readBuffer.readFromFile(magicByteLength + 4)) {
		readBuffer.setError("reading magic byte has failed");
	}

	// get and check the magic byte
	const QString magicByte = readBuffer.readUTF8EncodedString(magicByteLength);
	if (BINARY_OSM_MAGIC_BYTE != magicByte) {
		readBuffer.setError("invalid magic byte: " + magicByte);
	}

	// get and check the size of the remaining file header (4 bytes)
	const int remainingHeaderSize = readBuffer.readInt();
	if (remainingHeaderSize < HEADER_SIZE_MIN || remainingHeaderSize > HEADER_SIZE_MAX) {
		readBuffer.setError(QString("invalid remaining header size: %1").arg(remainingHeaderSize));
	}

	// read the header data into the buffer
	if (!readBuffer.readFromFile(remainingHeaderSize)) {
		readBuffer.setError(QString("reading header data has failed: %1").arg(remainingHeaderSize));
	}

	readBuffer >> header.m_mapFileInfo;

	QVector<SubFileParameter> tempSubFileParameters(header.m_mapFileInfo.numberOfSubFiles());
	header.m_zoomLevelMinimum = std::numeric_limits<byte>::max();
	header.m_zoomLevelMaximum = std::numeric_limits<byte>::min();

	// get and check the information for each sub-file
	for (byte currentSubFile = 0; currentSubFile < header.m_mapFileInfo.numberOfSubFiles(); ++currentSubFile) {
		const SubFileParameter subFileParameter = SubFileParameter::fromReadBuffer(readBuffer, header.m_mapFileInfo.debugFile());

		// add the current sub-file to the list of sub-files
		tempSubFileParameters[currentSubFile] = subFileParameter;

		// update the global minimum and maximum zoom level information
		if (header.m_zoomLevelMinimum > subFileParameter.zoomLevelMin()) {
			header.m_zoomLevelMinimum = subFileParameter.zoomLevelMin();
		}
		if (header.m_zoomLevelMaximum < subFileParameter.zoomLevelMax()) {
			header.m_zoomLevelMaximum = subFileParameter.zoomLevelMax();
		}
	}

	// create and fill the lookup table for the sub-files
	header.m_subFileParameters = QVector<SubFileParameter>(header.m_zoomLevelMaximum + 1);
	for (int currentMapFile = 0; currentMapFile < header.m_mapFileInfo.numberOfSubFiles(); ++currentMapFile) {
		SubFileParameter subFileParameter = tempSubFileParameters[currentMapFile];
		for (byte zoomLevel = subFileParameter.zoomLevelMin(); zoomLevel <= subFileParameter.zoomLevelMax(); ++zoomLevel) {
			header.m_subFileParameters[zoomLevel] = subFileParameter;
		}
	}

	return readBuffer;
}
