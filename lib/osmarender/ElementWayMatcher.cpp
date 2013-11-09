#include "ElementWayMatcher.h"

ElementWayMatcher::ElementWayMatcher()
{
}

bool ElementWayMatcher::isCoveredBy(const ElementMatcher &other) const
{
	return other.matches(ElementWay);
}

bool ElementWayMatcher::matches(ElementMatcher::Element element) const
{
	return element == ElementWay;
}
