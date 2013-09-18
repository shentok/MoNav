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
 * You should have received a copy of the GNU Lesser General Public License aqint64 with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MercatorProjection.h"

#include "BoundingBox.h"
#include "TileId.h"

#include <qmath.h>

qint64 MercatorProjection::latitudeToPixelY(double latitude, byte zoomLevel, unsigned short tileSize)
{
	double sinLatitude = qSin(latitude * (M_PI / 180));
	qint64 mapSize = TileId::getMapSize(zoomLevel, tileSize);
	// FIXME improve this formula so that it works correctly without the clipping
	double pixelY = (0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * M_PI)) * mapSize;
	return qMin<qint64>(qMax<qint64>(0, pixelY), mapSize);
}

qint64 MercatorProjection::latitudeToTileY(double latitude, byte zoomLevel)
{
	const double sinLatitude = qSin(latitude * (M_PI / 180));
	const qint64 mapSize = 1 << zoomLevel;
	// FIXME improve this formula so that it works correctly without the clipping
	double pixelY = (0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * M_PI)) * mapSize;
	return qMin<qint64>(qMax<qint64>(0, pixelY), mapSize);
}

qint64 MercatorProjection::longitudeToPixelX(double longitude, byte zoomLevel, unsigned short tileSize)
{
	const qint64 mapSize = TileId::getMapSize(zoomLevel, tileSize);
	return (longitude + 180) / 360 * mapSize;
}

qint64 MercatorProjection::longitudeToTileX(double longitude, byte zoomLevel)
{
	const qint64 mapSize = 1 << zoomLevel;
	return (longitude + 180) / 360 * mapSize;
}

LatLong MercatorProjection::tileToCoordinates(const TileId &tile)
{
	Q_ASSERT(tile.tileX >= 0 );
	Q_ASSERT(tile.tileY >= 0);

	const double mapSize = 1 << tile.zoomLevel;
	Q_ASSERT (tile.tileX <= mapSize); // "invalid tileX coordinate at zoom level " + zoomLevel + ": " + tileX
	Q_ASSERT (tile.tileY <= mapSize); // "invalid tileY coordinate at zoom level " + zoomLevel + ": " + tileY

	const double longitude = 360 * ((tile.tileX / mapSize) - 0.5);

	const double y = 0.5 - (tile.tileY / mapSize);
	const double latitude = 90 - 360 * qAtan(qExp(-y * (2 * M_PI))) / M_PI;

	return LatLong(latitude, longitude);
}

qint64 MercatorProjection::boundaryTileBottom(const BoundingBox &boundingBox, int zoomLevel)
{
	return latitudeToTileY(boundingBox.minLatitude(), zoomLevel);
}

qint64 MercatorProjection::boundaryTileLeft(const BoundingBox &boundingBox, int zoomLevel)
{
	return longitudeToTileX(boundingBox.minLongitude(), zoomLevel);
}

qint64 MercatorProjection::boundaryTileRight(const BoundingBox &boundingBox, int zoomLevel)
{
	return longitudeToTileX(boundingBox.maxLongitude(), zoomLevel);
}

qint64 MercatorProjection::boundaryTileTop(const BoundingBox &boundingBox, int zoomLevel)
{
	return latitudeToTileY(boundingBox.maxLatitude(), zoomLevel);
}

qint64 MercatorProjection::blocksHeight(const BoundingBox &boundingBox, int zoomLevel)
{
	return boundaryTileBottom(boundingBox, zoomLevel) - boundaryTileTop(boundingBox, zoomLevel) + 1;
}

qint64 MercatorProjection::blocksWidth(const BoundingBox &boundingBox, int zoomLevel)
{
	return boundaryTileRight(boundingBox, zoomLevel) - boundaryTileLeft(boundingBox, zoomLevel) + 1;
}
