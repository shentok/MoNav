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

#ifndef SYMBOLCONTAINER_H
#define SYMBOLCONTAINER_H

#include <QImage>
#include <QPoint>

typedef QPoint Point;

class SymbolContainer
{
public:
	SymbolContainer(const QImage &symbol, const Point &point);

	SymbolContainer(const QImage &symbol, const Point &point, bool alignCenter, float theta);

	QImage symbol() const { return m_symbol; }
	Point point() const { return m_point; }
	int width() const { return m_symbol.width(); }
	int height() const { return m_symbol.height(); }
	bool alignCenter() const { return m_alignCenter; }
	float theta() const { return m_theta; }

private:
	QImage m_symbol;
	Point m_point;
	bool m_alignCenter;
	float m_theta;
};

#endif // SYMBOLCONTAINER_H
