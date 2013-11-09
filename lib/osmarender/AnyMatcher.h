#ifndef MAPSFORGE_ANYMATCHER_H
#define MAPSFORGE_ANYMATCHER_H

#include "AttributeMatcher.h"

class AnyMatcher : public AttributeMatcher
{
public:
	AnyMatcher();

	bool isCoveredBy(const AttributeMatcher &attributeMatcher) const;

	bool matches(const QList<Tag> &tags) const;
};

#endif // MAPSFORGE_ANYMATCHER_H
