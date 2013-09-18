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

#ifndef MAPSFORGE_WAY_H
#define MAPSFORGE_WAY_H

#include "LatLong.h"
#include "Tag.h"

#include <QList>
#include <QVector>

/**
 * An immutable container for all data associated with a single way or area (closed way).
 */
class Way
{
public:
	Way(unsigned char layer, const QList<Tag> &tags, const QVector< QVector<LatLong> > &geoPoints, const LatLong &labelPosition);

	/**
	 * The layer of this way + 5 (to avoid negative values).
	 */
	unsigned char layer() const { return m_layer; }

	/**
	 * The tags of this way.
	 */
	QList<Tag> tags() const { return m_tags; }

	/**
	 * The geographical coordinates of the way nodes.
	 */
	QVector< QVector<LatLong> > geoPoints() const { return m_geoPoints; }

	/**
	 * The position of the area label (may be null).
	 */
	LatLong labelPosition() const { return m_labelPosition; }

private:
	unsigned char m_layer;
	QList<Tag> m_tags;
	QVector< QVector<LatLong> > m_geoPoints;
	LatLong m_labelPosition;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<( QDebug, const Way & );
#endif

#endif
