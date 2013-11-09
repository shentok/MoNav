#ifndef MAPSFORGE_VALUEMATCHER_H
#define MAPSFORGE_VALUEMATCHER_H

#include "AttributeMatcher.h"

#include <QStringList>

class ValueMatcher : public AttributeMatcher
{
public:
	ValueMatcher(const QStringList &valueList);

	bool isCoveredBy(const AttributeMatcher &attributeMatcher) const;

	bool matches(const QList<Tag> &tags) const;

private:
	QStringList m_values;
};

#endif // MAPSFORGE_VALUEMATCHER_H
