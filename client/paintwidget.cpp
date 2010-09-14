/*
Copyright 2010  Christian Vetter veaac.fdirct@gmail.com

This file is part of MoNav.

MoNav is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MoNav is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MoNav.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "paintwidget.h"
#include "ui_paintwidget.h"
#include "utils/qthelpers.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtDebug>

PaintWidget::PaintWidget(QWidget *parent) :
	QWidget( parent ),
	m_ui(new Ui::PaintWidget)
{
	m_ui->setupUi(this);
	m_lastMouseX = 0;
	m_lastMouseY = 0;
	m_wheelDelta = 0;
	m_fixed = false;
}

PaintWidget::~PaintWidget()
{
	delete m_ui;
}

void PaintWidget::setFixed( bool f )
{
	m_fixed = f;
}

void PaintWidget::setRenderer( IRenderer* r )
{
	m_renderer = r;
	m_renderer->SetUpdateSlot( this, SLOT(update()) );
}

void PaintWidget::setCenter( const ProjectedCoordinate c )
{
	m_request.center = c;
	if ( isVisible() )
		update();
}

void PaintWidget::setZoom( int z )
{
	m_request.zoom = z;
	if ( isVisible() )
		update();
}

void PaintWidget::setMaxZoom( int z )
{
	m_maxZoom = z;
}

void PaintWidget::setPosition( const UnsignedCoordinate p, double heading )
{
	m_request.position = p;
	m_request.heading = heading;
	if ( isVisible() )
		update();
}

void PaintWidget::setTarget( const UnsignedCoordinate t )
{
	m_request.target = t;
	if ( isVisible() )
		update();
}

void PaintWidget::setPOIs( QVector< UnsignedCoordinate > p )
{
	m_request.POIs = p;
	if ( isVisible() )
		update();
}

void PaintWidget::setPOI( UnsignedCoordinate p )
{
	m_request.POIs = QVector< UnsignedCoordinate >( 1, p );
	if ( isVisible() )
		update();
}

void PaintWidget::setRoute( QVector< IRouter::Node > r )
{
	m_request.route = r;
	if ( isVisible() )
		update();
}

void PaintWidget::setEdges( QVector< int > edgeSegments, QVector< UnsignedCoordinate > edges )
{
	m_request.edgeSegments = edgeSegments;
	m_request.edges = edges;
	if ( isVisible() )
		update();
}
void PaintWidget::setVirtualZoom( int z )
{
	m_request.virtualZoom = z;
	if ( isVisible() )
		update();
}

void PaintWidget::mousePressEvent( QMouseEvent* event )
{
	if ( m_fixed )
		return;
	if ( event->button() != Qt::LeftButton )
		return;
	m_startMouseX = m_lastMouseX = event->x();
	m_startMouseY = m_lastMouseY = event->y();
	m_drag = false;
}

void PaintWidget::mouseMoveEvent( QMouseEvent* event )
{
	if ( m_fixed )
		return;
	if ( ( event->buttons() & Qt::LeftButton ) == 0 )
		return;
	if ( abs( event->x() - m_startMouseX ) + abs( event->y() - m_startMouseY ) > 7 )
		m_drag = true;
	if ( !m_drag )
		return;
	m_request.center = m_renderer->Move( event->x() - m_lastMouseX, event->y() - m_lastMouseY, m_request );
	m_lastMouseX = event->x();
	m_lastMouseY = event->y();
	update();
}

void PaintWidget::mouseReleaseEvent( QMouseEvent* event )
{
	if ( m_fixed )
		return;
	if ( m_drag )
		return;
	if ( event->button() != Qt::LeftButton )
		return;
	if ( m_renderer == NULL )
		return;
	emit mouseClicked( m_renderer->PointToCoordinate( event->x() - width() / 2, event->y() - height() / 2, m_request ) );
}

void PaintWidget::wheelEvent( QWheelEvent * event )
{
	if ( m_renderer == NULL )
		return;

	// 15 degrees is a full mousewheel "click"
	int numDegrees = event->delta() / 8 + m_wheelDelta;
	int numSteps = numDegrees / 15;
	m_wheelDelta = numDegrees % 15;

	// limit zoom
	int newZoom = m_request.zoom + numSteps;
	if ( newZoom < 0 )
		newZoom = 0;
	if ( newZoom > m_maxZoom )
		newZoom = m_maxZoom;

	// avoid looping event calls
	if ( newZoom == m_request.zoom )
		return;

	// zoom in/out on current mouse position
	m_request.center = m_renderer->Move( width() / 2 - event->x(), height() / 2 - event->y(), m_request );
	m_request.zoom = newZoom;
	m_request.center = m_renderer->Move( event->x() - width() / 2, event->y() - height() / 2, m_request );

	emit zoomChanged( newZoom );
	update();
}

void PaintWidget::paintEvent( QPaintEvent* )
{
	if ( m_renderer == NULL )
		return;
	if ( !isVisible() )
		return;

	if ( m_fixed ) {
		m_request.center = m_request.position.ToProjectedCoordinate();

		//gradually change the screen rotation to match the heading
		double diff = m_request.rotation + m_request.heading;
		while ( diff <= -180 )
			diff += 360;
		while ( diff >= 180 )
			diff -=360;
		//to filter out noise stop when close enough
		if ( diff > 0 )
			diff = std::max( 0.0, diff - 15 );
		if ( diff < 0 )
			diff = std::min( 0.0, diff + 15 );
		m_request.rotation -= diff / 2;

		//normalize
		while ( m_request.rotation < 0 )
			m_request.rotation += 360;
		while ( m_request.rotation >= 360 )
			m_request.rotation -= 360;
		int radius = height() * 0.3;

		m_request.center = m_renderer->PointToCoordinate( 0, -radius, m_request );
	}

	QPainter painter( this );
	Timer time;
	m_renderer->Paint( &painter, m_request );
	qDebug() << "Rendering:" << time.elapsed() << "ms";
}

void PaintWidget::contextMenuEvent(QContextMenuEvent *event )
{
	emit contextMenu( event->globalPos() );
}
