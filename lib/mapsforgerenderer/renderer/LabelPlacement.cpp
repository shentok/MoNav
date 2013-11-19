#include "LabelPlacement.h"

#include "DependencyCache.h"

#include <queue>

/**
 * This class holds the reference positions for the two and four point greedy algorithms.
 */
class LabelPlacement::ReferencePosition : public QRectF
{
public:
	ReferencePosition() :
		QRectF(),
		m_nodeNumber(-1)
	{
	}

	ReferencePosition(qreal x, qreal y, int nodeNumber, qreal width, qreal height) :
		QRectF(x, y, width, height),
		m_nodeNumber(nodeNumber)
	{
		Q_ASSERT(nodeNumber >= 0);
	}

	bool isValid() const { return m_nodeNumber >= 0; }

	int nodeNumber() const { return m_nodeNumber; }

private:
	int m_nodeNumber;
};

struct LabelPlacement::ReferencePositionTopComparator
{
	bool operator()(const ReferencePosition &x, const ReferencePosition &y) const
	{
		return x.y() > y.y();
	}
};

struct LabelPlacement::ReferencePositionBottomComparator
{
	bool operator()(const ReferencePosition &x, const ReferencePosition &y) const
	{
		return (x.y() + x.height()) > (y.y() + y.height());
	}
};


LabelPlacement::LabelPlacement(const DependencyCache *dependencyCache, const TileId &tile) :
	m_dependencyCache(dependencyCache),
	tile(tile)
{
}

void LabelPlacement::placeLabels(QList<PointTextContainer> &labels, QList<SymbolContainer> &symbols, QList<PointTextContainer> &areaLabels)
{
	centerLabels(areaLabels);
	removeOutOfTileAreaLabels(areaLabels, m_dependencyCache->tileSize());
	removeOverlappingAreaLabels(areaLabels);
	m_dependencyCache->removeAreaLabelsInAlreadyDrawnAreas(areaLabels);

	removeOutOfTileSymbols(symbols, m_dependencyCache->tileSize());
	removeOverlappingSymbols(symbols);
	m_dependencyCache->removeSymbolsFromDrawnAreas(symbols);

	removeOutOfTileLabels(labels, m_dependencyCache->tileSize());
	removeEmptySymbolReferences(labels, symbols);
	m_dependencyCache->removeOverlappingLabelsWithDependencyLabels(labels);

	removeOverlappingSymbolsWithAreaLabels(symbols, areaLabels);

	m_dependencyCache->removeOverlappingSymbolsWithDependencyLabels(symbols);
	m_dependencyCache->removeOverlappingAreaLabelsWithDependencyLabels(areaLabels);

	m_dependencyCache->removeOverlappingSymbolsWithDepencySymbols(symbols, 2);
	m_dependencyCache->removeOverlappingAreaLabelsWithDependencySymbols(areaLabels);

	labels = processFourPointGreedy(labels, symbols, areaLabels);
}

void LabelPlacement::centerLabels(QList<PointTextContainer> &labels)
{
	for (QList<PointTextContainer>::Iterator it = labels.begin(); it != labels.end(); ++it) {
		it->rboundary().translate(-it->boundary().width() / 2, 0);
	}
}

