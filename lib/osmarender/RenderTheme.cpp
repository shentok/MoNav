#include "RenderTheme.h"

#include "RenderInstruction.h"
#include "Rule.h"

const int RenderTheme::MATCHING_CACHE_SIZE = 512;

RenderTheme::WayCacheKey::WayCacheKey(const QList<Tag> &tags, byte zoomLevel, bool isClosed) :
	m_hash(0)
{
	foreach(const Tag &tag, tags) {
		m_hash ^= qHash(tag.first);
		m_hash ^= qHash(tag.second);
	}

	m_hash ^= qHash(zoomLevel);
	m_hash ^= qHash(isClosed);
}

bool RenderTheme::WayCacheKey::operator==(const RenderTheme::WayCacheKey &other) const
{
	return m_hash == other.m_hash;
}

int qHash(const RenderTheme::WayCacheKey &wayCacheKey)
{
	return wayCacheKey.m_hash;
}

RenderTheme::RenderTheme(float baseStrokeWidth, float baseTextSize, const QColor &mapBackground) :
	m_baseStrokeWidth(baseStrokeWidth),
	m_baseTextSize(baseTextSize),
	m_levels(0),
	m_mapBackground(mapBackground)
{
	m_wayCache.setMaxCost(MATCHING_CACHE_SIZE);
}

RenderTheme::~RenderTheme()
{
	m_wayCache.clear();
}

int RenderTheme::getLevels() const
{
	return m_levels;
}

void RenderTheme::setMapBackground(const QColor &mapBackground)
{
	m_mapBackground = mapBackground;
}

QColor RenderTheme::getMapBackground() const
{
	return m_mapBackground;
}

void RenderTheme::matchClosedWay(RenderCallback *renderCallback, const QList<Tag> &tags, byte zoomLevel) const
{
	matchWay(renderCallback, tags, zoomLevel, true);
}

void RenderTheme::matchLinearWay(RenderCallback *renderCallback, const QList<Tag> &tags, byte zoomLevel) const
{
	matchWay(renderCallback, tags, zoomLevel, false);
}

void RenderTheme::matchNode(RenderCallback *renderCallback, const QList<Tag> &tags, byte zoomLevel) const
{
	foreach (const Rule *rule, m_rulesList) {
		rule->matchNode(renderCallback, tags, zoomLevel);
	}
}

void RenderTheme::scaleStrokeWidth(float scaleFactor)
{
	foreach (Rule *rule, m_rulesList) {
		rule->scaleStrokeWidth(scaleFactor * m_baseStrokeWidth);
	}
}

void RenderTheme::scaleTextSize(float scaleFactor)
{
	foreach (Rule *rule, m_rulesList) {
		rule->scaleTextSize(scaleFactor * m_baseTextSize);
	}
}

void RenderTheme::addRule(Rule *rule)
{
	m_rulesList.append(rule);
}

void RenderTheme::setLevels(int levels)
{
	m_levels = levels;
}

void RenderTheme::matchWay(RenderCallback *renderCallback, const QList<Tag> &tags, byte zoomLevel, bool isClosed) const
{
	const WayCacheKey matchingCacheKey = WayCacheKey(tags, zoomLevel, isClosed);

	QList<const RenderInstruction *> *matchingList = m_wayCache.object(matchingCacheKey);
	if (matchingList == 0) {
		// cache miss
		matchingList = new QList<const RenderInstruction *>();
		foreach (const Rule *rule, m_rulesList) {
			rule->matchWay(*matchingList, tags, zoomLevel, isClosed);
		}

		m_wayCache.insert(matchingCacheKey, matchingList);
	}

	foreach (const RenderInstruction *renderInstruction, *matchingList) {
		renderInstruction->renderWay(renderCallback, tags);
	}
}
