#include "NegativeMatcher.h"

NegativeMatcher::NegativeMatcher(const QStringList &keyList, const QStringList &valueList) :
	m_keys(keyList),
	m_values(valueList)
{
}

bool NegativeMatcher::isCoveredBy(const AttributeMatcher &/*attributeMatcher*/) const
{
	return false;
}

bool NegativeMatcher::matches(const QList<Tag> &tags) const
{
	if (keyListDoesNotContainKeys(tags)) {
		return true;
	}

	foreach (const Tag &tag, tags) {
		if (m_values.contains(tag.second)) {
			return true;
		}
	}
	return false;
}

bool NegativeMatcher::keyListDoesNotContainKeys(const QList<Tag> &tags) const
{
	foreach (const Tag &tag, tags) {
		if (m_keys.contains(tag.first)) {
			return false;
		}
	}

	return true;
}
