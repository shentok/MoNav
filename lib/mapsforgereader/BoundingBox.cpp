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

#include "BoundingBox.h"

#include "LatLong.h"
#include "ReadBuffer.h"

ReadBuffer &operator>>( ReadBuffer &reader, BoundingBox &bbox )
{
	int minILatitude = reader.readInt();
	int minILongitude = reader.readInt();
	int maxILatitude = reader.readInt();
	int maxILongitude = reader.readInt();

	bbox.m_minLatitude = LatLong::microdegreesToDegrees(minILatitude);
	bbox.m_minLongitude = LatLong::microdegreesToDegrees(minILongitude);
	bbox.m_maxLatitude = LatLong::microdegreesToDegrees(maxILatitude);
	bbox.m_maxLongitude = LatLong::microdegreesToDegrees(maxILongitude);

	return reader;
}