QList<PointTextContainer> LabelPlacement::processFourPointGreedy(const QList<PointTextContainer> &labels, const QList<SymbolContainer> &symbols, const QList<PointTextContainer> &areaLabels)
{
	// Array for the generated reference positions around the points of interests
	QVector<ReferencePosition> refPos(labels.size() * 4);

	const int distance = START_DISTANCE_TO_SYMBOLS;

	// creates the reference positions
	int z = 0;
	foreach (const PointTextContainer &label, labels) {
		if (label.symbol() != 0) {
			// up
			refPos[z * 4] = ReferencePosition(label.point().x() - label.boundary().width() / 2, label.point().y()
					- label.symbol()->symbol().height() / 2 - distance, z, label.boundary().width(),
					label.boundary().height());
			// down
			refPos[z * 4 + 1] = ReferencePosition(label.point().x() - label.boundary().width() / 2, label.point().y()
					+ label.symbol()->symbol().height() / 2 + label.boundary().height() + distance, z,
					label.boundary().width(), label.boundary().height());
			// left
			refPos[z * 4 + 2] = ReferencePosition(label.point().x() - label.symbol()->symbol().width() / 2
					- label.boundary().width() - distance, label.point().y() + label.boundary().height() / 2, z,
					label.boundary().width(), label.boundary().height());
			// right
			refPos[z * 4 + 3] = ReferencePosition(label.point().x() + label.symbol()->symbol().width() / 2 + distance, label.point().y()
					+ label.boundary().height() / 2 - 0.1f, z, label.boundary().width(), label.boundary().height());
		} else {
			refPos[z * 4] = ReferencePosition(label.point().x() - ((label.boundary().width()) / 2),
					label.point().y(), z, label.boundary().width(), label.boundary().height());
			refPos[z * 4 + 1] = ReferencePosition();
			refPos[z * 4 + 2] = ReferencePosition();
			refPos[z * 4 + 3] = ReferencePosition();
		}
		++z;
	}

	removeNonValidateReferencePositionSymbols(refPos, symbols);
	removeNonValidateReferencePositionAreaLabels(refPos, areaLabels);
	removeOutOfTileReferencePoints(refPos);
	removeOverlappingLabels(refPos);
	removeOverlappingSymbols(refPos);

	QList<PointTextContainer> result;

	// lists that sorts the reference points after the minimum top edge y position
	std::priority_queue<ReferencePosition, std::vector<ReferencePosition>, ReferencePositionTopComparator> priorTop;

	// do while it gives reference positions
	for (int i = 0; i < refPos.size(); ++i) {
		const ReferencePosition referencePosition = refPos[i];
		if (referencePosition.isValid()) {
			priorTop.push(referencePosition);
			const PointTextContainer label = labels.at(referencePosition.nodeNumber());
			result.append(PointTextContainer(label.text(), referencePosition.topLeft().toPoint(), label.font(), label.paintFront(), label.paintBack(), label.symbol()));
		}
	}

#if 0
	while (!priorTop.empty()) {
		const ReferencePosition referencePosition = priorTop.top();
		priorTop.pop();

		const PointTextContainer label = labels.at(referencePosition.nodeNumber());

		result.append(PointTextContainer(label.text(), referencePosition.topLeft().toPoint(), label.font(), label.paintFront(), label.paintBack(), label.symbol()));

		if (priorTop.size() == 0) {
			return result;
		}

		// lists that sorts the reference points after the minimum bottom edge y position
		std::priority_queue<ReferencePosition, std::vector<ReferencePosition>, ReferencePositionBottomComparator> priorBottom;

		while (!priorTop.empty()) {
			if (priorTop.top().nodeNumber() != referencePosition.nodeNumber()) {
				priorBottom.push(priorTop.top());
			}

			priorTop.pop();
		}

		QVector<ReferencePosition> removed;
		removed.reserve(priorBottom.size());

		while (!priorBottom.empty() && priorBottom.top().x() < referencePosition.x() + referencePosition.width()) {
			removed << priorBottom.top();
			priorBottom.pop();
		}
		// brute Force collision test (faster then sweep line for a small amount of objects)
		foreach (const ReferencePosition &pos, removed) {
			if (referencePosition.intersects(pos)) {
				continue;
			}
			else {
				priorBottom.push(pos);
			}
		}

		Q_ASSERT(priorTop.empty());

		while (!priorBottom.empty()) {
			priorTop.push(priorBottom.top());
			priorBottom.pop();
		}
	}
#endif

	return result;
}

void LabelPlacement::removeEmptySymbolReferences(QList<PointTextContainer> &nodes, const QList<SymbolContainer> &symbols)
{
#warning
#if 0
	for (int i = 0; i < nodes.size(); i++) {
		PointTextContainer &label = nodes.at(i);
		if (!symbols.contains(label.symbol())) {
			label.symbol() = null;
		}
	}
#endif
}

void LabelPlacement::removeNonValidateReferencePositionSymbols(QVector<ReferencePosition> &refPos, const QList<SymbolContainer> &symbols)
{
	const int distance = LABEL_DISTANCE_TO_SYMBOL;

	foreach (const SymbolContainer &symbolContainer, symbols) {
		const Rectangle rect1 = Rectangle((int) symbolContainer.point().x() - distance,
				(int) symbolContainer.point().y() - distance, (int) symbolContainer.point().x()
						+ symbolContainer.symbol().width() + distance, (int) symbolContainer.point().y()
						+ symbolContainer.symbol().height() + distance);

		for (int y = 0; y < refPos.size(); y++) {
			if (refPos[y].isValid()) {
				const Rectangle rect2 = Rectangle((int) refPos[y].x(), (int) (refPos[y].y() - refPos[y].height()),
						(int) (refPos[y].x() + refPos[y].width()), (int) (refPos[y].y()));

				if (rect2.intersects(rect1)) {
					refPos[y] = ReferencePosition();
				}
			}
		}
	}
}

