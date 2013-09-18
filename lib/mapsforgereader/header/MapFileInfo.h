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

#ifndef MAPSFORGE_MAPFILEINFO_H
#define MAPSFORGE_MAPFILEINFO_H

#include "../BoundingBox.h"
#include "../LatLong.h"
#include "../Tag.h"
#include "OptionalFields.h"

typedef char byte;

/**
 * Contains the immutable metadata of a map file.
 * 
 * @see MapDatabase#getMapFileInfo()
 */
class MapFileInfo
{
public:
	MapFileInfo() :
		m_fileVersion(0),
		m_fileSize(0),
		m_mapDate(0),
		m_boundingBox(),
		m_tilePixelSize(0),
		m_projectionName(),
		m_poiTags(),
		m_wayTags(),
		m_numberOfSubFiles()
	{
	}

	/**
	 * The bounding box of the map file.
	 */
	BoundingBox boundingBox() const { return m_boundingBox; }

	/**
	 * The comment field of the map file (may be null).
	 */
	QString comment() const { return m_optionalFields.comment; }

	/**
	 * The created by field of the map file (may be null).
	 */
	QString createdBy() const { return m_optionalFields.createdBy; }

	/**
	 * True if the map file includes debug information, false otherwise.
	 */
	bool debugFile() const { return m_optionalFields.isDebugFile; }

	/**
	 * The size of the map file, measured in bytes.
	 */
	qint64 fileSize() const { return m_fileSize; }

	/**
	 * The file version number of the map file.
	 */
	int fileVersion() const { return m_fileVersion; }

	/**
	 * The preferred language for names as defined in ISO 3166-1 (may be null).
	 */
	QString languagePreference() const { return m_optionalFields.languagePreference; }

	/**
	 * The date of the map data in milliseconds since January 1, 1970.
	 */
	qint64 mapDate() const { return m_mapDate; }

	/**
	 * The number of sub-files in the map file.
	 */
	byte numberOfSubFiles() const { return m_numberOfSubFiles; }

	/**
	 * The POI tags.
	 */
	QVector<Tag> poiTags() const { return m_poiTags; }

	/**
	 * The name of the projection used in the map file.
	 */
	QString projectionName() const { return m_projectionName; }

	/**
	 * The map start position from the file header (may be null).
	 */
	LatLong startPosition() const { return m_optionalFields.startPosition; }

	/**
	 * The map start zoom level from the file header (may be null).
	 */
	byte startZoomLevel() const { return m_optionalFields.startZoomLevel; }

	/**
	 * The size of the tiles in pixels.
	 */
	int tilePixelSize() const { return m_tilePixelSize; }

	/**
	 * The way tags.
	 */
	QVector<Tag> wayTags() const { return m_wayTags; }

private:
	friend ReadBuffer &operator>>(ReadBuffer &, MapFileInfo &);
	int m_fileVersion;
	qint64 m_fileSize;
	qint64 m_mapDate;
	BoundingBox m_boundingBox;
	int m_tilePixelSize;
	QString m_projectionName;
	OptionalFields m_optionalFields;
	QVector<Tag> m_poiTags;
	QVector<Tag> m_wayTags;
	byte m_numberOfSubFiles;
};

ReadBuffer &operator>>(ReadBuffer &readBuffer, MapFileInfo &mapFileInfo);

#endif
