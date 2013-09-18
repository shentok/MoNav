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

#ifndef MAPSFORGE_BOUNDINGBOX_H
#define MAPSFORGE_BOUNDINGBOX_H

class ReadBuffer;

class BoundingBox
{
public:
	double minLatitude() const { return m_minLatitude; }
	double minLongitude() const { return m_minLongitude; }
	double maxLatitude() const { return m_maxLatitude; }
	double maxLongitude() const { return m_maxLongitude; }

private:
	friend ReadBuffer &operator>>(ReadBuffer &reader, BoundingBox &bbox);

	double m_minLatitude;
	double m_minLongitude;
	double m_maxLatitude;
	double m_maxLongitude;
};

ReadBuffer &operator>>( ReadBuffer &reader, BoundingBox &bbox );

#endif
