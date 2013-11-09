#ifndef MAPSFORGE_ELEMENTNODEMATCHER_H
#define MAPSFORGE_ELEMENTNODEMATCHER_H

#include "ElementMatcher.h"

class ElementNodeMatcher : public ElementMatcher
{
public:
	ElementNodeMatcher();

	bool isCoveredBy(const ElementMatcher &other) const;

	bool matches(Element element) const;
};

#endif // MAPSFORGE_ELEMENTNODEMATCHER_H
