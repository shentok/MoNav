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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mapdata.h"
#include "mappackageswidget.h"
#include "mapmoduleswidget.h"
#include "overlaywidget.h"
#include "generalsettingsdialog.h"

#include "addressdialog.h"
#include "bookmarksdialog.h"

#include "routinglogic.h"
#include "routedescriptiondialog.h"
#include "gpsdialog.h"
#include "globalsettings.h"

#include <QtDebug>
#include <QInputDialog>
#include <QSettings>
#include <QGridLayout>
#include <QResizeEvent>
#include <QMessageBox>
#include <QStyle>
#include <QScrollArea>

#ifdef Q_WS_MAEMO_5
	#include "fullscreenexitbutton.h"
	#include <QtGui/QX11Info>
	#include <X11/Xlib.h>
	#include <X11/Xatom.h>
#endif

struct MainWindow::PrivateImplementation {

	enum Mode {
		Source, Target, POI, NoSelection
	};

	OverlayWidget* targetOverlay;
	OverlayWidget* sourceOverlay;
	OverlayWidget* gotoOverlay;
	OverlayWidget* toolsOverlay;
	OverlayWidget* settingsOverlay;

	QMenu* targetMenu;
	QMenu* sourceMenu;
	QMenu* gotoMenu;
	QMenu* toolsMenu;
	QMenu* settingsMenu;

	int maxZoom;
	bool fixed;
	Mode mode;

	void setupMenu();
	void resizeIcons();
};

MainWindow::MainWindow( QWidget* parent ) :
		QMainWindow( parent ),
		m_ui( new Ui::MainWindow )
{
	m_ui->setupUi( this );
	d = new PrivateImplementation;

	setupMenu();
	m_ui->zoomBar->hide();
	m_ui->headerWidget->hide();
	m_ui->infoWidget->hide();

	// ensure that we're painting our background
	setAutoFillBackground(true);

	d->mode = PrivateImplementation::NoSelection;
	d->fixed = false;

	QSettings settings( "MoNavClient" );
	setGeometry( settings.value( "geometry", geometry() ).toRect() );
	GlobalSettings::loadSettings( &settings );

	resizeIcons();
	connectSlots();

#ifdef Q_WS_MAEMO_5
	grabZoomKeys( true );
	new FullScreenExitButton(this);
	showFullScreen();
#endif

	MapData* mapData = MapData::instance();

	if ( mapData->loadInformation() )
		mapData->loadLast();

	if ( !mapData->informationLoaded() ) {
		displayMapChooser();
	} else if ( !mapData->loaded() ) {
		displayModuleChooser();
	}
}

MainWindow::~MainWindow()
{
	QSettings settings( "MoNavClient" );
	settings.setValue( "geometry", geometry() );
	GlobalSettings::saveSettings( &settings );

	delete d;
	delete m_ui;
}

void MainWindow::connectSlots()
{
	MapData* mapData = MapData::instance();
	connect( m_ui->zoomBar, SIGNAL(valueChanged(int)), m_ui->paintArea, SLOT(setZoom(int)) );
	connect( m_ui->paintArea, SIGNAL(zoomChanged(int)), m_ui->zoomBar, SLOT(setValue(int)) );
	connect( m_ui->paintArea, SIGNAL(mouseClicked(ProjectedCoordinate)), this, SLOT(mouseClicked(ProjectedCoordinate)) );
	connect( m_ui->zoomIn, SIGNAL(clicked()), this, SLOT(addZoom()) );
	connect( m_ui->zoomOut, SIGNAL(clicked()), this, SLOT(substractZoom()) );

	connect( m_ui->infoIcon1, SIGNAL(clicked()), this, SLOT(showInstructions()) );
	connect( m_ui->infoIcon2, SIGNAL(clicked()), this, SLOT(showInstructions()) );

	connect( mapData, SIGNAL(informationChanged()), this, SLOT(informationLoaded()) );
	connect( mapData, SIGNAL(dataLoaded()), this, SLOT(dataLoaded()) );
	connect( RoutingLogic::instance(), SIGNAL(instructionsChanged()), this, SLOT(instructionsChanged()) );
	connect( m_ui->lockButton, SIGNAL(clicked()), this, SLOT(toogleLocked()) );

	connect( m_ui->show, SIGNAL(clicked()), this, SLOT(gotoMenu()) );
	connect( m_ui->tools, SIGNAL(clicked()), this, SLOT(toolsMenu()) );
	connect( m_ui->settings, SIGNAL(clicked()), this, SLOT(settingsMenu()) );
	connect( m_ui->source, SIGNAL(clicked()), this, SLOT(sourceMenu()) );
	connect( m_ui->target, SIGNAL(clicked()), this, SLOT(targetMenu()) );
}

