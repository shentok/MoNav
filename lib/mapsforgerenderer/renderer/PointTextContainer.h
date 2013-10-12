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

#ifndef POINTTEXTCONTAINER_H
#define POINTTEXTCONTAINER_H

#include <QFont>
#include <QPen>
#include <QRect>
#include <QString>

typedef QRect Rectangle;
typedef QPoint Point;
typedef QPen Paint;

class SymbolContainer;

class PointTextContainer
{
public:
	/**
	 * Create a new point container, that holds the x-y coordinates of a point, a text variable and one paint objects.
	 *
	 * @param text
	 *            the text of the point.
	 * @param x
	 *            the x coordinate of the point.
	 * @param y
	 *            the y coordinate of the point.
	 * @param paintFront
	 *            the paintFront for the point.
	 */
	PointTextContainer(const QString &text, const Point &point, const QFont &font, const Paint &paintFront);

	/**
	 * Create a new point container, that holds the x-y coordinates of a point, a text variable and two paint objects.
	 *
	 * @param text
	 *            the text of the point.
	 * @param x
	 *            the x coordinate of the point.
	 * @param y
	 *            the y coordinate of the point.
	 * @param paintFront
	 *            the paintFront for the point.
	 * @param paintBack
	 *            the paintBack for the point.
	 */
	PointTextContainer(const QString &text, const Point &point, const QFont &font, const Paint &paintFront, const Paint &paintBack);

	/**
	 * Create a new point container, that holds the x-y coordinates of a point, a text variable, two paint objects, and
	 * a reference on a symbol, if the text is connected with a POI.
	 *
	 * @param text
	 *            the text of the point.
	 * @param x
	 *            the x coordinate of the point.
	 * @param y
	 *            the y coordinate of the point.
	 * @param paintFront
	 *            the paintFront for the point.
	 * @param paintBack
	 *            the paintBack for the point.
	 * @param symbol
	 *            the connected Symbol.
	 */
	PointTextContainer(const QString &text, const Point &point, const QFont &font, const Paint &paintFront, const Paint &paintBack, const SymbolContainer *symbol);

	QString text() const { return m_text; }

	Point point() const { return m_point; }

	const Paint &paintFront() const { return m_paintFront; }
	const Paint &paintBack() const { return m_paintBack; }

	Rectangle boundary() const { return m_boundary; }
	int &rx() { return m_point.rx(); }
	const QFont &font() const { return m_font; }
	const SymbolContainer *symbol() const { return m_symbol; }

private:
	QString m_text;
	Paint m_paintFront;
	Paint m_paintBack;
	QFont m_font;
	const SymbolContainer *m_symbol;
	Point m_point;
	Rectangle m_boundary;
};

#endif // POINTTEXTCONTAINER_H
