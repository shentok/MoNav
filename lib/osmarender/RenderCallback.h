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

#ifndef MAPSFORGE_RENDERCALLBACK_H
#define MAPSFORGE_RENDERCALLBACK_H

#include <QPoint>

class QBrush;
class QFont;
class QImage;
class QPen;
class QString;

typedef QPoint Point;
typedef QPen Paint;
typedef char byte;

/**
 * Callback methods for rendering areas, ways and points of interest (POIs).
 */
class RenderCallback
{
public:
	virtual ~RenderCallback();

	/**
	 * Renders an area with the given parameters.
	 *
	 * @param fill
	 *            the paint to be used for rendering the area.
	 * @param stroke
	 *            an optional paint for the area casing (may be null).
	 * @param level
	 *            the drawing level on which the area should be rendered.
	 */
	virtual void renderArea(const QBrush &fill, const QPen &stroke, int level) = 0;

	/**
	 * Renders an area caption with the given text.
	 *
	 * @param caption
	 *            the text to be rendered.
	 * @param verticalOffset
	 *            the vertical offset of the caption.
	 * @param fill
	 *            the paint to be used for rendering the text.
	 * @param stroke
	 *            an optional paint for the text casing (may be null).
	 */
	virtual void renderAreaCaption(const QString &caption, float verticalOffset, const QFont &font, const Paint &fill, const Paint &stroke) = 0;

	/**
	 * Renders an area symbol with the given bitmap.
	 *
	 * @param symbol
	 *            the symbol to be rendered.
	 */
	virtual void renderAreaSymbol(const QImage &symbol) = 0;

	/**
	 * Renders a point of interest caption with the given text.
	 *
	 * @param caption
	 *            the text to be rendered.
	 * @param verticalOffset
	 *            the vertical offset of the caption.
	 * @param fill
	 *            the paint to be used for rendering the text.
	 * @param stroke
	 *            an optional paint for the text casing (may be null).
	 */
	virtual void renderPointOfInterestCaption(const QString &caption, float verticalOffset, const QFont &font, const Paint &fill, const Paint &stroke) = 0;

	/**
	 * Renders a point of interest circle with the given parameters.
	 *
	 * @param radius
	 *            the radius of the circle.
	 * @param fill
	 *            the paint to be used for rendering the circle.
	 * @param stroke
	 *            an optional paint for the circle casing (may be null).
	 * @param level
	 *            the drawing level on which the circle should be rendered.
	 */
	virtual void renderPointOfInterestCircle(float radius, const QBrush &fill, const QPen &stroke, int level) = 0;

	/**
	 * Renders a point of interest symbol with the given bitmap.
	 *
	 * @param symbol
	 *            the symbol to be rendered.
	 */
	virtual void renderPointOfInterestSymbol(const QImage &symbol) = 0;

	/**
	 * Renders a way with the given parameters.
	 *
	 * @param stroke
	 *            the paint to be used for rendering the way.
	 * @param level
	 *            the drawing level on which the way should be rendered.
	 */
	virtual void renderWay(const QPen &stroke, int level) = 0;

	/**
	 * Renders a way with the given symbol along the way path.
	 *
	 * @param symbol
	 *            the symbol to be rendered.
	 * @param alignCenter
	 *            true if the symbol should be centered, false otherwise.
	 * @param repeat
	 *            true if the symbol should be repeated, false otherwise.
	 */
	virtual void renderWaySymbol(const QImage &symbol, bool alignCenter, bool repeat) = 0;

	/**
	 * Renders a way with the given text along the way path.
	 *
	 * @param text
	 *            the text to be rendered.
	 * @param fill
	 *            the paint to be used for rendering the text.
	 * @param stroke
	 *            an optional paint for the text casing (may be null).
	 */
	virtual void renderWayText(const QString &text, const QFont &font, const Paint &fill, const Paint &stroke) = 0;
};

#endif // MAPSFORGE_RENDERCALLBACK_H
