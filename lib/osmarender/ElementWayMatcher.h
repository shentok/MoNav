#ifndef MAPSFORGE_ELEMENTWAYMATCHER_H
#define MAPSFORGE_ELEMENTWAYMATCHER_H

#include "ElementMatcher.h"

class ElementWayMatcher : public ElementMatcher
{
public:
	ElementWayMatcher();

	bool isCoveredBy(const ElementMatcher &other) const;

	bool matches(Element element) const;
};

#endif // MAPSFORGE_ELEMENTWAYMATCHER_H
