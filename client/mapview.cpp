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

#include "mapview.h"
#include "ui_mapview.h"

MapView::MapView(QWidget *parent) :
	 QDialog(parent, Qt::Window ),
    ui(new Ui::MapView)
{
    ui->setupUi(this);
	renderer = NULL;
	connectSlots();
}

MapView::~MapView()
{
    delete ui;
}

void MapView::connectSlots()
{
	connect( ui->zoomBar, SIGNAL(valueChanged(int)), ui->paintArea, SLOT(setZoom(int)) );
	connect( ui->paintArea, SIGNAL(zoomChanged(int)), ui->zoomBar, SLOT(setValue(int)) );
}

void MapView::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MapView::showEvent( QShowEvent * event )
{
	if ( renderer != NULL )
	{
		maxZoom = renderer->GetMaxZoom();
		ui->zoomBar->setMaximum( maxZoom );
		ui->paintArea->setZoom( 0 );
		ui->paintArea->setMaxZoom( maxZoom );
	}
}

void MapView::setRender( IRenderer* r )
{
	renderer = r;
	ui->paintArea->setRenderer( r );
}
