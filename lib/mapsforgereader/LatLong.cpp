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

#include "LatLong.h"

#include "ReadBuffer.h"

LatLong::LatLong() :
	m_lat(),
	m_lon()
{
}

LatLong::LatLong(double latitude, double longitude) :
	m_lat(latitude),
	m_lon(longitude)
{
}

double LatLong::microdegreesToDegrees(int coordinate)
{
	return coordinate / 1000000.0;
}

ReadBuffer &operator>>(ReadBuffer &readBuffer, LatLong &geoPoint)
{
	geoPoint.m_lat = LatLong::microdegreesToDegrees(readBuffer.readInt());
	geoPoint.m_lon = LatLong::microdegreesToDegrees(readBuffer.readInt());

	return readBuffer;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<( QDebug dbg, const LatLong &latLong )
{
	return dbg << QString("LatLong(%1, %2)").arg(latLong.lon()).arg(latLong.lat());
}
#endif
