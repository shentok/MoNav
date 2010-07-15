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
#include <QMessageBox>
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
	QSize maxSize = ui->mainMenuList->widget()->size();
	maxSize = maxSize.expandedTo( ui->targetMenuList->widget()->size() );
	maxSize = maxSize.expandedTo( ui->settingsMenuList->widget()->size() );
	maxSize += QSize( 2, 2 );
	ui->mainMenuList->setMinimumSize( maxSize );
	ui->targetMenuList->setMinimumSize( maxSize );
	ui->settingsMenuList->setMinimumSize( maxSize );
	ui->targetSourceWidget->hide();
	ui->settingsWidget->hide();
	this->updateGeometry();

	QSettings settings( "MoNavClient" );
	dataDirectory = settings.value( "dataDirectory" ).toString();
	UnsignedCoordinate oldSource;
	oldSource.x = settings.value( "source.x", 0 ).toUInt();
	oldSource.y = settings.value( "source.y", 0 ).toUInt();
	mode = Source;

	connectSlots();

	if ( !loadPlugins() )
		settingsDataDirectory();
	setSource( oldSource, 0 );
}

MainWindow::~MainWindow()
{
	unloadPlugins();
	QSettings settings( "MoNavClient" );
	settings.setValue( "dataDirectory", dataDirectory );
	settings.setValue( "source.x", source.x );
	settings.setValue( "source.y", source.y );

	//delete static plugins
	foreach ( QObject *plugin, QPluginLoader::staticInstances() ) {
		delete plugin;
	}
	delete ui;
}

void MainWindow::connectSlots()
{
	connect( ui->backButton, SIGNAL(clicked()), this, SLOT(back()) );
	connect( ui->sourceButton, SIGNAL(clicked()), this, SLOT(sourceMode()) );
	connect( ui->targetButton, SIGNAL(clicked()), this, SLOT(targetMode()) );
	connect( ui->routeButton, SIGNAL(clicked()), this, SLOT(routeView()) );
	connect( ui->mapButton, SIGNAL(clicked()), this, SLOT(browseMap()) );
	connect( ui->settingsButton, SIGNAL(clicked()), this, SLOT(settingsMenu()) );

	connect( ui->addressButton, SIGNAL(clicked()), this, SLOT(targetAddress()) );
	connect( ui->bookmarkButton, SIGNAL(clicked()), this, SLOT(targetBookmarks()) );
	connect( ui->gpsButton, SIGNAL(clicked()), this, SLOT(targetGPS()) );

	connect( ui->settingsAddressLookupButton, SIGNAL(clicked()), this, SLOT(settingsAddressLookup()) );
	connect( ui->settingsDataButton, SIGNAL(clicked()), this, SLOT(settingsDataDirectory()) );
	connect( ui->settingsGPSButton, SIGNAL(clicked()), this, SLOT(settingsGPS()) );
	connect( ui->settingsGPSLookupButton, SIGNAL(clicked()), this, SLOT(settingsGPSLookup()) );
	connect( ui->settingsMapButton, SIGNAL(clicked()), this, SLOT(settingsRenderer()) );
	connect( ui->settingsSystemButton, SIGNAL(clicked()), this, SLOT(settingsSystem()) );

}

bool MainWindow::loadPlugins()
{
	QDir dir( dataDirectory );
	QSettings pluginSettings( dir.filePath( "plugins.ini" ), QSettings::IniFormat );
	QString rendererName = pluginSettings.value( "renderer" ).toString();
	QString routerName = pluginSettings.value( "router" ).toString();
	QString gpsLookupName = pluginSettings.value( "gpsLookup" ).toString();
	QString addressLookupName = pluginSettings.value( "addressLookup" ).toString();

	QDir pluginDir( QApplication::applicationDirPath() );
	if ( pluginDir.cd( "plugins_client" ) ) {
		foreach ( QString fileName, pluginDir.entryList( QDir::Files ) ) {
			QPluginLoader* loader = new QPluginLoader( pluginDir.absoluteFilePath( fileName ) );
			if ( !loader->load() )
				qDebug( "%s", loader->errorString().toAscii().constData() );
			if ( testPlugin( loader->instance(), rendererName, routerName, gpsLookupName, addressLookupName ) )
				plugins.append( loader );
			else {
				loader->unload();
				delete loader;
			}
		}
	}

	foreach ( QObject *plugin, QPluginLoader::staticInstances() ) {
		testPlugin( plugin, rendererName, routerName, gpsLookupName, addressLookupName );
	}

	try
	{
		if ( renderer == NULL ) {
			qCritical() << "Renderer plugin not found: " << rendererName;
			return false;
		}
		renderer->SetInputDirectory( dataDirectory );
		if ( !renderer->LoadData() ) {
			qCritical() << "Could not load renderer data";
			return false;
		}

		if ( addressLookup == NULL ) {
			qCritical() << "AddressLookup plugin not found: " << addressLookupName;
			return false;
		}
		addressLookup->SetInputDirectory( dataDirectory );
		if ( !addressLookup->LoadData() ) {
			qCritical() << "Could not load address lookup data";
			return false;
		}

		if ( gpsLookup == NULL ) {
			qCritical() << "GPSLookup plugin not found: " << gpsLookupName;
			return false;
		}
		gpsLookup->SetInputDirectory( dataDirectory );
		if ( !gpsLookup->LoadData() ) {
			qCritical() << "Could not load gps lookup data";
			return false;
		}

		if ( router == NULL ) {
			qCritical() << "Router plugin not found: " << routerName;
			return false;
		}
		router->SetInputDirectory( dataDirectory );
		if ( !router->LoadData() ) {
			qCritical() << "Could not load router data";
			return false;
		}
	}
	catch ( ... )
	{
		qCritical() << "Caught exception while loading plugins";
		return false;
	}
	return true;
}

bool MainWindow::testPlugin( QObject* plugin, QString rendererName, QString routerName, QString gpsLookupName, QString addressLookupName )
{
	bool needed = false;
	if ( IRenderer *interface = qobject_cast< IRenderer* >( plugin ) ) {
		if ( interface->GetName() == rendererName ) {
			renderer = interface;
			needed = true;
		}
	}
	if ( IAddressLookup *interface = qobject_cast< IAddressLookup* >( plugin ) ) {
		if ( interface->GetName() == addressLookupName )
		{
			addressLookup = interface;
			needed = true;
		}
	}
	if ( IGPSLookup *interface = qobject_cast< IGPSLookup* >( plugin ) ) {
		if ( interface->GetName() == gpsLookupName ) {
			gpsLookup = interface;
			needed = true;
		}
	}
	if ( IRouter *interface = qobject_cast< IRouter* >( plugin ) ) {
		if ( interface->GetName() == routerName ) {
			router = interface;
			needed = true;
		}
	}
	return needed;
}

void MainWindow::unloadPlugins()
{
	renderer = NULL;
	router = NULL;
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
	window->setRoute( path );
	window->setContextMenuEnabled( true );
	window->setMode( MapView::Target );
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
		dataDirectory = QFileDialog::getExistingDirectory( this, "Enter Data Directory", dataDirectory );
		if ( dataDirectory == "" ) {
			QMessageBox::information( NULL, "Data Directory", "No Data Directory Specified" );
			abort();
		}
		unloadPlugins();
		if ( loadPlugins() )
			break;
	}
}

