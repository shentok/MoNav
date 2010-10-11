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
#include "utils/qthelpers.h"

#include <omp.h>
#include <QFileDialog>
#include <QSettings>
#include <QtDebug>
#include <QDir>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

PreprocessingWindow::PreprocessingWindow(QWidget *parent) :
	QMainWindow(parent),
	m_ui( new Ui::PreprocessingWindow )
{
	m_ui->setupUi(this);

	connectSlots();
	loadPlugins();

	QSettings settings( "MoNav" );
	settings.beginGroup( "Preprocessing" );
	m_ui->output->setText( settings.value( "output" ).toString() );

	QString importerName = settings.value( "importer", m_ui->importerComboBox->currentText() ).toString();
	QString routerName = settings.value( "router", m_ui->routerComboBox->currentText() ).toString();
	QString rendererName = settings.value( "renderer", m_ui->rendererComboBox->currentText() ).toString();
	QString gpsLookupName = settings.value( "gpsLookup", m_ui->gpsLookupComboBox->currentText() ).toString();
	QString addressLookupName = settings.value( "addressLookup", m_ui->addressLookupComboBox->currentText() ).toString();

	m_ui->importerComboBox->setCurrentIndex( m_ui->importerComboBox->findText( importerName ) );
	m_ui->routerComboBox->setCurrentIndex( m_ui->routerComboBox->findText( routerName ) );
	m_ui->rendererComboBox->setCurrentIndex( m_ui->rendererComboBox->findText( rendererName ) );
	m_ui->gpsLookupComboBox->setCurrentIndex( m_ui->gpsLookupComboBox->findText( gpsLookupName ) );
	m_ui->addressLookupComboBox->setCurrentIndex( m_ui->addressLookupComboBox->findText( addressLookupName ) );

	m_ui->name->setText( settings.value( "name", tr( "MyMap" ) ).toString() );
	m_ui->description->setText( settings.value( "description", tr( "This is my map data" ) ).toString() );
	m_ui->image->setText( settings.value( "image", ":/images/about.png" ).toString() );

	m_ui->threads->setMaximum( omp_get_max_threads() );
	m_ui->threads->setValue( settings.value( "threads", omp_get_max_threads() ).toInt() );

	restoreGeometry( settings.value( "geometry" ).toByteArray() );
	QList< int > splitterSizes;
	splitterSizes.push_back( 0 );
	splitterSizes.push_back( 1 );
	m_ui->splitter->setSizes( splitterSizes );
	m_ui->splitter->restoreState( settings.value( "splitter" ).toByteArray() );
}

PreprocessingWindow::~PreprocessingWindow()
{
	QSettings settings( "MoNav" );
	settings.beginGroup( "Preprocessing" );
	settings.setValue( "output", m_ui->output->text() );

	settings.setValue( "importer", m_ui->importerComboBox->currentText() );
	settings.setValue( "router", m_ui->routerComboBox->currentText() );
	settings.setValue( "renderer", m_ui->rendererComboBox->currentText() );
	settings.setValue( "gpsLookup", m_ui->gpsLookupComboBox->currentText() );
	settings.setValue( "addressLookup", m_ui->addressLookupComboBox->currentText() );

	settings.setValue( "name", m_ui->name->text() );
	settings.setValue( "description", m_ui->description->toPlainText() );
	settings.setValue( "image", m_ui->image->text() );

	settings.setValue( "threads", m_ui->threads->value() );

	settings.setValue( "geometry", saveGeometry() );
	settings.setValue( "splitter", m_ui->splitter->saveState() );
	unloadPlugins();
	delete m_ui;
}

