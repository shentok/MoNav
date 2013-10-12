#include "TileRasterer.h"

#include "renderer/CircleContainer.h"
#include "renderer/PointTextContainer.h"
#include "renderer/PolylineContainer.h"
#include "renderer/SymbolContainer.h"
#include "renderer/WayTextContainer.h"

#include <qmath.h>
#include <QFontMetrics>
#include <QPainter>

TileRasterer::TileRasterer(QPainter *painter) :
	m_painter(painter)
{
}

void TileRasterer::drawWays(const QVector<QVector<QList<ShapePaintContainer *> > > &ways)
{
	foreach (const QVector<QList<ShapePaintContainer *> > &shapePaintContainers, ways) {
		foreach (const QList<ShapePaintContainer *> &wayList, shapePaintContainers) {
			for (int index = wayList.size() - 1; index >= 0; --index) {
				m_painter->save();
				const ShapePaintContainer *const shapePaintContainer = wayList.at(index);
				shapePaintContainer->shapeContainer()->render(m_painter);
				m_painter->restore();
			}
		}
	}
}

void TileRasterer::drawSymbols(const QList<SymbolContainer> &symbols)
{
	for (int index = symbols.size() - 1; index >= 0; --index) {
		const SymbolContainer symbolContainer = symbols.at(index);
		const Point point = symbolContainer.point();

		if (symbolContainer.alignCenter()) {
			const int pivotX = symbolContainer.symbol().width() / 2;
			const int pivotY = symbolContainer.symbol().height() / 2;
			m_painter->translate(point.x(), point.y());
			m_painter->rotate(symbolContainer.theta());
			m_painter->translate(-pivotX, -pivotY);
		} else {
			m_painter->translate(point.x(), point.y());
			m_painter->rotate(symbolContainer.theta());
		}

		m_painter->drawImage(0, 0, symbolContainer.symbol());
		m_painter->resetTransform();
	}
}

void TileRasterer::drawWayNames(const QList<WayTextContainer> &wayTexts)
{
	m_painter->save();

	for (int index = wayTexts.size() - 1; index >= 0; --index) {
		const WayTextContainer wayText = wayTexts.at(index);
		const Paint &paint = wayText.paint();

		if (paint.style() == Qt::NoPen)
			continue;

		m_painter->setPen(paint);
		m_painter->setFont(wayText.font());

		const Point diff = wayText.p2() - wayText.p1();

		const qreal theta = qAtan2(diff.y(), diff.x()) * 180 / M_PI;
		m_painter->translate(wayText.p1());
		m_painter->rotate(theta);

		const qreal lineLength = qSqrt(diff.x()*diff.x() + diff.y()*diff.y());
		const int textWidth = QFontMetrics(wayText.font()).width(wayText.text());
		const int dx = (int) (lineLength - textWidth) / 2;
		const int xy = QFontMetrics(wayText.font()).height() / 3;

		m_painter->drawText(dx, xy, wayText.text());
		m_painter->resetTransform();
	}

	m_painter->restore();
}

void TileRasterer::drawNodes(const QList<PointTextContainer> &nodes)
{
	m_painter->save();

	for (int index = nodes.size() - 1; index >= 0; --index) {
		const PointTextContainer pointTextContainer = nodes.at(index);

		if (pointTextContainer.paintBack().style() != Qt::NoPen) {
			m_painter->setPen(pointTextContainer.paintBack());
			m_painter->setFont(pointTextContainer.font());
			m_painter->drawText(pointTextContainer.point(), pointTextContainer.text());
		}

		m_painter->setPen(pointTextContainer.paintFront());
		m_painter->setFont(pointTextContainer.font());
		m_painter->drawText(pointTextContainer.point(), pointTextContainer.text());
	}

	m_painter->restore();
}
