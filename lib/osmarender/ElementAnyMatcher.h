#ifndef MAPSFORGE_ELEMENTANYMATCHER_H
#define MAPSFORGE_ELEMENTANYMATCHER_H

#include "ElementMatcher.h"

class ElementAnyMatcher : public ElementMatcher
{
public:
	ElementAnyMatcher();

	bool isCoveredBy(const ElementMatcher &other) const;

	bool matches(Element element) const;
};

#endif // MAPSFORGE_ELEMENTANYMATCHER_H
