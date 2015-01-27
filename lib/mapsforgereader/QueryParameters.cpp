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

#include "QueryParameters.h"

#include "TileId.h"

QueryParameters::QueryParameters(int queryZoomLevel) :
	queryZoomLevel(queryZoomLevel),
	useTileBitmask(false)
{
}

QueryParameters::QueryParameters(int queryZoomLevel, const TileId &tile, int zoomLevelDifference) :
	queryZoomLevel(queryZoomLevel),
	queryTileBitmask(calculateTileBitmask(tile, zoomLevelDifference)),
	useTileBitmask(true)
{
}

int QueryParameters::getFirstLevelTileBitmask(const TileId &tile)
{
	if (tile.tileX % 2 == 0 && tile.tileY % 2 == 0) {
		// upper left quadrant
		return 0xcc00;
	} else if (tile.tileX % 2 == 1 && tile.tileY % 2 == 0) {
		// upper right quadrant
		return 0x3300;
	} else if (tile.tileX % 2 == 0 && tile.tileY % 2 == 1) {
		// lower left quadrant
		return 0xcc;
	} else {
		// lower right quadrant
		return 0x33;
	}
}

int QueryParameters::getSecondLevelTileBitmaskLowerLeft(qint64 subtileX, qint64 subtileY)
{
	if (subtileX % 2 == 0 && subtileY % 2 == 0) {
		// upper left sub-tile
		return 0x80;
	} else if (subtileX % 2 == 1 && subtileY % 2 == 0) {
		// upper right sub-tile
		return 0x40;
	} else if (subtileX % 2 == 0 && subtileY % 2 == 1) {
		// lower left sub-tile
		return 0x8;
	} else {
		// lower right sub-tile
		return 0x4;
	}
}

int QueryParameters::getSecondLevelTileBitmaskLowerRight(qint64 subtileX, qint64 subtileY)
{
	if (subtileX % 2 == 0 && subtileY % 2 == 0) {
		// upper left sub-tile
		return 0x20;
	} else if (subtileX % 2 == 1 && subtileY % 2 == 0) {
		// upper right sub-tile
		return 0x10;
	} else if (subtileX % 2 == 0 && subtileY % 2 == 1) {
		// lower left sub-tile
		return 0x2;
	} else {
		// lower right sub-tile
		return 0x1;
	}
}

int QueryParameters::getSecondLevelTileBitmaskUpperLeft(qint64 subtileX, qint64 subtileY)
{
	if (subtileX % 2 == 0 && subtileY % 2 == 0) {
		// upper left sub-tile
		return 0x8000;
	} else if (subtileX % 2 == 1 && subtileY % 2 == 0) {
		// upper right sub-tile
		return 0x4000;
	} else if (subtileX % 2 == 0 && subtileY % 2 == 1) {
		// lower left sub-tile
		return 0x800;
	} else {
		// lower right sub-tile
		return 0x400;
	}
}


int QueryParameters::getSecondLevelTileBitmaskUpperRight(qint64 subtileX, qint64 subtileY)
{
	if (subtileX % 2 == 0 && subtileY % 2 == 0) {
		// upper left sub-tile
		return 0x2000;
	} else if (subtileX % 2 == 1 && subtileY % 2 == 0) {
		// upper right sub-tile
		return 0x1000;
	} else if (subtileX % 2 == 0 && subtileY % 2 == 1) {
		// lower left sub-tile
		return 0x200;
	} else {
		// lower right sub-tile
		return 0x100;
	}
}

int QueryParameters::calculateTileBitmask(const TileId &tile, int zoomLevelDifference)
{
	if (zoomLevelDifference == 1) {
		return getFirstLevelTileBitmask(tile);
	}

	// calculate the XY numbers of the second level sub-tile
	qint64 subtileX = tile.tileX >> (zoomLevelDifference - 2);
	qint64 subtileY = tile.tileY >> (zoomLevelDifference - 2);

	// calculate the XY numbers of the parent tile
	qint64 parentTileX = subtileX >> 1;
	qint64 parentTileY = subtileY >> 1;

	// determine the correct bitmask for all 16 sub-tiles
	if (parentTileX % 2 == 0 && parentTileY % 2 == 0) {
		return getSecondLevelTileBitmaskUpperLeft(subtileX, subtileY);
	} else if (parentTileX % 2 == 1 && parentTileY % 2 == 0) {
		return getSecondLevelTileBitmaskUpperRight(subtileX, subtileY);
	} else if (parentTileX % 2 == 0 && parentTileY % 2 == 1) {
		return getSecondLevelTileBitmaskLowerLeft(subtileX, subtileY);
	} else {
		return getSecondLevelTileBitmaskLowerRight(subtileX, subtileY);
	}
}
