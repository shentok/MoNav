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
#include "mapdata.h"
#include "routinglogic.h"

#include <QtDebug>
#include <QInputDialog>
#include <QSettings>
#include <QGridLayout>

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
	QGridLayout* grid = qobject_cast< QGridLayout* >( layout() );
	if ( grid != NULL ) {
		int infoWidgetIndex = grid->indexOf( m_ui->infoWidget );
		grid->takeAt( infoWidgetIndex );
		grid->addWidget( m_ui->infoWidget, 1, 0 );
		QBoxLayout* box = qobject_cast< QBoxLayout* >( m_ui->infoWidget->layout() );
		if ( box != NULL )
			box->setDirection( QBoxLayout::TopToBottom );
		m_ui->infoButton->setArrowType( Qt::DownArrow );
		m_ui->infoWidget->setMaximumWidth( 300 );
	}
	m_ui->zoomBar->hide();
	m_ui->zoomIn->hide();
	m_ui->zoomOut->hide();
	grabZoomKeys( true );
	new FullScreenExitButton(this);
	showFullScreen();
#endif

	m_ui->modeButton->hide();

	// ensure that we're painting our background
	setAutoFillBackground(true);

	m_menu = NoMenu;
	m_mode = NoSelection;
	m_fixed = false;

	setupMenu();
	m_ui->headerWidget->hide();
	m_ui->menuButton->hide();
	m_ui->infoWidget->hide();

	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapView" );
	m_virtualZoom = settings.value( "virtualZoom", 1 ).toInt();
	if ( settings.contains( "geometry") )
		setGeometry( settings.value( "geometry" ).toRect() );

	dataLoaded();
	instructionsChanged();

	connectSlots();
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
	connect( m_ui->menuButton, SIGNAL(clicked()), this, SLOT(showContextMenu()) );
	connect( m_ui->zoomIn, SIGNAL(clicked()), this, SLOT(addZoom()) );
	connect( m_ui->zoomOut, SIGNAL(clicked()), this, SLOT(substractZoom()) );
	connect( m_ui->infoButton, SIGNAL(clicked()), this, SIGNAL(infoClicked()) );
	connect( MapData::instance(), SIGNAL(dataLoaded()), this, SLOT(dataLoaded()) );
	connect( RoutingLogic::instance(), SIGNAL(instructionsChanged()), this, SLOT(instructionsChanged()) );
	connect( m_ui->modeButton, SIGNAL(clicked()), this, SLOT(setModeNoSelection()) );
	connect( m_ui->lockButton, SIGNAL(clicked()), this, SLOT(toogleLocked()) );
}

