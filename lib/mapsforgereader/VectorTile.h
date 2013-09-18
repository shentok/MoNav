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

#ifndef MAPSFORGE_VECTORTILE_H
#define MAPSFORGE_VECTORTILE_H

#include "PointOfInterest.h"
#include "Way.h"

#include <QList>

/**
 * An immutable container for the data returned by the {@link MapDatabase}.
 */
class VectorTile
{
public:
	VectorTile();

	VectorTile(const QList<PointOfInterest> &pois, const QList<Way> &ways, bool isWater);

	/**
	 * True if the read area is completely covered by water, false otherwise.
	 */
	bool isWater() const;

	/**
	 * The read POIs.
	 */
	QList<PointOfInterest> pointsOfInterest() const;

	/**
	 * The read ways.
	 */
	QList<Way> ways() const;

private:
	bool m_isWater;
	QList<PointOfInterest> m_pointsOfInterest;
	QList<Way> m_ways;
};

#endif
