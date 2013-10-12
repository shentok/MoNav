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

#include "DatabaseRenderer.h"

#include "renderer/CircleContainer.h"
#include "renderer/ShapePaintContainer.h"

#include "mapsforgereader/MercatorProjection.h"
#include "mapsforgereader/PointOfInterest.h"
#include "mapsforgereader/Way.h"
#include "mapsforgereader/TileId.h"

#include "osmarender/RenderTheme.h"

#include <qmath.h>
#include <QFontMetrics>

#include <limits>

const Tag DatabaseRenderer::TAG_NATURAL_WATER = Tag("natural", "water");
const int DatabaseRenderer::DISTANCE_BETWEEN_SYMBOLS = 200;
const int DatabaseRenderer::SEGMENT_SAFETY_DISTANCE = 30;
const int DatabaseRenderer::DISTANCE_BETWEEN_WAY_NAMES = 500;

DatabaseRenderer::DatabaseRenderer(const RenderTheme *renderTheme) :
	m_renderTheme(renderTheme),
	m_ways(11),
	m_currentDrawingLayer(0)
{
	const int levels = renderTheme->getLevels();

	for (int i = 0; i < m_ways.size(); ++i) {
		m_ways[i] = QVector<QList<ShapePaintContainer *> >(levels);
	}
}

DatabaseRenderer::~DatabaseRenderer()
{
	clear();
}

void DatabaseRenderer::matchPointOfInterest(const PointOfInterest &pointOfInterest, const TileId &tile, int tileSize)
{
	m_currentDrawingLayer = getValidLayer(pointOfInterest.layer);
	m_currentPoiPosition = scaleLatLong(pointOfInterest.position, tile, tileSize);
	m_renderTheme->matchNode(this, pointOfInterest.tags, tile.zoomLevel);
}

void DatabaseRenderer::matchWay(const Way &way, const TileId &tile, int tileSize)
{
	m_currentDrawingLayer = getValidLayer(way.layer());
	// TODO what about the label position?

	QVector<QVector<LatLong> > latLongs = way.geoPoints();
	m_currentCoordinates = QVector<QVector<Point> >(latLongs.size());
	for (int i = 0; i < m_currentCoordinates.size(); ++i) {
		m_currentCoordinates[i] = QVector<Point>(latLongs[i].size());

		for (int j = 0; j < m_currentCoordinates[i].size(); ++j) {
			m_currentCoordinates[i][j] = scaleLatLong(latLongs[i][j], tile, tileSize);
		}
	}

	if (isClosedWay(m_currentCoordinates[0])) {
		m_renderTheme->matchClosedWay(this, way.tags(), tile.zoomLevel);
	} else {
		m_renderTheme->matchLinearWay(this, way.tags(), tile.zoomLevel);
	}
}

void DatabaseRenderer::matchWater(const TileId &tile, int tileSize)
{
	m_currentDrawingLayer = 0;
	m_currentCoordinates = QVector<QVector<Point> >() << ( QVector<Point>() << Point(0, 0) << Point(tileSize, 0) << Point(tileSize, tileSize) << Point(0, tileSize) );
	m_renderTheme->matchClosedWay(this, QList<Tag>() << TAG_NATURAL_WATER, tile.zoomLevel);
}

void DatabaseRenderer::renderArea(const QBrush &fill, const QPen &stroke, int level)
{
	m_ways[m_currentDrawingLayer][level] << new ShapePaintContainer(new PolylineBrushContainer(m_currentCoordinates, fill));
	m_ways[m_currentDrawingLayer][level] << new ShapePaintContainer(new PolylinePenContainer(m_currentCoordinates, stroke));
}

void DatabaseRenderer::renderAreaCaption(const QString &caption, float verticalOffset, const QFont &font, const Paint &fill, const Paint &stroke)
{
	Point centerPosition = calculateCenterOfBoundingBox(m_currentCoordinates[0]);
	m_areaLabels << PointTextContainer(caption, centerPosition, font, fill, stroke);
}