void MainWindow::setupMenu()
{
	d->gotoMenu = new QMenu( tr( "Show" ), this );
	d->gotoMenu->addAction( QIcon( ":/images/oxygen/network-wireless.png" ), tr( "GPS-Location" ), this, SLOT(gotoGPS()) );
	d->gotoMenu->addAction( QIcon( ":/images/source.png" ), tr( "Departure" ), this, SLOT(gotoSource()) );
	d->gotoMenu->addAction( QIcon( ":/images/target.png" ), tr( "Destination" ), this, SLOT(gotoTarget()) );
	d->gotoMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmark" ), this, SLOT(gotoBookmark()) );
	d->gotoMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address" ), this, SLOT(gotoAddress()) );

	d->gotoOverlay = new OverlayWidget( this, tr( "Show" ) );
	d->gotoOverlay->addActions( d->gotoMenu->actions() );

	d->sourceMenu = new QMenu( tr( "Departure" ), this );
	d->sourceMenu->addAction( QIcon( ":/images/map.png" ), tr( "Tap on Map" ), this, SLOT(setModeSourceSelection()) );
	d->sourceMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmark" ), this, SLOT(sourceByBookmark()) );
	d->sourceMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address" ), this, SLOT(sourceByAddress()) );
	d->sourceMenu->addAction( QIcon( ":/images/oxygen/network-wireless.png" ), tr( "GPS Receiver" ), this, SLOT(sourceByGPS()) );

	d->sourceOverlay = new OverlayWidget( this, tr( "Departure" ) );
	d->sourceOverlay->addActions( d->sourceMenu->actions() );

	d->targetMenu = new QMenu( tr( "Destination" ), this );
	d->targetMenu->addAction( QIcon( ":/images/map.png" ), tr( "Tap on Map" ), this, SLOT(setModeTargetSelection()) );
	d->targetMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmark" ), this, SLOT(targetByBookmark()) );
	d->targetMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address" ), this, SLOT(targetByAddress()) );

	d->targetOverlay = new OverlayWidget( this, tr( "Destination" ) );
	d->targetOverlay->addActions( d->targetMenu->actions() );

	d->toolsMenu = new QMenu( tr( "Tools" ), this );
	d->toolsMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmarks" ), this, SLOT(bookmarks()) );

	d->toolsOverlay = new OverlayWidget( this, tr( "Tools" ) );
	d->toolsOverlay->addActions( d->toolsMenu->actions() );

	d->settingsMenu = new QMenu( tr( "Settings" ), this );
	d->settingsMenu->addAction( QIcon( ":/images/oxygen/folder-tar.png" ), tr( "Map Packages" ), this, SLOT(displayMapChooser()) );
	d->settingsMenu->addAction( QIcon( ":/images/oxygen/folder-tar.png" ), tr( "Map Modules" ), this, SLOT(displayModuleChooser()) );
	d->settingsMenu->addAction( QIcon( ":/images/oxygen/preferences-system.png" ), tr( "General" ), this, SLOT(settingsGeneral()) );
	d->settingsMenu->addAction( QIcon( ":/images/oxygen/network-wireless.png" ), tr( "GPS Lookup" ), this, SLOT(settingsGPSLookup()) );
	d->settingsMenu->addAction( QIcon( ":/images/map.png" ), tr( "Renderer" ), this, SLOT(settingsRenderer()) );
	d->settingsMenu->addAction( QIcon( ":/images/route.png" ), tr( "Router" ), this, SLOT(settingsRouter()) );
	d->settingsMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address Lookup" ), this, SLOT(settingsAddressLookup()) );
	d->settingsMenu->addAction( QIcon( ":/images/oxygen/hwinfo.png" ), tr( "GPS Receiver" ), this, SLOT(settingsGPS()) );

	d->settingsOverlay = new OverlayWidget( this, tr( "Settings" ) );
	d->settingsOverlay->addActions( d->settingsMenu->actions() );
}

