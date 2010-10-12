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
#include "mapdatawidget.h"
#include "mapdata.h"
#include "routinglogic.h"
#include "routedescriptiondialog.h"

#include <QtDebug>
#include <QInputDialog>
#include <QSettings>
#include <QGridLayout>
#include <QResizeEvent>
#include <QMessageBox>

#ifdef Q_WS_MAEMO_5
#include "fullscreenexitbutton.h"
#include <QtGui/QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

MapView::MapView( QWidget *parent ) :
		QDialog(parent, Qt::Window ),
		m_ui(new Ui::MapView)
{
	m_ui->setupUi(this);
#ifdef Q_WS_MAEMO_5
	setAttribute( Qt::WA_Maemo5StackedWindow );
	grabZoomKeys( true );
	new FullScreenExitButton(this);
	showFullScreen();
#endif

	this->setWindowIcon( QIcon( ":/images/source.png" ) );

	m_ui->zoomBar->hide();

	// ensure that we're painting our background
	setAutoFillBackground(true);

	m_mode = NoSelection;
	m_fixed = false;

	setupMenu();
	m_ui->headerWidget->hide();
	m_ui->infoWidget->hide();

	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapView" );
	m_virtualZoom = settings.value( "virtualZoom", 1 ).toInt();
	if ( settings.contains( "geometry") )
		setGeometry( settings.value( "geometry" ).toRect() );
	m_useMenus = settings.value( "useMenus", false ).toBool();

	connectSlots();

	if ( MapData::instance()->loaded() )
		dataLoaded();
	else
		settingsDataDirectory();
}

MapView::~MapView()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapView" );
	settings.setValue( "virtualZoom", m_virtualZoom );
	settings.setValue( "geometry", geometry() );

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
	connect( m_ui->zoomIn, SIGNAL(clicked()), this, SLOT(addZoom()) );
	connect( m_ui->zoomOut, SIGNAL(clicked()), this, SLOT(substractZoom()) );

	connect( m_ui->infoIcon1, SIGNAL(clicked()), this, SLOT(showInstructions()) );
	connect( m_ui->infoIcon2, SIGNAL(clicked()), this, SLOT(showInstructions()) );

	connect( MapData::instance(), SIGNAL(dataLoaded()), this, SLOT(dataLoaded()) );
	connect( RoutingLogic::instance(), SIGNAL(instructionsChanged()), this, SLOT(instructionsChanged()) );
	connect( m_ui->lockButton, SIGNAL(clicked()), this, SLOT(toogleLocked()) );

	connect( m_ui->show, SIGNAL(clicked()), this, SLOT(gotoMenu()) );
	connect( m_ui->tools, SIGNAL(clicked()), this, SLOT(toolsMenu()) );
	connect( m_ui->settings, SIGNAL(clicked()), this, SLOT(settingsMenu()) );
	connect( m_ui->source, SIGNAL(clicked()), this, SLOT(sourceMenu()) );
	connect( m_ui->target, SIGNAL(clicked()), this, SLOT(targetMenu()) );
}

