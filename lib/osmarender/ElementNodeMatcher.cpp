#include "ElementNodeMatcher.h"

ElementNodeMatcher::ElementNodeMatcher()
{
}

bool ElementNodeMatcher::isCoveredBy(const ElementMatcher &other) const
{
	return other.matches(ElementNode);
}

bool ElementNodeMatcher::matches(Element element) const
{
	return element == ElementNode;
}
