#include "KeyMatcher.h"

KeyMatcher::KeyMatcher(const QStringList keyList) :
	AttributeMatcher(),
	m_keys(keyList)
{
}

bool KeyMatcher::isCoveredBy(const AttributeMatcher &attributeMatcher) const
{
	if (&attributeMatcher == this) {
		return true;
	}

	QList<Tag> tags;
	foreach (const QString &key, m_keys) {
		tags << Tag(key, "");
	}

	return attributeMatcher.matches(tags);
}

bool KeyMatcher::matches(const QList<Tag> &tags) const
{
	foreach (const Tag &tag, tags) {
		if (m_keys.contains(tag.first)) {
			return true;
		}
	}

	return false;
}
