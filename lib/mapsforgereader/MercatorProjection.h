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

#ifndef MAPSFORGE_MERCATORPROJECTION_H
#define MAPSFORGE_MERCATORPROJECTION_H

#include "LatLong.h"

#include <QString>

class BoundingBox;
class TileId;

typedef char byte;

/**
 * An implementation of the spherical Mercator projection.
 */
class MercatorProjection
{
public:
	/**
	 * Converts a latitude coordinate (in degrees) to a pixel Y coordinate at a certain zoom level.
	 *
	 * @param latitude
	 *            the latitude coordinate that should be converted.
	 * @param zoomLevel
	 *            the zoom level at which the coordinate should be converted.
	 * @return the pixel Y coordinate of the latitude value.
	 */
	static qint64 latitudeToPixelY(double latitude, byte zoomLevel, unsigned short tileSize);

	/**
	 * Converts a latitude coordinate (in degrees) to a tile Y number at a certain zoom level.
	 *
	 * @param latitude
	 *            the latitude coordinate that should be converted.
	 * @param zoomLevel
	 *            the zoom level at which the coordinate should be converted.
	 * @return the tile Y number of the latitude value.
	 */
	static qint64 latitudeToTileY(double latitude, byte zoomLevel);

	/**
	 * Converts a longitude coordinate (in degrees) to a pixel X coordinate at a certain zoom level.
	 *
	 * @param longitude
	 *            the longitude coordinate that should be converted.
	 * @param zoomLevel
	 *            the zoom level at which the coordinate should be converted.
	 * @return the pixel X coordinate of the longitude value.
	 */
	static qint64 longitudeToPixelX(double longitude, byte zoomLevel, unsigned short tileSize);

	/**
	 * Converts a longitude coordinate (in degrees) to the tile X number at a certain zoom level.
	 *
	 * @param longitude
	 *            the longitude coordinate that should be converted.
	 * @param zoomLevel
	 *            the zoom level at which the coordinate should be converted.
	 * @return the tile X number of the longitude value.
	 */
	static qint64 longitudeToTileX(double longitude, byte zoomLevel);

	/**
	 * Converts a pixel Y coordinate at a certain zoom level to a latitude coordinate.
	 *
	 * @param pixelY
	 *            the pixel Y coordinate that should be converted.
	 * @param zoomLevel
	 *            the zoom level at which the coordinate should be converted.
	 * @return the latitude value of the pixel Y coordinate.
	 * @throws IllegalArgumentException
	 *             if the given pixelY coordinate is invalid.
	 */
	static LatLong tileToCoordinates(const TileId &tile);

	/**
	 * Y number of the tile at the bottom boundary in the grid.
	 */
	static qint64 boundaryTileBottom(const BoundingBox &boundingBox, int zoomLevel);

	/**
	 * X number of the tile at the left boundary in the grid.
	 */
	static qint64 boundaryTileLeft(const BoundingBox &boundingBox, int zoomLevel);

	/**
	 * X number of the tile at the right boundary in the grid.
	 */
	static qint64 boundaryTileRight(const BoundingBox &boundingBox, int zoomLevel);

	/**
	 * Y number of the tile at the top boundary in the grid.
	 */
	static qint64 boundaryTileTop(const BoundingBox &boundingBox, int zoomLevel);

	/**
	 * Vertical amount of blocks in the grid.
	 */
	static qint64 blocksHeight(const BoundingBox &boundingBox, int zoomLevel);

	/**
	 * Horizontal amount of blocks in the grid.
	 */
	static qint64 blocksWidth(const BoundingBox &boundingBox, int zoomLevel);

private:
	MercatorProjection();
};

#endif
