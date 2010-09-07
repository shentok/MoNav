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
#include "addressdialog.h"
#include "bookmarksdialog.h"
#include <QtDebug>
#include <QInputDialog>
#include <QSettings>

MapView::MapView( QWidget *parent ) :
	 QDialog(parent, Qt::Window ),
	 m_ui(new Ui::MapView)
{
	m_renderer = NULL;
	m_addressLookup = NULL;
	m_menu = NoMenu;
	m_mode = POI;
	m_heading = 0;
	m_fixed = false;
	m_toMapview = false;

	m_ui->setupUi(this);
	setupMenu();
	m_ui->headerWidget->hide();
	m_ui->menuButton->hide();
	m_ui->infoWidget->hide();

	connectSlots();

	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapView" );
	m_virtualZoom = settings.value( "virtualZoom", 1 ).toInt();
}

MapView::~MapView()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapView" );
	settings.setValue( "virtualZoom", m_virtualZoom );
	delete m_ui;
}

void MapView::connectSlots()
{
	connect( m_ui->zoomBar, SIGNAL(valueChanged(int)), m_ui->paintArea, SLOT(setZoom(int)) );
	connect( m_ui->paintArea, SIGNAL(zoomChanged(int)), m_ui->zoomBar, SLOT(setValue(int)) );
	connect( m_ui->paintArea, SIGNAL(mouseClicked(ProjectedCoordinate)), this, SLOT(mouseClicked(ProjectedCoordinate)) );
	connect( m_ui->previousButton, SIGNAL(clicked()), this, SLOT(previousPlace()) );
	connect( m_ui->nextButton, SIGNAL(clicked()), this, SLOT(nextPlace()) );
	connect( m_ui->paintArea, SIGNAL(contextMenu(QPoint)), this, SLOT(showContextMenu(QPoint)) );
	connect( m_ui->menuButton, SIGNAL(clicked()), this, SLOT(showContextMenu()) );
	connect( m_ui->zoomIn, SIGNAL(clicked()), this, SLOT(addZoom()) );
	connect( m_ui->zoomOut, SIGNAL(clicked()), this, SLOT(substractZoom()) );
}

void MapView::setupMenu()
{
	m_contextMenu = new QMenu( this );
	m_gotoSourceAction = m_contextMenu->addAction( tr( "Goto Source" ), this, SLOT(gotoSource()) );
	m_gotoGPSAction = m_contextMenu->addAction( tr( "Goto GPS" ), this, SLOT(gotoGPS()) );
	m_gotoTargetAction = m_contextMenu->addAction( tr( "Goto Target" ), this, SLOT(gotoTarget()) );
	m_gotoAddressAction = m_contextMenu->addAction( tr( "Goto Address" ), this, SLOT(gotoAddress()) );
	m_contextMenu->addSeparator();
	m_bookmarkAction = m_contextMenu->addAction( tr( "Bookmarks" ), this, SLOT(bookmarks()) );
	m_contextMenu->addSeparator();
	m_magnifyAction = m_contextMenu->addAction( tr( "Magnify" ), this, SLOT(magnify()) );
	m_contextMenu->addSeparator();
	m_modeGroup = new QActionGroup( this );
	m_modeSourceAction = new QAction( tr( "Choose Source" ), m_modeGroup );
	m_modeSourceAction->setCheckable( true );
	m_modeTargetAction = new QAction( tr( "Choose Target" ), m_modeGroup );
	m_modeTargetAction->setCheckable( true );
	m_contextMenu->addActions( m_modeGroup->actions() );

	m_routeMenu = new QMenu( this );
	m_routeMenu->insertAction( NULL, m_magnifyAction );
	m_routeMenu->addAction( tr( "Goto Mapview" ), this, SLOT(gotoMapview()) );
}

bool MapView::exitedToMapview()
{
	return m_toMapview;
}

void MapView::setRender( IRenderer* r )
{
	m_renderer = r;
	m_maxZoom = m_renderer->GetMaxZoom();
	m_ui->zoomBar->setMaximum( m_maxZoom );
	m_ui->zoomBar->setValue( m_maxZoom );
	m_ui->paintArea->setRenderer( r );
	m_ui->paintArea->setZoom( m_maxZoom );
	m_ui->paintArea->setMaxZoom( m_maxZoom );
	m_ui->paintArea->setVirtualZoom( m_virtualZoom );
}

void MapView::setCenter( ProjectedCoordinate center )
{
	m_ui->paintArea->setCenter( center );
}

void MapView::setAddressLookup( IAddressLookup* al )
{
	m_addressLookup = al;
}

void MapView::setSource( UnsignedCoordinate s, double h )
{
	if ( m_source.x == s.x && m_source.y == s.y && h == m_heading )
		return;

	emit sourceChanged( s, m_heading );
	m_source = s;
	m_heading = h;
	m_ui->paintArea->setPosition( m_source, m_heading );
}

void MapView::setTarget( UnsignedCoordinate t )
{
	if ( m_target.x == t.x && m_target.y == t.y )
		return;

	emit targetChanged( t );
	m_target = t;
	m_ui->paintArea->setTarget( m_target );
}

void MapView::setMenu( Menu m )
{
	m_menu = m;
	m_ui->menuButton->setVisible( m_menu != NoMenu );
}

void MapView::setMode( Mode m )
{
	m_mode = m;
	m_modeSourceAction->setChecked( m_mode == Source );
	m_modeTargetAction->setChecked( m_mode == Target );
}

void MapView::setFixed( bool fixed )
{
	m_ui->paintArea->setFixed( fixed );
}

void MapView::setRoute( QVector< UnsignedCoordinate > path )
{
	m_ui->paintArea->setRoute( path );
}

