#include "ElementAnyMatcher.h"

ElementAnyMatcher::ElementAnyMatcher()
{
}

bool ElementAnyMatcher::isCoveredBy(const ElementMatcher &other) const
{
	return &other == this;
}

bool ElementAnyMatcher::matches(Element /*element*/) const
{
	return true;
}
