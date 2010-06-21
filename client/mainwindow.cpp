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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	renderer = NULL;

    ui->setupUi(this);
	ui->targetSourceWidget->hide();
	ui->settingsWidget->hide();
	layout()->setSizeConstraint( QLayout::SetFixedSize );
	mapView = new MapView( this );

	QSettings settings( "MoNavClient" );
	dataDirectory = settings.value( "dataDirectory" ).toString();

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
	connect( ui->targetMenuButton, SIGNAL(clicked()), this, SLOT(targetMode()) );
	connect( ui->sourceMenuButton, SIGNAL(clicked()), this, SLOT(sourceMode()) );
	connect( ui->routeButton, SIGNAL(clicked()), this, SLOT(routeView()) );
	connect( ui->browseButton, SIGNAL(clicked()), this, SLOT(browseMap()) );
	connect( ui->aboutButton, SIGNAL(clicked()), this, SLOT(about()) );

	connect( ui->mapButton, SIGNAL(clicked()), this, SLOT(targetMap()) );
	connect( ui->cityCenterButton, SIGNAL(clicked()), this, SLOT(targetCityCenter()) );
	connect( ui->bookmarksButton, SIGNAL(clicked()), this, SLOT(targetBookmarks()) );
	connect( ui->gpsButton, SIGNAL(clicked()), this, SLOT(targetGPS()) );
	connect( ui->addressButton, SIGNAL(clicked()), this, SLOT(targetAddress()) );

	connect( ui->systemSettingsButton, SIGNAL(clicked()), this, SLOT(settingsSystem()) );
	connect( ui->rendererSettingsButton, SIGNAL(clicked()), this, SLOT(settingsRenderer()) );
	connect( ui->gpsLookupSettingsButton, SIGNAL(clicked()), this, SLOT(settingsGPSLookup()) );
	connect( ui->addressLookupSettingsButton, SIGNAL(clicked()), this, SLOT(settingsAddressLookup()) );
	connect( ui->dataDirectoryButton, SIGNAL(clicked()), this, SLOT(settingsDataDirectory()) );
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
				mapView->setRender( renderer );
			}
		}
	}

	try
	{
		renderer->SetInputDirectory( dataDirectory );
		if ( !renderer->LoadData() )
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
	foreach( QPluginLoader* pluginLoader, plugins )
	{
		pluginLoader->unload();
		delete pluginLoader;
	}
	plugins.clear();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::browseMap()
{
	mapView->exec();
}

void MainWindow::sourceMode()
{

}

void MainWindow::targetMode()
{

}

void MainWindow::about()
{

}

void MainWindow::routeView()
{

}


void MainWindow::targetBookmarks()
{

}

void MainWindow::targetCityCenter()
{

}

void MainWindow::targetAddress()
{

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

}

void MainWindow::settingsGPSLookup()
{

}

void MainWindow::settingsAddressLookup()
{

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