void MapView::setupMenu()
{
	m_gotoMenu = new QMenu( tr( "Show" ), this );
	m_gotoGPSAction = m_gotoMenu->addAction( QIcon( ":/images/oxygen/network-wireless.png" ), tr( "GPS-Location" ), this, SLOT(gotoGPS()) );
	m_gotoSourceAction = m_gotoMenu->addAction( QIcon( ":/images/source.png" ), tr( "Departure" ), this, SLOT(gotoSource()) );
	m_gotoTargetAction = m_gotoMenu->addAction( QIcon( ":/images/target.png" ), tr( "Destination" ), this, SLOT(gotoTarget()) );
	m_gotoBookmarkAction = m_gotoMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmark" ), this, SLOT(gotoBookmark()) );
	m_gotoAddressAction = m_gotoMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address" ), this, SLOT(gotoAddress()) );

	m_gotoOverlay = new OverlayWidget( this, tr( "Show" ) );
	m_gotoOverlay->addActions( m_gotoMenu->actions() );

	m_sourceMenu = new QMenu( tr( "Departure" ), this );
	m_sourceByTapAction = m_sourceMenu->addAction( QIcon( ":/images/map.png" ), tr( "Tap on Map" ), this, SLOT(setModeSourceSelection()) );
	m_sourceByBookmarkAction = m_sourceMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmark" ), this, SLOT(sourceByBookmark()) );
	m_sourceByAddressAction = m_sourceMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address" ), this, SLOT(sourceByAddress()) );
	m_sourceByAddressAction = m_sourceMenu->addAction( QIcon( ":/images/oxygen/network-wireless.png" ), tr( "GPS Reciever" ), this, SLOT(sourceByGPS()) );

	m_sourceOverlay = new OverlayWidget( this, tr( "Departure" ) );
	m_sourceOverlay->addActions( m_sourceMenu->actions() );

	m_targetMenu = new QMenu( tr( "Destination" ), this );
	m_targetByTapAction = m_targetMenu->addAction( QIcon( ":/images/map.png" ), tr( "Tap on Map" ), this, SLOT(setModeTargetSelection()) );
	m_targetByBookmarkAction = m_targetMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmark" ), this, SLOT(targetByBookmark()) );
	m_targetByAddressAction = m_targetMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address" ), this, SLOT(targetByAddress()) );

	m_targetOverlay = new OverlayWidget( this, tr( "Destination" ) );
	m_targetOverlay->addActions( m_targetMenu->actions() );

	m_toolsMenu = new QMenu( tr( "Tools" ), this );
	m_bookmarkAction = m_toolsMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmarks" ), this, SLOT(bookmarks()) );
	m_magnifyAction = m_toolsMenu->addAction( QIcon( ":/images/oxygen/zoom-in.png" ), tr( "Magnify" ), this, SLOT(magnify()) );

	m_toolsOverlay = new OverlayWidget( this, tr( "Tools" ) );
	m_toolsOverlay->addActions( m_toolsMenu->actions() );

	m_settingsMenu = new QMenu( tr( "Settings" ), this );
	m_settingsDataDirectory = m_settingsMenu->addAction( QIcon( ":/images/oxygen/folder-tar.png" ), tr( "Map Data" ), this, SLOT(settingsDataDirectory()) );
	m_settingsGeneral = m_settingsMenu->addAction( QIcon( ":/images/oxygen/preferences-system.png" ), tr( "General" ), this, SLOT(settingsGeneral()) );
	m_settingsGPSLookup = m_settingsMenu->addAction( QIcon( ":/images/oxygen/network-wireless.png" ), tr( "GPS Lookup" ), this, SLOT(settingsGPSLookup()) );
	m_settingsRenderer = m_settingsMenu->addAction( QIcon( ":/images/map.png" ), tr( "Renderer" ), this, SLOT(settingsRenderer()) );
	m_settingsRouter = m_settingsMenu->addAction( QIcon( ":/images/route.png" ), tr( "Router" ), this, SLOT(settingsRouter()) );
	m_settingsAddressLookup = m_settingsMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address Lookup" ), this, SLOT(settingsAddressLookup()) );
	m_settingsGPS = m_settingsMenu->addAction( QIcon( ":/images/oxygen/hwinfo.png" ), tr( "GPS Reciever" ), this, SLOT(settingsGPS()) );

	m_settingsOverlay = new OverlayWidget( this, tr( "Settings" ) );
	m_settingsOverlay->addActions( m_settingsMenu->actions() );
}

void MapView::showInstructions()
{
	RouteDescriptionDialog* window = new RouteDescriptionDialog( this );
	window->exec();
	delete window;
}

void MapView::settingsGeneral()
{

}

void MapView::settingsRenderer()
{
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer != NULL )
		renderer->ShowSettings();
}

void MapView::settingsRouter()
{
	IRouter* router = MapData::instance()->router();
	if ( router != NULL )
		router->ShowSettings();
}

void MapView::settingsGPSLookup()
{
	IGPSLookup* gpsLookup = MapData::instance()->gpsLookup();
	if( gpsLookup != NULL )
		gpsLookup->ShowSettings();
}

void MapView::settingsAddressLookup()
{
	IAddressLookup* addressLookup = MapData::instance()->addressLookup();
	if ( addressLookup != NULL )
		addressLookup->ShowSettings();
}

void MapView::settingsGPS()
{

}

void MapView::settingsDataDirectory()
{
	MapDataWidget* window = new MapDataWidget( this );
	window->exec( true );
	if ( !MapData::instance()->loaded() ) {
		QMessageBox::information( this, "Map Data", "No Data Directory Specified" );
		QApplication::closeAllWindows();
	}
	delete window;
}

