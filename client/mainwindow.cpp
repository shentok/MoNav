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
#include "routedescriptiondialog.h"
#include "descriptiongenerator.h"
#include "utils/qthelpers.h"
#include <QDir>
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

	m_renderer = NULL;
	m_addressLookup = NULL;
	m_gpsLookup = NULL;
	m_router = NULL;

	m_heading = 0;

	m_sourceSet = false;
	m_targetSet = false;

	m_ui->targetSourceWidget->hide();
	m_ui->settingsWidget->hide();

	QSettings settings( "MoNavClient" );
	m_dataDirectory = settings.value( "dataDirectory", QDir::homePath() ).toString();
	m_source.x = settings.value( "source.x", 0 ).toUInt();
	m_source.y = settings.value( "source.y", 0 ).toUInt();
	if ( settings.contains( "menu.geometry") )
		setGeometry( settings.value( "menu.geometry" ).toRect() );
	mode = Source;

#ifndef NOQTMOBILE
	m_gpsSource = QGeoPositionInfoSource::createDefaultSource( this );
	if ( m_gpsSource == 0 ) {
		qWarning() << "No GPS Sensor found! GPS Updates are not available";
		m_ui->gpsButton->setEnabled( false );
	}
#endif

	connectSlots();

	if ( m_dataDirectory.isEmpty() ) {
		QMessageBox::information( this, "Data Directory", "Please specifiy a data directory" );
		settingsDataDirectory();
	} else {
		if ( !loadPlugins() )
			settingsDataDirectory();
	}

	m_updateSource = true;
	m_updateTarget = false;
#ifndef NOQTMOBILE
	if ( m_gpsSource != NULL )
		m_gpsSource->startUpdates();
#endif
}

MainWindow::~MainWindow()
{
	unloadPlugins();
	QSettings settings( "MoNavClient" );
	settings.setValue( "dataDirectory", m_dataDirectory );
	settings.setValue( "source.x", m_source.x );
	settings.setValue( "source.y", m_source.y );
	settings.setValue( "menu.geometry", geometry() );

	//delete static plugins
	foreach ( QObject *plugin, QPluginLoader::staticInstances() )
		delete plugin;

	delete m_ui;
}

void MainWindow::connectSlots()
{
	connect( m_ui->sourceButton, SIGNAL(clicked()), this, SLOT(sourceMode()) );
	connect( m_ui->targetButton, SIGNAL(clicked()), this, SLOT(targetMode()) );
	connect( m_ui->routeButton, SIGNAL(clicked()), this, SLOT(routeView()) );
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

#ifndef NOQTMOBILE
	if ( m_gpsSource != NULL)
		connect( m_gpsSource, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(positionUpdated(QGeoPositionInfo)) );
#endif
}

bool MainWindow::loadPlugins()
{
	QDir dir( m_dataDirectory );
	QString configFilename = dir.filePath( "plugins.ini" );
	if ( !QFile::exists( configFilename ) ) {
		qCritical() << "Not a valid data directory: Missing plugins.ini";
		return false;
	}
	QSettings pluginSettings( configFilename, QSettings::IniFormat );
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
				m_plugins.append( loader );
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
		if ( m_renderer == NULL ) {
			qCritical() << "Renderer plugin not found: " << rendererName;
			return false;
		}
		m_renderer->SetInputDirectory( m_dataDirectory );
		if ( !m_renderer->LoadData() ) {
			qCritical() << "Could not load renderer data";
			return false;
		}

		if ( m_addressLookup == NULL ) {
			qCritical() << "AddressLookup plugin not found: " << addressLookupName;
			return false;
		}
		m_addressLookup->SetInputDirectory( m_dataDirectory );
		if ( !m_addressLookup->LoadData() ) {
			qCritical() << "Could not load address lookup data";
			return false;
		}

		if ( m_gpsLookup == NULL ) {
			qCritical() << "GPSLookup plugin not found: " << gpsLookupName;
			return false;
		}
		m_gpsLookup->SetInputDirectory( m_dataDirectory );
		if ( !m_gpsLookup->LoadData() ) {
			qCritical() << "Could not load gps lookup data";
			return false;
		}

		if ( m_router == NULL ) {
			qCritical() << "Router plugin not found: " << routerName;
			return false;
		}
		m_router->SetInputDirectory( m_dataDirectory );
		if ( !m_router->LoadData() ) {
			qCritical() << "Could not load router data";
			return false;
		}
	}
	catch ( ... )
	{
		qCritical() << "Caught exception while loading plugins";
		return false;
	}

	m_targetSet = false;
	m_sourceSet = false;
	UnsignedCoordinate oldSource = m_source;
	UnsignedCoordinate oldTarget = m_target;
	m_source = UnsignedCoordinate();
	m_target = UnsignedCoordinate();
	setSource( oldSource, m_heading );
	setTarget( oldTarget );

	return true;
}

