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
#include <QInputDialog>
#include "addressdialog.h"

MapView::MapView(QWidget *parent) :
	 QDialog(parent, Qt::Window ),
    ui(new Ui::MapView)
{
	ui->setupUi(this);
	setupMenu();
	ui->headerWidget->hide();
	ui->infoWidget->hide();
	renderer = NULL;
	gpsLookup = NULL;
	addressLookup = NULL;
	contextMenuEnabled = false;
	mode = POI;
	connectSlots();
	heading = 0;
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
	connect( ui->paintArea, SIGNAL(contextMenu(QPoint)), this, SLOT(showContextMenu(QPoint)) );
}

void MapView::setupMenu()
{
	contextMenu = new QMenu( this );
	gotoSourceAction = contextMenu->addAction( tr( "Goto Source" ), this, SLOT(gotoSource()) );
	gotoGPSAction = contextMenu->addAction( tr( "Goto GPS" ), this, SLOT(gotoGPS()) );
	gotoTargetAction = contextMenu->addAction( tr( "Goto Target" ), this, SLOT(gotoTarget()) );
	gotoAddressAction = contextMenu->addAction( tr( "Goto Address" ), this, SLOT(gotoAddress()) );
	contextMenu->addSeparator();
	modeGroup = new QActionGroup( this );
	modeSourceAction = new QAction( tr( "Choose Source" ), modeGroup );
	modeSourceAction->setCheckable( true );
	modeTargetAction = new QAction( tr( "Choose Target" ), modeGroup );
	modeTargetAction->setCheckable( true );
	modePOIAction = new QAction( tr( "Choose POI" ), modeGroup );
	modePOIAction->setCheckable( true );
	modePOIAction->setChecked( true );
	contextMenu->addActions( modeGroup->actions() );
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

void MapView::setAddressLookup( IAddressLookup* al )
{
	addressLookup = al;
}

void MapView::setSource( UnsignedCoordinate s, double h )
{
	if ( source.x != s.x || source.y != s.y|| h != heading )
		emit sourceChanged( s, heading );
	source = s;
	heading = h;
	ui->paintArea->setPosition( source, heading );
}

void MapView::setTarget( UnsignedCoordinate t )
{
	if ( target.x != t.x || target.y != t.y )
		emit targetChanged( t );
	target = t;
	ui->paintArea->setTarget( target );
}

void MapView::setContextMenuEnabled( bool e )
{
	contextMenuEnabled = e;
}

void MapView::setMode( Mode m )
{
	mode = m;
}

void MapView::mouseClicked( ProjectedCoordinate clickPos )
{
	if ( gpsLookup == NULL )
		return;
	QVector< IGPSLookup::Result > result;
	gpsLookup->GetNearEdges( &result, UnsignedCoordinate( clickPos ), 100 );
	if ( result.size() == 0 )
		return;
	if ( mode == POI ) {
		QVector< UnsignedCoordinate > points;
		selected = result.first().nearestPoint;
		points.push_back( selected );
		ui->paintArea->setPOIs( points );
	}
	if ( mode == Source ) {
		setSource( result.first().nearestPoint, 0 );
	}
	if ( mode == Target ) {
		setTarget( result.first().nearestPoint );
	}
}

void MapView::nextPlace()
{
	place = ( place + 1 ) % places.size();
	ui->headerLabel->setText( QString( tr( "Choose City (%1/%2)" ) ).arg( place + 1 ).arg( places.size() ) );
	ui->paintArea->setCenter( places[place].ToProjectedCoordinate() );
}

void MapView::previousPlace()
{
	place = ( place + places.size() - 1 ) % places.size();
	ui->headerLabel->setText( QString( tr( "Choose City (%1/%2)" ) ).arg( place + 1 ).arg( places.size() ) );
	ui->paintArea->setCenter( places[place].ToProjectedCoordinate() );
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

	int value = window->exec();
	int id = -1;
	if ( value == Accepted )
		id = window->place;
	delete window;

	return id;
}

void MapView::setPlaces( QVector< UnsignedCoordinate > p )
{
	places = p;
	place = 0;

	ui->headerLabel->setText( QString( tr( "Choose City (%1/%2)" ) ).arg( 1 ).arg( p.size() ) );
	ui->headerWidget->show();

	ui->paintArea->setCenter( places.first().ToProjectedCoordinate() );
	ui->paintArea->setPOIs( p );
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

	int value = window->exec();

	if ( value == Accepted )
		*result = window->selected;
	delete window;

	return value == Accepted;
}

void MapView::setEdges( QVector< int > segmentLength, QVector< UnsignedCoordinate > coordinates )
{
	ui->headerLabel->setText( tr( "Choose Coordinate" ) );
	ui->headerWidget->show();
	ui->nextButton->hide();
	ui->previousButton->hide();

	ui->paintArea->setCenter( coordinates.first().ToProjectedCoordinate() );
	ui->paintArea->setEdges( segmentLength, coordinates );
}

void MapView::showContextMenu( QPoint globalPos )
{
	if ( !contextMenuEnabled )
		return;
	gotoSourceAction->setEnabled( source.x != 0 || source.y != 0 );
	gotoTargetAction->setEnabled( target.x != 0 || target.y != 0 );
	gotoAddressAction->setEnabled( addressLookup != NULL );

	contextMenu->exec( globalPos );
	QAction* action = modeGroup->checkedAction();
	if ( action == modeSourceAction )
		mode = Source;
	if ( action == modeTargetAction )
		mode = Target;
	if ( action == modePOIAction )
		mode = POI;
}

void MapView::gotoSource()
{
	ui->paintArea->setCenter( source.ToProjectedCoordinate() );
}

void MapView::gotoGPS()
{
	bool ok = false;
	double latitude = QInputDialog::getDouble( this, "Enter Coordinate", "Enter Latitude", 0, -90, 90, 5, &ok );
	if ( !ok )
		return;
	double longitude = QInputDialog::getDouble( this, "Enter Coordinate", "Enter Latitude", 0, -180, 180, 5, &ok );
	if ( !ok )
		return;
	GPSCoordinate gps( latitude, longitude );
	ui->paintArea->setCenter( ProjectedCoordinate( gps ) );
}

void MapView::gotoTarget()
{
	ui->paintArea->setCenter( target.ToProjectedCoordinate() );
}

void MapView::gotoAddress()
{
	if ( addressLookup == NULL )
		return;
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, addressLookup, renderer, gpsLookup, this, true ) )
		return;
	ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}