void MainWindow::resizeIcons()
{
	// a bit hackish right now
	// find all suitable children and resize their icons
	// TODO find cleaner way
	int iconSize = GlobalSettings::iconSize();
	foreach ( QToolButton* button, this->findChildren< QToolButton* >() )
		button->setIconSize( QSize( iconSize, iconSize ) );
	foreach ( QToolBar* button, this->findChildren< QToolBar* >() )
		button->setIconSize( QSize( iconSize, iconSize ) );
	m_ui->waypointsWidget->setMinimumSize( m_ui->waypointsWidget->widget()->sizeHint() );
}

void MainWindow::showInstructions()
{
	RouteDescriptionWidget* widget = new RouteDescriptionWidget( this );
	int index = m_ui->stacked->addWidget( widget );
	m_ui->stacked->setCurrentIndex( index );
	connect( widget, SIGNAL(closed()), widget, SLOT(deleteLater()) );
}

void MainWindow::displayMapChooser()
{
	MapPackagesWidget* widget = new MapPackagesWidget();
	int index = m_ui->stacked->addWidget( widget );
	m_ui->stacked->setCurrentIndex( index );
	widget->show();
	setWindowTitle( "MoNav - Map Packages" );
	connect( widget, SIGNAL(mapChanged()), this, SLOT(mapLoaded()) );
}

void MainWindow::displayModuleChooser()
{
	MapModulesWidget* widget = new MapModulesWidget();
	int index = m_ui->stacked->addWidget( widget );
	m_ui->stacked->setCurrentIndex( index );
	widget->show();
	setWindowTitle( "MoNav - Map Modules" );
	connect( widget, SIGNAL(selected()), this, SLOT(modulesLoaded()) );
	connect( widget, SIGNAL(cancelled()), this, SLOT(modulesCancelled()) );
}

void MainWindow::mapLoaded()
{
	MapData* mapData = MapData::instance();
	if ( !mapData->informationLoaded() )
		return;

	if ( m_ui->stacked->count() <= 1 )
		return;

	QWidget* widget = m_ui->stacked->currentWidget();
	widget->deleteLater();

	if ( !mapData->loadLast() )
		displayModuleChooser();
}

void MainWindow::modulesCancelled()
{
	if ( m_ui->stacked->count() <= 1 )
		return;

	QWidget* widget = m_ui->stacked->currentWidget();
	widget->deleteLater();

	MapData* mapData = MapData::instance();
	if ( !mapData->loaded() ) {
		if ( !mapData->loadLast() )
			displayMapChooser();
	}
}

void MainWindow::modulesLoaded()
{
	if ( m_ui->stacked->count() <= 1 )
		return;

	QWidget* widget = m_ui->stacked->currentWidget();
	widget->deleteLater();
}

void MainWindow::informationLoaded()
{
	MapData* mapData = MapData::instance();
	if ( !mapData->informationLoaded() )
		return;

	this->setWindowTitle( "MoNav - " + mapData->information().name );
}

void MainWindow::dataLoaded()
{
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;

	d->maxZoom = renderer->GetMaxZoom();
	m_ui->zoomBar->setMaximum( d->maxZoom );
	m_ui->zoomBar->setValue( d->maxZoom );
	m_ui->paintArea->setMaxZoom( d->maxZoom );
	m_ui->paintArea->setZoom( d->maxZoom );
	m_ui->paintArea->setVirtualZoom( GlobalSettings::magnification() );
	m_ui->paintArea->setCenter( RoutingLogic::instance()->source().ToProjectedCoordinate() );
}

