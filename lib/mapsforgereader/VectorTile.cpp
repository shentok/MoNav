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

#include "VectorTile.h"

VectorTile::VectorTile() :
	m_isWater(false)
{
}

VectorTile::VectorTile(const QList<PointOfInterest> &pois, const QList<Way> &ways, bool isWater) :
	m_isWater(isWater),
	m_pointsOfInterest(pois),
	m_ways(ways)
{
}

bool VectorTile::isWater() const
{
	return m_isWater;
}


QList<PointOfInterest> VectorTile::pointsOfInterest() const
{
	return m_pointsOfInterest;
}

QList<Way> VectorTile::ways() const
{
	return m_ways;
}
