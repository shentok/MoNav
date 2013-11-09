#include "RenderThemeHandler.h"

#include "instructions/Area.h"
#include "instructions/Caption.h"
#include "instructions/Circle.h"
#include "instructions/Line.h"
#include "instructions/LineSymbol.h"
#include "instructions/PathText.h"
#include "instructions/Symbol.h"
#include "AnyMatcher.h"
#include "KeyMatcher.h"
#include "NegativeMatcher.h"
#include "ValueMatcher.h"
#include "NegativeRule.h"
#include "PositiveRule.h"
#include "RenderTheme.h"
#include "Rule.h"

#include <QFileInfo>
#include <QDebug>
#include <QFont>
#include <QUrl>

#include <limits>

const QString RenderThemeHandler::ELEMENT_NAME_RULE = "rule";
const QString RenderThemeHandler::UNEXPECTED_ELEMENT = "unexpected element:";

const ClosedAnyMatcher RenderThemeHandler::s_closedMatcherAny;
const ClosedWayMatcher RenderThemeHandler::s_closedMatcherYes;
const LinearWayMatcher RenderThemeHandler::s_closedMatcherNo;

const ElementAnyMatcher RenderThemeHandler::s_elementMatcherAny;
const ElementNodeMatcher RenderThemeHandler::s_elementMatcherNode;
const ElementWayMatcher RenderThemeHandler::s_elementMatcherWay;

RenderThemeHandler::RenderThemeHandler(const QString &relativePathPrefix) :
	m_relativePathPrefix(relativePathPrefix),
	m_renderTheme(0)
{
}

RenderThemeHandler::~RenderThemeHandler()
{
	delete m_renderTheme;
}

RenderTheme *RenderThemeHandler::releaseRenderTheme()
{
	if (!m_errorMessage.isEmpty()) {
		qWarning() << m_errorMessage.toAscii().constData();
		delete m_renderTheme;
		m_renderTheme = 0;
	}

	if (m_renderTheme == 0) {
		qWarning() << "failed to load render theme";
	}

	RenderTheme *const renderTheme = m_renderTheme;

	m_renderTheme = 0;

	return renderTheme;
}

bool RenderThemeHandler::endDocument()
{
	if (m_renderTheme == 0) {
		m_errorMessage = "missing element: rendertheme";
		return false;
	}

	m_renderTheme->setLevels(m_level);

	return true;
}

bool RenderThemeHandler::endElement(const QString &uri, const QString &localName, const QString &qName)
{
	if (m_elementStack.isEmpty()) {
		m_errorMessage = "unexpected end of file";
		return false;
	}

	m_elementStack.pop();

	if (qName == ELEMENT_NAME_RULE) {
		m_ruleStack.pop();
	}

	return true;
}

bool RenderThemeHandler::error(const QXmlParseException &exception)
{
	qWarning() << exception.message();

	return true;
}

QString RenderThemeHandler::errorString() const
{
	return m_errorMessage;
}

bool RenderThemeHandler::startDocument()
{
	m_level = 0;
	m_errorMessage.clear();
	delete m_renderTheme;
	m_renderTheme = 0;
	m_elementStack.clear();
	m_ruleStack.clear();

	return true;
}

