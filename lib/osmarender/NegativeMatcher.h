#ifndef MAPSFORGE_NEGATIVEMATCHER_H
#define MAPSFORGE_NEGATIVEMATCHER_H

#include "AttributeMatcher.h"

#include <QStringList>

class NegativeMatcher : public AttributeMatcher
{
public:
	NegativeMatcher(const QStringList &keyList, const QStringList &valueList);

	bool isCoveredBy(const AttributeMatcher &attributeMatcher) const;

	bool matches(const QList<Tag> &tags) const;

private:
	bool keyListDoesNotContainKeys(const QList<Tag> &tags) const;

private:
	QStringList m_keys;
	QStringList m_values;
};

#endif // MAPSFORGE_NEGATIVEMATCHER_H
