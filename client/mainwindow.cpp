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
#include "mapview.h"
#include "bookmarksdialog.h"
#include "addressdialog.h"
#include "routedescriptiondialog.h"
#include "utils/qthelpers.h"
#include "mapdata.h"
#include "mapdatawidget.h"
#include "routinglogic.h"

#include <QSettings>
#include <QFileDialog>
#include <QtDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
		QMainWindow(parent),
		m_ui(new Ui::MainWindow)
{
	m_ui->setupUi(this);
#ifdef Q_WS_MAEMO_5
	setAttribute( Qt::WA_Maemo5StackedWindow );
#endif

	m_ui->targetSourceWidget->hide();
	m_ui->settingsWidget->hide();

	QSettings settings( "MoNavClient" );
	QString dataDirectory = settings.value( "dataDirectory", QDir::homePath() ).toString();
	MapData::instance()->setPath( dataDirectory );
	UnsignedCoordinate source;
	source.x = settings.value( "source.x", 0 ).toUInt();
	source.y = settings.value( "source.y", 0 ).toUInt();
	RoutingLogic::instance()->setSource( source );
	RoutingLogic::instance()->setGPSLink( true );
	if ( settings.contains( "menu.geometry") )
		setGeometry( settings.value( "menu.geometry" ).toRect() );
	mode = Source;

	connectSlots();

	if ( MapData::instance()->path().isEmpty() || !loadPlugins() )
		settingsDataDirectory();
}

MainWindow::~MainWindow()
{
	MapData::instance()->unload();

	QSettings settings( "MoNavClient" );
	settings.setValue( "dataDirectory", MapData::instance()->path() );
	UnsignedCoordinate source = RoutingLogic::instance()->source();
	settings.setValue( "source.x", source.x );
	settings.setValue( "source.y", source.y );
	settings.setValue( "menu.geometry", geometry() );

	delete m_ui;
}

void MainWindow::connectSlots()
{
	connect( m_ui->sourceButton, SIGNAL(clicked()), this, SLOT(sourceMode()) );
	connect( m_ui->targetButton, SIGNAL(clicked()), this, SLOT(targetMode()) );
	connect( m_ui->mapButton, SIGNAL(clicked()), this, SLOT(browseMap()) );
	connect( m_ui->settingsButton, SIGNAL(clicked()), this, SLOT(settingsMenu()) );

	connect( m_ui->addressButton, SIGNAL(clicked()), this, SLOT(targetAddress()) );
	connect( m_ui->bookmarkButton, SIGNAL(clicked()), this, SLOT(targetBookmarks()) );
	connect( m_ui->gpsButton, SIGNAL(clicked()), this, SLOT(targetGPS()) );

	connect( m_ui->settingsAddressLookupButton, SIGNAL(clicked()), this, SLOT(settingsAddressLookup()) );
	connect( m_ui->settingsDataButton, SIGNAL(clicked()), this, SLOT(settingsDataDirectory()) );
	connect( m_ui->settingsGPSButton, SIGNAL(clicked()), this, SLOT(settingsGPS()) );
	connect( m_ui->settingsGPSLookupButton, SIGNAL(clicked()), this, SLOT(settingsGPSLookup()) );
	connect( m_ui->settingsMapButton, SIGNAL(clicked()), this, SLOT(settingsRenderer()) );
	connect( m_ui->settingsSystemButton, SIGNAL(clicked()), this, SLOT(settingsSystem()) );
}

bool MainWindow::loadPlugins()
{
	MapData* mapData = MapData::instance();
	if ( !mapData->containsMapData() ) {
		qDebug() << "directory does not contain map data";
		return false;
	}
	if ( !mapData->loadInformation() ) {
		qDebug() << "could not load map data information";
		return false;
	}
	if ( !mapData->canBeLoaded() ) {
		qDebug() << "map data not compatible";
		return false;
	}
	if ( !mapData->load() ) {
		qDebug() << "could not load map data";
		return false;
	}

	return true;
}

void MainWindow::routeDescription()
{
	RouteDescriptionDialog* window = new RouteDescriptionDialog();
	window->exec();
	delete window;
}

void MainWindow::browseMap()
{
	MapView* window = new MapView( this );

	connect( window, SIGNAL(infoClicked()), this, SLOT(routeDescription()) );

	window->exec();

	delete window;
}

void MainWindow::sourceMode()
{
	mode = Source;
	m_ui->mainMenuWidget->hide();
	m_ui->targetSourceWidget->show();
	m_ui->targetSourceMenuLabel->setText( tr( "Source Menu" ) );
}

void MainWindow::targetMode()
{
	mode = Target;
	m_ui->mainMenuWidget->hide();
	m_ui->targetSourceWidget->show();
	m_ui->targetSourceMenuLabel->setText( tr( "Target Menu" ) );
}

void MainWindow::settingsMenu()
{
	m_ui->settingsWidget->show();
	m_ui->mainMenuWidget->hide();
}

void MainWindow::targetBookmarks()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this ) )
		return;

	if ( mode == Source )
		RoutingLogic::instance()->setSource( result );
	else if ( mode == Target )
		RoutingLogic::instance()->setTarget( result );

	m_ui->backButton->click();
}

void MainWindow::targetAddress()
{
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, this ) )
		return;

	if ( mode == Source )
		RoutingLogic::instance()->setSource( result );
	else if ( mode == Target )
		RoutingLogic::instance()->setTarget( result );

	m_ui->backButton->click();
}

void MainWindow::targetGPS()
{
	RoutingLogic::instance()->setGPSLink( true );
	QMessageBox::information( this, "GPS", "Activated GPS source" );
}


void MainWindow::settingsSystem()
{

}

void MainWindow::settingsRenderer()
{
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer != NULL )
		renderer->ShowSettings();
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

}

void MainWindow::settingsDataDirectory()
{
	MapDataWidget* window = new MapDataWidget( this );
	connect( window, SIGNAL(loaded()), m_ui->backButton, SLOT(click()) );
	connect( window, SIGNAL(loaded()), window, SLOT(close()) );
	while ( true ) {
		window->exec();
		if ( !MapData::instance()->loaded() ) {
			QMessageBox::information( this, "Map Data", "No Data Directory Specified" );
			close();
			return;
		}
		break;
	}
	delete window;
}

