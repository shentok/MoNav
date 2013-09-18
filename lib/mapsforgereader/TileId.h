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

#ifndef MAPSFORGE_TILEID_H
#define MAPSFORGE_TILEID_H

#include <qglobal.h>

typedef char byte;

class TileId
{
public:
	TileId(int x, int y, int z);

	bool operator<(const TileId &other) const;

	/**
	 * @param zoomLevel
	 *            the zoom level for which the size of the world map should be returned.
	 * @return the horizontal and vertical size of the map in pixel at the given zoom level.
	 * @throws IllegalArgumentException
	 *             if the given zoom level is negative.
	 */
	static qint64 getMapSize(byte zoomLevel, unsigned short tileSize);

	/**
	 * @return the maximum valid tile number for the given zoom level, 2<sup>zoomLevel</sup> -1.
	 */
	static quint64 getMaxTileNumber(byte zoomLevel);

	/**
	 * @return the pixel coordinate of the x attribute.
	 */
	qint64 pixelX(unsigned short tileSize) const;

	/**
	 * @return the pixel coordinate of the x attribute.
	 */
	qint64 pixelY(unsigned short tileSize) const;

	/**
	 * Converts a pixel X coordinate to the tile X number.
	 *
	 * @param pixelX
	 *            the pixel X coordinate that should be converted.
	 * @param zoomLevel
	 *            the zoom level at which the coordinate should be converted.
	 * @return the tile X number.
	 */
	static qint64 pixelXToTileX(double pixelX, byte zoomLevel, unsigned short tileSize);

	/**
	 * Converts a pixel Y coordinate to the tile Y number.
	 *
	 * @param pixelY
	 *            the pixel Y coordinate that should be converted.
	 * @param zoomLevel
	 *            the zoom level at which the coordinate should be converted.
	 * @return the tile Y number.
	 */
	static qint64 pixelYToTileY(double pixelY, byte zoomLevel, unsigned short tileSize);

	int tileX;
	int tileY;
	int zoomLevel;
};

#endif
