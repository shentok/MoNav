#ifndef MAPSFORGE_ELEMENTMATCHER_H
#define MAPSFORGE_ELEMENTMATCHER_H

class ElementMatcher
{
public:
	enum Element {
		ElementNode,
		ElementWay
	};

	ElementMatcher();

	virtual ~ElementMatcher();

	virtual bool isCoveredBy(const ElementMatcher &other) const = 0;

	virtual bool matches(Element element) const = 0;
};

#endif // MAPSFORGE_ELEMENTMATCHER_H
