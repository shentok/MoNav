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

#ifndef MAPSFORGE_RENDERTHEMEHANDLER_H
#define MAPSFORGE_RENDERTHEMEHANDLER_H

#include <QtXml/QXmlDefaultHandler>

#include "ClosedAnyMatcher.h"
#include "ClosedWayMatcher.h"
#include "LinearWayMatcher.h"

#include "ElementAnyMatcher.h"
#include "ElementNodeMatcher.h"
#include "ElementWayMatcher.h"

#include <QColor>
#include <QString>
#include <QStack>

class QIODevice;
class QUrl;

class AttributeMatcher;
class ClosedMatcher;
class ElementMatcher;
class RenderTheme;
class Rule;

/**
 * SAX2 handler to parse XML render theme files.
 */
class RenderThemeHandler : public QXmlDefaultHandler
{
public:
	RenderThemeHandler(const QString &relativePathPrefix);

	~RenderThemeHandler();

	RenderTheme *releaseRenderTheme();

	bool endDocument();

	bool endElement(const QString &uri, const QString &localName, const QString &qName);

	bool error(const QXmlParseException &exception);

	QString errorString() const;

	bool startDocument();

	bool startElement(const QString &uri, const QString &localName, const QString &qName, const QXmlAttributes &attributes);

	bool warning(const QXmlParseException &exception);

private:
	QColor parseColor(const QString &value);

	float parseNonNegativeFloat(const QString &name, const QString &value);

	unsigned char parseNonNegativeByte(const QString &name, const QString &value);

	bool parseBool(const QString &name, const QString &value);

	QVector<qreal> parseFloatArray(const QString &name, const QString &value);

	Qt::PenCapStyle parseCap(const QString &name, const QString &value);

	static const AttributeMatcher *optimizeKeyMatcher(const AttributeMatcher *matcher, const QStack<Rule *> &ruleStack);
	static const AttributeMatcher *optimizeValueMatcher(const AttributeMatcher *matcher, const QStack<Rule *> &ruleStack);
	static const ClosedMatcher *optimize(const ClosedMatcher *matcher, const QStack<Rule *> &ruleStack);
	static const ElementMatcher *optimize(const ElementMatcher *matcher, const QStack<Rule *> &ruleStack);

	enum Element {
		ElementNone,
		ElementRenderTheme,
		ElementRenderingInstruction,
		ElementRule
	};

	bool checkState(const QString &elementName, Element element);

	QString absolutePath(const QUrl &url) const;

private:
	static const QString ELEMENT_NAME_RULE;
	static const QString UNEXPECTED_ELEMENT;

	const QString m_relativePathPrefix;
	QStack<Element> m_elementStack;
	RenderTheme *m_renderTheme;
	QStack<Rule *> m_ruleStack;
	int m_level;
	QString m_errorMessage;

	static const ClosedAnyMatcher s_closedMatcherAny;
	static const ClosedWayMatcher s_closedMatcherYes;
	static const LinearWayMatcher s_closedMatcherNo;

	static const ElementAnyMatcher s_elementMatcherAny;
	static const ElementNodeMatcher s_elementMatcherNode;
	static const ElementWayMatcher s_elementMatcherWay;
};

#endif // MAPSFORGE_RENDERTHEMEHANDLER_H
