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

#include "Way.h"

Way::Way(unsigned char layer, const QList<Tag> &tags, const QVector<QVector<LatLong> > &geoPoints, const LatLong &labelPosition) :
	m_layer(layer),
	m_tags(tags),
	m_geoPoints(geoPoints),
	m_labelPosition(labelPosition)
{
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<( QDebug dbg, const Way &way )
{
	return dbg << way.geoPoints();
}
#endif
