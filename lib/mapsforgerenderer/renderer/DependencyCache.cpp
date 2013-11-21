#include "DependencyCache.h"

#include "LabelPlacement.h"

/**
 * This class holds all the information off the possible dependencies on a object.
 */
class DependencyCache::DependencyOnTile
{
public:
	/**
	 * Initialize label, symbol and drawn.
	 */
	DependencyOnTile() :
		m_isDrawn(false)
	{
	}

	void setDrawn(bool drawn) { m_isDrawn = drawn; }

	bool isDrawn() const { return m_isDrawn; }

	/**
	 * @param toAdd
	 *            a dependency Symbol
	 */
	void addSymbol(const SymbolContainer &orig, const Point &point)
	{
		const SymbolContainer symbol(orig.symbol(), point, orig.alignCenter(), orig.theta());
		m_symbols.append(symbol);
	}

	/**
	 * @param toAdd
	 *            a Dependency Text
	 */
	void addText(const PointTextContainer &orig, const Point &point)
	{
		const PointTextContainer label(orig.text(), point, orig.font(), orig.paintFront(), orig.paintBack(), orig.symbol());
		m_labels.append(label);
	}

	QList<PointTextContainer> labels() const { return m_labels; }
	QList<SymbolContainer> symbols() const { return m_symbols; }

private:
	bool m_isDrawn;
	QList<PointTextContainer> m_labels;
	QList<SymbolContainer> m_symbols;
};

DependencyCache::DependencyCache() :
	m_dependencyTable(),
	m_currentDependencyOnTile(0),
	currentTile(0, 0, 0),
	m_tileSize(0)
{
}

DependencyCache::~DependencyCache()
{
	qDeleteAll(m_dependencyTable);
}

void DependencyCache::fillDependencyOnTile(QList<PointTextContainer> &labels, QList<SymbolContainer> &symbols) const
{
	labels << m_currentDependencyOnTile->labels();
	symbols << m_currentDependencyOnTile->symbols();
}

void DependencyCache::setCurrentTile(const TileId &tile, unsigned short tileSize)
{
	if (m_tileSize != tileSize) {
		m_dependencyTable.clear();
		m_tileSize = tileSize;
	}

	this->currentTile = tile;

	if (m_dependencyTable.value(this->currentTile) == 0) {
		m_dependencyTable.insert(this->currentTile, new DependencyOnTile());
	}
	m_currentDependencyOnTile = m_dependencyTable.value(this->currentTile);

	const quint64 maxTileNumber = TileId::getMaxTileNumber(this->currentTile.zoomLevel);

	m_hasLeft = false;
	if (this->currentTile.tileX > 0) {
		const TileId tileId = TileId(this->currentTile.tileX - 1, this->currentTile.tileY, this->currentTile.zoomLevel);
		const DependencyOnTile *const tmp = m_dependencyTable.value(tileId);
		m_hasLeft = tmp == 0 ? false : tmp->isDrawn();
	}

	m_hasRight = false;
	if (this->currentTile.tileX < maxTileNumber) {
		const TileId tileId = TileId(this->currentTile.tileX + 1, this->currentTile.tileY, this->currentTile.zoomLevel);
		const DependencyOnTile *const tmp = m_dependencyTable.value(tileId);
		m_hasRight = tmp == 0 ? false : tmp->isDrawn();
	}

	m_hasUp = false;
	if (this->currentTile.tileY > 0) {
		const TileId tileId = TileId(this->currentTile.tileX, this->currentTile.tileY - 1, this->currentTile.zoomLevel);
		const DependencyOnTile *const tmp = m_dependencyTable.value(tileId);
		m_hasUp = tmp == 0 ? false : tmp->isDrawn();
	}

	m_hasDown = false;
	if (this->currentTile.tileY < maxTileNumber) {
		const TileId tileId = TileId(this->currentTile.tileX, this->currentTile.tileY + 1, this->currentTile.zoomLevel);
		const DependencyOnTile *const tmp = m_dependencyTable.value(tileId);
		m_hasDown = tmp == 0 ? false : tmp->isDrawn();
	}
}

