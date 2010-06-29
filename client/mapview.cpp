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
#include <QtDebug>

MapView::MapView(QWidget *parent) :
	 QDialog(parent, Qt::Window ),
    ui(new Ui::MapView)
{
	ui->setupUi(this);
	ui->headerWidget->hide();
	ui->infoWidget->hide();
	renderer = NULL;
	gpsLookup = NULL;
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
	connect( ui->paintArea, SIGNAL(mouseClicked(ProjectedCoordinate)), this, SLOT(mouseClicked(ProjectedCoordinate)) );
	connect( ui->previousButton, SIGNAL(clicked()), this, SLOT(previousPlace()) );
	connect( ui->nextButton, SIGNAL(clicked()), this, SLOT(nextPlace()) );
}

void MapView::showEvent( QShowEvent * /*event*/ )
{
	if ( renderer != NULL )
	{
		maxZoom = renderer->GetMaxZoom();
		ui->zoomBar->setMaximum( maxZoom );
		ui->paintArea->setZoom( maxZoom );
		ui->paintArea->setMaxZoom( maxZoom );
	}
}

void MapView::setRender( IRenderer* r )
{
	renderer = r;
	ui->paintArea->setRenderer( r );
}

void MapView::setGPSLookup( IGPSLookup*g )
{
	gpsLookup = g;
}

void MapView::setCenter( ProjectedCoordinate center )
{
	ui->paintArea->setCenter( center );
}

void MapView::setSource( UnsignedCoordinate s, double heading )
{
	source = s;
	renderer->SetPosition( source, heading );
}

void MapView::mouseClicked( ProjectedCoordinate clickPos )
{
	if ( gpsLookup == NULL )
		return;
	QVector< IGPSLookup::Result > result;
	gpsLookup->GetNearEdges( &result, UnsignedCoordinate( clickPos ), 100 );
	if ( result.size() == 0 )
		return;
	QVector< UnsignedCoordinate > points;
	selected = result.first().nearestPoint;
	points.push_back( selected );
	renderer->SetPoints( points );
	ui->paintArea->update();
}

void MapView::nextPlace()
{
	place = ( place + 1 ) % places.size();
	ui->headerLabel->setText( QString( tr( "Choose City (%1/%2)" ) ).arg( place + 1 ).arg( places.size() ) );
	ui->paintArea->setCenter( places[place].ToProjectedCoordinate() );
	ui->paintArea->update();
}

void MapView::previousPlace()
{
	place = ( place + places.size() - 1 ) % places.size();
	ui->headerLabel->setText( QString( tr( "Choose City (%1/%2)" ) ).arg( place + 1 ).arg( places.size() ) );
	ui->paintArea->setCenter( places[place].ToProjectedCoordinate() );
	ui->paintArea->update();
}

int MapView::selectPlaces( QVector< UnsignedCoordinate > places, IRenderer* renderer, QWidget* p )
{
	if ( places.size() == 0 )
		return -1;
	if ( renderer == NULL )
		return -1;

	MapView* window = new MapView( p );
	window->setRender( renderer );
	window->setPlaces( places );

	window->exec();
	if ( window->result() != Accepted )
		return -1;

	int id = window->place;
	delete window;
	renderer->SetPoints( QVector< UnsignedCoordinate >() );
	return id;
}

void MapView::setPlaces( QVector< UnsignedCoordinate > p )
{
	places = p;
	place = 0;

	ui->headerLabel->setText( QString( tr( "Choose City (%1/%2)" ) ).arg( 1 ).arg( p.size() ) );
	ui->headerWidget->show();

	ui->paintArea->setCenter( places.first().ToProjectedCoordinate() );
	renderer->SetPoints( p );

	ui->paintArea->update();
}

bool MapView::selectStreet( UnsignedCoordinate* result, QVector< int >segmentLength, QVector< UnsignedCoordinate > coordinates, IRenderer* renderer, IGPSLookup* gpsLookup, QWidget* p )
{
	if ( result == NULL )
		return false;
	if ( segmentLength.size() == 0 )
		return false;
	if ( coordinates.size() == 0 )
		return false;
	if ( renderer == 0 )
		return false;
	if ( gpsLookup == 0 )
		return false;

	MapView* window = new MapView( p );
	window->setRender( renderer );
	window->setGPSLookup( gpsLookup );
	window->setEdges( segmentLength, coordinates );

	window->exec();
	if ( window->result() != Accepted )
		return false;

	*result = window->selected;
	delete window;
	renderer->SetPoints( QVector< UnsignedCoordinate >() );
	renderer->SetEdges( QVector< int >(), QVector< UnsignedCoordinate >() );
	return true;
}

void MapView::setEdges( QVector< int > segmentLength, QVector< UnsignedCoordinate > coordinates )
{
	ui->headerLabel->setText( tr( "Choose Coordinate" ) );
	ui->headerWidget->show();
	ui->nextButton->hide();
	ui->previousButton->hide();

	ui->paintArea->setCenter( coordinates.first().ToProjectedCoordinate() );
	renderer->SetEdges( segmentLength, coordinates );

	ui->paintArea->update();
}