bool MainWindow::testPlugin( QObject* plugin, QString rendererName, QString routerName, QString gpsLookupName, QString addressLookupName )
{
	bool needed = false;
	if ( IRenderer *interface = qobject_cast< IRenderer* >( plugin ) ) {
		if ( interface->GetName() == rendererName ) {
			m_renderer = interface;
			needed = true;
		}
	}
	if ( IAddressLookup *interface = qobject_cast< IAddressLookup* >( plugin ) ) {
		if ( interface->GetName() == addressLookupName )
		{
			m_addressLookup = interface;
			needed = true;
		}
	}
	if ( IGPSLookup *interface = qobject_cast< IGPSLookup* >( plugin ) ) {
		if ( interface->GetName() == gpsLookupName ) {
			m_gpsLookup = interface;
			needed = true;
		}
	}
	if ( IRouter *interface = qobject_cast< IRouter* >( plugin ) ) {
		if ( interface->GetName() == routerName ) {
			m_router = interface;
			needed = true;
		}
	}
	return needed;
}

void MainWindow::unloadPlugins()
{
	m_renderer = NULL;
	m_router = NULL;
	m_addressLookup = NULL;
	m_gpsLookup = NULL;
	foreach( QPluginLoader* pluginLoader, m_plugins )
	{
		pluginLoader->unload();
		delete pluginLoader;
	}
	m_plugins.clear();
}

void MainWindow::setSource( UnsignedCoordinate source, double heading )
{
	m_updateSource = false;
	if ( m_source == source && m_heading == heading )
		return;

	m_source = source;
	m_heading = heading;

	emit sourceChanged( m_source, m_heading );

	Timer time;
	m_sourceSet = m_gpsLookup->GetNearestEdge( &m_sourcePos, m_source, 300, m_heading == 0 ? 0 : 10, m_heading );
	qDebug() << "GPS Lookup:" << time.elapsed() << "ms";

	if ( !m_sourceSet ) {
		m_pathNodes.clear();
		m_descriptionIcons.clear();
		m_descriptionLabels.clear();
		emit routeChanged( m_pathNodes, m_descriptionIcons, m_descriptionLabels );
		return;
	}

	computeRoute();
}

void MainWindow::setTarget( UnsignedCoordinate target )
{
	m_updateTarget = false;
	if ( m_target == target )
		return;

	m_target = target;

	emit targetChanged( m_target );

	Timer time;
	m_targetSet = m_gpsLookup->GetNearestEdge( &m_targetPos, m_target, 300 );
	qDebug() << "GPS Lookup:" << time.elapsed() << "ms";

	if ( !m_targetSet ) {
		m_pathNodes.clear();
		m_descriptionIcons.clear();
		m_descriptionLabels.clear();
		emit routeChanged( m_pathNodes, m_descriptionIcons, m_descriptionLabels );
		return;
	}

	computeRoute();
}

void MainWindow::computeRoute()
{
	if ( !m_sourceSet || !m_targetSet )
		return;

	double distance;
	m_pathNodes.clear();
	m_pathEdges.clear();

	Timer time;
	bool found = m_router->GetRoute( &distance, &m_pathNodes, &m_pathEdges, m_sourcePos, m_targetPos );
	qDebug() << "routing:" << time.elapsed() << "ms";
	qDebug() << "distance: " << distance << "; nodes:" << m_pathNodes.size() << "; edges:" << m_pathEdges.size();

	if ( !found ) {
		m_pathNodes.clear();
		m_pathEdges.clear();
		m_descriptionIcons.clear();
		m_descriptionLabels.clear();
	}

	DescriptionGenerator generator;
	generator.descriptions( &m_descriptionIcons, &m_descriptionLabels, m_router, m_pathNodes, m_pathEdges, 60 );

	emit routeChanged( m_pathNodes, m_descriptionIcons, m_descriptionLabels );
}

void MainWindow::routeDescription()
{
	RouteDescriptionDialog* window = new RouteDescriptionDialog();
	DescriptionGenerator generator;
	generator.descriptions( &m_descriptionIcons, &m_descriptionLabels, m_router, m_pathNodes, m_pathEdges );
	window->setDescriptions( m_descriptionIcons, m_descriptionLabels );
	window->exec();
	delete window;
}

