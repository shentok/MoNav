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
	void addSymbol(const Dependency<QImage> &toAdd)
	{
		m_symbols.append(toAdd);
	}

	/**
	 * @param toAdd
	 *            a Dependency Text
	 */
	void addText(const Dependency<PointTextContainer> &toAdd)
	{
		m_labels.append(toAdd);
	}

	QList<Dependency<PointTextContainer> > labels() const { return m_labels; }
	QList<Dependency<QImage> > symbols() const { return m_symbols; }

private:
	bool m_isDrawn;
	QList<Dependency<PointTextContainer> > m_labels;
	QList<Dependency<QImage> > m_symbols;
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
	foreach (const Dependency<PointTextContainer> &depLabel, m_currentDependencyOnTile->labels()) {
		if (depLabel.value.paintBack().style() != Qt::NoPen) {
			labels << PointTextContainer(depLabel.value.text(), depLabel.point, depLabel.value.font(), depLabel.value.paintFront(), depLabel.value.paintBack());
		} else {
			labels << PointTextContainer(depLabel.value.text(), depLabel.point, depLabel.value.font(), depLabel.value.paintFront());
		}
	}

	foreach (const Dependency<QImage> &depSmb, m_currentDependencyOnTile->symbols()) {
		symbols << SymbolContainer(depSmb.value, depSmb.point);
	}
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
	if (areaLabels.isEmpty())
		return;

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

QList<DependencyCache::Dependency<QImage> > DependencyCache::symbols() const
{
	Q_ASSERT(m_currentDependencyOnTile != 0);

	return m_currentDependencyOnTile->symbols();
}