bool RenderThemeHandler::startElement(const QString &uri, const QString &localName, const QString &qName, const QXmlAttributes &attributes)
{
	Q_UNUSED(uri)
	Q_UNUSED(localName)

	if (qName == "rendertheme") {
		if (!checkState(qName, ElementRenderTheme)) {
			return false;
		}

		QColor mapBackground = QColor(Qt::white);
		float baseStrokeWidth = 1;
		float baseTextSize = 1;
		bool hasVersion = false;

		for (int i = 0; i < attributes.length(); ++i) {
			const QString name = attributes.qName(i);
			const QString value = attributes.value(i);

			if (name == "xmlns") {
				continue;
			} else if (name == "xmlns:xsi") {
				continue;
			} else if (name == "xsi:schemaLocation") {
				continue;
			} else if (name == "version") {
				hasVersion = true;
				if (value.trimmed() != "3") {
					return false;
				}
			} else if (name == "map-background") {
				mapBackground = parseColor(value);
			} else if (name == "base-stroke-width") {
				baseStrokeWidth = parseNonNegativeFloat(name, value);
			} else if (name == "base-text-size") {
				baseTextSize = parseNonNegativeFloat(name, value);
			} else {
				return false;
			}
		}

		if (!m_errorMessage.isEmpty()) {
			return false;
		}

		if (!hasVersion) {
			m_errorMessage = "missing attribute: version";
			return false;
		}

		m_renderTheme = new RenderTheme(baseStrokeWidth, baseTextSize, mapBackground);
	}

	else if (qName == ELEMENT_NAME_RULE) {
		if (!checkState(qName, ElementRule)) {
			return false;
		}

		bool hasKeys = false;
		bool hasValues = false;
		const ElementMatcher *elementMatcher = 0;
		const ClosedMatcher *closedMatcher = &s_closedMatcherAny;
		QStringList keyList;
		QStringList valueList;
		char zoomMin = 0;
		char zoomMax = std::numeric_limits<char>::max();

		for (int i = 0; i < attributes.length(); ++i) {
			const QString name = attributes.qName(i);
			const QString value = attributes.value(i);

			if (name == "e") {
				if (elementMatcher) {
					m_errorMessage = "'e' attibute given twice";
					return false;
				}

				if (value == "any") {
					elementMatcher = &s_elementMatcherAny;
				}
				else if (value == "node") {
					elementMatcher = &s_elementMatcherNode;
				}
				else if (value == "way") {
					elementMatcher = &s_elementMatcherWay;
				}
				else {
					m_errorMessage = QString("unknown value for attribute \"%1\": %2").arg(qName).arg(value);
					return false;
				}
			} else if (name == "k") {
				if (hasKeys) {
					m_errorMessage = "'k' attribute given twice";
					return false;
				}

				keyList = value.split('|');
				hasKeys = true;
			} else if (name == "v") {
				if (hasValues) {
					m_errorMessage = "'v' attribute given twice";
					return false;
				}

				valueList = value.split('|');
				hasValues = true;
			} else if (name == "closed") {
				if (value == "any") {
					closedMatcher = &s_closedMatcherAny;
				}
				else if (value == "no") {
					closedMatcher = &s_closedMatcherNo;
				}
				else if (value == "yes") {
					closedMatcher = &s_closedMatcherYes;
				}
				else {
					m_errorMessage = QString("unknown value for attribute \"closed\": %1").arg(value);
					return false;
				}
			} else if (name == "zoom-min") {
				zoomMin = parseNonNegativeByte(name, value);
			} else if (name == "zoom-max") {
				zoomMax = parseNonNegativeByte(name, value);
			} else {
				m_errorMessage = QString("unknown attribute: %1").arg(name);
				return false;
			}
		}

		if (elementMatcher == 0) {
			m_errorMessage = QString("expected element '%1'' to have attribute 'e'").arg(qName);
			return false;
		}
		if (!hasKeys) {
			m_errorMessage = QString("expected element '%1'' to have attribute 'k'").arg(qName);
			return false;
		}
		if (!hasValues) {
			m_errorMessage = QString("expected element '%1'' to have attribute 'v'").arg(qName);
			return false;
		}
		if (zoomMin > zoomMax) {
			m_errorMessage = QString("'zoom-min' > 'zoom-max': %1 %2").arg(zoomMin).arg(zoomMax);
			return false;
		}

		elementMatcher = optimize(elementMatcher, m_ruleStack);
		closedMatcher = optimize(closedMatcher, m_ruleStack);

		Rule *rule = 0;
		if (valueList.removeOne("~")) {
			NegativeMatcher *negativeMatcher = new NegativeMatcher(keyList, valueList);
			rule = new NegativeRule(closedMatcher, elementMatcher, zoomMin, zoomMax, negativeMatcher);
		}
		else {
			const AttributeMatcher *keyMatcher = (keyList.at(0) == "*") ? static_cast<AttributeMatcher *>(new AnyMatcher) : static_cast<AttributeMatcher *>(new KeyMatcher(keyList));
			const AttributeMatcher *valueMatcher = (valueList.at(0) == "*") ? static_cast<AttributeMatcher *>(new AnyMatcher) : static_cast<AttributeMatcher *>(new ValueMatcher(valueList));

			keyMatcher = optimizeKeyMatcher(keyMatcher, m_ruleStack);
			valueMatcher = optimizeValueMatcher(valueMatcher, m_ruleStack);

			rule = new PositiveRule(closedMatcher, elementMatcher, zoomMin, zoomMax, keyMatcher, valueMatcher);
		}

		if (m_ruleStack.isEmpty()) {
			Q_ASSERT(m_renderTheme != 0);
			m_renderTheme->addRule(rule);
		}
		else {
			m_ruleStack.top()->addSubRule(rule);
		}
		m_ruleStack.push(rule);
	}

	else if (qName == "area") {
		if (!checkState(qName, ElementRenderingInstruction)) {
			return false;
		}

		const int level = m_level++;
		float strokeWidth = 0;

		QBrush fill;
		fill.setColor(Qt::black);

		QPen stroke;
		stroke.setColor(Qt::transparent);
		stroke.setCapStyle(Qt::RoundCap);

		for (int i = 0; i < attributes.length(); ++i) {
			const QString name = attributes.qName(i);
			const QString value = attributes.value(i);

			if (name == "src") {
				fill.setTextureImage(QImage(absolutePath(value)));
			}
			else if (name == "fill") {
				const QColor color = parseColor(value);
				if (!color.isValid()) {
					return false;
				}
				fill.setStyle(Qt::SolidPattern);
				fill.setColor(color);
			}
			else if (name == "stroke") {
				const QColor color = parseColor(value);
				if (!color.isValid()) {
					return false;
				}
				stroke.setColor(color);
			}
			else if (name == "stroke-width") {
				strokeWidth = parseNonNegativeFloat(name, value);
			}
			else {
				m_errorMessage = QString("unexpected attribute in element %1: %2").arg(qName).arg(name);
				return false;
			}
		}

		Area *area = new Area(fill, stroke, strokeWidth, level);
		m_ruleStack.top()->addRenderingInstruction(area);
	}

	else if (qName == "caption") {
		if (!checkState(qName, ElementRenderingInstruction)) {
			return false;
		}

		bool hasText = false;
		QString textKey;
		float dy = 0;
		float fontSize = 0;

		QPen fill;
		fill.setColor(Qt::black);
#warning		fill.setTextAlign(Paint::LEFT);

		QPen stroke;
		stroke.setColor(Qt::black);
#warning		stroke.setTextAlign(Paint::LEFT);

		QFont font;

		for (int i = 0; i < attributes.length(); ++i) {
			const QString name = attributes.qName(i);
			const QString value = attributes.value(i);

			if (name == "k") {
				hasText = true;
				textKey = value;
			} else if (name == "dy") {
				bool ok;
				dy = value.toFloat(&ok);
				if (!ok) {
					m_errorMessage = QString("value for attribute 'dy' is not a float: %1").arg(value);
					return false;
				}
			} else if (name == "font-family") {
				font.setFamily(value);
			} else if (name == "font-style") {
#if (QT_VERSION >= 0x040800)
				font.setStyleName(value);
#endif
			} else if (name == "font-size") {
				fontSize = parseNonNegativeFloat(name, value);
			} else if (name == "fill") {
				fill.setColor(parseColor(value));
			} else if (name == "stroke") {
				stroke.setColor(parseColor(value));
			} else if (name == "stroke-width") {
				stroke.setWidthF(parseNonNegativeFloat(name, value));
			}
			else {
				m_errorMessage = QString("unexpected attribute in element %1: %2").arg(qName).arg(name);
				return false;
			}
		}

		if (!hasText) {
			m_errorMessage = QString("expected element '%1'' to have attribute 'k'").arg(qName);
			return false;
		}

		Caption *caption = new Caption(fill, stroke, font, dy, fontSize, textKey);
		m_ruleStack.top()->addRenderingInstruction(caption);
	}

	else if (qName == "circle") {
		if (!checkState(qName, ElementRenderingInstruction)) {
			return false;
		}

		bool hasRadius = false;
		const int level = m_level++;
		float radius = 0;
		float strokeWidth = 0;
		bool scaleRadius;

		QBrush fill;
		fill.setColor(Qt::transparent);

		QPen stroke;
		stroke.setColor(Qt::transparent);

		for (int i = 0; i < attributes.length(); ++i) {
			const QString name = attributes.qName(i);
			const QString value = attributes.value(i);

			if (name == "radius") {
				hasRadius = true;
				radius = parseNonNegativeFloat(name, value);
				if (!m_errorMessage.isEmpty()) {
					return false;
				}
			} else if (name == "scale-radius") {
				scaleRadius = parseBool(name, value);
				if (!m_errorMessage.isEmpty()) {
					return false;
				}
			} else if (name == "fill") {
				fill.setStyle(Qt::SolidPattern);
				fill.setColor(parseColor(value));
			} else if (name == "stroke") {
				stroke.setColor(parseColor(value));
			} else if (name == "stroke-width") {
				strokeWidth = parseNonNegativeFloat(name, value);
			}
			else {
				m_errorMessage = QString("unexpected attribute in element %1: %2").arg(qName).arg(name);
				return false;
			}
		}

		if (!hasRadius) {
			m_errorMessage = QString("expected element '%1' to have attribute 'radius'").arg(qName);
			return false;
		}

		Circle *circle = new Circle(fill, stroke, radius, strokeWidth, scaleRadius, level);
		m_ruleStack.top()->addRenderingInstruction(circle);
	}

	else if (qName == "line") {
		if (!checkState(qName, ElementRenderingInstruction)) {
			return false;
		}

		const int level = m_level++;
		float strokeWidth = 0;

		QPen stroke;
		stroke.setColor(Qt::black);
		stroke.setCapStyle(Qt::RoundCap);

		for (int i = 0; i < attributes.length(); ++i) {
			const QString name = attributes.qName(i);
			const QString value = attributes.value(i);

			if (name == "src") {
				QBrush brush;
				brush.setTextureImage(QImage(absolutePath(value)));
				stroke.setBrush(brush);
			} else if (name == "stroke") {
				stroke.setColor(parseColor(value));
			} else if (name == "stroke-width") {
				strokeWidth = parseNonNegativeFloat(name, value);
			} else if (name == "stroke-dasharray") {
				stroke.setDashPattern(parseFloatArray(name, value));
			} else if (name == "stroke-linecap") {
				stroke.setCapStyle(parseCap(name, value));
			}
			else {
				m_errorMessage = QString("unexpected attribute in element %1: %2").arg(qName).arg(name);
				return false;
			}
		}

		Line *line = new Line(stroke, strokeWidth, level);
		m_ruleStack.top()->addRenderingInstruction(line);
	}

	else if (qName == "lineSymbol") {
		if (!checkState(qName, ElementRenderingInstruction)) {
			return false;
		}

		bool hasSrc = false;
		bool alignCenter = false;
		bool repeat = false;
		QImage symbol;

		for (int i = 0; i < attributes.length(); ++i) {
			const QString name = attributes.qName(i);
			const QString value = attributes.value(i);

			if (name == "src") {
				if (hasSrc) {
					return false;
				}

				hasSrc = true;
				symbol = QImage(absolutePath(value));
			} else if (name == "align-center") {
				alignCenter = parseBool(name, value);
			} else if (name == "repeat") {
				repeat = parseBool(name, value);
			}
			else {
				m_errorMessage = QString("unexpected attribute in element %1: %2").arg(qName).arg(name);
				return false;
			}
		}

		if (!hasSrc) {
			m_errorMessage = QString("expected element '%1' to have attribute 'src'").arg(qName);
			return false;
		}

		LineSymbol *lineSymbol = new LineSymbol(alignCenter, symbol, repeat);
		m_ruleStack.top()->addRenderingInstruction(lineSymbol);
	}

	else if (qName == "pathText") {
		if (!checkState(qName, ElementRenderingInstruction)) {
			return false;
		}

		bool hasK = false;
		float fontSize = 0;
		QString textKey;

		QPen fill;
		fill.setColor(Qt::black);
#warning		fill.setTextAlign(Paint::CENTER);

		QPen stroke;
		stroke.setColor(Qt::black);
#warning		stroke.setTextAlign(Paint::CENTER);

		QFont font;

		for (int i = 0; i < attributes.length(); ++i) {
			const QString name = attributes.qName(i);
			const QString value = attributes.value(i);

			if (name == "k") {
				hasK = true;
				textKey = value;
			} else if (name == "font-family") {
				font.setFamily(value);
			} else if (name == "font-style") {
#if (QT_VERSION >= 0x040800)
				font.setStyleName(value);
#endif
			} else if (name == "font-size") {
				fontSize = parseNonNegativeFloat(name, value);
			} else if (name == "fill") {
				fill.setColor(parseColor(value));
			} else if (name == "stroke") {
				stroke.setColor(parseColor(value));
			} else if (name == "stroke-width") {
				stroke.setWidthF(parseNonNegativeFloat(name, value));
			}
			else {
				m_errorMessage = QString("unexpected attribute in element %1: %2").arg(qName).arg(name);
				return false;
			}
		}

		if (!hasK) {
			m_errorMessage = QString("expected element '%1' to have attribute 'k'").arg(qName);
			return false;
		}

		PathText *pathText = new PathText(fill, stroke, font, fontSize, textKey);
		m_ruleStack.top()->addRenderingInstruction(pathText);
	}

	else if (qName == "symbol") {
		if (!checkState(qName, ElementRenderingInstruction)) {
			return false;
		}

		bool hasImage = false;
		QImage bitmap;

		for (int i = 0; i < attributes.length(); ++i) {
			const QString name = attributes.qName(i);
			const QString value = attributes.value(i);

			if (name == "src") {
				hasImage = true;
				bitmap = QImage(absolutePath(value));
			}
			else {
				m_errorMessage = QString("unexpected attribute in element %1: %2").arg(qName).arg(name);
				return false;
			}
		}

		if (!hasImage) {
			m_errorMessage = QString("expected element '%1' to have attribute 'src'").arg(qName);
			return false;
		}

		Symbol *symbol = new Symbol(bitmap);
		m_ruleStack.top()->addRenderingInstruction(symbol);
	}

	else {
		m_errorMessage = QString("unknown element \"%1\"").arg(qName);
		qWarning() << m_errorMessage.toLatin1().constData();
		return false;
	}

	return true;
}

