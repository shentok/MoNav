#ifndef MAPSFORGE_POSITIVERULE_H
#define MAPSFORGE_POSITIVERULE_H

#include "Rule.h"

class AttributeMatcher;

class PositiveRule : public Rule
{
public:
	PositiveRule(const ClosedMatcher *closedMatcher, const ElementMatcher *elementMatcher, byte zoomMin, byte zoomMax, const AttributeMatcher *keyMatcher, const AttributeMatcher *valueMatcher);

	~PositiveRule();

	bool matchesNode(const QList<Tag> &tags, byte zoomLevel) const;

	bool matchesWay(const QList<Tag> &tags, byte zoomLevel, bool isClosed) const;

	const AttributeMatcher *keyMatcher() const;

	const AttributeMatcher *valueMatcher() const;

private:
	const AttributeMatcher *const m_keyMatcher;
	const AttributeMatcher *const m_valueMatcher;
};

#endif // MAPSFORGE_POSITIVERULE_H