void LabelPlacement::removeNonValidateReferencePositionAreaLabels(QVector<LabelPlacement::ReferencePosition> &refPos, const QList<PointTextContainer> &areaLabels)
{
	const int distance = LABEL_DISTANCE_TO_LABEL;

	foreach (const PointTextContainer &areaLabel, areaLabels) {
		const Rectangle rect1 = Rectangle((int) areaLabel.point().x() - distance, (int) areaLabel.point().y() - areaLabel.boundary().height()
				- distance, (int) areaLabel.point().x() + areaLabel.boundary().width() + distance, (int) areaLabel.point().y()
				+ distance);

		for (int y = 0; y < refPos.size(); y++) {
			if (refPos[y].isValid()) {
				const Rectangle rect2 = Rectangle((int) refPos[y].x(), (int) (refPos[y].y() - refPos[y].height()),
						(int) (refPos[y].x() + refPos[y].width()), (int) (refPos[y].y()));

				if (rect2.intersects(rect1)) {
					refPos[y] = ReferencePosition();
				}
			}
		}
	}
}

void LabelPlacement::removeOutOfTileReferencePoints(QVector<ReferencePosition> &refPos) const
{
	for (int i = 0; i < refPos.size(); i++) {
		const ReferencePosition &ref = refPos[i];

		if (!ref.isValid()) {
			continue;
		}

		if (m_dependencyCache->hasUp() && ref.y() + ref.height() < 0) {
			refPos[i] = ReferencePosition();
			continue;
		}

		if (m_dependencyCache->hasDown() && ref.y() >= m_dependencyCache->tileSize()) {
			refPos[i] = ReferencePosition();
			continue;
		}

		if (m_dependencyCache->hasLeft() && ref.x() + ref.width() < 0) {
			refPos[i] = ReferencePosition();
			continue;
		}

		if (m_dependencyCache->hasRight() && ref.x() > m_dependencyCache->tileSize()) {
			refPos[i] = ReferencePosition();
		}
	}
}

void LabelPlacement::removeOverlappingLabels(QVector<LabelPlacement::ReferencePosition> &refPos) const
{
	const int dis = 2;
	foreach (const PointTextContainer &label1, m_dependencyCache->labels()) {
		const Rectangle rect1 = Rectangle((int) label1.point().x() - dis,
				(int) (label1.point().y() - label1.boundary().height()) - dis,
				(int) (label1.point().x() + label1.boundary().width() + dis),
				(int) (label1.point().y() + dis));

		for (int y = 0; y < refPos.size(); y++) {
			if (refPos[y].isValid()) {
				const Rectangle rect2 = Rectangle((int) refPos[y].x(), (int) (refPos[y].y() - refPos[y].height()),
						(int) (refPos[y].x() + refPos[y].width()), (int) (refPos[y].y()));

				if (rect2.intersects(rect1)) {
					refPos[y] = ReferencePosition();
				}
			}
		}
	}
}

void LabelPlacement::removeOverlappingSymbols(QVector<LabelPlacement::ReferencePosition> &refPos) const
{
	foreach (const SymbolContainer &symbol2, m_dependencyCache->symbols()) {
		const Rectangle rect1 = Rectangle((int) symbol2.point().x(), (int) (symbol2.point().y()),
				(int) (symbol2.point().x() + symbol2.width()),
				(int) (symbol2.point().y() + symbol2.height()));

		for (int y = 0; y < refPos.size(); y++) {
			if (refPos[y].isValid()) {
				const Rectangle rect2 = Rectangle((int) refPos[y].x(), (int) (refPos[y].y() - refPos[y].height()),
						(int) (refPos[y].x() + refPos[y].width()), (int) (refPos[y].y()));

				if (rect2.intersects(rect1)) {
					refPos[y] = ReferencePosition();
				}
			}
		}
	}
}

void LabelPlacement::removeOutOfTileAreaLabels(QList<PointTextContainer> &areaLabels, unsigned short tileSize)
{
	for (int i = 0; i < areaLabels.size(); i++) {
		const PointTextContainer &label = areaLabels.at(i);

		if (label.point().x() > tileSize) {
			areaLabels.removeAt(i);

			i--;
		} else if (label.point().y() - label.boundary().height() > tileSize) {
			areaLabels.removeAt(i);

			i--;
		} else if (label.point().x() + label.boundary().width() < 0.0f) {
			areaLabels.removeAt(i);

			i--;
		} else if (label.point().y() + label.boundary().height() < 0.0f) {
			areaLabels.removeAt(i);

			i--;
		}
	}
}