void DependencyCache::removeAreaLabelsInAlreadyDrawnAreas(QList<PointTextContainer> &areaLabels) const
{
	for (int i = 0; i < areaLabels.size(); i++) {
		const PointTextContainer container = areaLabels.at(i);

		if (m_hasUp && container.point().y() - container.boundary().height() < 0.0f) {
			areaLabels.removeAt(i);
			i--;
			continue;
		}
		if (m_hasDown && container.point().y() > m_tileSize) {
			areaLabels.removeAt(i);
			i--;
			continue;
		}
		if (m_hasLeft && container.point().x() < 0.0f) {
			areaLabels.removeAt(i);
			i--;
			continue;
		}
		if (m_hasRight && container.point().x() + container.boundary().width() > m_tileSize) {
			areaLabels.removeAt(i);
			i--;
			continue;
		}
	}
}

void DependencyCache::removeSymbolsFromDrawnAreas(QList<SymbolContainer> &symbols) const
{
	for (int i = 0; i < symbols.size(); i++) {
		const SymbolContainer container = symbols.at(i);

		if (m_hasUp && container.point().y() < 0) {
			symbols.removeAt(i);
			i--;
			continue;
		}

		if (m_hasDown && container.point().y() + container.symbol().height() > m_tileSize) {
			symbols.removeAt(i);
			i--;
			continue;
		}
		if (m_hasLeft && container.point().x() < 0) {
			symbols.removeAt(i);
			i--;
			continue;
		}
		if (m_hasRight && container.point().x() + container.symbol().width() > m_tileSize) {
			symbols.removeAt(i);
			i--;
			continue;
		}
	}
}

unsigned short DependencyCache::tileSize() const
{
	return m_tileSize;
}

QList<SymbolContainer> DependencyCache::symbols() const
{
	Q_ASSERT(m_currentDependencyOnTile != 0);

	return m_currentDependencyOnTile->symbols();
}

QList<PointTextContainer> DependencyCache::labels() const
{
	Q_ASSERT(m_currentDependencyOnTile != 0);

	return m_currentDependencyOnTile->labels();
}

bool DependencyCache::hasLeft() const
{
	return m_hasLeft;
}

bool DependencyCache::hasRight() const
{
	return m_hasRight;
}

bool DependencyCache::hasUp() const
{
	return m_hasUp;
}

bool DependencyCache::hasDown() const
{
	return m_hasDown;
}