bool RenderThemeHandler::warning(const QXmlParseException &exception)
{
	qWarning() << exception.message();

	return true;
}

QColor RenderThemeHandler::parseColor(const QString &colorString)
{
	if (colorString.isEmpty() || colorString.at(0) != '#') {
		m_errorMessage = QString("unsupported color format: %1").arg(colorString);
		return QColor();
	} else if (colorString.length() == 7) {
		bool ok = true;
		const int red = colorString.mid(1, 2).toInt(&ok, 16);
		const int green = colorString.mid(3, 2).toInt(&ok, 16);
		const int blue = colorString.mid(5, 2).toInt(&ok, 16);
		return QColor(red, green, blue);
	} else if (colorString.length() == 9) {
		bool ok = true;
		const int alpha = colorString.mid(1, 2).toInt(&ok, 16);
		const int red = colorString.mid(3, 2).toInt(&ok, 16);
		const int green = colorString.mid(5, 2).toInt(&ok, 16);
		const int blue = colorString.mid(7, 2).toInt(&ok, 16);
		return QColor(red, green, blue, alpha);
	}

	m_errorMessage = QString("unsupported color format: %1)").arg(colorString);
	return QColor();
}

float RenderThemeHandler::parseNonNegativeFloat(const QString &name, const QString &value)
{
	bool ok;
	const float result = value.toFloat(&ok);

	if (!ok) {
		m_errorMessage = QString("%1 is not a float: %2").arg(name).arg(value);
	}

	return result;
}

