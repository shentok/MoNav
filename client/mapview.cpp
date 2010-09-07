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
#include "bookmarksdialog.h"
#include <QSettings>
#include <QTime>

MapView::MapView( QWidget *parent ) :
	 QDialog(parent, Qt::Window ),
    ui(new Ui::MapView)
{
	renderer = NULL;
	gpsLookup = NULL;
	addressLookup = NULL;
	menu = NoMenu;
	mode = POI;
	heading = 0;
	fixed = false;
	toMapview = false;

	ui->setupUi(this);
	setupMenu();
	ui->headerWidget->hide();
	ui->menuButton->hide();
	ui->infoWidget->hide();

	connectSlots();

	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapView" );
	virtualZoom = settings.value( "virtualZoom", 1 ).toInt();
}

MapView::~MapView()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapView" );
	settings.setValue( "virtualZoom", virtualZoom );
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
	connect( ui->menuButton, SIGNAL(clicked()), this, SLOT(showContextMenu()) );
	connect( ui->zoomIn, SIGNAL(clicked()), this, SLOT(addZoom()) );
	connect( ui->zoomOut, SIGNAL(clicked()), this, SLOT(substractZoom()) );
}

void MapView::setupMenu()
{
	contextMenu = new QMenu( this );
	gotoSourceAction = contextMenu->addAction( tr( "Goto Source" ), this, SLOT(gotoSource()) );
	gotoGPSAction = contextMenu->addAction( tr( "Goto GPS" ), this, SLOT(gotoGPS()) );
	gotoTargetAction = contextMenu->addAction( tr( "Goto Target" ), this, SLOT(gotoTarget()) );
	gotoAddressAction = contextMenu->addAction( tr( "Goto Address" ), this, SLOT(gotoAddress()) );
	contextMenu->addSeparator();
	bookmarkAction = contextMenu->addAction( tr( "Bookmarks" ), this, SLOT(bookmarks()) );
	contextMenu->addSeparator();
	magnifyAction = contextMenu->addAction( tr( "Magnify" ), this, SLOT(magnify()) );
	contextMenu->addSeparator();
	modeGroup = new QActionGroup( this );
	modeSourceAction = new QAction( tr( "Choose Source" ), modeGroup );
	modeSourceAction->setCheckable( true );
	modeTargetAction = new QAction( tr( "Choose Target" ), modeGroup );
	modeTargetAction->setCheckable( true );
	contextMenu->addActions( modeGroup->actions() );

	routeMenu = new QMenu( this );
	routeMenu->insertAction( NULL, magnifyAction );
	routeMenu->addAction( tr( "Goto Mapview" ), this, SLOT(gotoMapview()) );
}

bool MapView::exitedToMapview()
{
	return toMapview;
}

void MapView::setRender( IRenderer* r )
{
	renderer = r;
	maxZoom = renderer->GetMaxZoom();
	ui->zoomBar->setMaximum( maxZoom );
	ui->zoomBar->setValue( maxZoom );
	ui->paintArea->setRenderer( r );
	ui->paintArea->setZoom( maxZoom );
	ui->paintArea->setMaxZoom( maxZoom );
	ui->paintArea->setVirtualZoom( virtualZoom );
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
	if ( source.x == s.x && source.y == s.y && h == heading )
		return;

	emit sourceChanged( s, heading );
	source = s;
	heading = h;
	ui->paintArea->setPosition( source, heading );
}

void MapView::setTarget( UnsignedCoordinate t )
{
	if ( target.x == t.x && target.y == t.y )
		return;

	emit targetChanged( t );
	target = t;
	ui->paintArea->setTarget( target );
}

void MapView::setMenu( Menu m )
{
	menu = m;
	ui->menuButton->setVisible( menu != NoMenu );
}

void MapView::setMode( Mode m )
{
	mode = m;
	modeSourceAction->setChecked( mode == Source );
	modeTargetAction->setChecked( mode == Target );
}

void MapView::setFixed( bool fixed )
{
	ui->paintArea->setFixed( fixed );
}

void MapView::setRoute( QVector< UnsignedCoordinate > path )
{
	ui->paintArea->setRoute( path );
}

void MapView::mouseClicked( ProjectedCoordinate clickPos )
{
	if ( mode == Source ) {
		emit sourceChanged( UnsignedCoordinate( clickPos ), 0 );
		return;
	}
	if ( mode == Target ) {
		emit targetChanged( UnsignedCoordinate( clickPos ) );
		return;
	}
	if ( gpsLookup == NULL )
		return;
	IGPSLookup::Result result;
	QTime time;
	time.start();
	bool found = gpsLookup->GetNearestEdge( &result, UnsignedCoordinate( clickPos ), 200 );
	qDebug() << "GPS Lookup:" << time.elapsed() << "ms";
	if ( !found )
		return;
	if ( mode == POI ) {
		QVector< UnsignedCoordinate > points;
		selected = result.nearestPoint;
		points.push_back( selected );
		ui->paintArea->setPOIs( points );
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

	ProjectedCoordinate center;
	foreach( UnsignedCoordinate coordinate, coordinates ) {
		ProjectedCoordinate pos = coordinate.ToProjectedCoordinate();
		center.x += pos.x;
		center.y += pos.y;
	}
	center.x /= coordinates.size();
	center.y /= coordinates.size();

	ui->paintArea->setCenter( center );
	ui->paintArea->setEdges( segmentLength, coordinates );
}

void MapView::showContextMenu()
{
	showContextMenu( this->mapToGlobal( ui->menuButton->pos() ) );
}

void MapView::showContextMenu( QPoint globalPos )
{
	if ( menu == NoMenu )
		return;
	if ( menu == ContextMenu ) {
		gotoSourceAction->setEnabled( source.x != 0 || source.y != 0 );
		gotoTargetAction->setEnabled( target.x != 0 || target.y != 0 );
		gotoAddressAction->setEnabled( addressLookup != NULL );

		contextMenu->exec( globalPos );
		QAction* action = modeGroup->checkedAction();
		if ( action == modeSourceAction )
			mode = Source;
		if ( action == modeTargetAction )
			mode = Target;
		return;
	}
	if ( menu == RouteMenu ) {
		routeMenu->exec( globalPos );
	}
}

void MapView::gotoSource()
{
	ui->paintArea->setCenter( source.ToProjectedCoordinate() );
	ui->paintArea->setZoom( maxZoom );
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
	ui->paintArea->setZoom( maxZoom );
}

void MapView::gotoTarget()
{
	ui->paintArea->setCenter( target.ToProjectedCoordinate() );
	ui->paintArea->setZoom( maxZoom );
}

void MapView::gotoAddress()
{
	if ( addressLookup == NULL )
		return;
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, addressLookup, renderer, gpsLookup, this, true ) )
		return;
	ui->paintArea->setCenter( result.ToProjectedCoordinate() );
	ui->paintArea->setZoom( maxZoom );
}

void MapView::bookmarks()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this, source, target ) )
		return;

	ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MapView::addZoom()
{
	ui->zoomBar->triggerAction( QAbstractSlider::SliderSingleStepAdd );
}

void MapView::substractZoom()
{
	ui->zoomBar->triggerAction( QAbstractSlider::SliderSingleStepSub );
}

void MapView::magnify()
{
	bool ok = false;
	int result = QInputDialog::getInt( this, "Magnification", "Enter Factor", virtualZoom, 1, 10, 1, &ok );
	if ( !ok )
		return;
	virtualZoom = result;
	ui->paintArea->setVirtualZoom( virtualZoom );
}

void MapView::gotoMapview()
{
	toMapview = true;
	accept();
}