void DatabaseRenderer::renderAreaSymbol(const QImage &symbol)
{
	const Point centerPosition = calculateCenterOfBoundingBox(m_currentCoordinates[0]);
	const Point halfSymbolSize = Point(symbol.width() / 2, symbol.height() / 2);
	const Point shiftedCenterPosition = centerPosition - halfSymbolSize;
	m_pointSymbols << SymbolContainer(symbol, shiftedCenterPosition);
}

void DatabaseRenderer::renderPointOfInterestCaption(const QString &caption, float verticalOffset, const QFont &font, const Paint &fill, const Paint &stroke)
{
	m_nodes << PointTextContainer(caption, Point(m_currentPoiPosition.x(), m_currentPoiPosition.y() + verticalOffset), font, fill, stroke);
}

void DatabaseRenderer::renderPointOfInterestCircle(float radius, const QBrush &fill, const QPen &stroke, int level)
{
	m_ways[m_currentDrawingLayer][level] << new ShapePaintContainer(new CircleBrushContainer(m_currentPoiPosition, radius, fill));
	m_ways[m_currentDrawingLayer][level] << new ShapePaintContainer(new CirclePenContainer(m_currentPoiPosition, radius, stroke));
}

void DatabaseRenderer::renderPointOfInterestSymbol(const QImage &symbol)
{
	int halfSymbolWidth = symbol.width() / 2;
	int halfSymbolHeight = symbol.height() / 2;
	double pointX = m_currentPoiPosition.x() - halfSymbolWidth;
	double pointY = m_currentPoiPosition.y() - halfSymbolHeight;
	Point shiftedCenterPosition = Point(pointX, pointY);
	m_pointSymbols << SymbolContainer(symbol, shiftedCenterPosition);
}

void DatabaseRenderer::renderWay(const QPen &stroke, int level)
{
	m_ways[m_currentDrawingLayer][level] << new ShapePaintContainer(new PolylinePenContainer(m_currentCoordinates, stroke));
}

void DatabaseRenderer::renderWaySymbol(const QImage &symbolBitmap, bool alignCenter, bool repeatSymbol)
{
	int skipPixels = SEGMENT_SAFETY_DISTANCE;

	// get the first way point coordinates
	Point previous = m_currentCoordinates[0][0];

	// draw the symbol on each way segment
	for (int i = 1; i < m_currentCoordinates[0].size(); ++i) {
		// get the current way point coordinates
		const Point current = m_currentCoordinates[0][i];

		// calculate the length of the current segment (Euclidian distance)
		Point diff = current - previous;
		const double segmentLengthInPixel = qSqrt(diff.x() * diff.x() + diff.y() * diff.y());
		float segmentLengthRemaining = (float) segmentLengthInPixel;

		while (segmentLengthRemaining - skipPixels > SEGMENT_SAFETY_DISTANCE) {
			// calculate the percentage of the current segment to skip
			const float segmentSkipPercentage = skipPixels / segmentLengthRemaining;

			// move the previous point forward towards the current point
			previous += diff * segmentSkipPercentage;
			const float theta = qAtan2(current.y() - previous.y(), current.x() - previous.x()) * 180 / M_PI;

			m_waySymbols << SymbolContainer(symbolBitmap, previous, alignCenter, theta);

			// check if the symbol should only be rendered once
			if (!repeatSymbol) {
				return;
			}

			// recalculate the distances
			diff = current - previous;

			// recalculate the remaining length of the current segment
			segmentLengthRemaining -= skipPixels;

			// set the amount of pixels to skip before repeating the symbol
			skipPixels = DISTANCE_BETWEEN_SYMBOLS;
		}

		skipPixels -= segmentLengthRemaining;
		if (skipPixels < SEGMENT_SAFETY_DISTANCE) {
			skipPixels = SEGMENT_SAFETY_DISTANCE;
		}

		// set the previous way point coordinates for the next loop
		previous = current;
	}
}

