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
#include <QDir>
#include <QSettings>
#include <QFileDialog>
#include <QtDebug>
#include "mapview.h"
#include "bookmarksdialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	renderer = NULL;
	addressLookup = NULL;
	gpsLookup = NULL;
	router = NULL;

	heading = 0;

	sourceSet = false;
	targetSet = false;

	ui->setupUi(this);
	ui->targetSourceWidget->hide();
	ui->settingsWidget->hide();

	QSettings settings( "MoNavClient" );
	dataDirectory = settings.value( "dataDirectory" ).toString();
	mode = Source;

	connectSlots();

	settingsDataDirectory();
}

MainWindow::~MainWindow()
{
	unloadPlugins();
	QSettings settings( "MoNavClient" );
	settings.setValue( "dataDirectory", dataDirectory );
    delete ui;
}

void MainWindow::connectSlots()
{
	connect( ui->backButton, SIGNAL(clicked()), this, SLOT(back()) );

	connect( ui->mainMenuList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(menuClicked(QListWidgetItem*)) );
	connect( ui->targetMenuList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(menuClicked(QListWidgetItem*)) );
	connect( ui->settingsMenuList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(menuClicked(QListWidgetItem*)) );
}

bool MainWindow::loadPlugins()
{
	QDir pluginDir( QApplication::applicationDirPath() );
	if ( !pluginDir.cd( "plugins_client" ) )
		return false;

	QDir dir( dataDirectory );
	QSettings pluginSettings( dir.filePath( "plugins.ini" ), QSettings::IniFormat );
	QString rendererName = pluginSettings.value( "renderer" ).toString();
	QString routerName = pluginSettings.value( "router" ).toString();
	QString gpsLookupName = pluginSettings.value( "gpsLookup" ).toString();
	QString addressLookupName = pluginSettings.value( "addressLookup" ).toString();

	foreach ( QString fileName, pluginDir.entryList( QDir::Files ) ) {
		QPluginLoader* loader = new QPluginLoader( pluginDir.absoluteFilePath( fileName ) );
		if ( !loader->load() )
			qDebug( "%s", loader->errorString().toAscii().constData() );

		if ( IRenderer *interface = qobject_cast< IRenderer* >( loader->instance() ) )
		{
			if ( interface->GetName() == rendererName )
			{
				plugins.append( loader );
				renderer = interface;
			}
		}
		else if ( IAddressLookup *interface = qobject_cast< IAddressLookup* >( loader->instance() ) )
		{
			if ( interface->GetName() == addressLookupName )
			{
				plugins.append( loader );
				addressLookup = interface;
			}
		}
		else if ( IGPSLookup *interface = qobject_cast< IGPSLookup* >( loader->instance() ) )
		{
			if ( interface->GetName() == gpsLookupName )
			{
				plugins.append( loader );
				gpsLookup = interface;
			}
		}
		else if ( IRouter *interface = qobject_cast< IRouter* >( loader->instance() ) )
		{
			if ( interface->GetName() == routerName )
			{
				plugins.append( loader );
				router = interface;
			}
		}
	}

	try
	{
		if ( renderer == NULL )
			return false;
		renderer->SetInputDirectory( dataDirectory );
		if ( !renderer->LoadData() )
			return false;

		if ( addressLookup == NULL )
			return false;
		addressLookup->SetInputDirectory( dataDirectory );
		if ( !addressLookup->LoadData() )
			return false;

		if ( gpsLookup == NULL )
			return false;
		gpsLookup->SetInputDirectory( dataDirectory );
		if ( !gpsLookup->LoadData() )
			return false;

		if ( router == NULL )
			return false;
		router->SetInputDirectory( dataDirectory );
		if ( !router->LoadData() )
			return false;
	}
	catch ( ... )
	{
		return false;
	}
	return true;
}

void MainWindow::unloadPlugins()
{
	renderer = NULL;
	addressLookup = NULL;
	gpsLookup = NULL;
	foreach( QPluginLoader* pluginLoader, plugins )
	{
		pluginLoader->unload();
		delete pluginLoader;
	}
	plugins.clear();
}

void MainWindow::setSource( UnsignedCoordinate s, double h )
{
	if ( source.x == s.x && source.y == s.y && heading == h )
		return;
	source = s;
	heading = h;
	QVector< IGPSLookup::Result > result;
	if ( !gpsLookup->GetNearEdges( &result, source, 100, heading == 0 ? 0 : 10, heading ) )
		return;
	sourcePos = result.first();
	sourceSet = true;
	computeRoute();
}

void MainWindow::setTarget( UnsignedCoordinate t )
{
	if ( target.x == t.x && target.y == t.y )
		return;
	target = t;
	QVector< IGPSLookup::Result > result;
	if ( !gpsLookup->GetNearEdges( &result, target, 100 ) )
		return;
	targetPos = result.first();
	targetSet = true;
	computeRoute();
}

