#include "NegativeRule.h"

#include "ClosedMatcher.h"
#include "ElementMatcher.h"
#include "NegativeMatcher.h"

NegativeRule::NegativeRule(const ClosedMatcher *closedMatcher, const ElementMatcher *elementMatcher, byte zoomMin, byte zoomMax, const NegativeMatcher *matcher) :
	Rule(closedMatcher, elementMatcher, zoomMin, zoomMax),
	m_attributeMatcher(matcher)
{
}

NegativeRule::~NegativeRule()
{
	delete m_attributeMatcher;
}

bool NegativeRule::matchesNode(const QList<Tag> &tags, byte zoomLevel) const
{
	return m_zoomMin <= zoomLevel && m_zoomMax >= zoomLevel && m_elementMatcher->matches(ElementMatcher::ElementNode)
			&& m_attributeMatcher->matches(tags);
}

bool NegativeRule::matchesWay(const QList<Tag> &tags, byte zoomLevel, bool isClosed) const
{
	return m_zoomMin <= zoomLevel && m_zoomMax >= zoomLevel && m_elementMatcher->matches(ElementMatcher::ElementWay)
			&& m_closedMatcher->matches(isClosed) && m_attributeMatcher->matches(tags);
}
