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

#include "Tag.h"

#include "ReadBuffer.h"

ReadBuffer &operator>>(ReadBuffer &readBuffer, Tag &tag)
{
	// get and check the POI tag
	QString strTag = readBuffer.readUTF8EncodedString();
	if (strTag.isNull()) {
		readBuffer.setError(QString("tag must not be null"));
	}
	else {
		const int splitPosition = strTag.indexOf('=');
		tag.first = strTag.left(splitPosition);
		tag.second = strTag.mid(splitPosition+1);
	}

	return readBuffer;
}

ReadBuffer &operator>>(ReadBuffer &readBuffer, QVector<Tag> &tags)
{
	tags.clear();

	// get and check the number of POI tags (2 bytes)
	int numberOfTags = readBuffer.readShort();
	if (numberOfTags < 0) {
		readBuffer.setError(QString("invalid number of POI tags: %1").arg(numberOfTags));
	}

	tags.reserve(numberOfTags);
	for (int currentTagId = 0; currentTagId < numberOfTags; ++currentTagId) {
		Tag tag;
		readBuffer >> tag;
		tags.append(tag);
	}

	return readBuffer;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<( QDebug dbg, const Tag &tag )
{
	return dbg << QString( "Tag(\"%1\"=\"%2\")" ).arg( tag.first ).arg( tag.second );
}
#endif
