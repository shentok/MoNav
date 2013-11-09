#ifndef MAPSFORGE_LINEARWAYMATCHER_H
#define MAPSFORGE_LINEARWAYMATCHER_H

#include "ClosedMatcher.h"

class LinearWayMatcher : public ClosedMatcher
{
public:
	LinearWayMatcher();

	bool isCoveredBy(const ClosedMatcher &other) const;

	bool matches(bool isClosed) const;
};

#endif // MAPSFORGE_LINEARWAYMATCHER_H
