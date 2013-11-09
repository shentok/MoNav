#ifndef MAPSFORGE_NEGATIVERULE_H
#define MAPSFORGE_NEGATIVERULE_H

#include "Rule.h"

class NegativeMatcher;

class NegativeRule : public Rule
{
public:
	NegativeRule(const ClosedMatcher *closedMatcher, const ElementMatcher *elementMatcher, byte zoomMin, byte zoomMax, const NegativeMatcher *matcher);

	~NegativeRule();

	bool matchesNode(const QList<Tag> &tags, byte zoomLevel) const;

	bool matchesWay(const QList<Tag> &tags, byte zoomLevel, bool isClosed) const;

private:
	const NegativeMatcher *const m_attributeMatcher;
};

#endif // MAPSFORGE_NEGATIVERULE_H
