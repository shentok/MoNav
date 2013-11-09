#ifndef MAPSFORGE_KEYMATCHER_H
#define MAPSFORGE_KEYMATCHER_H

#include "AttributeMatcher.h"

#include <QStringList>

class KeyMatcher : public AttributeMatcher
{
public:
	KeyMatcher(const QStringList keyList);

	bool isCoveredBy(const AttributeMatcher &attributeMatcher) const;

	bool matches(const QList<Tag> &tags) const;

private:
	QStringList m_keys;
};

#endif // MAPSFORGE_KEYMATCHER_H