void PreprocessingWindow::connectSlots()
{
	connect( m_ui->outputBrowse, SIGNAL(clicked()), this, SLOT(outputBrowse()) );
	connect( m_ui->output, SIGNAL(textChanged(QString)), this, SLOT(outputChanged(QString)) );

	connect( m_ui->imageBrowse, SIGNAL(clicked()), this, SLOT(imageBrowse()) );
	connect( m_ui->image, SIGNAL(textChanged(QString)), this, SLOT(imageChanged(QString)) );

	connect( m_ui->threads, SIGNAL(valueChanged(int)), this, SLOT(threadsChanged(int)) );

	connect( m_ui->importerPreprocessButton, SIGNAL(clicked()), this, SLOT(importerPreprocessing()) );
	connect( m_ui->rendererPreprocessButton, SIGNAL(clicked()), this, SLOT(rendererPreprocessing()) );
	connect( m_ui->routerPreprocessButton, SIGNAL(clicked()), this, SLOT(routerPreprocessing()) );
	connect( m_ui->gpsLookupPreprocessButton, SIGNAL(clicked()), this, SLOT(gpsLookupPreprocessing()) );
	connect( m_ui->addressLookupPreprocessButton, SIGNAL(clicked()), this, SLOT(addressLookupPreprocessing()) );
	connect( m_ui->deleteTemporaryButton, SIGNAL(clicked()), this, SLOT(deleteTemporary()) );
	connect( m_ui->writeConfigButton, SIGNAL(clicked()), this, SLOT(writeConfig()) );

	connect( m_ui->allPreprocessButton, SIGNAL(clicked()), this, SLOT(preprocessAll()) );
}

