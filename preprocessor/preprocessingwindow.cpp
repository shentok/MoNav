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

#include "preprocessingwindow.h"
#include "ui_preprocessingwindow.h"
#include <QFileDialog>
#include <QSettings>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

PreprocessingWindow::PreprocessingWindow(QWidget *parent) :
	QMainWindow(parent),
	aboutDialog( NULL ),
	ui(new Ui::PreprocessingWindow)
{
	ui->setupUi(this);

	connectSlots();
	loadPlugins();

	QSettings settings( "MoNav" );
	settings.beginGroup( "Preprocessing" );
	ui->outputEdit->setText( settings.value( "Output Directory" ).toString() );
}

void PreprocessingWindow::connectSlots()
{
	connect( ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()) );
	connect( ui->actionManual, SIGNAL(triggered()), this, SLOT(manual()) );
	connect( ui->actionExit, SIGNAL(triggered()), this, SLOT(close()) );
	connect( ui->browseButton, SIGNAL(clicked()), this, SLOT(browse()) );
	connect( ui->importerSettingsButton, SIGNAL(clicked()), this, SLOT(importerSettings()) );
	connect( ui->importerPreprocessButton, SIGNAL(clicked()), this, SLOT(importerPreprocessing()) );
	connect( ui->rendererSettingsButton, SIGNAL(clicked()), this, SLOT(rendererSettings()) );
	connect( ui->rendererPreprocessButton, SIGNAL(clicked()), this, SLOT(rendererPreprocessing()) );
	connect( ui->routerSettingsButton, SIGNAL(clicked()), this, SLOT(routerSettings()) );
	connect( ui->routerPreprocessButton, SIGNAL(clicked()), this, SLOT(routerPreprocessing()) );
	connect( ui->gpsLookupSettingsButton, SIGNAL(clicked()), this, SLOT(gpsLookupSettings()) );
	connect( ui->gpsLookupPreprocessButton, SIGNAL(clicked()), this, SLOT(gpsLookupPreprocessing()) );
	connect( ui->addressLookupSettingsButton, SIGNAL(clicked()), this, SLOT(addressLookupSettings()) );
	connect( ui->addressLookupPreprocessButton, SIGNAL(clicked()), this, SLOT(addressLookupPreprocessing()) );
	connect( ui->allPreprocessButton, SIGNAL(clicked()), this, SLOT(preprocessAll()) );
	connect( ui->deleteTemporaryButton, SIGNAL(clicked()), this, SLOT(deleteTemporary()) );
	connect( ui->writeConfigButton, SIGNAL(clicked()), this, SLOT(writeConfig()) );
}

void PreprocessingWindow::loadPlugins()
{
	QDir pluginDir( QApplication::applicationDirPath() );
	if ( pluginDir.cd("plugins_preprocessor") ) {
		foreach ( QString fileName, pluginDir.entryList( QDir::Files ) ) {
			QPluginLoader* loader = new QPluginLoader( pluginDir.absoluteFilePath( fileName ) );
			loader->setLoadHints( QLibrary::ExportExternalSymbolsHint );
			if ( !loader->load() )
				qDebug( "%s", loader->errorString().toAscii().constData() );

			if ( testPlugin( loader->instance() ) )
				plugins.push_back( loader );
			else {
				loader->unload();
				delete loader;
			}
		}
	}

	foreach ( QObject *plugin, QPluginLoader::staticInstances() ) {
		testPlugin( plugin );
	}

	if ( importerPlugins.size() == 0 )
	{
		ui->importerPreprocessButton->setEnabled( false );
		ui->importerSettingsButton->setEnabled( false );
		ui->deleteTemporaryButton->setEnabled( false );
	}
	if ( importerPlugins.size() == 0 || rendererPlugins.size() == 0 )
	{
		ui->rendererPreprocessButton->setEnabled( false );
		ui->rendererSettingsButton->setEnabled( false );
	}
	if ( importerPlugins.size() == 0 || routerPlugins.size() == 0 )
	{
		ui->routerPreprocessButton->setEnabled( false );
		ui->routerSettingsButton->setEnabled( false );
	}
	if ( importerPlugins.size() == 0 || gpsLookupPlugins.size() == 0 )
	{
		ui->gpsLookupPreprocessButton->setEnabled( false );
		ui->gpsLookupSettingsButton->setEnabled( false );
	}
	if ( importerPlugins.size() == 0 || addressLookupPlugins.size() == 0 )
	{
		ui->addressLookupPreprocessButton->setEnabled( false );
		ui->addressLookupSettingsButton->setEnabled( false );
	}
}

