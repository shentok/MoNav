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

#include "TileId.h"

#include <qmath.h>
#include <QString>

TileId::TileId(int x, int y, int z) :
	tileX(x),
	tileY(y),
	zoomLevel(z)
{}

bool TileId::operator<(const TileId &other) const
{
	if (zoomLevel < other.zoomLevel)
		return true;

	if (zoomLevel > other.zoomLevel)
		return false;

	Q_ASSERT(zoomLevel == other.zoomLevel);

	if (tileX < other.tileX)
		return true;

	if (tileX > other.tileX)
		return false;

	Q_ASSERT(tileX == other.tileX);

	if (tileY < other.tileY)
		return true;

	return false;
}

qint64 TileId::getMapSize(byte zoomLevel, unsigned short tileSize)
{
	Q_ASSERT(zoomLevel >= 0 && QString("zoom level must not be negative: %1").arg(zoomLevel).constData());

	return (qint64) tileSize << zoomLevel;
}

quint64 TileId::getMaxTileNumber(byte zoomLevel)
{
	Q_ASSERT(zoomLevel >= 0 && QString("zoomLevel must not be negative: %1").arg(zoomLevel).toAscii().constData());

	return (2 << (zoomLevel - 1)) - 1;
}

qint64 TileId::pixelX(unsigned short tileSize) const
{
	return tileX * tileSize;
}

qint64 TileId::pixelY(unsigned short tileSize) const
{
	return tileY * tileSize;
}

qint64 TileId::pixelXToTileX(double pixelX, byte zoomLevel, unsigned short tileSize)
{
	return qMin<qint64>(qMax<qint64>(pixelX / tileSize, 0), qPow(2, zoomLevel) - 1);
}

qint64 TileId::pixelYToTileY(double pixelY, byte zoomLevel, unsigned short tileSize)
{
	return (qint64) qMin<double>(qMax<double>(pixelY / tileSize, 0), qPow(2, zoomLevel) - 1);
}
