#include "PositiveRule.h"

#include "AttributeMatcher.h"
#include "ClosedMatcher.h"
#include "ElementAnyMatcher.h"

PositiveRule::PositiveRule(const ClosedMatcher *closedMatcher, const ElementMatcher *elementMatcher, byte zoomMin, byte zoomMax, const AttributeMatcher *keyMatcher, const AttributeMatcher *valueMatcher) :
	Rule(closedMatcher, elementMatcher, zoomMin, zoomMax),
	m_keyMatcher(keyMatcher),
	m_valueMatcher(valueMatcher)
{
}

PositiveRule::~PositiveRule()
{
	delete m_valueMatcher;
	delete m_keyMatcher;
}

bool PositiveRule::matchesNode(const QList<Tag> &tags, byte zoomLevel) const
{
	return m_zoomMin <= zoomLevel && m_zoomMax >= zoomLevel && m_elementMatcher->matches(ElementMatcher::ElementNode)
			&& m_keyMatcher->matches(tags) && m_valueMatcher->matches(tags);
}

bool PositiveRule::matchesWay(const QList<Tag> &tags, byte zoomLevel, bool isClosed) const
{
	return m_zoomMin <= zoomLevel && m_zoomMax >= zoomLevel && m_elementMatcher->matches(ElementMatcher::ElementWay)
			&& m_closedMatcher->matches(isClosed) && m_keyMatcher->matches(tags)
			&& m_valueMatcher->matches(tags);
}

const AttributeMatcher *PositiveRule::keyMatcher() const
{
	return m_keyMatcher;
}

const AttributeMatcher *PositiveRule::valueMatcher() const
{
	return m_valueMatcher;
}