bool PreprocessingWindow::testPlugin( QObject* plugin )
{
	bool needed = false;
	if ( IImporter *interface = qobject_cast< IImporter* >( plugin ) )
	{
		importerPlugins.append( interface );
		ui->importerComboBox->addItem( interface->GetName() );
		needed = true;
	}
	if ( IPreprocessor *interface = qobject_cast< IPreprocessor* >( plugin ) )
	{
		QString name = interface->GetName();
		if ( interface->GetType() == IPreprocessor::Renderer )
		{
			rendererPlugins.append( interface );
			ui->rendererComboBox->addItem( name );
		}
		if ( interface->GetType() == IPreprocessor::Router )
		{
			routerPlugins.append( interface );
			ui->routerComboBox->addItem( name );
		}
		if ( interface->GetType() == IPreprocessor::GPSLookup )
		{
			gpsLookupPlugins.append( interface );
			ui->gpsLookupComboBox->addItem( name );
		}
		if ( interface->GetType() == IPreprocessor::AddressLookup )
		{
			addressLookupPlugins.append( interface );
			ui->addressLookupComboBox->addItem( name );
		}
		needed = true;
	}
	return needed;
}

void PreprocessingWindow::unloadPlugins()
{
	foreach( QPluginLoader* pluginLoader, plugins )
	{
		pluginLoader->unload();
		delete pluginLoader;
	}
	foreach ( QObject *plugin, QPluginLoader::staticInstances() ) {
		delete plugin;
	}
}

PreprocessingWindow::~PreprocessingWindow()
{
	QSettings settings( "MoNav" );
	settings.beginGroup( "Preprocessing" );
	settings.setValue( "Output Directory", ui->outputEdit->text() );
	unloadPlugins();
	delete ui;
}

void PreprocessingWindow::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi( this );
		break;
	default:
		break;
	}
}

void PreprocessingWindow::about()
{
	if ( aboutDialog == NULL )
		aboutDialog = new AboutDialog( this );
	aboutDialog->exec();
}

void PreprocessingWindow::browse()
{
	QString dir = ui->outputEdit->text();
	dir = QFileDialog::getExistingDirectory(this, tr( "Open Output Directory" ), dir);
	if ( dir != "" )
		ui->outputEdit->setText( dir );
}

void PreprocessingWindow::importerSettings()
{
	int index = ui->importerComboBox->currentIndex();
	importerPlugins[index]->ShowSettings();
}

void PreprocessingWindow::importerPreprocessing()
{
	int index = ui->importerComboBox->currentIndex();
	importerPlugins[index]->SetOutputDirectory( ui->outputEdit->text() );
	bool result = importerPlugins[index]->Preprocess();
	if ( result )
		ui->importerLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		ui->importerLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	ui->rendererLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	ui->routerLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	ui->gpsLookupLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	ui->addressLookupLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
}

void PreprocessingWindow::rendererSettings()
{
	int index = ui->rendererComboBox->currentIndex();
	QString name = rendererPlugins[index]->GetName();
	rendererPlugins[index]->ShowSettings();
}

void PreprocessingWindow::rendererPreprocessing()
{
	int importerIndex = ui->importerComboBox->currentIndex();
	importerPlugins[importerIndex]->SetOutputDirectory( ui->outputEdit->text() );
	int index = ui->rendererComboBox->currentIndex();
	rendererPlugins[index]->SetOutputDirectory( ui->outputEdit->text() );
	bool result = rendererPlugins[index]->Preprocess( importerPlugins[importerIndex] );
	if ( result )
		ui->rendererLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		ui->rendererLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
}

