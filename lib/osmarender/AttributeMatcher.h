#ifndef MAPSFORGE_ATTRIBUTEMATCHER_H
#define MAPSFORGE_ATTRIBUTEMATCHER_H

#include <QList>

#include <utility>

typedef std::pair<QString, QString> Tag;

class AttributeMatcher
{
public:
	AttributeMatcher();

	virtual ~AttributeMatcher();

	virtual bool isCoveredBy(const AttributeMatcher &attributeMatcher) const = 0;

	virtual bool matches(const QList<Tag> &tags) const = 0;
};

#endif // MAPSFORGE_ATTRIBUTEMATCHER_H
