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

PaintWidget::PaintWidget(QWidget *parent) :
	 QWidget(parent ),
    ui(new Ui::PaintWidget)
{
	ui->setupUi(this);
	center.x = 0.5;
	center.y = 0.5;
	zoom = 0;
	lastMouseX = 0;
	lastMouseY = 0;
	wheelDelta = 0;
}

PaintWidget::~PaintWidget()
{
    delete ui;
}

void PaintWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void PaintWidget::setRenderer( IRenderer* r )
{
	renderer = r;
}

void PaintWidget::setCenter( const ProjectedCoordinate& c )
{
	center = c;
	if ( isVisible() )
	{
		update();
	}
}

void PaintWidget::setZoom( int z )
{
	zoom = z;
	if ( isVisible() )
	{
		update();
	}
}

void PaintWidget::setMaxZoom( int z )
{
	maxZoom = z;
}

void PaintWidget::mousePressEvent( QMouseEvent* event )
{
	if ( event->button() != Qt::LeftButton )
		return;
	startMouseX = lastMouseX = event->x();
	startMouseY = lastMouseY = event->y();
	drag = false;
}

void PaintWidget::mouseMoveEvent( QMouseEvent* event )
{
	if ( ( event->buttons() & Qt::LeftButton ) == 0 )
		return;
	if ( abs( event->x() - startMouseX ) + abs( event->y() - startMouseY ) > 7 )
		drag = true;
	if ( !drag )
		return;
	center = renderer->Move( center, event->x() - lastMouseX, event->y() - lastMouseY, zoom );
	lastMouseX = event->x();
	lastMouseY = event->y();
	update();
}

void PaintWidget::mouseReleaseEvent( QMouseEvent* event )
{
	if ( drag )
		return;
	if ( event->button() != Qt::LeftButton )
		return;
	if ( renderer == NULL )
		return;
	emit mouseClicked( renderer->PointToCoordinate( center, event->x() - width() / 2, event->y() - height() / 2, zoom ) );
}

void PaintWidget::wheelEvent( QWheelEvent * event )
{
	if ( renderer == NULL )
		return;

	int numDegrees = event->delta() / 8 + wheelDelta;
	int numSteps = numDegrees / 15;
	wheelDelta = numDegrees % 15;

	int newZoom = zoom + numSteps;
	if ( newZoom < 0 )
		newZoom = 0;
	if ( newZoom > maxZoom )
		newZoom = maxZoom;

	if ( newZoom == zoom )
		return;

	center = renderer->Move( center, width() / 2 - event->x(), height() / 2 - event->y(), zoom );
	zoom = newZoom;
	center = renderer->Move( center, event->x() - width() / 2, event->y() - height() / 2, zoom );

	emit zoomChanged( newZoom );
	update();
}

void PaintWidget::paintEvent( QPaintEvent* )
{
	if ( renderer == NULL )
		return;

	QPainter painter( this );
	renderer->Paint( &painter, center, zoom, 0, 0 );
}