void PreprocessingWindow::routerSettings()
{
	int index = ui->routerComboBox->currentIndex();
	routerPlugins[index]->ShowSettings();
}

void PreprocessingWindow::routerPreprocessing()
{
	int importerIndex = ui->importerComboBox->currentIndex();
	importerPlugins[importerIndex]->SetOutputDirectory( ui->outputEdit->text() );
	int index = ui->routerComboBox->currentIndex();
	routerPlugins[index]->SetOutputDirectory( ui->outputEdit->text() );
	bool result = routerPlugins[index]->Preprocess( importerPlugins[importerIndex] );
	if ( result )
		ui->routerLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		ui->routerLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	ui->gpsLookupLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
}

void PreprocessingWindow::gpsLookupSettings()
{
	int index = ui->gpsLookupComboBox->currentIndex();
	gpsLookupPlugins[index]->ShowSettings();
}

void PreprocessingWindow::gpsLookupPreprocessing()
{
	int importerIndex = ui->importerComboBox->currentIndex();
	importerPlugins[importerIndex]->SetOutputDirectory( ui->outputEdit->text() );
	int index = ui->gpsLookupComboBox->currentIndex();
	gpsLookupPlugins[index]->SetOutputDirectory( ui->outputEdit->text() );
	bool result = gpsLookupPlugins[index]->Preprocess( importerPlugins[importerIndex] );
	if ( result )
		ui->gpsLookupLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		ui->gpsLookupLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
}

void PreprocessingWindow::addressLookupSettings()
{
	int index = ui->addressLookupComboBox->currentIndex();
	addressLookupPlugins[index]->ShowSettings();
}

void PreprocessingWindow::addressLookupPreprocessing()
{
	int importerIndex = ui->importerComboBox->currentIndex();
	importerPlugins[importerIndex]->SetOutputDirectory( ui->outputEdit->text() );
	int index = ui->addressLookupComboBox->currentIndex();
	addressLookupPlugins[index]->SetOutputDirectory( ui->outputEdit->text() );
	bool result = addressLookupPlugins[index]->Preprocess( importerPlugins[importerIndex] );
	if ( result )
		ui->addressLookupLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		ui->addressLookupLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
}

void PreprocessingWindow::preprocessAll()
{
	qDebug() << "Importer";
	importerPreprocessing();
	QCoreApplication::processEvents();
	qDebug() << "Router";
	routerPreprocessing();
	QCoreApplication::processEvents();
	qDebug() << "Renderer";
	rendererPreprocessing();
	QCoreApplication::processEvents();
	qDebug() << "GPS Lookup";
	gpsLookupPreprocessing();
	QCoreApplication::processEvents();
	qDebug() << "Address Lookup";
	addressLookupPreprocessing();
}

void PreprocessingWindow::writeConfig()
{
	QDir dir( ui->outputEdit->text() );
	QSettings pluginSettings( dir.filePath( "plugins.ini" ), QSettings::IniFormat );
	pluginSettings.setValue( "router", ui->routerComboBox->currentText() );
	pluginSettings.setValue( "renderer", ui->rendererComboBox->currentText() );
	pluginSettings.setValue( "gpsLookup", ui->gpsLookupComboBox->currentText() );
	pluginSettings.setValue( "addressLookup", ui->addressLookupComboBox->currentText() );
	QMessageBox::information( this, "Config", "Plugin config file plugins.ini has been written" );
}

void PreprocessingWindow::deleteTemporary()
{
	int index = ui->importerComboBox->currentIndex();
	importerPlugins[index]->SetOutputDirectory( ui->outputEdit->text() );
	importerPlugins[index]->DeleteTemporaryFiles();
	QMessageBox::information( this, "Temporary Files", "Deleted temporary files" );
}

void PreprocessingWindow::manual()
{
	QDesktopServices::openUrl( QUrl( "http://code.google.com/p/monav/" ) );
}
