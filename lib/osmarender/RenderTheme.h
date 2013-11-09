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

#ifndef MAPASFORGE_RENDERTHEME_H
#define MAPASFORGE_RENDERTHEME_H

#include <QCache>
#include <QColor>
#include <QList>
#include <QVector>

#include <utility>

typedef std::pair<QString, QString> Tag;

typedef char byte;

class RenderCallback;
class RenderInstruction;
class Rule;

/**
 * A RenderTheme defines how ways and nodes are drawn.
 */
class RenderTheme
{
public:
	RenderTheme(float baseStrokeWidth, float baseTextSize, const QColor &mapBackground);

	/**
	 * Must be called when this RenderTheme gets destroyed to clean up and free resources.
	 */
	~RenderTheme();

	/**
	 * @return the number of distinct drawing levels required by this RenderTheme.
	 */
	int getLevels() const;

	/**
	 * @return the map background color of this RenderTheme.
	 */
	QColor getMapBackground() const;

	/**
	 * Matches a closed way with the given parameters against this RenderTheme.
	 *
	 * @param renderCallback
	 *            the callback implementation which will be executed on each match.
	 * @param tags
	 *            the tags of the way.
	 * @param zoomLevel
	 *            the zoom level at which the way should be matched.
	 */
	void matchClosedWay(RenderCallback *renderCallback, const QList<Tag> &tags, byte zoomLevel) const;

	/**
	 * Matches a linear way with the given parameters against this RenderTheme.
	 *
	 * @param renderCallback
	 *            the callback implementation which will be executed on each match.
	 * @param tags
	 *            the tags of the way.
	 * @param zoomLevel
	 *            the zoom level at which the way should be matched.
	 */
	void matchLinearWay(RenderCallback *renderCallback, const QList<Tag> &tags, byte zoomLevel) const;

	/**
	 * Matches a node with the given parameters against this RenderTheme.
	 *
	 * @param renderCallback
	 *            the callback implementation which will be executed on each match.
	 * @param tags
	 *            the tags of the node.
	 * @param zoomLevel
	 *            the zoom level at which the node should be matched.
	 */
	void matchNode(RenderCallback *renderCallback, const QList<Tag> &tags, byte zoomLevel) const;

	/**
	 * Scales the stroke width of this RenderTheme by the given factor.
	 *
	 * @param scaleFactor
	 *            the factor by which the stroke width should be scaled.
	 */
	void scaleStrokeWidth(float scaleFactor);

	/**
	 * Scales the text size of this RenderTheme by the given factor.
	 *
	 * @param scaleFactor
	 *            the factor by which the text size should be scaled.
	 */
	void scaleTextSize(float scaleFactor);

	void addRule(Rule *rule);

	void setLevels(int levels);

	void setMapBackground(const QColor &mapBackground);

private:
	void matchWay(RenderCallback *renderCallback, const QList<Tag> &tags, byte zoomLevel, bool isClosed) const;

private:
	static const int MATCHING_CACHE_SIZE;

	class WayCacheKey
	{
	public:
		WayCacheKey(const QList<Tag> &tags, byte zoomLevel, bool isClosed);

		bool operator==(const WayCacheKey& other) const;

	private:
		friend int qHash(const WayCacheKey& );
		int m_hash;
	};
	friend int qHash(const RenderTheme::WayCacheKey& );

	const float m_baseStrokeWidth;
	const float m_baseTextSize;
	int m_levels;
	QColor m_mapBackground;
	mutable QCache<WayCacheKey, QList<const RenderInstruction *> > m_wayCache; // FIXME mutable
	QVector<Rule *> m_rulesList;
};

int qHash(const RenderTheme::WayCacheKey& );

#endif // MAPASFORGE_RENDERTHEME_H
