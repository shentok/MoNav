#include "Rule.h"

#include "RenderInstruction.h"

Rule::Rule(const ClosedMatcher *closedMatcher, const ElementMatcher *elementMatcher, byte zoomMin, byte zoomMax) :
	m_closedMatcher(closedMatcher),
	m_elementMatcher(elementMatcher),
	m_zoomMax(zoomMax),
	m_zoomMin(zoomMin),
	m_renderInstructions(),
	m_subRules()
{
}

Rule::~Rule()
{
}

void Rule::addRenderingInstruction(RenderInstruction *renderInstruction)
{
	m_renderInstructions << renderInstruction;
}

void Rule::addSubRule(Rule *rule)
{
	m_subRules << rule;
}

void Rule::matchNode(RenderCallback *renderCallback, const QList<Tag> &tags, byte zoomLevel) const
{
	if (matchesNode(tags, zoomLevel)) {
		foreach (const RenderInstruction *renderInstruction, m_renderInstructions) {
			renderInstruction->renderNode(renderCallback, tags);
		}
		foreach (const Rule *rule, m_subRules) {
			rule->matchNode(renderCallback, tags, zoomLevel);
		}
	}
}

void Rule::matchWay(QList<const RenderInstruction *> &matchingList, const QList<Tag> &tags, byte zoomLevel, bool isClosed) const
{
	if (matchesWay(tags, zoomLevel, isClosed)) {
		foreach (RenderInstruction *renderInstruction, m_renderInstructions) {
			matchingList << renderInstruction;
		}
		foreach (const Rule *subRule, m_subRules) {
			subRule->matchWay(matchingList, tags, zoomLevel, isClosed);
		}
	}
}

void Rule::scaleStrokeWidth(float scaleFactor)
{
	foreach (RenderInstruction *renderInstruction, m_renderInstructions) {
		renderInstruction->scaleStrokeWidth(scaleFactor);
	}
	foreach (Rule *subRule, m_subRules) {
		subRule->scaleStrokeWidth(scaleFactor);
	}
}

void Rule::scaleTextSize(float scaleFactor)
{
	foreach (RenderInstruction *renderInstruction, m_renderInstructions) {
		renderInstruction->scaleTextSize(scaleFactor);
	}
	foreach (Rule *subRule, m_subRules) {
		subRule->scaleTextSize(scaleFactor);
	}
}

const ClosedMatcher *Rule::closedMatcher() const
{
	return m_closedMatcher;
}

const ElementMatcher *Rule::elementMatcher() const
{
	return m_elementMatcher;
}