unsigned char RenderThemeHandler::parseNonNegativeByte(const QString &name, const QString &value)
{
	bool ok;
	const unsigned short result = value.toUShort(&ok);

	if (!ok) {
		m_errorMessage = QString("%1 is not a number: %2").arg(name).arg(value);
	}
	else if (result > std::numeric_limits<unsigned char>::max()) {
		m_errorMessage = QString("value of attribute '%1' exceeds maximum of %2: %3").arg(name).arg(std::numeric_limits<unsigned char>::max()).arg(result);
	}

	return result;
}

bool RenderThemeHandler::parseBool(const QString &name, const QString &value)
{
	if (value == "true") {
		return true;
	}
	if (value == "false") {
		return false;
	}

	m_errorMessage = QString("unknown value for attribute '%1': %2").arg(name).arg(value);

	return false;
}

QVector<qreal> RenderThemeHandler::parseFloatArray(const QString &name, const QString &value)
{
	const QStringList entries = value.split(',', QString::SkipEmptyParts);

	if (entries.isEmpty()) {
		m_errorMessage = QString("unknown value for attribute '%1': %2").arg(name).arg(value);
		return QVector<qreal>();
	}

	QVector<qreal> result;

	foreach (const QString &entry, entries) {
		const float number = parseNonNegativeFloat("", entry);
		if (!m_errorMessage.isEmpty()) {
			return QVector<qreal>();
		}

		result << number * 0.2;
	}

	return result;
}

