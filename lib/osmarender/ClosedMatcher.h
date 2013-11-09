#ifndef MAPSFORGE_CLOSEDMATCHER_H
#define MAPSFORGE_CLOSEDMATCHER_H

class ClosedMatcher
{
public:
	ClosedMatcher();

	virtual ~ClosedMatcher();

	virtual bool isCoveredBy(const ClosedMatcher &other) const = 0;

	virtual bool matches(bool isClosed) const = 0;
};

#endif // MAPSFORGE_CLOSEDMATCHER_H