void DatabaseRenderer::renderWayText(const QString &text, const QFont &font, const Paint &fill, const Paint &stroke)
{
	// calculate the way name length plus some margin of safety
	const int wayNameWidth = QFontMetrics(font).width(text) + 10;

	int skipPixels = 0;

	// find way segments long enough to draw the way name on them
	for (int i = 1; i < m_currentCoordinates[0].size(); ++i) {
		// get the first way point coordinates
		const Point previous = m_currentCoordinates[0][i-1];
		// get the current way point coordinates
		const Point current = m_currentCoordinates[0][i];

		// calculate the length of the current segment (Euclidian distance)
		const Point diff = current - previous;
		double segmentLengthInPixel = qSqrt(diff.x() * diff.x() + diff.y() * diff.y());

		if (skipPixels > 0) {
			skipPixels -= segmentLengthInPixel;
		} else if (segmentLengthInPixel > wayNameWidth) {
			// check to prevent inverted way names
			const Point p1 = previous.x() <= current.x() ? previous : current;
			const Point p2 = previous.x() <= current.x() ? current : previous;

			m_wayNames << WayTextContainer(p1, p2, text, font, fill);
			if (stroke.style() != Qt::NoPen) {
				m_wayNames << WayTextContainer(p1, p2, text, font, stroke);
			}

			skipPixels = DISTANCE_BETWEEN_WAY_NAMES;
		}
	}
}

void DatabaseRenderer::clear()
{
	for (int i = 0; i < m_ways.size(); ++i) {
		QVector<QList<ShapePaintContainer *> > &innerWayList = m_ways[i];
		for (int j = 0; j < innerWayList.size(); ++j) {
			qDeleteAll(innerWayList[j]);
			innerWayList[j].clear();
		}
	}

	m_areaLabels.clear();
	m_nodes.clear();
	m_pointSymbols.clear();
	m_wayNames.clear();
	m_waySymbols.clear();
}

bool DatabaseRenderer::isClosedWay(const QVector<Point> &way)
{
	return way[0] == way[way.size() - 1];
}

Point DatabaseRenderer::calculateCenterOfBoundingBox(const QVector<Point> &coordinates)
{
	Point pointMin = coordinates[0];
	Point pointMax = coordinates[0];

	for (int i = 1; i < coordinates.size(); ++i) {
		const Point immutablePoint = coordinates[i];
		pointMin.rx() = qMin(pointMin.x(), immutablePoint.x());
		pointMax.rx() = qMax(pointMax.x(), immutablePoint.x());
		pointMin.ry() = qMin(pointMin.y(), immutablePoint.y());
		pointMax.ry() = qMax(pointMax.y(), immutablePoint.y());
	}

	return Point((pointMin.x() + pointMax.x()) / 2, (pointMin.y() + pointMax.y()) / 2);
}

Point DatabaseRenderer::scaleLatLong(const LatLong &latLong, const TileId &tile, int tileSize)
{
	double pixelX = MercatorProjection::longitudeToPixelX(latLong.lon(), tile.zoomLevel, tileSize) - tile.pixelX(tileSize);
	double pixelY = MercatorProjection::latitudeToPixelY(latLong.lat(), tile.zoomLevel, tileSize) - tile.pixelY(tileSize);

	return Point((float) pixelX, (float) pixelY);
}

byte DatabaseRenderer::getValidLayer(byte layer) const
{
	if (layer < 0) {
		return 0;
	} else if (layer >= m_ways.size()) {
		return m_ways.size() - 1;
	} else {
		return layer;
	}
}

QList<SymbolContainer> DatabaseRenderer::waySymbols() const
{
    return m_waySymbols;
}

QVector<QVector<QList<ShapePaintContainer *> > > DatabaseRenderer::ways() const
{
    return m_ways;
}

QList<WayTextContainer> DatabaseRenderer::wayNames() const
{
    return m_wayNames;
}

QList<SymbolContainer> DatabaseRenderer::pointSymbols() const
{
    return m_pointSymbols;
}

QList<PointTextContainer> DatabaseRenderer::nodes() const
{
    return m_nodes;
}

QList<PointTextContainer> DatabaseRenderer::areaLabels() const
{
    return m_areaLabels;
}
