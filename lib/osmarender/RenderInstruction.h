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

#ifndef MAPSFORGE_RENDERINSTRUCTION_H
#define MAPSFORGE_RENDERINSTRUCTION_H

#include <QList>

#include <utility>

typedef std::pair<QString, QString> Tag;

class RenderCallback;

/**
 * A RenderInstruction is a basic graphical primitive to draw a map.
 */
class RenderInstruction
{
public:
	virtual ~RenderInstruction();

	/**
	 * @param renderCallback
	 *            a reference to the receiver of all render callbacks.
	 * @param tags
	 *            the tags of the node.
	 */
	virtual void renderNode(RenderCallback *renderCallback, const QList<Tag> &tags) const = 0;

	/**
	 * @param renderCallback
	 *            a reference to the receiver of all render callbacks.
	 * @param tags
	 *            the tags of the way.
	 */
	virtual void renderWay(RenderCallback *renderCallback, const QList<Tag> &tags) const = 0;

	/**
	 * Scales the stroke width of this RenderInstruction by the given factor.
	 *
	 * @param scaleFactor
	 *            the factor by which the stroke width should be scaled.
	 */
	virtual void scaleStrokeWidth(float scaleFactor) = 0;

	/**
	 * Scales the text size of this RenderInstruction by the given factor.
	 *
	 * @param scaleFactor
	 *            the factor by which the text size should be scaled.
	 */
	virtual void scaleTextSize(float scaleFactor) = 0;
};

#endif // MAPSFORGE_RENDERINSTRUCTION_H
