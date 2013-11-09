#include "ClosedWayMatcher.h"

ClosedWayMatcher::ClosedWayMatcher()
{
}

bool ClosedWayMatcher::isCoveredBy(const ClosedMatcher &other) const
{
	return other.matches(true);
}

bool ClosedWayMatcher::matches(bool isClosed) const
{
	return isClosed == true;
}