void DependencyCache::fillDependencyLabels(const QList<PointTextContainer> &pTC)
{
	TileId left = TileId(this->currentTile.tileX - 1, this->currentTile.tileY, this->currentTile.zoomLevel);
	TileId right = TileId(this->currentTile.tileX + 1, this->currentTile.tileY, this->currentTile.zoomLevel);
	TileId up = TileId(this->currentTile.tileX, this->currentTile.tileY - 1, this->currentTile.zoomLevel);
	TileId down = TileId(this->currentTile.tileX, this->currentTile.tileY + 1, this->currentTile.zoomLevel);

	TileId leftup = TileId(this->currentTile.tileX - 1, this->currentTile.tileY - 1, this->currentTile.zoomLevel);
	TileId leftdown = TileId(this->currentTile.tileX - 1, this->currentTile.tileY + 1, this->currentTile.zoomLevel);
	TileId rightup = TileId(this->currentTile.tileX + 1, this->currentTile.tileY - 1, this->currentTile.zoomLevel);
	TileId rightdown = TileId(this->currentTile.tileX + 1, this->currentTile.tileY + 1, this->currentTile.zoomLevel);

	foreach (const PointTextContainer &label, pTC) {
		bool alreadyAdded = false;

		// up
		if ((label.point().y() - label.boundary().height() < 0.0f) && (!m_dependencyTable.value(up)->isDrawn())) {
			alreadyAdded = true;

			m_currentDependencyOnTile->addText(label, label.point());

			{
				DependencyOnTile *const linkedDep = m_dependencyTable.value(up);
				linkedDep->addText(label, label.point() + Point(0, m_tileSize));
			}

			if ((label.point().x() < 0.0f) && (!m_dependencyTable.value(leftup)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(leftup);
				linkedDep->addText(label, label.point() + Point(m_tileSize, m_tileSize));
			}

			if ((label.point().x() + label.boundary().width() > m_tileSize)
					&& (!m_dependencyTable.value(rightup)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(rightup);
				linkedDep->addText(label, label.point() + Point(-m_tileSize, m_tileSize));
			}
		}

		// down
		if ((label.point().y() > m_tileSize) && (!m_dependencyTable.value(down)->isDrawn())) {
			if (!alreadyAdded) {
				alreadyAdded = true;

				m_currentDependencyOnTile->addText(label, label.point());
			}

			{
				DependencyOnTile *const linkedDep = m_dependencyTable.value(down);
				linkedDep->addText(label, label.point() - Point(0, m_tileSize));
			}

			if ((label.point().x() < 0.0f) && (!m_dependencyTable.value(leftdown)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(leftdown);
				linkedDep->addText(label, label.point() + Point(m_tileSize, -m_tileSize));
			}

			if ((label.point().x() + label.boundary().width() > m_tileSize)
					&& (!m_dependencyTable.value(rightdown)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(rightdown);
				linkedDep->addText(label, label.point() - Point(m_tileSize, m_tileSize));
			}
		}

		// left
		if ((label.point().x() < 0.0f) && (!m_dependencyTable.value(left)->isDrawn())) {
			if (!alreadyAdded) {
				alreadyAdded = true;

				m_currentDependencyOnTile->addText(label, label.point());
			}

			{
				DependencyOnTile *const linkedDep = m_dependencyTable.value(left);
				linkedDep->addText(label, label.point() + Point(m_tileSize, 0));
			}
		}

		// right
		if ((label.point().x() + label.boundary().width() > m_tileSize) && (!m_dependencyTable.value(right)->isDrawn())) {
			if (!alreadyAdded) {
				alreadyAdded = true;

				m_currentDependencyOnTile->addText(label, label.point());
			}

			{
				DependencyOnTile *const linkedDep = m_dependencyTable.value(right);
				linkedDep->addText(label, label.point() - Point(m_tileSize, 0));
			}
		}

		// check symbols

		if ((label.symbol() != 0) && (!alreadyAdded)) {
			if ((label.symbol()->point().y() <= 0.0f) && (!m_dependencyTable.value(up)->isDrawn())) {
				alreadyAdded = true;
				m_currentDependencyOnTile->addText(label, label.point());

				{
					DependencyOnTile *const linkedDep = m_dependencyTable.value(up);
					linkedDep->addText(label, label.point() + Point(0, m_tileSize));
				}

				if ((label.symbol()->point().x() < 0.0f) && (!m_dependencyTable.value(leftup)->isDrawn())) {
					DependencyOnTile *const linkedDep = m_dependencyTable.value(leftup);
					linkedDep->addText(label, label.point() + Point(m_tileSize, m_tileSize));
				}

				if ((label.symbol()->point().x() + label.symbol()->symbol().width() > m_tileSize)
						&& (!m_dependencyTable.value(rightup)->isDrawn())) {
					DependencyOnTile *const linkedDep = m_dependencyTable.value(rightup);
					linkedDep->addText(label, label.point() + Point(-m_tileSize, m_tileSize));
				}
			}

			if ((label.symbol()->point().y() + label.symbol()->symbol().height() >= m_tileSize)
					&& (!m_dependencyTable.value(down)->isDrawn())) {
				if (!alreadyAdded) {
					alreadyAdded = true;

					m_currentDependencyOnTile->addText(label, label.point());
				}

				{
					DependencyOnTile *const linkedDep = m_dependencyTable.value(down);
					linkedDep->addText(label, label.point() + Point(0, m_tileSize));
				}

				if ((label.symbol()->point().x() < 0.0f) && (!m_dependencyTable.value(leftdown)->isDrawn())) {
					DependencyOnTile *const linkedDep = m_dependencyTable.value(leftdown);
					linkedDep->addText(label, label.point() + Point(m_tileSize, -m_tileSize));
				}

				if ((label.symbol()->point().x() + label.symbol()->symbol().width() > m_tileSize)
						&& (!m_dependencyTable.value(rightdown)->isDrawn())) {
					DependencyOnTile *const linkedDep = m_dependencyTable.value(rightdown);
					linkedDep->addText(label, label.point() - Point(m_tileSize, m_tileSize));
				}
			}

			if ((label.symbol()->point().x() <= 0.0f) && (!m_dependencyTable.value(left)->isDrawn())) {
				if (!alreadyAdded) {
					alreadyAdded = true;

					m_currentDependencyOnTile->addText(label, label.point());
				}

				DependencyOnTile *const linkedDep = m_dependencyTable.value(left);
				linkedDep->addText(label, label.point() - Point(m_tileSize, 0));
			}

			if ((label.symbol()->point().x() + label.symbol()->symbol().width() >= m_tileSize)
					&& (!m_dependencyTable.value(right)->isDrawn())) {
				if (!alreadyAdded) {
					alreadyAdded = true;

					m_currentDependencyOnTile->addText(label, label.point());
				}

				DependencyOnTile *const linkedDep = m_dependencyTable.value(right);
				linkedDep->addText(label, label.point() + Point(m_tileSize, 0));
			}
		}
	}
}

void DependencyCache::fillDependencyOnTile2(const QList<PointTextContainer> &labels, const QList<SymbolContainer> &symbols, const QList<PointTextContainer> &areaLabels)
{
	m_currentDependencyOnTile->setDrawn(true);

	TileId left = TileId(this->currentTile.tileX - 1, this->currentTile.tileY, this->currentTile.zoomLevel);
	TileId right = TileId(this->currentTile.tileX + 1, this->currentTile.tileY, this->currentTile.zoomLevel);
	TileId up = TileId(this->currentTile.tileX, this->currentTile.tileY - 1, this->currentTile.zoomLevel);
	TileId down = TileId(this->currentTile.tileX, this->currentTile.tileY + 1, this->currentTile.zoomLevel);

	TileId leftup = TileId(this->currentTile.tileX - 1, this->currentTile.tileY - 1, this->currentTile.zoomLevel);
	TileId leftdown = TileId(this->currentTile.tileX - 1, this->currentTile.tileY + 1, this->currentTile.zoomLevel);
	TileId rightup = TileId(this->currentTile.tileX + 1, this->currentTile.tileY - 1, this->currentTile.zoomLevel);
	TileId rightdown = TileId(this->currentTile.tileX + 1, this->currentTile.tileY + 1, this->currentTile.zoomLevel);

	if (m_dependencyTable.value(up) == 0) {
		m_dependencyTable.insert(up, new DependencyOnTile());
	}
	if (m_dependencyTable.value(down) == 0) {
		m_dependencyTable.insert(down, new DependencyOnTile());
	}
	if (m_dependencyTable.value(left) == 0) {
		m_dependencyTable.insert(left, new DependencyOnTile());
	}
	if (m_dependencyTable.value(right) == 0) {
		m_dependencyTable.insert(right, new DependencyOnTile());
	}
	if (m_dependencyTable.value(leftdown) == 0) {
		m_dependencyTable.insert(leftdown, new DependencyOnTile());
	}
	if (m_dependencyTable.value(rightup) == 0) {
		m_dependencyTable.insert(rightup, new DependencyOnTile());
	}
	if (m_dependencyTable.value(leftup) == 0) {
		m_dependencyTable.insert(leftup, new DependencyOnTile());
	}
	if (m_dependencyTable.value(rightdown) == 0) {
		m_dependencyTable.insert(rightdown, new DependencyOnTile());
	}

	fillDependencyLabels(labels);
	fillDependencyLabels(areaLabels);
	fillDependencySymbols(symbols);
}

void DependencyCache::fillDependencySymbols(const QList<SymbolContainer> &symbols)
{
	TileId left = TileId(this->currentTile.tileX - 1, this->currentTile.tileY, this->currentTile.zoomLevel);
	TileId right = TileId(this->currentTile.tileX + 1, this->currentTile.tileY, this->currentTile.zoomLevel);
	TileId up = TileId(this->currentTile.tileX, this->currentTile.tileY - 1, this->currentTile.zoomLevel);
	TileId down = TileId(this->currentTile.tileX, this->currentTile.tileY + 1, this->currentTile.zoomLevel);

	TileId leftup = TileId(this->currentTile.tileX - 1, this->currentTile.tileY - 1, this->currentTile.zoomLevel);
	TileId leftdown = TileId(this->currentTile.tileX - 1, this->currentTile.tileY + 1, this->currentTile.zoomLevel);
	TileId rightup = TileId(this->currentTile.tileX + 1, this->currentTile.tileY - 1, this->currentTile.zoomLevel);
	TileId rightdown = TileId(this->currentTile.tileX + 1, this->currentTile.tileY + 1, this->currentTile.zoomLevel);

	foreach (const SymbolContainer &container, symbols) {
		bool alreadyAdded = false;

		// up
		if ((container.point().y() < 0.0f) && (!m_dependencyTable.value(up)->isDrawn())) {
			if (!alreadyAdded) {
				alreadyAdded = true;
				m_currentDependencyOnTile->addSymbol(container, container.point());
			}

			{
				DependencyOnTile *const linkedDep = m_dependencyTable.value(up);
				linkedDep->addSymbol(container, container.point() + Point(0, m_tileSize));
			}

			if ((container.point().x() < 0.0f) && (!m_dependencyTable.value(leftup)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(leftup);
				linkedDep->addSymbol(container, container.point() + Point(m_tileSize, m_tileSize));
			}

			if ((container.point().x() + container.symbol().width() > m_tileSize) && (!m_dependencyTable.value(rightup)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(rightup);
				linkedDep->addSymbol(container, container.point() + Point(-m_tileSize, m_tileSize));
			}
		}

		// down
		if ((container.point().y() + container.symbol().height() > m_tileSize) && (!m_dependencyTable.value(down)->isDrawn())) {
			if (!alreadyAdded) {
				alreadyAdded = true;
				m_currentDependencyOnTile->addSymbol(container, container.point());
			}

			{
				DependencyOnTile *const linkedDep = m_dependencyTable.value(down);
				linkedDep->addSymbol(container, container.point() - Point(0, m_tileSize));
			}

			if ((container.point().x() < 0.0f) && (!m_dependencyTable.value(leftdown)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(leftdown);
				linkedDep->addSymbol(container, container.point() + Point(m_tileSize, -m_tileSize));
			}

			if ((container.point().x() + container.symbol().width() > m_tileSize) && (!m_dependencyTable.value(rightdown)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(rightdown);
				linkedDep->addSymbol(container, container.point() - Point(m_tileSize, m_tileSize));
			}
		}

		// left
		if ((container.point().x() < 0.0f) && (!m_dependencyTable.value(left)->isDrawn())) {
			if (!alreadyAdded) {
				alreadyAdded = true;
				m_currentDependencyOnTile->addSymbol(container, container.point());
			}

			DependencyOnTile *const linkedDep = m_dependencyTable.value(left);
			linkedDep->addSymbol(container, container.point() + Point(m_tileSize, 0));
		}

		// right
		if ((container.point().x() + container.symbol().width() > m_tileSize) && (!m_dependencyTable.value(right)->isDrawn())) {
			if (!alreadyAdded) {
				alreadyAdded = true;
				m_currentDependencyOnTile->addSymbol(container, container.point());
			}

			DependencyOnTile *const linkedDep = m_dependencyTable.value(right);
			linkedDep->addSymbol(container, container.point() - Point(m_tileSize, 0));
		}
	}
}

void DependencyCache::removeOverlappingAreaLabelsWithDependencyLabels(QList<PointTextContainer> &areaLabels) const
{
	if (m_currentDependencyOnTile->labels().isEmpty())
		return;

	foreach (const PointTextContainer &label1, m_currentDependencyOnTile->labels()) {
		const Rectangle rect1 = Rectangle((int) (label1.point().x()),
				(int) (label1.point().y() - label1.boundary().height()),
				(int) (label1.point().x() + label1.boundary().width()),
				(int) (label1.point().y()));

		for (int i = 0; i < areaLabels.size(); i++) {
			const PointTextContainer pTC = areaLabels.at(i);

			const Rectangle rect2 = Rectangle((int) pTC.point().x(), (int) pTC.point().y() - pTC.boundary().height(), (int) pTC.point().x()
					+ pTC.boundary().width(), (int) pTC.point().y());

			if (rect2.intersects(rect1)) {
				areaLabels.removeAt(i);
				i--;
			}
		}
	}
}

void DependencyCache::removeOverlappingAreaLabelsWithDependencySymbols(QList<PointTextContainer> &areaLabels) const
{
	foreach (const SymbolContainer &symbol, m_currentDependencyOnTile->symbols()) {
		const Rectangle rect1 = Rectangle((int) symbol.point().x(), (int) symbol.point().y(), (int) symbol.point().x()
				+ symbol.width(), (int) symbol.point().y() + symbol.height());

		for (int i = 0; i < areaLabels.size(); i++) {
			const PointTextContainer label = areaLabels.at(i);

			const Rectangle rect2 = Rectangle((int) (label.point().x()), (int) (label.point().y() - label.boundary().height()),
					(int) (label.point().x() + label.boundary().width()), (int) (label.point().y()));

			if (rect2.intersects(rect1)) {
				areaLabels.removeAt(i);
				i--;
			}
		}
	}
}

void DependencyCache::removeOverlappingLabelsWithDependencyLabels(QList<PointTextContainer> &labels) const
{
	foreach (const PointTextContainer &dep, m_currentDependencyOnTile->labels()) {
		for (int j = 0; j < labels.size(); j++) {
			if ((labels.at(j).text() == dep.text())
					&& (labels.at(j).paintFront() == dep.paintFront())
					&& (labels.at(j).paintBack() == dep.paintBack())) {
				labels.removeAt(j);
				j--;
			}
		}
	}
}

void DependencyCache::removeOverlappingSymbolsWithDepencySymbols(QList<SymbolContainer> &symbols, int dis) const
{
	foreach (const SymbolContainer &symbol1, m_currentDependencyOnTile->symbols()) {
		const Rectangle rect1 = Rectangle((int) symbol1.point().x() - dis, (int) symbol1.point().y() - dis, (int) symbol1.point().x()
				+ symbol1.width() + dis, (int) symbol1.point().y() + symbol1.height() + dis);

		for (int j = 0; j < symbols.size(); j++) {
			const SymbolContainer symbol2 = symbols.at(j);
			const Rectangle rect2 = Rectangle((int) symbol2.point().x(), (int) symbol2.point().y(),
					(int) symbol2.point().x() + symbol2.symbol().width(),
					(int) symbol2.point().y() + symbol2.symbol().height());

			if (rect2.intersects(rect1)) {
				symbols.removeAt(j);
				j--;
			}
		}
	}
}

void DependencyCache::removeOverlappingSymbolsWithDependencyLabels(QList<SymbolContainer> &symbols) const
{
	foreach (const PointTextContainer &label1, m_currentDependencyOnTile->labels()) {
		const Rectangle rect1 = Rectangle((int) (label1.point().x()),
				(int) (label1.point().y() - label1.boundary().height()),
				(int) (label1.point().x() + label1.boundary().width()),
				(int) (label1.point().y()));

		for (int i = 0; i < symbols.size(); i++) {
			const SymbolContainer smb = symbols.at(i);

			const Rectangle rect2 = Rectangle((int) smb.point().x(), (int) smb.point().y(), (int) smb.point().x()
					+ smb.symbol().width(), (int) smb.point().y() + smb.symbol().height());

			if (rect2.intersects(rect1)) {
				symbols.removeAt(i);
				i--;
			}
		}
	}
}
