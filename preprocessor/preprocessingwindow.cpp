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
	m_aboutDialog( NULL ),
	m_ui(new Ui::PreprocessingWindow)
{
	m_ui->setupUi(this);

	connectSlots();
	loadPlugins();

	QSettings settings( "MoNav" );
	settings.beginGroup( "Preprocessing" );
	m_ui->outputEdit->setText( settings.value( "Output Directory" ).toString() );
}

void PreprocessingWindow::connectSlots()
{
	connect( m_ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()) );
	connect( m_ui->actionManual, SIGNAL(triggered()), this, SLOT(manual()) );
	connect( m_ui->actionExit, SIGNAL(triggered()), this, SLOT(close()) );
	connect( m_ui->browseButton, SIGNAL(clicked()), this, SLOT(browse()) );
	connect( m_ui->importerSettingsButton, SIGNAL(clicked()), this, SLOT(importerSettings()) );
	connect( m_ui->importerPreprocessButton, SIGNAL(clicked()), this, SLOT(importerPreprocessing()) );
	connect( m_ui->rendererSettingsButton, SIGNAL(clicked()), this, SLOT(rendererSettings()) );
	connect( m_ui->rendererPreprocessButton, SIGNAL(clicked()), this, SLOT(rendererPreprocessing()) );
	connect( m_ui->routerSettingsButton, SIGNAL(clicked()), this, SLOT(routerSettings()) );
	connect( m_ui->routerPreprocessButton, SIGNAL(clicked()), this, SLOT(routerPreprocessing()) );
	connect( m_ui->gpsLookupSettingsButton, SIGNAL(clicked()), this, SLOT(gpsLookupSettings()) );
	connect( m_ui->gpsLookupPreprocessButton, SIGNAL(clicked()), this, SLOT(gpsLookupPreprocessing()) );
	connect( m_ui->addressLookupSettingsButton, SIGNAL(clicked()), this, SLOT(addressLookupSettings()) );
	connect( m_ui->addressLookupPreprocessButton, SIGNAL(clicked()), this, SLOT(addressLookupPreprocessing()) );
	connect( m_ui->allPreprocessButton, SIGNAL(clicked()), this, SLOT(preprocessAll()) );
	connect( m_ui->deleteTemporaryButton, SIGNAL(clicked()), this, SLOT(deleteTemporary()) );
	connect( m_ui->writeConfigButton, SIGNAL(clicked()), this, SLOT(writeConfig()) );
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

			if ( testPlugin( loader->instance() ) ) {
				m_plugins.push_back( loader );
			} else {
				loader->unload();
				delete loader;
			}
		}
	}

	foreach ( QObject *plugin, QPluginLoader::staticInstances() )
		testPlugin( plugin );

	if ( m_importerPlugins.size() == 0 )
	{
		m_ui->importerPreprocessButton->setEnabled( false );
		m_ui->importerSettingsButton->setEnabled( false );
		m_ui->deleteTemporaryButton->setEnabled( false );
	}
	if ( m_importerPlugins.size() == 0 || m_rendererPlugins.size() == 0 )
	{
		m_ui->rendererPreprocessButton->setEnabled( false );
		m_ui->rendererSettingsButton->setEnabled( false );
	}
	if ( m_importerPlugins.size() == 0 || m_routerPlugins.size() == 0 )
	{
		m_ui->routerPreprocessButton->setEnabled( false );
		m_ui->routerSettingsButton->setEnabled( false );
	}
	if ( m_importerPlugins.size() == 0 || m_gpsLookupPlugins.size() == 0 )
	{
		m_ui->gpsLookupPreprocessButton->setEnabled( false );
		m_ui->gpsLookupSettingsButton->setEnabled( false );
	}
	if ( m_importerPlugins.size() == 0 || m_addressLookupPlugins.size() == 0 )
	{
		m_ui->addressLookupPreprocessButton->setEnabled( false );
		m_ui->addressLookupSettingsButton->setEnabled( false );
	}
}

