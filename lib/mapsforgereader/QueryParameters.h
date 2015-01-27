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

#ifndef MAPSFORGE_QUERYPARAMETERS_H
#define MAPSFORGE_QUERYPARAMETERS_H

#include "qglobal.h"

class TileId;

struct QueryParameters
{
public:
	QueryParameters(int queryZoomLevel);

	QueryParameters(int queryZoomLevel, const TileId &tile, int zoomLevelDifference);

	int queryZoomLevel;
	int queryTileBitmask;
	bool useTileBitmask;

private:
	static int getFirstLevelTileBitmask(const TileId &tile);

	static int getSecondLevelTileBitmaskLowerLeft(qint64 subtileX, qint64 subtileY);

	static int getSecondLevelTileBitmaskLowerRight(qint64 subtileX, qint64 subtileY);

	static int getSecondLevelTileBitmaskUpperLeft(qint64 subtileX, qint64 subtileY);

	static int getSecondLevelTileBitmaskUpperRight(qint64 subtileX, qint64 subtileY);

	static int calculateTileBitmask(const TileId &tile, int zoomLevelDifference);
};

#endif
