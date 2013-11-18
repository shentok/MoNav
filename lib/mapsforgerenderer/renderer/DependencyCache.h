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
#ifndef DEPENDENCYCACHE_H
#define DEPENDENCYCACHE_H

#include "mapsforgereader/TileId.h"

#include "PointTextContainer.h"

#include <QPoint>
#include <QMap>
#include <QList>
#include <QImage>

typedef QPoint Point;

/**
 * This class process the methods for the Dependency Cache. It's connected with the LabelPlacement class. The main goal
 * is, to remove double labels and symbols that are already rendered, from the actual object. Labels and symbols that,
 * would be rendered on an already drawn Tile, will be deleted too.
 */
class DependencyCache
{
public:
	/**
	 * The class holds the data for a symbol with dependencies on other tiles.
	 *
	 * @param <T>
	 *            only two types are reasonable. The DependencySymbol or DependencyText class.
	 */
	template<typename T>
	class Dependency
	{
	public:
		Dependency(const T &value, const Point &point) :
			value(value),
			point(point)
		{
		}

		T value;
		Point point;
	};

	/**
	 * Constructor for this class, that creates a hashtable for the dependencies.
	 */
	DependencyCache();

	~DependencyCache();

	/**
	 * This method fills the entries in the dependency cache of the tiles, if their dependencies.
	 *
	 * @param labels
	 *            current labels, that will be displayed.
	 * @param symbols
	 *            current symbols, that will be displayed.
	 * @param areaLabels
	 *            current areaLabels, that will be displayed.
	 */
	void fillDependencyOnTile(QList<PointTextContainer> &labels, QList<SymbolContainer> &symbols) const;

	void fillDependencyOnTile2(const QList<PointTextContainer> &labels, const QList<SymbolContainer> &symbols,
			const QList<PointTextContainer> &areaLabels);

	/**
	 * This method must be called, before the dependencies will be handled correctly. Because it sets the actual Tile
	 * and looks if it has already dependencies.
	 */
	void setCurrentTile(const TileId &tile, unsigned short tileSize);

	/**
	 * Removes the are labels from the actual list, that would be rendered in a Tile that has already be drawn.
	 *
	 * @param areaLabels
	 *            current area Labels, that will be displayed
	 */
	void removeAreaLabelsInAlreadyDrawnAreas(QList<PointTextContainer> &areaLabels) const;

	void removeSymbolsFromDrawnAreas(QList<SymbolContainer> &symbols) const;

	void removeOverlappingAreaLabelsWithDependencyLabels(QList<PointTextContainer> &areaLabels) const;

	void removeOverlappingAreaLabelsWithDependencySymbols(QList<PointTextContainer> &areaLabels) const;

	void removeOverlappingLabelsWithDependencyLabels(QList<PointTextContainer> &labels) const;

	void removeOverlappingSymbolsWithDepencySymbols(QList<SymbolContainer> &symbols, int dis) const;

	void removeOverlappingSymbolsWithDependencyLabels(QList<SymbolContainer> &symbols) const;

	unsigned short tileSize() const;

	QList<Dependency<QImage> > symbols() const;

	QList<Dependency<PointTextContainer> > labels() const;

	bool hasLeft() const;
	bool hasRight() const;
	bool hasUp() const;
	bool hasDown() const;

private:
	/**
	 * Fills the dependency entry from the object and the neighbor tiles with the dependency information, that are
	 * necessary for drawing. To do that every label and symbol that will be drawn, will be checked if it produces
	 * dependencies with other tiles.
	 *
	 * @param pTC
	 *            list of the labels
	 */
	void fillDependencyLabels(const QList<PointTextContainer> &pTC);

	void fillDependencySymbols(const QList<SymbolContainer> &symbols);

private:
	class DependencyOnTile;

	/**
	 * Hash table, that connects the Tiles with their entries in the dependency cache.
	 */
	QMap<TileId, DependencyOnTile *> m_dependencyTable;

	DependencyOnTile *m_currentDependencyOnTile;
	TileId currentTile;
	unsigned short m_tileSize;
	bool m_hasUp;
	bool m_hasDown;
	bool m_hasLeft;
	bool m_hasRight;
};

#endif // DEPENDENCYCACHE_H