bool PreprocessingWindow::testPlugin( QObject* plugin )
{
	bool needed = false;
	if ( IImporter *interface = qobject_cast< IImporter* >( plugin ) )
	{
		m_importerPlugins.append( interface );
		m_ui->importerComboBox->addItem( interface->GetName() );
		needed = true;
	}
	if ( IPreprocessor *interface = qobject_cast< IPreprocessor* >( plugin ) )
	{
		QString name = interface->GetName();
		if ( interface->GetType() == IPreprocessor::Renderer )
		{
			m_rendererPlugins.append( interface );
			m_ui->rendererComboBox->addItem( name );
		}
		if ( interface->GetType() == IPreprocessor::Router )
		{
			m_routerPlugins.append( interface );
			m_ui->routerComboBox->addItem( name );
		}
		if ( interface->GetType() == IPreprocessor::GPSLookup )
		{
			m_gpsLookupPlugins.append( interface );
			m_ui->gpsLookupComboBox->addItem( name );
		}
		if ( interface->GetType() == IPreprocessor::AddressLookup )
		{
			m_addressLookupPlugins.append( interface );
			m_ui->addressLookupComboBox->addItem( name );
		}
		needed = true;
	}
	return needed;
}

void PreprocessingWindow::unloadPlugins()
{
	foreach( QPluginLoader* pluginLoader, m_plugins )
	{
		pluginLoader->unload();
		delete pluginLoader;
	}
	foreach ( QObject *plugin, QPluginLoader::staticInstances() )
		delete plugin;
}

PreprocessingWindow::~PreprocessingWindow()
{
	QSettings settings( "MoNav" );
	settings.beginGroup( "Preprocessing" );
	settings.setValue( "Output Directory", m_ui->outputEdit->text() );
	unloadPlugins();
	delete m_ui;
}

void PreprocessingWindow::about()
{
	if ( m_aboutDialog == NULL )
		m_aboutDialog = new AboutDialog( this );
	m_aboutDialog->exec();
}

void PreprocessingWindow::browse()
{
	QString dir = m_ui->outputEdit->text();
	dir = QFileDialog::getExistingDirectory(this, tr( "Open Output Directory" ), dir);
	if ( dir != "" )
		m_ui->outputEdit->setText( dir );
}

void PreprocessingWindow::importerSettings()
{
	int index = m_ui->importerComboBox->currentIndex();
	m_importerPlugins[index]->ShowSettings();
}

bool PreprocessingWindow::importerPreprocessing()
{
	int index = m_ui->importerComboBox->currentIndex();
	m_importerPlugins[index]->SetOutputDirectory( m_ui->outputEdit->text() );
	bool result = m_importerPlugins[index]->Preprocess();
	if ( result )
		m_ui->importerLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		m_ui->importerLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	m_ui->rendererLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	m_ui->routerLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	m_ui->gpsLookupLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	m_ui->addressLookupLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	return result;
}

void PreprocessingWindow::rendererSettings()
{
	int index = m_ui->rendererComboBox->currentIndex();
	QString name = m_rendererPlugins[index]->GetName();
	m_rendererPlugins[index]->ShowSettings();
}

bool PreprocessingWindow::rendererPreprocessing()
{
	int importerIndex = m_ui->importerComboBox->currentIndex();
	m_importerPlugins[importerIndex]->SetOutputDirectory( m_ui->outputEdit->text() );
	int index = m_ui->rendererComboBox->currentIndex();
	m_rendererPlugins[index]->SetOutputDirectory( m_ui->outputEdit->text() );
	bool result = m_rendererPlugins[index]->Preprocess( m_importerPlugins[importerIndex] );
	if ( result )
		m_ui->rendererLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		m_ui->rendererLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	return result;
}

void PreprocessingWindow::routerSettings()
{
	int index = m_ui->routerComboBox->currentIndex();
	m_routerPlugins[index]->ShowSettings();
}