QList<DependencyCache::Dependency<PointTextContainer> > DependencyCache::labels() const
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

			m_currentDependencyOnTile->addText(Dependency<PointTextContainer>(label, Point(label.point().x(), label.point().y())));

			{
				DependencyOnTile *const linkedDep = m_dependencyTable.value(up);
				linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x(), label.point().y() + m_tileSize)));
			}

			if ((label.point().x() < 0.0f) && (!m_dependencyTable.value(leftup)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(leftup);
				linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x() + m_tileSize, label.point().y() + m_tileSize)));
			}

			if ((label.point().x() + label.boundary().width() > m_tileSize)
					&& (!m_dependencyTable.value(rightup)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(rightup);
				linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x() - m_tileSize, label.point().y() + m_tileSize)));
			}
		}

		// down
		if ((label.point().y() > m_tileSize) && (!m_dependencyTable.value(down)->isDrawn())) {
			if (!alreadyAdded) {
				alreadyAdded = true;

				m_currentDependencyOnTile->addText(Dependency<PointTextContainer>(label, Point(label.point().x(), label.point().y())));
			}

			{
				DependencyOnTile *const linkedDep = m_dependencyTable.value(down);
				linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x(), label.point().y() - m_tileSize)));
			}

			if ((label.point().x() < 0.0f) && (!m_dependencyTable.value(leftdown)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(leftdown);
				linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x() + m_tileSize, label.point().y() - m_tileSize)));
			}

			if ((label.point().x() + label.boundary().width() > m_tileSize)
					&& (!m_dependencyTable.value(rightdown)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(rightdown);
				linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x() - m_tileSize, label.point().y() - m_tileSize)));
			}
		}

		// left
		if ((label.point().x() < 0.0f) && (!m_dependencyTable.value(left)->isDrawn())) {
			if (!alreadyAdded) {
				alreadyAdded = true;

				m_currentDependencyOnTile->addText(Dependency<PointTextContainer>(label, Point(label.point().x(), label.point().y())));
			}

			{
				DependencyOnTile *const linkedDep = m_dependencyTable.value(left);
				linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x() + m_tileSize, label.point().y())));
			}
		}

		// right
		if ((label.point().x() + label.boundary().width() > m_tileSize) && (!m_dependencyTable.value(right)->isDrawn())) {
			if (!alreadyAdded) {
				alreadyAdded = true;

				m_currentDependencyOnTile->addText(Dependency<PointTextContainer>(label, Point(label.point().x(), label.point().y())));
			}

			{
				DependencyOnTile *const linkedDep = m_dependencyTable.value(right);
				linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x() - m_tileSize, label.point().y())));
			}
		}

		// check symbols

		if ((label.symbol() != 0) && (!alreadyAdded)) {
			if ((label.symbol()->point().y() <= 0.0f) && (!m_dependencyTable.value(up)->isDrawn())) {
				alreadyAdded = true;
				m_currentDependencyOnTile->addText(Dependency<PointTextContainer>(label, Point(label.point().x(), label.point().y())));

				{
					DependencyOnTile *const linkedDep = m_dependencyTable.value(up);
					linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x(), label.point().y() + m_tileSize)));
				}

				if ((label.symbol()->point().x() < 0.0f) && (!m_dependencyTable.value(leftup)->isDrawn())) {
					DependencyOnTile *const linkedDep = m_dependencyTable.value(leftup);
					linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x() + m_tileSize,
							label.point().y() + m_tileSize)));
				}

				if ((label.symbol()->point().x() + label.symbol()->symbol().width() > m_tileSize)
						&& (!m_dependencyTable.value(rightup)->isDrawn())) {
					DependencyOnTile *const linkedDep = m_dependencyTable.value(rightup);
					linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x() - m_tileSize,
							label.point().y() + m_tileSize)));
				}
			}

			if ((label.symbol()->point().y() + label.symbol()->symbol().height() >= m_tileSize)
					&& (!m_dependencyTable.value(down)->isDrawn())) {
				if (!alreadyAdded) {
					alreadyAdded = true;

					m_currentDependencyOnTile->addText(Dependency<PointTextContainer>(label, Point(label.point().x(),
							label.point().y())));
				}

				{
					DependencyOnTile *const linkedDep = m_dependencyTable.value(down);
					linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x(), label.point().y() + m_tileSize)));
				}

				if ((label.symbol()->point().x() < 0.0f) && (!m_dependencyTable.value(leftdown)->isDrawn())) {
					DependencyOnTile *const linkedDep = m_dependencyTable.value(leftdown);
					linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x() + m_tileSize,
							label.point().y() - m_tileSize)));
				}

				if ((label.symbol()->point().x() + label.symbol()->symbol().width() > m_tileSize)
						&& (!m_dependencyTable.value(rightdown)->isDrawn())) {
					DependencyOnTile *const linkedDep = m_dependencyTable.value(rightdown);
					linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x() - m_tileSize,
							label.point().y() - m_tileSize)));
				}
			}

			if ((label.symbol()->point().x() <= 0.0f) && (!m_dependencyTable.value(left)->isDrawn())) {
				if (!alreadyAdded) {
					alreadyAdded = true;

					m_currentDependencyOnTile->addText(Dependency<PointTextContainer>(label, Point(label.point().x(),
							label.point().y())));
				}

				DependencyOnTile *const linkedDep = m_dependencyTable.value(left);
				linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x() - m_tileSize, label.point().y())));
			}

			if ((label.symbol()->point().x() + label.symbol()->symbol().width() >= m_tileSize)
					&& (!m_dependencyTable.value(right)->isDrawn())) {
				if (!alreadyAdded) {
					alreadyAdded = true;

					m_currentDependencyOnTile->addText(Dependency<PointTextContainer>(label, Point(label.point().x(), label.point().y())));
				}

				DependencyOnTile *const linkedDep = m_dependencyTable.value(right);
				linkedDep->addText(Dependency<PointTextContainer>(label, Point(label.point().x() + m_tileSize, label.point().y())));
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
				m_currentDependencyOnTile->addSymbol(Dependency<QImage>(container.symbol(), Point(container.point().x(), container.point().y())));
			}

			{
				DependencyOnTile *const linkedDep = m_dependencyTable.value(up);
				linkedDep->addSymbol(Dependency<QImage>(container.symbol(), Point(container.point().x(), container.point().y() + m_tileSize)));
			}

			if ((container.point().x() < 0.0f) && (!m_dependencyTable.value(leftup)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(leftup);
				linkedDep->addSymbol(Dependency<QImage>(container.symbol(), Point(container.point().x() + m_tileSize, container.point().y() + m_tileSize)));
			}

			if ((container.point().x() + container.symbol().width() > m_tileSize) && (!m_dependencyTable.value(rightup)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(rightup);
				linkedDep->addSymbol(Dependency<QImage>(container.symbol(), Point(container.point().x() - m_tileSize, container.point().y() + m_tileSize)));
			}
		}

		// down
		if ((container.point().y() + container.symbol().height() > m_tileSize) && (!m_dependencyTable.value(down)->isDrawn())) {
			if (!alreadyAdded) {
				alreadyAdded = true;
				m_currentDependencyOnTile->addSymbol(Dependency<QImage>(container.symbol(), Point(container.point().x(), container.point().y())));
			}

			{
				DependencyOnTile *const linkedDep = m_dependencyTable.value(down);
				linkedDep->addSymbol(Dependency<QImage>(container.symbol(), Point(container.point().x(), container.point().y() - m_tileSize)));
			}

			if ((container.point().x() < 0.0f) && (!m_dependencyTable.value(leftdown)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(leftdown);
				linkedDep->addSymbol(Dependency<QImage>(container.symbol(), Point(container.point().x() + m_tileSize, container.point().y() - m_tileSize)));
			}

			if ((container.point().x() + container.symbol().width() > m_tileSize) && (!m_dependencyTable.value(rightdown)->isDrawn())) {
				DependencyOnTile *const linkedDep = m_dependencyTable.value(rightdown);
				linkedDep->addSymbol(Dependency<QImage>(container.symbol(), Point(container.point().x() - m_tileSize, container.point().y() - m_tileSize)));
			}
		}

		// left
		if ((container.point().x() < 0.0f) && (!m_dependencyTable.value(left)->isDrawn())) {
			if (!alreadyAdded) {
				alreadyAdded = true;
				m_currentDependencyOnTile->addSymbol(Dependency<QImage>(container.symbol(), Point(container.point().x(), container.point().y())));
			}

			DependencyOnTile *const linkedDep = m_dependencyTable.value(left);
			linkedDep->addSymbol(Dependency<QImage>(container.symbol(), Point(container.point().x() + m_tileSize, container.point().y())));
		}

		// right
		if ((container.point().x() + container.symbol().width() > m_tileSize) && (!m_dependencyTable.value(right)->isDrawn())) {
			if (!alreadyAdded) {
				alreadyAdded = true;
				m_currentDependencyOnTile->addSymbol(Dependency<QImage>(container.symbol(), Point(container.point().x(), container.point().y())));
			}

			DependencyOnTile *const linkedDep = m_dependencyTable.value(right);
			linkedDep->addSymbol(Dependency<QImage>(container.symbol(), Point(container.point().x() - m_tileSize, container.point().y())));
		}
	}
}

