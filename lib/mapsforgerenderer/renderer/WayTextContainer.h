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
#ifndef WAYTEXTCONTAINER_H
#define WAYTEXTCONTAINER_H

#include <QFont>
#include <QPen>
#include <QString>
#include <QPoint>

typedef QPoint Point;
typedef QPen Paint;

class WayTextContainer
{
public:
	WayTextContainer(const Point &p1, const Point &p2, const QString &text, const QFont &font, const Paint &paint);

	Point p1() const { return m_p1; }
	Point p2() const { return m_p2; }
	QString text() const { return m_text; }
	const Paint &paint() const { return m_paint; }
	const QFont &font() const { return m_font; }

private:
	Point m_p1;
	Point m_p2;
	QString m_text;
	Paint m_paint;
	QFont m_font;
};

#endif // WAYTEXTCONTAINER_H