void MainWindow::computeRoute()
{
	if ( !sourceSet || !targetSet )
		return;
	double distance;
	path.clear();
	if ( !router->GetRoute( &distance, &path, sourcePos, targetPos ) )
		path.clear();
	qDebug() << "Distance: " << distance << "; Path Segments: " << path.size();
	emit routeChanged( path );
}

void MainWindow::menuClicked( QListWidgetItem* item )
{
	QString label = item->text();
	if ( label == tr( "Source" ) )
		sourceMode();
	else if ( label == tr( "Target" ) )
		targetMode();
	else if ( label == tr( "Route" ) )
		routeView();
	else if ( label == tr( "Map" ) )
		browseMap();
	else if ( label == tr( "Settings" ) )
		settingsMenu();
	else if ( label == tr( "Address" ) )
		targetAddress();
	else if ( label == tr( "Bookmarks" ) )
		targetBookmarks();
	else if ( label == tr( "GPS" ) )
		targetGPS();
	else if ( label == tr( "System Settings" ) )
		settingsSystem();
	else if ( label == tr( "Map Renderer Settings" ) )
		settingsRenderer();
	else if ( label == tr( "GPS Lookup Settings" ) )
		settingsGPSLookup();
	else if ( label == tr( "Address Lookup Settings" ) )
		settingsAddressLookup();
	else if ( label == tr( "GPS Settings" ) )
		settingsGPS();
	else if ( label == tr( "Data Directory Settings" ) ) {
		dataDirectory = "";
		settingsDataDirectory();
	}
}

void MainWindow::back()
{
	ui->targetSourceWidget->hide();
	ui->settingsWidget->hide();
	ui->mainMenuWidget->show();
}

void MainWindow::browseMap()
{
	MapView* window = new MapView( this );
	window->setRender( renderer );
	window->setGPSLookup( gpsLookup );
	window->setAddressLookup( addressLookup );
	window->setCenter( source.ToProjectedCoordinate() );
	window->setSource( source, heading );
	window->setTarget( target );
	window->setContextMenuEnabled( true );
	connect( window, SIGNAL(sourceChanged(UnsignedCoordinate,double)), this, SLOT(setSource(UnsignedCoordinate,double)) );
	connect( window, SIGNAL(targetChanged(UnsignedCoordinate)), this, SLOT(setTarget(UnsignedCoordinate)) );
	connect( this, SIGNAL(routeChanged(QVector<UnsignedCoordinate>)), window, SLOT(setRoute(QVector<UnsignedCoordinate>)) );
	window->exec();
	delete window;
}

void MainWindow::sourceMode()
{
	mode = Source;
	ui->mainMenuWidget->hide();
	ui->targetSourceWidget->show();
	ui->targetSourceMenuLabel->setText( tr( "Source Menu" ) );
}

void MainWindow::targetMode()
{
	mode = Target;
	ui->mainMenuWidget->hide();
	ui->targetSourceWidget->show();
	ui->targetSourceMenuLabel->setText( tr( "Target Menu" ) );
}

void MainWindow::routeView()
{
	MapView* window = new MapView( this );
	window->setRender( renderer );
	window->setGPSLookup( gpsLookup );
	window->exec();
	delete window;
}

void MainWindow::settingsMenu()
{
	ui->settingsWidget->show();
	ui->mainMenuWidget->hide();
}

void MainWindow::targetBookmarks()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this, source, target ) )
		return;

	if ( mode == Source )
		setSource( result, 0 );
	else if ( mode == Target )
		setTarget( result );
}

void MainWindow::targetAddress()
{
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, addressLookup, renderer, gpsLookup, this ) )
		return;

	if ( mode == Source )
		setSource( result, 0 );
	else if ( mode == Target )
		setTarget( result );
}

void MainWindow::targetGPS()
{

}


void MainWindow::settingsSystem()
{

}

void MainWindow::settingsRenderer()
{
	if ( renderer != NULL )
		renderer->ShowSettings();
}

void MainWindow::settingsGPSLookup()
{
	if( gpsLookup != NULL )
		gpsLookup->ShowSettings();
}

void MainWindow::settingsAddressLookup()
{
	if ( addressLookup != NULL )
		addressLookup->ShowSettings();
}

void MainWindow::settingsGPS()
{

}

void MainWindow::settingsDataDirectory()
{
	while ( true )
	{
		unloadPlugins();
		if ( loadPlugins() )
			break;
		QString newDir = QFileDialog::getExistingDirectory( this, "Enter Data Directory", dataDirectory );
		if ( newDir == "" )
			break;
		dataDirectory = newDir;
	}
}

