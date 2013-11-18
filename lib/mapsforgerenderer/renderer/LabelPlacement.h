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
#ifndef LABELPLACEMENT_H
#define LABELPLACEMENT_H

#include "PointTextContainer.h"
#include "SymbolContainer.h"

#include "mapsforgereader/TileId.h"

#include <QList>

class DependencyCache;

/**
 * This class place the labels form POIs, area labels and normal labels. The main target is avoiding collisions of these
 * different labels.
 */
class LabelPlacement
{
	class ReferencePosition;

public:
	/**
	 * @param tile current tile
	 **/
	LabelPlacement(const DependencyCache *dependencyCache, const TileId &tile);

	/**
	 * The inputs are all the label and symbol objects of the current object. The output is overlap free label and
	 * symbol placement with the greedy strategy. The placement model is either the two fixed point or the four fixed
	 * point model.
	 *
	 * @param labels
	 *            labels from the current object.
	 * @param symbols
	 *            symbols of the current object.
	 * @param areaLabels
	 *            area labels from the current object.
	 * @return the processed list of labels.
	 */
	void placeLabels(QList<PointTextContainer> &labels, QList<SymbolContainer> &symbols, QList<PointTextContainer> &areaLabels);

private:
	/**
	 * Centers the labels.
	 *
	 * @param labels
	 *            labels to center
	 */
	static void centerLabels(QList<PointTextContainer> &labels);

	/**
	 * This method uses an adapted greedy strategy for the fixed four position model, above, under left and right form
	 * the point of interest. It uses no priority search tree, because it will not function with symbols only with
	 * points. Instead it uses two minimum heaps. They work similar to a sweep line algorithm but have not a O(n log n
	 * +k) runtime. To find the rectangle that has the top edge, I use also a minimum Heap. The rectangles are sorted by
	 * their y coordinates.
	 *
	 * @param labels
	 *            label positions and text
	 * @param symbols
	 *            symbol positions
	 * @param areaLabels
	 *            area label positions and text
	 * @return list of labels without overlaps with symbols and other labels by the four fixed position greedy strategy
	 */
	QList<PointTextContainer> processFourPointGreedy(const QList<PointTextContainer> &labels,
			const QList<SymbolContainer> &symbols, const QList<PointTextContainer> &areaLabels);

	static void removeEmptySymbolReferences(QList<PointTextContainer> &nodes, const QList<SymbolContainer> &symbols);

	/**
	 * The greedy algorithms need possible label positions, to choose the best among them. This method removes the
	 * reference points, that are not validate. Not validate means, that the Reference overlap with another symbol or
	 * label or is outside of the object.
	 *
	 * @param refPos
	 *            list of the potential positions
	 * @param symbols
	 *            actual list of the symbols
	 */
	static void removeNonValidateReferencePositionSymbols(QVector<ReferencePosition> &refPos, const QList<SymbolContainer> &symbols);

	/**
	 * The greedy algorithms need possible label positions, to choose the best among them. This method removes the
	 * reference points, that are not validate. Not validate means, that the Reference overlap with another symbol or
	 * label or is outside of the object.
	 *
	 * @param refPos
	 *            list of the potential positions
	 * @param areaLabels
	 *            actual list of the area labels
	 */
	static void removeNonValidateReferencePositionAreaLabels(QVector<ReferencePosition> &refPos, const QList<PointTextContainer> &areaLabels);

	/**
	 * When the LabelPlacement class generates potential label positions for an POI, there should be no possible
	 * positions, that collide with existing symbols or labels in the dependency Cache. This class implements this
	 * functionality.
	 *
	 * @param refPos
	 *            possible label positions form the two or four point Greedy
	 */
	void removeOutOfTileReferencePoints(QVector<ReferencePosition> &refPos) const;

	/**
	 * Removes all Reverence Points that intersects with Labels from the Dependency Cache.
	 */
	void removeOverlappingLabels(QVector<LabelPlacement::ReferencePosition> &refPos) const;

	void removeOverlappingSymbols(QVector<LabelPlacement::ReferencePosition> &refPos) const;

	/**
	 * This method removes the area labels, that are not visible in the actual object.
	 *
	 * @param areaLabels
	 *            area Labels from the actual object
	 */
	static void removeOutOfTileAreaLabels(QList<PointTextContainer> &areaLabels, unsigned short tileSize);

	/**
	 * This method removes the labels, that are not visible in the actual object.
	 *
	 * @param labels
	 *            Labels from the actual object
	 */
	static void removeOutOfTileLabels(QList<PointTextContainer> &labels, unsigned short tileSize);

	/**
	 * This method removes the Symbols, that are not visible in the actual object.
	 *
	 * @param symbols
	 *            Symbols from the actual object
	 */
	static void removeOutOfTileSymbols(QList<SymbolContainer> &symbols, unsigned short tileSize);

	/**
	 * This method removes all the area labels, that overlap each other. So that the output is collision free
	 *
	 * @param areaLabels
	 *            area labels from the actual object
	 */
	static void removeOverlappingAreaLabels(QList<PointTextContainer> &areaLabels);

	/**
	 * This method removes all the Symbols, that overlap each other. So that the output is collision free.
	 *
	 * @param symbols
	 *            symbols from the actual object
	 */
	static void removeOverlappingSymbols(QList<SymbolContainer> &symbols);

	/**
	 * Removes the the symbols that overlap with area labels.
	 *
	 * @param symbols
	 *            list of symbols
	 * @param pTC
	 *            list of labels
	 */
	static void removeOverlappingSymbolsWithAreaLabels(QList<SymbolContainer> &symbols, const QList<PointTextContainer> &pTC);

	struct ReferencePositionTopComparator;
	struct ReferencePositionBottomComparator;

private:
	static const int LABEL_DISTANCE_TO_LABEL = 2;
	static const int LABEL_DISTANCE_TO_SYMBOL = 2;
	static const int START_DISTANCE_TO_SYMBOLS = 4;
	static const int SYMBOL_DISTANCE_TO_SYMBOL = 2;

	const DependencyCache *const m_dependencyCache;
	TileId tile;
};

#endif // LABELPLACEMENT_H
