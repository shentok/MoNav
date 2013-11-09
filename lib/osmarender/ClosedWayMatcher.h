#ifndef MAPSFORGE_CLOSEDWAYMATCHER_H
#define MAPSFORGE_CLOSEDWAYMATCHER_H

#include "ClosedMatcher.h"

class ClosedWayMatcher : public ClosedMatcher
{
public:
	ClosedWayMatcher();

	bool isCoveredBy(const ClosedMatcher &other) const;

	bool matches(bool isClosed) const;
};

#endif // MAPSFORGE_CLOSEDWAYMATCHER_H
