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
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <algorithm>
#include <QtDebug>
#include <QTime>

PaintWidget::PaintWidget(QWidget *parent) :
	 QWidget(parent ),
    ui(new Ui::PaintWidget)
{
	ui->setupUi(this);
	lastMouseX = 0;
	lastMouseY = 0;
	wheelDelta = 0;
	fixed = false;
}

PaintWidget::~PaintWidget()
{
    delete ui;
}

void PaintWidget::setFixed( bool f )
{
	fixed = f;
}

void PaintWidget::setRenderer( IRenderer* r )
{
	renderer = r;
	renderer->setSlot( this, SLOT(update()) );
}

void PaintWidget::setCenter( const ProjectedCoordinate c )
{
	request.center = c;
	if ( isVisible() )
		update();
}

void PaintWidget::setZoom( int z )
{
	request.zoom = z;
	if ( isVisible() )
		update();
}

void PaintWidget::setMaxZoom( int z )
{
	maxZoom = z;
}

void PaintWidget::setPosition( const UnsignedCoordinate p, double heading )
{
	request.position = p;
	request.heading = heading;
	if ( isVisible() )
		update();
}

void PaintWidget::setTarget( const UnsignedCoordinate t )
{
	request.target = t;
	if ( isVisible() )
		update();
}

void PaintWidget::setPOIs( QVector< UnsignedCoordinate > p )
{
	request.POIs = p;
	if ( isVisible() )
		update();
}

void PaintWidget::setRoute( QVector< UnsignedCoordinate > r )
{
	request.route = r;
	if ( isVisible() )
		update();
}

void PaintWidget::setEdges( QVector< int > edgeSegments, QVector< UnsignedCoordinate > edges )
{
	request.edgeSegments = edgeSegments;
	request.edges = edges;
	if ( isVisible() )
		update();
}
void PaintWidget::setVirtualZoom( int z )
{
	request.virtualZoom = z;
	if ( isVisible() )
		update();
}

void PaintWidget::mousePressEvent( QMouseEvent* event )
{
	if ( fixed )
		return;
	if ( event->button() != Qt::LeftButton )
		return;
	startMouseX = lastMouseX = event->x();
	startMouseY = lastMouseY = event->y();
	drag = false;
}

void PaintWidget::mouseMoveEvent( QMouseEvent* event )
{
	if ( fixed )
		return;
	if ( ( event->buttons() & Qt::LeftButton ) == 0 )
		return;
	if ( abs( event->x() - startMouseX ) + abs( event->y() - startMouseY ) > 7 )
		drag = true;
	if ( !drag )
		return;
	request.center = renderer->Move( event->x() - lastMouseX, event->y() - lastMouseY, request );
	lastMouseX = event->x();
	lastMouseY = event->y();
	update();
}

void PaintWidget::mouseReleaseEvent( QMouseEvent* event )
{
	if ( fixed )
		return;
	if ( drag )
		return;
	if ( event->button() != Qt::LeftButton )
		return;
	if ( renderer == NULL )
		return;
	emit mouseClicked( renderer->PointToCoordinate( event->x() - width() / 2, event->y() - height() / 2, request ) );
}

void PaintWidget::wheelEvent( QWheelEvent * event )
{
	if ( renderer == NULL )
		return;

	// 15 degrees is a full mousewheel "click"
	int numDegrees = event->delta() / 8 + wheelDelta;
	int numSteps = numDegrees / 15;
	wheelDelta = numDegrees % 15;

	// limit zoom
	int newZoom = request.zoom + numSteps;
	if ( newZoom < 0 )
		newZoom = 0;
	if ( newZoom > maxZoom )
		newZoom = maxZoom;

	// avoid looping event calls
	if ( newZoom == request.zoom )
		return;

	// zoom in/out on current mouse position
	request.center = renderer->Move( width() / 2 - event->x(), height() / 2 - event->y(), request );
	request.zoom = newZoom;
	request.center = renderer->Move( event->x() - width() / 2, event->y() - height() / 2, request );

	emit zoomChanged( newZoom );
	update();
}

void PaintWidget::paintEvent( QPaintEvent* )
{
	if ( renderer == NULL )
		return;
	if ( !isVisible() )
		return;

	if ( fixed ) {
		request.center = request.position.ToProjectedCoordinate();

		//gradually change the screen rotation to match the heading
		double diff = request.rotation + request.heading;
		while ( diff <= -180 )
			diff += 360;
		while ( diff >= 180 )
			diff -=360;
		//to filter out noise stop when close enough
		if ( diff > 0 )
			diff = std::max( 0.0, diff - 15 );
		if ( diff < 0 )
			diff = std::min( 0.0, diff + 15 );
		request.rotation -= diff / 2;

		//normalize
		while ( request.rotation < 0 )
			request.rotation += 360;
		while ( request.rotation >= 360 )
			request.rotation -= 360;
		int radius = height() * 0.3;

		request.center = renderer->PointToCoordinate( 0, -radius, request );
	}

	QPainter painter( this );
	QTime time;
	time.start();
	renderer->Paint( &painter, request );
	qDebug() << "Rendering:" << time.elapsed() << "ms";
}

void PaintWidget::contextMenuEvent(QContextMenuEvent *event )
{
	emit contextMenu( event->globalPos() );
}
