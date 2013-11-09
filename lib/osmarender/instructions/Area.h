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

#ifndef MAPSFORGE_AREA_H
#define MAPSFORGE_AREA_H

#include "../RenderInstruction.h"

#include <QList>
#include <QBrush>
#include <QPen>

class RenderCallback;

/**
 * Represents a closed polygon on the map.
 */
class Area : public RenderInstruction
{
public:
	Area(const QBrush &fill, const QPen &stroke, float strokeWidth, int level);

	void renderNode(RenderCallback *renderCallback, const QList<Tag> &tags) const;

	void renderWay(RenderCallback *renderCallback, const QList<Tag> &tags) const;

	void scaleStrokeWidth(float scaleFactor);

	void scaleTextSize(float scaleFactor);

private:
	QBrush m_fill;
	QPen m_stroke;
	int m_level;
	float m_strokeWidth;
};

#endif // MAPSFORGE_AREA_H
