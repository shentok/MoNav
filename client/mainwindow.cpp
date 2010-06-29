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
#include "mapview.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	renderer = NULL;
	addressLookup = NULL;
	gpsLookup = NULL;

	ui->setupUi(this);
	ui->targetSourceWidget->hide();
	ui->settingsWidget->hide();
	addressDialog = new AddressDialog( this );

	QSettings settings( "MoNavClient" );
	dataDirectory = settings.value( "dataDirectory" ).toString();
	mode = Source;

	source.x = 1 << 30;
	source.y = 1 << 30;

	connectSlots();

	settingsDataDirectory();
}

MainWindow::~MainWindow()
{
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

	foreach ( QString fileName, pluginDir.entryList( QDir::Files ) ) {
		QPluginLoader* loader = new QPluginLoader( pluginDir.absoluteFilePath( fileName ) );
		if ( !loader->load() )
			qDebug( "%s", loader->errorString().toAscii().constData() );

		if ( IRenderer *interface = qobject_cast< IRenderer* >( loader->instance() ) )
		{
			if ( interface->GetName() == "Mapnik Renderer" )
			{
				plugins.append( loader );
				renderer = interface;
			}
		}
		else if ( IAddressLookup *interface = qobject_cast< IAddressLookup* >( loader->instance() ) )
		{
			if ( interface->GetName() == "Unicode Tournament Trie" )
			{
				plugins.append( loader );
				addressLookup = interface;
			}
		}
		else if ( IGPSLookup *interface = qobject_cast< IGPSLookup* >( loader->instance() ) )
		{
			if ( interface->GetName() == "GPS Grid" )
			{
				plugins.append( loader );
				gpsLookup = interface;
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
		addressDialog->setRenderer( renderer );

		if ( addressLookup == false )
			return false;
		addressLookup->SetInputDirectory( dataDirectory );
		if ( !addressLookup->LoadData() )
			return false;
		addressDialog->setAddressLookup( addressLookup );

		if ( gpsLookup == false )
			return false;
		gpsLookup->SetInputDirectory( dataDirectory );
		if ( !gpsLookup->LoadData() )
			return false;
		addressDialog->setGPSLookup( gpsLookup );
	}
	catch ( ... )
	{
		return false;
	}
	return true;
}

void MainWindow::unloadPlugins()
{
	addressDialog->setAddressLookup( NULL );
	addressDialog->setRenderer( NULL );
	addressDialog->setGPSLookup( NULL );
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

void MainWindow::setSource( UnsignedCoordinate s, double heading )
{
	source = s;
}

void MainWindow::setTarget( UnsignedCoordinate t, double heading )
{
	target = t;
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
	else if ( label == tr( "Via Map" ) )
		targetMap();
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
	else if ( label == tr( "Data Directory Settings" ) )
		settingsDataDirectory();
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
	window->setCenter( source.ToProjectedCoordinate() );
	window->setSource( source, 0 );
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

}

void MainWindow::targetAddress()
{
	addressDialog->resetCity();
	if ( mode == Source )
		connect( addressDialog, SIGNAL(coordinateChosen(UnsignedCoordinate, double)), this, SLOT(setSource(UnsignedCoordinate, double)));
	else if ( mode == Target )
		connect( addressDialog, SIGNAL(coordinateChosen(UnsignedCoordinate, double)), this, SLOT(setTarget(UnsignedCoordinate, double)));
	addressDialog->exec();
	disconnect( addressDialog, SIGNAL(coordinateChosen(UnsignedCoordinate, double)) );
}

void MainWindow::targetMap()
{

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

