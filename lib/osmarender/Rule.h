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

#ifndef MAPSFORGE_RULE_H
#define MAPSFORGE_RULE_H

#include <QList>
#include <QMap>

#include <utility>

typedef std::pair<QString, QString> Tag;

typedef char byte;

class ClosedMatcher;
class ElementMatcher;
class RenderCallback;
class RenderInstruction;

class Rule
{
public:
	Rule(const ClosedMatcher *closedMatcher, const ElementMatcher *elementMatcher, byte zoomMin, byte zoomMax);

	virtual ~Rule();

	void addRenderingInstruction(RenderInstruction *renderInstruction);

	void addSubRule(Rule *rule);

	virtual bool matchesNode(const QList<Tag> &tags, byte zoomLevel) const = 0;

	virtual bool matchesWay(const QList<Tag> &tags, byte zoomLevel, bool isClosed) const = 0;

	void matchNode(RenderCallback *renderCallback, const QList<Tag> &tags, byte zoomLevel) const;

	void matchWay(QList<const RenderInstruction *> &matchingList, const QList<Tag> &tags, byte zoomLevel, bool isClosed) const;

	void scaleStrokeWidth(float scaleFactor);

	void scaleTextSize(float scaleFactor);

	const ClosedMatcher *closedMatcher() const;

	const ElementMatcher *elementMatcher() const;

protected:
	const ClosedMatcher *const m_closedMatcher;
	const ElementMatcher *const m_elementMatcher;
	const byte m_zoomMax;
	const byte m_zoomMin;
	QList<RenderInstruction *> m_renderInstructions;
	QList<Rule *> m_subRules;
};

#endif // MAPSFORGE_RULE_H
