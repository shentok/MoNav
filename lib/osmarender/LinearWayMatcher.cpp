#include "LinearWayMatcher.h"

LinearWayMatcher::LinearWayMatcher()
{
}

bool LinearWayMatcher::isCoveredBy(const ClosedMatcher &other) const
{
	return other.matches(false);
}

bool LinearWayMatcher::matches(bool isClosed) const
{
	return !isClosed;
}