void MainWindow::instructionsChanged()
{
	if ( !d->fixed )
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

// SETTINGS

void MainWindow::settingsGeneral()
{
	GeneralSettingsDialog* window = new GeneralSettingsDialog;
	window->exec();
	delete window;
	resizeIcons();
	m_ui->paintArea->setVirtualZoom( GlobalSettings::magnification() );
}

void MainWindow::settingsRenderer()
{
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer != NULL )
		renderer->ShowSettings();
}

void MainWindow::settingsRouter()
{
	IRouter* router = MapData::instance()->router();
	if ( router != NULL )
		router->ShowSettings();
}

void MainWindow::settingsGPSLookup()
{
	IGPSLookup* gpsLookup = MapData::instance()->gpsLookup();
	if( gpsLookup != NULL )
		gpsLookup->ShowSettings();
}

void MainWindow::settingsAddressLookup()
{
	IAddressLookup* addressLookup = MapData::instance()->addressLookup();
	if ( addressLookup != NULL )
		addressLookup->ShowSettings();
}

void MainWindow::settingsGPS()
{
	GPSDialog* window = new GPSDialog( this );
	window->exec();
	delete window;
}

// MENUES

void MainWindow::gotoMenu()
{
	if ( GlobalSettings::menuMode() == GlobalSettings::MenuPopup ) {
		QPoint position = m_ui->show->mapToGlobal( QPoint( m_ui->show->width() / 2, m_ui->show->height() / 2 ) );
		d->gotoMenu->exec( position );
	} else {
		d->gotoOverlay->show();
	}
}

void MainWindow::toolsMenu()
{
	if ( GlobalSettings::menuMode() == GlobalSettings::MenuPopup ) {
		QPoint position = m_ui->tools->mapToGlobal( QPoint( m_ui->tools->width() / 2, m_ui->tools->height() / 2 ) );
		d->toolsMenu->exec( position );
	} else {
		d->toolsOverlay->show();
	}
}

void MainWindow::settingsMenu()
{
	if ( GlobalSettings::menuMode() == GlobalSettings::MenuPopup ) {
		QPoint position = m_ui->settings->mapToGlobal( QPoint( m_ui->settings->width() / 2, m_ui->settings->height() / 2 ) );
		d->settingsMenu->exec( position );
	} else {
		d->settingsOverlay->show();
	}
}

void MainWindow::sourceMenu()
{
	if ( GlobalSettings::menuMode() == GlobalSettings::MenuPopup ) {
		QPoint position = m_ui->source->mapToGlobal( QPoint( m_ui->source->width() / 2, m_ui->source->height() / 2 ) );
		d->sourceMenu->exec( position );
	} else {
		d->sourceOverlay->show();
	}
}
void MainWindow::targetMenu()
{
	if ( GlobalSettings::menuMode() == GlobalSettings::MenuPopup ) {
		QPoint position = m_ui->target->mapToGlobal( QPoint( m_ui->target->width() / 2, m_ui->target->height() / 2 ) );
		d->targetMenu->exec( position );
	} else {
		d->targetOverlay->show();
	}
}

// EVENTS

void MainWindow::resizeEvent( QResizeEvent* event )
{
	QBoxLayout* box = qobject_cast< QBoxLayout* >( m_ui->infoWidget->layout() );
	assert ( box != NULL );
	if ( event->size().width() > event->size().height() )
		box->setDirection( QBoxLayout::LeftToRight );
	else
		box->setDirection( QBoxLayout::TopToBottom );
}