Qt::PenCapStyle RenderThemeHandler::parseCap(const QString &name, const QString &value)
{
	if (value == "butt") {
		return Qt::FlatCap;
	}
	if (value == "square") {
		return Qt::SquareCap;
	}

	m_errorMessage = QString("unknown value for attribute '%1': %2").arg(name).arg(value);

	return Qt::FlatCap;
}

const AttributeMatcher *RenderThemeHandler::optimizeKeyMatcher(const AttributeMatcher *matcher, const QStack<Rule *> &ruleStack)
{
	if (matcher == 0) {
		return 0;
	}

	foreach (const Rule *rule, ruleStack) {
		const PositiveRule *positiveRule = dynamic_cast<const PositiveRule *>(rule);
		if (positiveRule != 0) {
			if (positiveRule->keyMatcher()->isCoveredBy(*matcher)) {
				delete matcher;
				return new AnyMatcher;
			}
		}
	}

	return matcher;
}

const AttributeMatcher *RenderThemeHandler::optimizeValueMatcher(const AttributeMatcher *matcher, const QStack<Rule *> &ruleStack)
{
	if (matcher == 0) {
		return 0;
	}

	foreach (const Rule *rule, ruleStack) {
		const PositiveRule *positiveRule = dynamic_cast<const PositiveRule *>(rule);
		if (positiveRule != 0) {
			if (positiveRule->valueMatcher()->isCoveredBy(*matcher)) {
				delete matcher;
				return new AnyMatcher;
			}
		}
	}

	return matcher;
}