void MapView::mouseClicked( ProjectedCoordinate clickPos )
{
	UnsignedCoordinate coordinate( clickPos );
	if ( m_mode == Source ) {
		emit sourceChanged( coordinate, 0 );
		return;
	}
	if ( m_mode == Target ) {
		emit targetChanged( coordinate );
		return;
	}

	m_selected = coordinate;
	m_ui->paintArea->setPOI( coordinate );
}

void MapView::nextPlace()
{
	m_place = ( m_place + 1 ) % m_places.size();
	m_ui->headerLabel->setText( QString( tr( "Choose City (%1/%2)" ) ).arg( m_place + 1 ).arg( m_places.size() ) );
	m_ui->paintArea->setCenter( m_places[m_place].ToProjectedCoordinate() );
}

void MapView::previousPlace()
{
	m_place = ( m_place + m_places.size() - 1 ) % m_places.size();
	m_ui->headerLabel->setText( QString( tr( "Choose City (%1/%2)" ) ).arg( m_place + 1 ).arg( m_places.size() ) );
	m_ui->paintArea->setCenter( m_places[m_place].ToProjectedCoordinate() );
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
		id = window->m_place;
	delete window;

	return id;
}

void MapView::setPlaces( QVector< UnsignedCoordinate > p )
{
	m_places = p;
	m_place = 0;

	m_ui->headerLabel->setText( QString( tr( "Choose City (%1/%2)" ) ).arg( 1 ).arg( p.size() ) );
	m_ui->headerWidget->show();

	m_ui->paintArea->setCenter( m_places.first().ToProjectedCoordinate() );
	m_ui->paintArea->setPOIs( p );
}

bool MapView::selectStreet( UnsignedCoordinate* result, QVector< int >segmentLength, QVector< UnsignedCoordinate > coordinates, IRenderer* renderer, QWidget* p )
{
	if ( result == NULL )
		return false;
	if ( segmentLength.size() == 0 )
		return false;
	if ( coordinates.size() == 0 )
		return false;
	if ( renderer == 0 )
		return false;

	MapView* window = new MapView( p );
	window->setRender( renderer );
	window->setEdges( segmentLength, coordinates );

	int value = window->exec();

	if ( value == Accepted )
		*result = window->m_selected;
	delete window;

	return value == Accepted;
}

void MapView::setEdges( QVector< int > segmentLength, QVector< UnsignedCoordinate > coordinates )
{
	m_ui->headerLabel->setText( tr( "Choose Coordinate" ) );
	m_ui->headerWidget->show();
	m_ui->nextButton->hide();
	m_ui->previousButton->hide();

	ProjectedCoordinate center;
	foreach( UnsignedCoordinate coordinate, coordinates ) {
		ProjectedCoordinate pos = coordinate.ToProjectedCoordinate();
		center.x += pos.x;
		center.y += pos.y;
	}
	center.x /= coordinates.size();
	center.y /= coordinates.size();

	m_ui->paintArea->setCenter( center );
	m_ui->paintArea->setEdges( segmentLength, coordinates );

	UnsignedCoordinate centerPos( center );
	m_selected = centerPos;
	m_ui->paintArea->setPOI( centerPos );
}

void MapView::showContextMenu()
{
	showContextMenu( this->mapToGlobal( m_ui->menuButton->pos() ) );
}

void MapView::showContextMenu( QPoint globalPos )
{
	if ( m_menu == NoMenu )
		return;
	if ( m_menu == ContextMenu ) {
		m_gotoSourceAction->setEnabled( m_source.x != 0 || m_source.y != 0 );
		m_gotoTargetAction->setEnabled( m_target.x != 0 || m_target.y != 0 );
		m_gotoAddressAction->setEnabled( m_addressLookup != NULL );

		m_contextMenu->exec( globalPos );
		QAction* action = m_modeGroup->checkedAction();
		if ( action == m_modeSourceAction )
			m_mode = Source;
		if ( action == m_modeTargetAction )
			m_mode = Target;
		return;
	}
	if ( m_menu == RouteMenu ) {
		m_routeMenu->exec( globalPos );
	}
}

void MapView::gotoSource()
{
	m_ui->paintArea->setCenter( m_source.ToProjectedCoordinate() );
	m_ui->paintArea->setZoom( m_maxZoom );
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
	m_ui->paintArea->setCenter( ProjectedCoordinate( gps ) );
	m_ui->paintArea->setZoom( m_maxZoom );
}

void MapView::gotoTarget()
{
	m_ui->paintArea->setCenter( m_target.ToProjectedCoordinate() );
	m_ui->paintArea->setZoom( m_maxZoom );
}

void MapView::gotoAddress()
{
	if ( m_addressLookup == NULL )
		return;
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, m_addressLookup, m_renderer, this, true ) )
		return;
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
	m_ui->paintArea->setZoom( m_maxZoom );
}

void MapView::bookmarks()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this, m_source, m_target ) )
		return;

	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MapView::addZoom()
{
	m_ui->zoomBar->triggerAction( QAbstractSlider::SliderSingleStepAdd );
}

void MapView::substractZoom()
{
	m_ui->zoomBar->triggerAction( QAbstractSlider::SliderSingleStepSub );
}

void MapView::magnify()
{
	bool ok = false;
	int result = QInputDialog::getInt( this, "Magnification", "Enter Factor", m_virtualZoom, 1, 10, 1, &ok );
	if ( !ok )
		return;
	m_virtualZoom = result;
	m_ui->paintArea->setVirtualZoom( m_virtualZoom );
}

void MapView::gotoMapview()
{
	m_toMapview = true;
	accept();
}