void MapView::gotoMenu()
{
	if ( m_useMenus ) {
		QPoint position = m_ui->show->mapToGlobal( QPoint( m_ui->show->width() / 2, m_ui->show->height() / 2 ) );
		m_gotoMenu->exec( position );
	} else {
		m_gotoOverlay->show();
	}
}

void MapView::toolsMenu()
{
	if ( m_useMenus ) {
		QPoint position = m_ui->tools->mapToGlobal( QPoint( m_ui->tools->width() / 2, m_ui->tools->height() / 2 ) );
		m_toolsMenu->exec( position );
	} else {
		m_toolsOverlay->show();
	}
}

void MapView::settingsMenu()
{
	if ( m_useMenus ) {
		QPoint position = m_ui->settings->mapToGlobal( QPoint( m_ui->settings->width() / 2, m_ui->settings->height() / 2 ) );
		m_settingsMenu->exec( position );
	} else {
		m_settingsOverlay->show();
	}
}

void MapView::sourceMenu()
{
	if ( m_useMenus ) {
		QPoint position = m_ui->source->mapToGlobal( QPoint( m_ui->source->width() / 2, m_ui->source->height() / 2 ) );
		m_sourceMenu->exec( position );
	} else {
		m_sourceOverlay->show();
	}
}
void MapView::targetMenu()
{
	if ( m_useMenus ) {
		QPoint position = m_ui->target->mapToGlobal( QPoint( m_ui->target->width() / 2, m_ui->target->height() / 2 ) );
		m_targetMenu->exec( position );
	} else {
		m_targetOverlay->show();
	}
}

void MapView::resizeEvent( QResizeEvent* event )
{
	QBoxLayout* box = qobject_cast< QBoxLayout* >( m_ui->infoWidget->layout() );
	assert ( box != NULL );
	if ( event->size().width() > event->size().height() )
		box->setDirection( QBoxLayout::LeftToRight );
	else
		box->setDirection( QBoxLayout::TopToBottom );
}

#ifdef Q_WS_MAEMO_5
void MapView::grabZoomKeys(bool grab)
{
	if ( !winId() ) {
		qWarning() << "Can't grab keys unless we have a window id";
		return;
	}

	unsigned long val = ( grab ) ? 1 : 0;
	Atom atom = XInternAtom(QX11Info::display(), "_HILDON_ZOOM_KEY_ATOM", False);
	if ( !atom ) {
		qWarning() << "Unable to obtain _HILDON_ZOOM_KEY_ATOM. This will only work on a Maemo 5 device!";
		return;
	}

	XChangeProperty ( QX11Info::display(),
							winId(),
							atom,
							XA_INTEGER,
							32,
							PropModeReplace,
							reinterpret_cast< unsigned char* >( &val ),
							1 );
}

void MapView::keyPressEvent( QKeyEvent* event )
{
	switch (event->key()) {
	case Qt::Key_F7:
		addZoom();
		event->accept();
		break;

	case Qt::Key_F8:
		this->substractZoom();
		event->accept();
		break;
	}
	QWidget::keyPressEvent(event);
}

#endif

void MapView::dataLoaded()
{
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;

	m_maxZoom = renderer->GetMaxZoom();
	m_ui->zoomBar->setMaximum( m_maxZoom );
	m_ui->zoomBar->setValue( m_maxZoom );
	m_ui->paintArea->setMaxZoom( m_maxZoom );
	m_ui->paintArea->setZoom( m_maxZoom );
	m_ui->paintArea->setVirtualZoom( m_virtualZoom );
	m_ui->paintArea->setCenter( RoutingLogic::instance()->source().ToProjectedCoordinate() );
}

void MapView::setModeSourceSelection()
{
	m_mode = Source;
}

void MapView::setModeTargetSelection()
{
	m_mode = Target;
}

void MapView::setModePOISelection()
{
	m_mode = POI;
}

void MapView::setModeNoSelection()
{
	m_mode = NoSelection;
}

void MapView::instructionsChanged()
{
	if ( !m_fixed )
		return;

	QStringList label;
	QStringList icon;

	RoutingLogic::instance()->instructions( &label, &icon, 60 );

	m_ui->infoWidget->setHidden( label.empty() );

	if ( label.isEmpty() )
		return;

	m_ui->infoLabel1->setText( label[0] );
	m_ui->infoIcon1->setIcon( QIcon( icon[0] ) );

	m_ui->infoIcon2->setHidden( label.size() == 1 );
	m_ui->infoLabel2->setHidden( label.size() == 1 );

	if ( label.size() == 1 )
		return;

	m_ui->infoLabel2->setText( label[1] );
	m_ui->infoIcon2->setIcon( QIcon( icon[1] ) );
}

