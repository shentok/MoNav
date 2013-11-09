#ifndef MAPSFORGE_CLOSEDANYMATCHER_H
#define MAPSFORGE_CLOSEDANYMATCHER_H

#include "ClosedMatcher.h"

class ClosedAnyMatcher : public ClosedMatcher
{
public:
	ClosedAnyMatcher();

	bool isCoveredBy(const ClosedMatcher &other) const;

	bool matches(bool isClosed) const;
};

#endif // MAPSFORGE_CLOSEDANYMATCHER_H