void PreprocessingWindow::loadPlugins()
{
	QDir pluginDir( QApplication::applicationDirPath() );
	if ( pluginDir.cd("plugins_preprocessor") ) {
		foreach ( QString fileName, pluginDir.entryList( QDir::Files ) ) {
			QPluginLoader* loader = new QPluginLoader( pluginDir.absoluteFilePath( fileName ) );
			loader->setLoadHints( QLibrary::ExportExternalSymbolsHint );
			if ( !loader->load() )
				qDebug() << loader->errorString();

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

	if ( m_importerPlugins.size() == 0  || m_rendererPlugins.size() == 0  || m_routerPlugins.size() == 0 || m_gpsLookupPlugins.size() == 0  || m_addressLookupPlugins.size() == 0 )
		qFatal( "plugin types are missing" );
}

bool PreprocessingWindow::testPlugin( QObject* plugin )
{
	bool needed = false;
	if ( IImporter *interface = qobject_cast< IImporter* >( plugin ) )
	{
		m_importerPlugins.append( interface );
		m_ui->importerComboBox->addItem( interface->GetName() );
		m_ui->importerSettings->addTab( interface->GetSettings(), interface->GetName() );
		needed = true;
	}
	if ( IPreprocessor *interface = qobject_cast< IPreprocessor* >( plugin ) )
	{
		QString name = interface->GetName();
		if ( interface->GetType() == IPreprocessor::Renderer )
		{
			m_rendererPlugins.append( interface );
			m_ui->rendererComboBox->addItem( name );
			m_ui->rendererSettings->addTab( interface->GetSettings(), interface->GetName() );
		}
		if ( interface->GetType() == IPreprocessor::Router )
		{
			m_routerPlugins.append( interface );
			m_ui->routerComboBox->addItem( name );
			m_ui->routerSettings->addTab( interface->GetSettings(), interface->GetName() );
		}
		if ( interface->GetType() == IPreprocessor::GPSLookup )
		{
			m_gpsLookupPlugins.append( interface );
			m_ui->gpsLookupComboBox->addItem( name );
			m_ui->gpsLookupSettings->addTab( interface->GetSettings(), interface->GetName() );
		}
		if ( interface->GetType() == IPreprocessor::AddressLookup )
		{
			m_addressLookupPlugins.append( interface );
			m_ui->addressLookupComboBox->addItem( name );
			m_ui->addressLookupSettings->addTab( interface->GetSettings(), interface->GetName() );
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

void PreprocessingWindow::outputBrowse()
{
	QString dir = m_ui->output->text();
	dir = QFileDialog::getExistingDirectory(this, tr( "Open Output Directory" ), dir);
	if ( !dir.isEmpty() )
		m_ui->output->setText( dir );
}

void PreprocessingWindow::imageBrowse()
{
	QString file = m_ui->image->text();
	file = QFileDialog::getOpenFileName( this, tr( "Open Image File" ), file );
	if ( !file.isEmpty() )
		m_ui->image->setText( file );
}

void PreprocessingWindow::imageChanged( QString text )
{
	bool valid = true;
	QPalette pal;
	if ( !QFile::exists( text ) ) {
		valid = false;
	} else {
		QPixmap image( text );
		if ( image.isNull() )
			valid = false;
		else
			m_ui->imagePreview->setPixmap( image );
	}
	if ( !valid ) {
		m_ui->imagePreview->clear();
		pal.setColor( QPalette::Text, Qt::red );
	}
	m_ui->image->setPalette( pal );
}

void PreprocessingWindow::outputChanged( QString text )
{
	QPalette pal;
	if ( !QFile::exists( text ) )
		pal.setColor( QPalette::Text, Qt::red );
	m_ui->output->setPalette( pal );
}

void PreprocessingWindow::threadsChanged( int threads )
{
	omp_set_num_threads( threads );
}

bool PreprocessingWindow::importerPreprocessing()
{
	int index = m_ui->importerComboBox->currentIndex();
	m_importerPlugins[index]->SetOutputDirectory( m_ui->output->text() );
	bool result = m_importerPlugins[index]->Preprocess();
	if ( result )
		m_ui->importerLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		m_ui->importerLabel->setPixmap( QPixmap( ":/images/reload.png" ) );
	m_ui->rendererLabel->setPixmap( QPixmap( ":/images/reload.png" ) );
	m_ui->routerLabel->setPixmap( QPixmap( ":/images/reload.png" ) );
	m_ui->gpsLookupLabel->setPixmap( QPixmap( ":/images/reload.png" ) );
	m_ui->addressLookupLabel->setPixmap( QPixmap( ":/images/reload.png" ) );
	return result;
}

bool PreprocessingWindow::rendererPreprocessing()
{
	int importerIndex = m_ui->importerComboBox->currentIndex();
	m_importerPlugins[importerIndex]->SetOutputDirectory( m_ui->output->text() );
	int index = m_ui->rendererComboBox->currentIndex();
	m_rendererPlugins[index]->SetOutputDirectory( m_ui->output->text() );
	bool result = m_rendererPlugins[index]->Preprocess( m_importerPlugins[importerIndex] );
	if ( result )
		m_ui->rendererLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		m_ui->rendererLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	return result;
}

bool PreprocessingWindow::routerPreprocessing()
{
	int importerIndex = m_ui->importerComboBox->currentIndex();
	m_importerPlugins[importerIndex]->SetOutputDirectory( m_ui->output->text() );
	int index = m_ui->routerComboBox->currentIndex();
	m_routerPlugins[index]->SetOutputDirectory( m_ui->output->text() );
	bool result = m_routerPlugins[index]->Preprocess( m_importerPlugins[importerIndex] );
	if ( result )
		m_ui->routerLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		m_ui->routerLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	m_ui->gpsLookupLabel->setPixmap( QPixmap( ":/images/reload.png" ) );
	return result;
}

bool PreprocessingWindow::gpsLookupPreprocessing()
{
	int importerIndex = m_ui->importerComboBox->currentIndex();
	m_importerPlugins[importerIndex]->SetOutputDirectory( m_ui->output->text() );
	int index = m_ui->gpsLookupComboBox->currentIndex();
	m_gpsLookupPlugins[index]->SetOutputDirectory( m_ui->output->text() );
	bool result = m_gpsLookupPlugins[index]->Preprocess( m_importerPlugins[importerIndex] );
	if ( result )
		m_ui->gpsLookupLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	else
		m_ui->gpsLookupLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
	return result;
}

bool PreprocessingWindow::addressLookupPreprocessing()
{
	int importerIndex = m_ui->importerComboBox->currentIndex();
	m_importerPlugins[importerIndex]->SetOutputDirectory( m_ui->output->text() );
	int index = m_ui->addressLookupComboBox->currentIndex();
	m_addressLookupPlugins[index]->SetOutputDirectory( m_ui->output->text() );
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
	if ( !importerPreprocessing() ) {
		m_ui->buildAllLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
		return;
	}
	QCoreApplication::processEvents();
	qDebug() << "===Router===";
	if ( !routerPreprocessing() ) {
		m_ui->buildAllLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
		return;
	}
	QCoreApplication::processEvents();
	qDebug() << "===Renderer===";
	if ( !rendererPreprocessing() ) {
		m_ui->buildAllLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
		return;
	}
	QCoreApplication::processEvents();
	qDebug() << "===GPS Lookup===";
	if ( !gpsLookupPreprocessing() ) {
		m_ui->buildAllLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
		return;
	}
	QCoreApplication::processEvents();
	qDebug() << "===Address Lookup===";
	if ( !addressLookupPreprocessing() ) {
		m_ui->buildAllLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
		return;
	}
	QCoreApplication::processEvents();
	qDebug() << "===Config File===";
	if ( !writeConfig() )  {
		m_ui->buildAllLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
		return;
	}
	QCoreApplication::processEvents();
	qDebug() << "===Delete Temporary Files===";
	if ( !deleteTemporary() )  {
		m_ui->buildAllLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
		return;
	}
	m_ui->buildAllLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
}

bool PreprocessingWindow::writeConfig()
{
	QSettings pluginSettings( fileInDirectory( m_ui->output->text(), "MoNav.ini" ), QSettings::IniFormat );

	pluginSettings.setValue( "configVersion", 1 );

	pluginSettings.setValue( "router", m_ui->routerComboBox->currentText() );
	pluginSettings.setValue( "renderer", m_ui->rendererComboBox->currentText() );
	pluginSettings.setValue( "gpsLookup", m_ui->gpsLookupComboBox->currentText() );
	pluginSettings.setValue( "addressLookup", m_ui->addressLookupComboBox->currentText() );

	int index = m_ui->routerComboBox->currentIndex();
	pluginSettings.setValue( "routerFileFormatVersion", m_routerPlugins[index]->GetFileFormatVersion() );
	index = m_ui->rendererComboBox->currentIndex();
	pluginSettings.setValue( "rendererFileFormatVersion", m_rendererPlugins[index]->GetFileFormatVersion() );
	index = m_ui->gpsLookupComboBox->currentIndex();
	pluginSettings.setValue( "gpsLookupFileFormatVersion", m_gpsLookupPlugins[index]->GetFileFormatVersion() );
	index = m_ui->addressLookupComboBox->currentIndex();
	pluginSettings.setValue( "addressLookupFileFormatVersion", m_addressLookupPlugins[index]->GetFileFormatVersion() );

	pluginSettings.setValue( "name", m_ui->name->text() );
	pluginSettings.setValue( "description", m_ui->description->toPlainText() );
	QImage image( m_ui->image->text() );
	if ( !image.isNull() )
		pluginSettings.setValue( "image", image );

	pluginSettings.sync();
	if ( pluginSettings.status() != QSettings::NoError ) {
		m_ui->configLabel->setPixmap( QPixmap( ":/images/notok.png" ) );
		return false;
	}

	m_ui->configLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	return true;
}

bool PreprocessingWindow::deleteTemporary()
{
	int index = m_ui->importerComboBox->currentIndex();
	m_importerPlugins[index]->SetOutputDirectory( m_ui->output->text() );
	m_importerPlugins[index]->DeleteTemporaryFiles();
	m_ui->deleteTemporaryLabel->setPixmap( QPixmap( ":/images/ok.png" ) );
	return true;
}

