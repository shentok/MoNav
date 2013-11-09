#include "ValueMatcher.h"

ValueMatcher::ValueMatcher(const QStringList &valueList) :
	AttributeMatcher(),
	m_values(valueList)
{
}

bool ValueMatcher::isCoveredBy(const AttributeMatcher &attributeMatcher) const
{
	if (&attributeMatcher == this) {
		return true;
	}

	QList<Tag> tags;
	foreach (const QString &value, m_values) {
		tags << Tag("", value);
	}

	return attributeMatcher.matches(tags);
}

bool ValueMatcher::matches(const QList<Tag> &tags) const
{
	foreach (const Tag &tag, tags) {
		if (m_values.contains(tag.second)) {
			return true;
		}
	}

	return false;
}