void DependencyCache::removeOverlappingAreaLabelsWithDependencyLabels(QList<PointTextContainer> &areaLabels) const
{
	if (m_currentDependencyOnTile->labels().isEmpty())
		return;

	foreach (const Dependency<PointTextContainer> &depLabel, m_currentDependencyOnTile->labels()) {
		const Rectangle rect1 = Rectangle((int) (depLabel.point.x()),
				(int) (depLabel.point.y() - depLabel.value.boundary().height()),
				(int) (depLabel.point.x() + depLabel.value.boundary().width()),
				(int) (depLabel.point.y()));

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
	foreach (const Dependency<QImage> &depSmb, m_currentDependencyOnTile->symbols()) {
		const Rectangle rect1 = Rectangle((int) depSmb.point.x(), (int) depSmb.point.y(), (int) depSmb.point.x()
				+ depSmb.value.width(), (int) depSmb.point.y() + depSmb.value.height());

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
	foreach (const Dependency<PointTextContainer> &dep, m_currentDependencyOnTile->labels()) {
		for (int j = 0; j < labels.size(); j++) {
			if ((labels.at(j).text() == dep.value.text())
					&& (labels.at(j).paintFront() == dep.value.paintFront())
					&& (labels.at(j).paintBack() == dep.value.paintBack())) {
				labels.removeAt(j);
				j--;
			}
		}
	}
}

void DependencyCache::removeOverlappingSymbolsWithDepencySymbols(QList<SymbolContainer> &symbols, int dis) const
{
	foreach (const Dependency<QImage> &sym2, m_currentDependencyOnTile->symbols()) {
		const Rectangle rect1 = Rectangle((int) sym2.point.x() - dis, (int) sym2.point.y() - dis, (int) sym2.point.x()
				+ sym2.value.width() + dis, (int) sym2.point.y() + sym2.value.height() + dis);

		for (int j = 0; j < symbols.size(); j++) {
			const SymbolContainer symbolContainer = symbols.at(j);
			const Rectangle rect2 = Rectangle((int) symbolContainer.point().x(), (int) symbolContainer.point().y(),
					(int) symbolContainer.point().x() + symbolContainer.symbol().width(),
					(int) symbolContainer.point().y() + symbolContainer.symbol().height());

			if (rect2.intersects(rect1)) {
				symbols.removeAt(j);
				j--;
			}
		}
	}
}

void DependencyCache::removeOverlappingSymbolsWithDependencyLabels(QList<SymbolContainer> &symbols) const
{
	foreach (const Dependency<PointTextContainer> &depLabel, m_currentDependencyOnTile->labels()) {
		const Rectangle rect1 = Rectangle((int) (depLabel.point.x()),
				(int) (depLabel.point.y() - depLabel.value.boundary().height()),
				(int) (depLabel.point.x() + depLabel.value.boundary().width()),
				(int) (depLabel.point.y()));

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