const ClosedMatcher *RenderThemeHandler::optimize(const ClosedMatcher *matcher, const QStack<Rule *> &ruleStack)
{
	if (matcher == 0) {
		return 0;
	}

	if (dynamic_cast<const ClosedAnyMatcher *>(matcher) != 0) {
		return matcher;
	}

	foreach (const Rule *rule, ruleStack) {
		if (rule->closedMatcher()->isCoveredBy(*matcher)) {
			return &s_closedMatcherAny;
		}
		else if (!matcher->isCoveredBy(*rule->closedMatcher())) {
			qWarning() << "unreachable rule: closed";
		}
	}

	return matcher;
}

const ElementMatcher *RenderThemeHandler::optimize(const ElementMatcher *elementMatcher, const QStack<Rule *> &ruleStack)
{
	if (elementMatcher == 0) {
		return 0;
	}

	if (dynamic_cast<const ElementAnyMatcher *>(elementMatcher) != 0) {
		return elementMatcher;
	}

	foreach (const Rule *rule, ruleStack) {
		if (rule->elementMatcher()->isCoveredBy(*elementMatcher)) {
			return &s_elementMatcherAny;
		}
		else if (!elementMatcher->isCoveredBy(*rule->elementMatcher())) {
			qWarning() << "unreachable rule (e)";
		}
	}

	return elementMatcher;
}

bool RenderThemeHandler::checkState(const QString &elementName, Element element)
{
	switch (element) {
	case ElementRenderTheme:
		if (m_elementStack.size() != 0) {
			m_errorMessage = "\"rendertheme\" is only recognized as root element";
			return false;
		}
		m_elementStack.push(ElementRenderTheme);
		return true;

	case ElementRule:
		if (m_elementStack.size() < 1 || (m_elementStack.top() != ElementRenderTheme && m_elementStack.top() != ElementRule)) {
			m_errorMessage = QString("unexpected element %1").arg(elementName);
			return false;
		}
		m_elementStack.push(ElementRule);
		return true;

	case ElementRenderingInstruction:
		if (m_elementStack.size() < 2 || m_elementStack.top() != ElementRule) {
			m_errorMessage = QString("unexpected element: %1").arg(elementName);
			return false;
		}
		m_elementStack.push(ElementRenderingInstruction);
		return true;

	case ElementNone:
		Q_ASSERT(false);
		return false;
	}

	return false;
}

QString RenderThemeHandler::absolutePath(const QUrl &url) const
{
	const QString prefix = url.scheme() == "jar" ? QString::fromUtf8(":/osmarender") : m_relativePathPrefix;
	const QString path = url.path().startsWith("/org/mapsforge/android/maps/rendertheme/osmarender/") ? url.path().mid(51) : url.path();
	const QString absolute = prefix + '/' + path;

	QFileInfo fileInfo(absolute);
	if (!fileInfo.exists()) {
		qWarning() << absolute << "does not exist";
	}

	return absolute;
}