void MapView::setupMenu()
{
	m_contextMenu = new QMenu( this );

	QMenu* contextSubMenu = m_contextMenu->addMenu( QIcon( ":/images/map.png" ), tr("Show") );
	m_gotoGPSAction = contextSubMenu->addAction( QIcon( ":/images/oxygen/network-wireless.png" ), tr( "GPS-Location" ), this, SLOT(gotoGPS()) );
	m_gotoSourceAction = contextSubMenu->addAction( QIcon( ":/images/source.png" ), tr( "Departure" ), this, SLOT(gotoSource()) );
	m_gotoTargetAction = contextSubMenu->addAction( QIcon( ":/images/target.png" ), tr( "Destination" ), this, SLOT(gotoTarget()) );
	m_gotoBookmarkAction = contextSubMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmark..." ), this, SLOT(gotoBookmark()) );
	m_gotoAddressAction = contextSubMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address..." ), this, SLOT(gotoAddress()) );

	contextSubMenu = m_contextMenu->addMenu( QIcon( ":/images/source.png" ), tr( "Departure" ) );
	m_sourceByTapAction = contextSubMenu->addAction( QIcon( ":/images/map.png" ), tr( "Tap on Map" ), this, SLOT(setModeSourceSelection()) );
	m_sourceByBookmarkAction = contextSubMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmark..." ), this, SLOT(sourceByBookmark()) );
	m_sourceByAddressAction = contextSubMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address..." ), this, SLOT(sourceByAddress()) );

	contextSubMenu = m_contextMenu->addMenu( QIcon( ":/images/target.png" ), tr( "Destination" ) );
	m_targetByTapAction = contextSubMenu->addAction( QIcon( ":/images/map.png" ), tr( "Tap on Map" ), this, SLOT(setModeTargetSelection()) );
	m_targetByBookmarkAction = contextSubMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmark..." ), this, SLOT(targetByBookmark()) );
	m_targetByAddressAction = contextSubMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address..." ), this, SLOT(targetByAddress()) );

	contextSubMenu = m_contextMenu->addMenu( QIcon( ":/images/oxygen/configure.png" ), tr( "Tools" ) );
	m_bookmarkAction = contextSubMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmarks..." ), this, SLOT(bookmarks()) );
	m_magnifyAction = contextSubMenu->addAction( QIcon( ":/images/oxygen/zoom-in.png" ), tr( "Magnify..." ), this, SLOT(magnify()) );
	m_toggleInfoWidgetAction = contextSubMenu->addAction( QIcon( ":/images/directions/forward.png" ), tr( "Turn Directions" ), this, SLOT(toggleInfoWidget()) );
	m_toggleInfoWidgetAction->setCheckable( true );
	m_toggleInfoWidgetAction->setChecked( false );
	m_toggleInfoWidgetAction->setEnabled( false );
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

void MapView::setMenu( Menu m )
{
	m_menu = m;
	m_ui->menuButton->setVisible( m_menu != NoMenu );
}

void MapView::setModeSourceSelection()
{
	m_mode = Source;
	m_ui->modeButton->setIcon( QIcon( ":/images/source.png" ) );
	m_ui->modeButton->show();
}

void MapView::setModeTargetSelection()
{
	m_mode = Target;
	m_ui->modeButton->setIcon( QIcon( ":/images/target.png" ) );
	m_ui->modeButton->show();
}

void MapView::setModePOISelection()
{
	m_mode = POI;
	m_ui->modeButton->hide();
}

void MapView::setModeNoSelection()
{
	m_mode = NoSelection;
	m_ui->modeButton->hide();
}

void MapView::toggleInfoWidget()
{
	if ( m_ui->infoWidget->isVisible() )
		m_ui->infoWidget->hide();
	else
		m_ui->infoWidget->show();

	m_toggleInfoWidgetAction->setChecked( m_ui->infoWidget->isVisible() );
}

void MapView::instructionsChanged()
{
	QStringList label;
	QStringList icon;

	RoutingLogic::instance()->instructions( &label, &icon, 60 );

	m_toggleInfoWidgetAction->setEnabled( !label.isEmpty() );
	m_toggleInfoWidgetAction->setChecked( !label.isEmpty() );
	m_ui->infoWidget->setHidden( label.isEmpty() );

	if ( label.isEmpty() )
		return;

	m_ui->infoLabel1->setText( label[0] );
	m_ui->infoIcon1->setPixmap( QPixmap( icon[0] ) );

	m_ui->infoIcon2->setHidden( label.size() == 1 );
	m_ui->infoLabel2->setHidden( label.size() == 1 );

	if ( label.size() == 1 )
		return;

	m_ui->infoLabel2->setText( label[1] );
	m_ui->infoIcon2->setPixmap( QPixmap( icon[1] ) );
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
	m_ui->lockButton->hide();

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
	m_ui->lockButton->hide();

	ProjectedCoordinate center = coordinates[coordinates.size()/2].ToProjectedCoordinate();

	m_ui->paintArea->setCenter( center );
	m_ui->paintArea->setEdges( segmentLength, coordinates );

	UnsignedCoordinate centerPos( center );
	m_selected = centerPos;
	m_ui->paintArea->setPOI( centerPos );
}

void MapView::showContextMenu()
{
#ifdef Q_WS_MAEMO_5
	showContextMenu( mapToGlobal( QPoint( width() / 2, height() / 2 ) ) );
#else
	showContextMenu( mapToGlobal( m_ui->menuButton->pos() ) );
#endif
}

void MapView::showContextMenu( QPoint globalPos )
{
	if ( m_menu == NoMenu )
		return;
	if ( m_menu == ContextMenu ) {
		m_gotoSourceAction->setEnabled( RoutingLogic::instance()->source().IsValid() );
		m_gotoTargetAction->setEnabled( RoutingLogic::instance()->target().IsValid() );
		bool addressLookupAvailable = MapData::instance()->addressLookup() != NULL;
		m_gotoAddressAction->setEnabled( addressLookupAvailable );
		m_targetByAddressAction->setEnabled( addressLookupAvailable );
		m_sourceByAddressAction->setEnabled( addressLookupAvailable );

		m_contextMenu->exec( globalPos );
		return;
	}
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
	if ( m_fixed )
		m_ui->lockButton->setIcon( QIcon( ":images/oxygen/emblem-locked.png") );
	else
		m_ui->lockButton->setIcon( QIcon( ":images/oxygen/emblem-unlocked.png") );
}