void MapView::mouseClicked( ProjectedCoordinate clickPos )
{
	UnsignedCoordinate coordinate( clickPos );
	if ( m_mode == Source ) {
		RoutingLogic::instance()->setSource( coordinate );
	} else if ( m_mode == Target ) {
		RoutingLogic::instance()->setTarget( coordinate );
	} else if ( m_mode == POI ){
		m_selected = coordinate;
		m_ui->paintArea->setPOI( coordinate );
		return;
	}

	//m_mode = None; // might be contra-productiv for some use cases. E.g., many new users just want to click around the map and wonder about the blazingly fast routing *g*
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

int MapView::selectPlaces( QVector< UnsignedCoordinate > places, QWidget* p )
{
	if ( places.size() == 0 )
		return -1;

	MapView* window = new MapView( p );
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
	m_ui->lockWidget->hide();
	m_ui->waypointsWidget->hide();
	m_ui->menuWidget->hide();

	m_ui->paintArea->setCenter( m_places.first().ToProjectedCoordinate() );
	m_ui->paintArea->setPOIs( p );
}

bool MapView::selectStreet( UnsignedCoordinate* result, QVector< int >segmentLength, QVector< UnsignedCoordinate > coordinates, QWidget* p )
{
	if ( result == NULL )
		return false;
	if ( segmentLength.size() == 0 )
		return false;
	if ( coordinates.size() == 0 )
		return false;

	MapView* window = new MapView( p );
	window->setEdges( segmentLength, coordinates );
	window->setModePOISelection();

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
	m_ui->lockWidget->hide();
	m_ui->waypointsWidget->hide();
	m_ui->menuWidget->hide();

	ProjectedCoordinate center = coordinates[coordinates.size()/2].ToProjectedCoordinate();

	m_ui->paintArea->setCenter( center );
	m_ui->paintArea->setEdges( segmentLength, coordinates );

	UnsignedCoordinate centerPos( center );
	m_selected = centerPos;
	m_ui->paintArea->setPOI( centerPos );
}

void MapView::showContextMenu( QPoint /*globalPos*/ )
{
}

void MapView::gotoSource()
{
	UnsignedCoordinate coordinate = RoutingLogic::instance()->source();
	if ( !coordinate.IsValid() )
		return;
	m_ui->paintArea->setCenter( coordinate.ToProjectedCoordinate() );
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
	UnsignedCoordinate coordinate = RoutingLogic::instance()->target();
	if ( !coordinate.IsValid() )
		return;
	m_ui->paintArea->setCenter( coordinate.ToProjectedCoordinate() );
	m_ui->paintArea->setZoom( m_maxZoom );
}

void MapView::gotoBookmark()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this ) )
		return;
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MapView::gotoAddress()
{
	if ( MapData::instance()->addressLookup() == NULL )
		return;
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, this, true ) )
		return;
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MapView::sourceByBookmark()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this ) )
		return;
	RoutingLogic::instance()->setSource( result );
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MapView::sourceByAddress()
{
	if ( MapData::instance()->addressLookup() == NULL )
		return;
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, this ) )
		return;
	RoutingLogic::instance()->setSource( result );
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MapView::sourceByGPS()
{
	RoutingLogic::instance()->setGPSLink( true );
}

void MapView::targetByBookmark()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this ) )
		return;
	RoutingLogic::instance()->setTarget( result );
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MapView::targetByAddress()
{
	if ( MapData::instance()->addressLookup() == NULL )
		return;
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, this ) )
		return;
	RoutingLogic::instance()->setTarget( result );
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MapView::bookmarks()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this ) )
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

void MapView::toogleLocked()
{
	m_fixed = !m_fixed;
	m_ui->paintArea->setFixed( m_fixed );
	if ( m_fixed ) {
		m_ui->lockButton->setIcon( QIcon( ":images/oxygen/emblem-locked.png") );
		instructionsChanged();
	} else {
		m_ui->lockButton->setIcon( QIcon( ":images/oxygen/emblem-unlocked.png") );
		m_ui->infoWidget->hide();
	}
}

