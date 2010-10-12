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

#include "overlaywidget.h"
#include <QMouseEvent>
#include <QCursor>

OverlayWidget::OverlayWidget( QWidget *parent, QString title ) :
	QWidget( parent )
{
	m_centralWidget = NULL;
	m_grid = new QGridLayout( this );
	m_grid->setColumnStretch( 0, 1 );
	m_grid->setColumnStretch( 1, 0 );
	m_grid->setColumnStretch( 2, 1 );
	m_grid->setRowStretch( 0, 1 );
	m_grid->setRowStretch( 1, 0 );
	m_grid->setRowStretch( 2, 1 );

	m_centralWidget = new QToolBar( title );
	m_centralWidget->setAutoFillBackground( true );
	m_centralWidget->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
	m_grid->addWidget( m_centralWidget, 1, 1 );

	hide();

	QPalette pal = palette();
	pal.setColor( QPalette::Window, QColor( 0, 0, 0, 128 ) );
	setPalette( pal );

	setAutoFillBackground( true );

	connect( m_centralWidget, SIGNAL(actionTriggered(QAction*)), this, SLOT(hide()) );
}

void OverlayWidget::addAction( QAction *action )
{
	m_centralWidget->addAction( action );
}

void OverlayWidget::addActions( QList< QAction* > actions )
{
	m_centralWidget->addActions( actions );
}

QList< QAction* > OverlayWidget::actions() const
{
	return m_centralWidget->actions();
}

void OverlayWidget::hideEvent( QHideEvent* event )
{
	if ( event->spontaneous() || parentWidget() == NULL )
		return;
	parentWidget()->removeEventFilter( this );
}

void OverlayWidget::showEvent( QShowEvent* event )
{
	if ( event->spontaneous() || parentWidget() == NULL )
		return;
	raise();
	setFixedSize( parentWidget()->size() );
	setOrientation();
	parentWidget()->installEventFilter( this );
	QCursor::setPos( mapToGlobal( m_centralWidget->pos() ) );
}

bool OverlayWidget::eventFilter( QObject* obj, QEvent* ev )
{
	if ( obj != parent() )
		 return QWidget::eventFilter( obj, ev );

	QWidget *parent = parentWidget();

	if ( ev->type() == QEvent::Resize ) {
		setFixedSize( parent->size() );
		setOrientation();
	}

	return QWidget::eventFilter( obj, ev );
}

void OverlayWidget::mouseReleaseEvent( QMouseEvent * event )
{
	event->accept();
	if ( event->button() == Qt::LeftButton )
		hide();
}

void OverlayWidget::setOrientation()
{
	if ( width() > height() )
		m_centralWidget->setOrientation( Qt::Horizontal );
	else
		m_centralWidget->setOrientation( Qt::Vertical );
}
