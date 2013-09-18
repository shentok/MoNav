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

#include "OptionalFields.h"

#include "../ReadBuffer.h"
#include "../LatLong.h"

ReadBuffer &operator>>(ReadBuffer &readBuffer, OptionalFields &optionalFields)
{
	const byte flags = readBuffer.readByte();
	optionalFields.isDebugFile = (flags & 0x80) != 0;
	optionalFields.hasStartPosition = (flags & 0x40) != 0;
	optionalFields.hasStartZoomLevel = (flags & 0x20) != 0;
	optionalFields.hasLanguagePreference = (flags & 0x10) != 0;
	optionalFields.hasComment = (flags & 0x08) != 0;
	optionalFields.hasCreatedBy = (flags & 0x04) != 0;

	if (optionalFields.hasStartPosition) {
		readBuffer >> optionalFields.startPosition;
	}

	if (optionalFields.hasStartZoomLevel) {
		// get and check the start zoom level (1 byte)
		const byte mapStartZoomLevel = readBuffer.readByte();
		if (/*mapStartZoomLevel < 0 ||*/ mapStartZoomLevel > OptionalFields::START_ZOOM_LEVEL_MAX) {
			readBuffer.setError(QString("invalid map start zoom level: %1").arg(mapStartZoomLevel));
		}

		optionalFields.startZoomLevel = mapStartZoomLevel;
	}

	if (optionalFields.hasLanguagePreference) {
		optionalFields.languagePreference = readBuffer.readUTF8EncodedString();
		if (optionalFields.languagePreference.length() != OptionalFields::LANGUAGE_PREFERENCE_LENGTH) {
			readBuffer.setError(QString("invalid language preference: %1").arg(optionalFields.languagePreference));
		}

	}

	if (optionalFields.hasComment) {
		optionalFields.comment = readBuffer.readUTF8EncodedString();
	}

	if (optionalFields.hasCreatedBy) {
		optionalFields.createdBy = readBuffer.readUTF8EncodedString();
	}

	return readBuffer;
}
