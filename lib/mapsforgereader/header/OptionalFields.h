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

#ifndef MAPSFORGE_OPTIONALFIELDS_H
#define MAPSFORGE_OPTIONALFIELDS_H

#include "../LatLong.h"

typedef char byte;

class ReadBuffer;

class OptionalFields
{
public:
	OptionalFields() :
		hasComment(false),
		hasCreatedBy(false),
		hasLanguagePreference(false),
		hasStartPosition(false),
		hasStartZoomLevel(false),
		isDebugFile(false)
	{
	}

private:
	friend class MapFileInfo;
	friend ReadBuffer &operator>>(ReadBuffer &, OptionalFields &);

	/**
	 * The length of the language preference string.
	 */
	static const int LANGUAGE_PREFERENCE_LENGTH = 2;

	/**
	 * Maximum valid start zoom level.
	 */
	static const int START_ZOOM_LEVEL_MAX = 22;

	bool hasComment;
	bool hasCreatedBy;
	bool hasLanguagePreference;
	bool hasStartPosition;
	bool hasStartZoomLevel;
	bool isDebugFile;
	QString comment;
	QString createdBy;
	QString languagePreference;
	LatLong startPosition;
	byte startZoomLevel;
};

ReadBuffer &operator>>(ReadBuffer &readBuffer, OptionalFields &optionalFields);

#endif