void LabelPlacement::removeOutOfTileLabels(QList<PointTextContainer> &labels, unsigned short tileSize)
{
	for (int i = 0; i < labels.size();) {
		const PointTextContainer &label = labels.at(i);

		if (label.point().x() - label.boundary().width() / 2 > tileSize) {
			labels.removeAt(i);
		} else if (label.point().y() - label.boundary().height() > tileSize) {
			labels.removeAt(i);
		} else if ((label.point().x() - label.boundary().width() / 2 + label.boundary().width()) < 0.0f) {
			labels.removeAt(i);
		} else if (label.point().y() < 0.0f) {
			labels.removeAt(i);
		} else {
			i++;
		}
	}
}

void LabelPlacement::removeOutOfTileSymbols(QList<SymbolContainer> &symbols, unsigned short tileSize)
{
	for (int i = 0; i < symbols.size();) {
		const SymbolContainer &symbolContainer = symbols.at(i);

		if (symbolContainer.point().x() > tileSize) {
			symbols.removeAt(i);
		} else if (symbolContainer.point().y() > tileSize) {
			symbols.removeAt(i);
		} else if (symbolContainer.point().x() + symbolContainer.symbol().width() < 0.0f) {
			symbols.removeAt(i);
		} else if (symbolContainer.point().y() + symbolContainer.symbol().height() < 0.0f) {
			symbols.removeAt(i);
		} else {
			i++;
		}
	}
}

void LabelPlacement::removeOverlappingAreaLabels(QList<PointTextContainer> &areaLabels)
{
	const int distance = LABEL_DISTANCE_TO_LABEL;

	for (int i = 0; i < areaLabels.size(); i++) {
		const PointTextContainer &label1 = areaLabels.at(i);
		const Rectangle rect1 = Rectangle((int) label1.point().x() - distance, (int) label1.point().y() - distance,
				(int) (label1.point().x() + label1.boundary().width()) + distance, (int) (label1.point().y()
						+ label1.boundary().height() + distance));

		for (int j = i + 1; j < areaLabels.size(); j++) {
			const PointTextContainer &label2 = areaLabels.at(j);
			const Rectangle rect2 = Rectangle((int) label2.point().x(), (int) label2.point().y(),
					(int) (label2.point().x() + label2.boundary().width()),
					(int) (label2.point().y() + label2.boundary().height()));

			if (rect1.intersects(rect2)) {
				areaLabels.removeAt(j);

				j--;
			}
		}
	}
}

void LabelPlacement::removeOverlappingSymbols(QList<SymbolContainer> &symbols)
{
	const int distance = SYMBOL_DISTANCE_TO_SYMBOL;

	for (int i = 0; i < symbols.size(); i++) {
		const SymbolContainer &symbolContainer1 = symbols.at(i);
		const Rectangle rect1 = Rectangle((int) symbolContainer1.point().x() - distance, (int) symbolContainer1.point().y()
				- distance, (int) symbolContainer1.point().x() + symbolContainer1.symbol().width() + distance,
				(int) symbolContainer1.point().y() + symbolContainer1.symbol().height() + distance);

		for (int j = i + 1; j < symbols.size(); j++) {
			const SymbolContainer &symbolContainer2 = symbols.at(j);
			const Rectangle rect2 = Rectangle((int) symbolContainer2.point().x(), (int) symbolContainer2.point().y(),
					(int) symbolContainer2.point().x() + symbolContainer2.symbol().width(),
					(int) symbolContainer2.point().y() + symbolContainer2.symbol().height());

			if (rect2.intersects(rect1)) {
				symbols.removeAt(j);
				j--;
			}
		}
	}
}

void LabelPlacement::removeOverlappingSymbolsWithAreaLabels(QList<SymbolContainer> &symbols, const QList<PointTextContainer> &pTC)
{
	int dis = LABEL_DISTANCE_TO_SYMBOL;

	for (int x = 0; x < pTC.size(); x++) {
		const PointTextContainer &label = pTC.at(x);
		const Rectangle rect1 = Rectangle((int) label.point().x() - dis, (int) (label.point().y() - label.boundary().height())
				- dis, (int) (label.point().x() + label.boundary().width() + dis), (int) (label.point().y() + dis));

		for (int y = 0; y < symbols.size(); y++) {
			const SymbolContainer &symbolContainer = symbols.at(y);
			const Rectangle rect2 = Rectangle((int) symbolContainer.point().x(), (int) symbolContainer.point().y(),
					(int) (symbolContainer.point().x() + symbolContainer.symbol().width()),
					(int) (symbolContainer.point().y() + symbolContainer.symbol().height()));

			if (rect1.intersects(rect2)) {
				symbols.removeAt(y);
				y--;
			}
		}
	}
}