bool PreprocessingWindow::routerPreprocessing()
{
	int importerIndex = m_ui->importerComboBox->currentIndex();
	m_importerPlugins[importerIndex]->SetOutputDirectory( m_ui->outputEdit->text() );
	int index = m_ui->routerComboBox->currentIndex();
	m_routerPlugins[index]->SetOutputDirectory( m_ui->outputEdit->text() );
	bool result = m_routerPlugins[index]->Preprocess( m_importerPlugins[importerIndex] );
	if ( result )
		m_ui->routerLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		m_ui->routerLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	m_ui->gpsLookupLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	return result;
}

void PreprocessingWindow::gpsLookupSettings()
{
	int index = m_ui->gpsLookupComboBox->currentIndex();
	m_gpsLookupPlugins[index]->ShowSettings();
}

bool PreprocessingWindow::gpsLookupPreprocessing()
{
	int importerIndex = m_ui->importerComboBox->currentIndex();
	m_importerPlugins[importerIndex]->SetOutputDirectory( m_ui->outputEdit->text() );
	int index = m_ui->gpsLookupComboBox->currentIndex();
	m_gpsLookupPlugins[index]->SetOutputDirectory( m_ui->outputEdit->text() );
	bool result = m_gpsLookupPlugins[index]->Preprocess( m_importerPlugins[importerIndex] );
	if ( result )
		m_ui->gpsLookupLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		m_ui->gpsLookupLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	return result;
}

void PreprocessingWindow::addressLookupSettings()
{
	int index = m_ui->addressLookupComboBox->currentIndex();
	m_addressLookupPlugins[index]->ShowSettings();
}

bool PreprocessingWindow::addressLookupPreprocessing()
{
	int importerIndex = m_ui->importerComboBox->currentIndex();
	m_importerPlugins[importerIndex]->SetOutputDirectory( m_ui->outputEdit->text() );
	int index = m_ui->addressLookupComboBox->currentIndex();
	m_addressLookupPlugins[index]->SetOutputDirectory( m_ui->outputEdit->text() );
	bool result = m_addressLookupPlugins[index]->Preprocess( m_importerPlugins[importerIndex] );
	if ( result )
		m_ui->addressLookupLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		m_ui->addressLookupLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	return result;
}

void PreprocessingWindow::preprocessAll()
{
	qDebug() << "===Importer===";
	if ( !importerPreprocessing() )
		return;
	QCoreApplication::processEvents();
	qDebug() << "===Router===";
	if ( !routerPreprocessing() )
		return;
	QCoreApplication::processEvents();
	qDebug() << "===Renderer===";
	if ( !rendererPreprocessing() )
		return;
	QCoreApplication::processEvents();
	qDebug() << "===GPS Lookup===";
	if ( !gpsLookupPreprocessing() )
		return;
	QCoreApplication::processEvents();
	qDebug() << "===Address Lookup===";
	if ( !addressLookupPreprocessing() )
		return;
}

void PreprocessingWindow::writeConfig()
{
	QDir dir( m_ui->outputEdit->text() );
	QSettings pluginSettings( dir.filePath( "plugins.ini" ), QSettings::IniFormat );
	pluginSettings.setValue( "router", m_ui->routerComboBox->currentText() );
	pluginSettings.setValue( "renderer", m_ui->rendererComboBox->currentText() );
	pluginSettings.setValue( "gpsLookup", m_ui->gpsLookupComboBox->currentText() );
	pluginSettings.setValue( "addressLookup", m_ui->addressLookupComboBox->currentText() );
	QMessageBox::information( this, "Config", "Plugin config file plugins.ini has been written" );
}

void PreprocessingWindow::deleteTemporary()
{
	int index = m_ui->importerComboBox->currentIndex();
	m_importerPlugins[index]->SetOutputDirectory( m_ui->outputEdit->text() );
	m_importerPlugins[index]->DeleteTemporaryFiles();
	QMessageBox::information( this, "Temporary Files", "Deleted temporary files" );
}

void PreprocessingWindow::manual()
{
	QDesktopServices::openUrl( QUrl( "http://code.google.com/p/monav/" ) );
}
