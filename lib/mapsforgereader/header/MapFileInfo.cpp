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

#include "MapFileInfo.h"

#include "../BoundingBox.h"
#include "../LatLong.h"
#include "../ReadBuffer.h"
#include "../Tag.h"
#include "../TileId.h"
#include "OptionalFields.h"

typedef char byte;

ReadBuffer &operator>>(ReadBuffer &readBuffer, MapFileInfo &mapFileInfo)
{
	static const int SUPPORTED_FILE_VERSION = 3;
	static const QString MERCATOR = "Mercator";

	// get and check the file version (4 bytes)
	const int fileVersion = readBuffer.readInt();
	if (fileVersion != SUPPORTED_FILE_VERSION) {
		readBuffer.setError(QString("unsupported file version: %1").arg(fileVersion));
	}

	// get and check the file size (8 bytes)
	const qint64 fileSize = readBuffer.readLong();
	if (fileSize != readBuffer.fileSize()) {
		readBuffer.setError(QString("invalid file size: %1").arg(fileSize));
	}

	// get and check the the map date (8 bytes)
	const qint64 mapDate = readBuffer.readLong();
	// is the map date before 2010-01-10 ?
	if (mapDate < 1200000000000LL) {
		readBuffer.setError(QString("invalid map date: %1").arg(mapDate));
	}

	BoundingBox bbox;
	readBuffer >> bbox;

	// get and check the tile pixel size (2 bytes)
	const int tilePixelSize = readBuffer.readShort();
	if (tilePixelSize < 1) {
		readBuffer.setError(QString("unsupported tile pixel size: %1").arg(tilePixelSize));
	}

	// get and check the projection name
	const QString projectionName = readBuffer.readUTF8EncodedString();
	if (projectionName != MERCATOR) {
		readBuffer.setError("unsupported projection: " + projectionName);
	}

	mapFileInfo.m_fileVersion = fileVersion;
	mapFileInfo.m_fileSize = fileSize;
	mapFileInfo.m_mapDate = mapDate;
	mapFileInfo.m_boundingBox = bbox;
	mapFileInfo.m_tilePixelSize = tilePixelSize;
	mapFileInfo.m_projectionName = projectionName;

	readBuffer >> mapFileInfo.m_optionalFields;

	readBuffer >> mapFileInfo.m_poiTags;
	readBuffer >> mapFileInfo.m_wayTags;

	// get and check the number of sub-files (1 byte)
	mapFileInfo.m_numberOfSubFiles = readBuffer.readByte();
	if (mapFileInfo.m_numberOfSubFiles < 1) {
		readBuffer.setError(QString("invalid number of sub-files: %1").arg(mapFileInfo.m_numberOfSubFiles));
	}

	return readBuffer;
}
