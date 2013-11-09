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

#ifndef MAPSFORGE_CAPTION_H
#define MAPSFORGE_CAPTION_H

#include "../RenderInstruction.h"

#include <QFont>
#include <QList>
#include <QPen>

typedef QPen Paint;

/**
 * Represents a text label on the map.
 */
class Caption : public RenderInstruction
{
public:
	Caption(const Paint &fill, const Paint &stroke, const QFont &font, float dy, float fontSize, const QString &caption);

	void renderNode(RenderCallback *renderCallback, const QList<Tag> &tags) const;

	void renderWay(RenderCallback *renderCallback, const QList<Tag> &tags) const;

	void scaleStrokeWidth(float scaleFactor);

	void scaleTextSize(float scaleFactor);

private:
	Paint m_fill;
	Paint m_stroke;
	QFont m_font;
	float m_dy;
	float m_fontSize;
	QString m_tagsKey;
};

#endif // MAPSFORGE_CAPTION_H
