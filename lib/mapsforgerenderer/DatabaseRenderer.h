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

#ifndef DATABASERENDERER_H
#define DATABASERENDERER_H

#include "osmarender/RenderCallback.h"

#include "mapsforgereader/Tag.h"

#include "renderer/PointTextContainer.h"
#include "renderer/WayTextContainer.h"
#include "renderer/PolylineContainer.h"
#include "renderer/SymbolContainer.h"

#include <QList>
#include <QPoint>
#include <QPainter>
#include <QVector>

class PointOfInterest;
class LatLong;
class TileId;
class Way;

class RenderTheme;
class RendererJob;
class ShapeContainer;
class ShapePaintContainer;

/**
 * A DatabaseRenderer renders map tiles by reading from a {@link MapDatabase}.
 */
class DatabaseRenderer : public RenderCallback
{
public:
	/**
	 * Constructs a new DatabaseRenderer.
	 *
	 * @param mapDatabase
	 *            the MapDatabase from which the map data will be read.
	 */
	DatabaseRenderer(const RenderTheme *renderTheme);

	~DatabaseRenderer();

	void matchPointOfInterest(const PointOfInterest &pointOfInterest, const TileId &tile, int tileSize);

	void matchWay(const Way &way, const TileId &tile, int tileSize);

	void matchWater(const TileId &tile, int tileSize);

	void renderArea(const QBrush &fill, const QPen &stroke, int level);

	void renderAreaCaption(const QString &caption, float verticalOffset, const QFont &font, const Paint &fill, const Paint &stroke);

	void renderAreaSymbol(const QImage &symbol);

	void renderPointOfInterestCaption(const QString &caption, float verticalOffset, const QFont &font, const Paint &fill, const Paint &stroke);

	void renderPointOfInterestCircle(float radius, const QBrush &fill, const QPen &stroke, int level);

	void renderPointOfInterestSymbol(const QImage &symbol);

	void renderWay(const QPen &stroke, int level);

	void renderWaySymbol(const QImage &symbolBitmap, bool alignCenter, bool repeatSymbol);

	void renderWayText(const QString &text, const QFont &font, const Paint &fill, const Paint &stroke);

	QList<PointTextContainer> areaLabels() const;

	QList<PointTextContainer> nodes() const;

	QList<SymbolContainer> pointSymbols() const;

	QList<WayTextContainer> wayNames() const;

	QVector<QVector<QList<ShapePaintContainer *> > > ways() const;

	QList<SymbolContainer> waySymbols() const;

	void clear();

private:
	/**
	 * @param way
	 *            the coordinates of the way.
	 * @return true if the given way is closed, false otherwise.
	 */
	static bool isClosedWay(const QVector<Point> &way);

	/**
	 * Calculates the center of the minimum bounding rectangle for the given coordinates.
	 *
	 * @param coordinates
	 *            the coordinates for which calculation should be done.
	 * @return the center coordinates of the minimum bounding rectangle.
	 */
	static Point calculateCenterOfBoundingBox(const QVector<Point> &m_currentCoordinates);

	/**
	 * Converts the given LatLong into XY coordinates on the current object.
	 *
	 * @param latLong
	 *            the LatLong to convert.
	 * @return the XY coordinates on the current object.
	 */
	static Point scaleLatLong(const LatLong &latLong, const TileId &tile, int tileSize);

	byte getValidLayer(byte layer) const;

	static const Tag TAG_NATURAL_WATER;

	/**
	 * Minimum distance in pixels before the symbol is repeated.
	 */
	static const int DISTANCE_BETWEEN_SYMBOLS;

	/**
	 * Distance in pixels to skip from both ends of a segment.
	 */
	static const int SEGMENT_SAFETY_DISTANCE;

	/**
	 * Minimum distance in pixels before the way name is repeated.
	 */
	static const int DISTANCE_BETWEEN_WAY_NAMES;

	const RenderTheme *const m_renderTheme;
	QList<PointTextContainer> m_areaLabels;
	QList<PointTextContainer> m_nodes;
	QList<SymbolContainer> m_pointSymbols;
	QList<WayTextContainer> m_wayNames;
	QVector<QVector<QList<ShapePaintContainer *> > > m_ways;
	QList<SymbolContainer> m_waySymbols;
	QVector<QVector<Point> > m_currentCoordinates;
	int m_currentDrawingLayer;
	Point m_currentPoiPosition;
};

#endif // DATABASERENDERER_H