void MainWindow::browseMap()
{
	MapView* window = new MapView( this );
	window->setRender( m_renderer );
	window->setAddressLookup( m_addressLookup );
	window->setCenter( m_source.ToProjectedCoordinate() );
	window->setSource( m_source, m_heading );
	window->setTarget( m_target );
	window->setRoute( m_pathNodes, m_descriptionIcons, m_descriptionLabels );
	window->setMenu( MapView::ContextMenu );

	connect( window, SIGNAL(sourceChanged(UnsignedCoordinate,double)), this, SLOT(setSource(UnsignedCoordinate,double)) );
	connect( window, SIGNAL(targetChanged(UnsignedCoordinate)), this, SLOT(setTarget(UnsignedCoordinate)) );
	connect( window, SIGNAL(infoClicked()), this, SLOT(routeDescription()) );
	connect( this, SIGNAL(routeChanged(QVector<IRouter::Node>,QStringList,QStringList)), window, SLOT(setRoute(QVector<IRouter::Node>,QStringList,QStringList)) );
	connect( this, SIGNAL(sourceChanged(UnsignedCoordinate,double)), window, SLOT(setSource(UnsignedCoordinate,double)) );
	connect( this, SIGNAL(targetChanged(UnsignedCoordinate)), window, SLOT(setTarget(UnsignedCoordinate)) );

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

void MainWindow::routeView()
{
	MapView* window = new MapView( this );
	window->setFixed( true );
	window->setRender( m_renderer );
	window->setSource( m_source, m_heading );
	window->setTarget( m_target );
	window->setRoute( m_pathNodes, m_descriptionIcons, m_descriptionLabels );
	window->setMenu( MapView::RouteMenu );

	connect( window, SIGNAL(infoClicked()), this, SLOT(routeDescription()) );
	connect( this, SIGNAL(routeChanged(QVector<IRouter::Node>,QStringList,QStringList)), window, SLOT(setRoute(QVector<IRouter::Node>,QStringList,QStringList)) );
	connect( this, SIGNAL(sourceChanged(UnsignedCoordinate,double)), window, SLOT(setSource(UnsignedCoordinate,double)) );

	window->exec();

	bool mapView = window->exitedToMapview();

	delete window;

	if ( mapView )
		browseMap();
}

void MainWindow::settingsMenu()
{
	m_ui->settingsWidget->show();
	m_ui->mainMenuWidget->hide();
}

void MainWindow::targetBookmarks()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this, m_source, m_target ) )
		return;

	if ( mode == Source )
		setSource( result, 0 );
	else if ( mode == Target )
		setTarget( result );

	m_ui->backButton->click();
}

void MainWindow::targetAddress()
{
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, m_addressLookup, m_renderer, this ) )
		return;

	if ( mode == Source )
		setSource( result, 0 );
	else if ( mode == Target )
		setTarget( result );

	m_ui->backButton->click();
}

void MainWindow::targetGPS()
{
	if ( mode == Target )
		m_updateTarget = true;
	if ( mode == Source )
		m_updateSource = true;
	QMessageBox::information( this, "GPS", "Activated GPS source" );

	m_ui->backButton->click();
}


void MainWindow::settingsSystem()
{

}

void MainWindow::settingsRenderer()
{
	if ( m_renderer != NULL )
		m_renderer->ShowSettings();
}

void MainWindow::settingsGPSLookup()
{
	if( m_gpsLookup != NULL )
		m_gpsLookup->ShowSettings();
}

void MainWindow::settingsAddressLookup()
{
	if ( m_addressLookup != NULL )
		m_addressLookup->ShowSettings();
}

void MainWindow::settingsGPS()
{

}

void MainWindow::settingsDataDirectory()
{
	while ( true )
	{
		QString input = QFileDialog::getExistingDirectory( this, "Enter Data Directory", m_dataDirectory );
		if ( input.isEmpty() ) {
			QMessageBox::information( NULL, "Data Directory", "No Data Directory Specified" );
			exit( -1 );
		}
		m_dataDirectory = input;
		unloadPlugins();
		if ( loadPlugins() )
			break;
	}

	m_ui->backButton->click();
}

#ifndef NOQTMOBILE
	void MainWindow::positionUpdated( const QGeoPositionInfo & update )
	{
		GPSCoordinate gps;
		gps.latitude = update.coordinate().latitude();
		gps.longitude = update.coordinate().longitude();
		UnsignedCoordinate pos( gps );
		if ( m_updateSource ) {
			if ( update.hasAttribute( QGeoPositionInfo::Direction ) )
				m_heading = update.attribute( QGeoPositionInfo::Direction );
			setSource( pos, m_heading );
			m_updateSource = true;
		}
		if ( m_updateTarget ) {
			setTarget( pos );
			m_updateTarget = true;
		}
	}
#endif

