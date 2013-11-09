#include "AnyMatcher.h"

AnyMatcher::AnyMatcher()
{
}

bool AnyMatcher::isCoveredBy(const AttributeMatcher &attributeMatcher) const
{
	return &attributeMatcher == this;
}

bool AnyMatcher::matches(const QList<Tag> &tags) const
{
	Q_UNUSED(tags)

	return true;
}