#ifdef Q_WS_MAEMO_5
void MainWindow::grabZoomKeys( bool grab )
{
	if ( !winId() ) {
		qWarning() << "Can't grab keys unless we have a window id";
		return;
	}

	unsigned long val = ( grab ) ? 1 : 0;
	Atom atom = XInternAtom( QX11Info::display(), "_HILDON_ZOOM_KEY_ATOM", False );
	if ( !atom ) {
		qWarning() << "Unable to obtain _HILDON_ZOOM_KEY_ATOM. This will only work on a Maemo 5 device!";
		return;
	}

	XChangeProperty ( QX11Info::display(), winId(), atom, XA_INTEGER, 32, PropModeReplace, reinterpret_cast< unsigned char* >( &val ), 1 );
}

void MainWindow::keyPressEvent( QKeyEvent* event )
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

// SLOTS

void MainWindow::setModeSourceSelection()
{
	d->mode = PrivateImplementation::Source;
}

void MainWindow::setModeTargetSelection()
{
	d->mode = PrivateImplementation::Target;
}

void MainWindow::setModeNoSelection()
{
	d->mode = PrivateImplementation::NoSelection;
}

void MainWindow::mouseClicked( ProjectedCoordinate clickPos )
{
	UnsignedCoordinate coordinate( clickPos );
	if ( d->mode == PrivateImplementation::Source ) {
		RoutingLogic::instance()->setSource( coordinate );
	} else if ( d->mode == PrivateImplementation::Target ) {
		RoutingLogic::instance()->setTarget( coordinate );
	} else if ( d->mode == PrivateImplementation::POI ){
		//m_selected = coordinate;
		//m_ui->paintArea->setPOI( coordinate );
		//return;
	}

	//m_mode = None; // might be contra-productiv for some use cases. E.g., many new users just want to click around the map and wonder about the blazingly fast routing *g*
}

void MainWindow::gotoSource()
{
	UnsignedCoordinate coordinate = RoutingLogic::instance()->source();
	if ( !coordinate.IsValid() )
		return;
	m_ui->paintArea->setCenter( coordinate.ToProjectedCoordinate() );
	m_ui->paintArea->setZoom( d->maxZoom );
}

void MainWindow::gotoGPS()
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
	m_ui->paintArea->setZoom( d->maxZoom );
}

void MainWindow::gotoTarget()
{
	UnsignedCoordinate coordinate = RoutingLogic::instance()->target();
	if ( !coordinate.IsValid() )
		return;
	m_ui->paintArea->setCenter( coordinate.ToProjectedCoordinate() );
	m_ui->paintArea->setZoom( d->maxZoom );
}

void MainWindow::gotoBookmark()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this ) )
		return;
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MainWindow::gotoAddress()
{
	if ( MapData::instance()->addressLookup() == NULL )
		return;
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, this, true ) )
		return;
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MainWindow::sourceByBookmark()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this ) )
		return;
	RoutingLogic::instance()->setSource( result );
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MainWindow::sourceByAddress()
{
	if ( MapData::instance()->addressLookup() == NULL )
		return;
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, this ) )
		return;
	RoutingLogic::instance()->setSource( result );
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MainWindow::sourceByGPS()
{
	RoutingLogic::instance()->setGPSLink( true );
}

void MainWindow::targetByBookmark()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this ) )
		return;
	RoutingLogic::instance()->setTarget( result );
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MainWindow::targetByAddress()
{
	if ( MapData::instance()->addressLookup() == NULL )
		return;
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, this ) )
		return;
	RoutingLogic::instance()->setTarget( result );
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MainWindow::bookmarks()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this ) )
		return;

	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MainWindow::addZoom()
{
	m_ui->zoomBar->triggerAction( QAbstractSlider::SliderSingleStepAdd );
}

void MainWindow::substractZoom()
{
	m_ui->zoomBar->triggerAction( QAbstractSlider::SliderSingleStepSub );
}

void MainWindow::toogleLocked()
{
	d->fixed = !d->fixed;
	m_ui->paintArea->setFixed( d->fixed );
	if ( d->fixed ) {
		m_ui->lockButton->setIcon( QIcon( ":images/oxygen/emblem-locked.png") );
		instructionsChanged();
	} else {
		m_ui->lockButton->setIcon( QIcon( ":images/oxygen/emblem-unlocked.png") );
		m_ui->infoWidget->hide();
	}
}
