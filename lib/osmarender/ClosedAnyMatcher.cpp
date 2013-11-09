#include "ClosedAnyMatcher.h"

ClosedAnyMatcher::ClosedAnyMatcher()
{
}

bool ClosedAnyMatcher::isCoveredBy(const ClosedMatcher &other) const
{
	return &other == this;
}

bool ClosedAnyMatcher::matches(bool /*isClosed*/) const
{
	return true;
}
