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

#ifndef MAPSFORGE_MAPFILEHEADER_H
#define MAPSFORGE_MAPFILEHEADER_H

#include "MapFileInfo.h"
#include "SubFileParameter.h"

/**
 * Reads and validates the header data from a binary map file.
 */
class MapFileHeader
{
public:
	MapFileHeader();

	/**
	 * @return a MapFileInfo containing the header data.
	 */
	MapFileInfo getMapFileInfo() const { return m_mapFileInfo; }

	/**
	 * @param zoomLevel
	 *            the originally requested zoom level.
	 * @return the closest possible zoom level which is covered by a sub-file.
	 */
	byte getQueryZoomLevel(byte zoomLevel) const;

	/**
	 * @param queryZoomLevel
	 *            the zoom level for which the sub-file parameters are needed.
	 * @return the sub-file parameters for the given zoom level.
	 */
	SubFileParameter getSubFileParameter(int queryZoomLevel);

private:
	friend ReadBuffer &operator>>(ReadBuffer &, MapFileHeader &);

	MapFileInfo m_mapFileInfo;
	QVector<SubFileParameter> m_subFileParameters;
	byte m_zoomLevelMaximum;
	byte m_zoomLevelMinimum;
};

/**
 * Reads and validates the header block from the map file.
 *
 * @param readBuffer
 *            the ReadBuffer for the file data.
 * @param fileSize
 *            the size of the map file in bytes.
 * @return a FileOpenResult containing an error message in case of a failure.
 * @throws IOException
 *             if an error occurs while reading the file.
 */
ReadBuffer &operator>>(ReadBuffer &readBuffer, MapFileHeader &header);

#endif
