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

#include "mapdatawidget.h"
#include "ui_mapdatawidget.h"

#include <QFile>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

MapDataWidget::MapDataWidget( QWidget *parent ) :
		QDialog( parent ),
		m_ui( new Ui::MapDataWidget )
{
	m_ui->setupUi( this );
	// Windows Mobile Window Flags
	setWindowFlags( windowFlags() & ( ~Qt::WindowOkButtonHint ) );
	setWindowFlags( windowFlags() | Qt::WindowCancelButtonHint );

	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Map Data Widget" );

	restoreGeometry( settings.value( "geometry", saveGeometry() ).toByteArray() );
	m_lastRoutingModule = settings.value( "routing" ).toString();
	m_lastRenderingModule = settings.value( "rendering" ).toString();
	m_lastAddressLookupModule = settings.value( "addressLookup" ).toString();
	QStringList directories = settings.value( "directories" ).toStringList();
	foreach ( QString directory, directories ) {
		if ( QFile::exists( directory ) )
			m_directories.push_back( directory );
	}

	m_ui->directory->addItems( directories );

	QString path = settings.value( "dataDirectory", QDir::homePath() ).toString();

	int index = m_ui->directory->findText( path );
	if ( index == -1 )
		m_ui->directory->lineEdit()->setText( path );
	else
		m_ui->directory->setCurrentIndex( index );

	m_ui->name->hide();

	connectSlots();
}

MapDataWidget::~MapDataWidget()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Map Data Widget" );
	settings.setValue( "geometry", saveGeometry() );
	settings.setValue( "routing", m_lastRoutingModule );
	settings.setValue( "rendering", m_lastRenderingModule );
	settings.setValue( "addressLookup", m_lastAddressLookupModule );
	settings.setValue( "directories", m_directories );
	settings.setValue( "dataDirectory", MapData::instance()->path() );
	delete m_ui;
}

void MapDataWidget::connectSlots()
{
	connect( m_ui->directory, SIGNAL(editTextChanged(QString)), this, SLOT(directoryChanged(QString)) );
	connect( m_ui->routing, SIGNAL(currentIndexChanged(int)), this, SLOT(modulesChanged()) );
	connect( m_ui->rendering, SIGNAL(currentIndexChanged(int)), this, SLOT(modulesChanged()) );
	connect( m_ui->addressLookup, SIGNAL(currentIndexChanged(int)), this, SLOT(modulesChanged()) );
	connect( m_ui->browse, SIGNAL(clicked()), this, SLOT(browse()) );
	connect( m_ui->load, SIGNAL(clicked()), this, SLOT(load()) );
}

int MapDataWidget::exec( bool autoLoad )
{
	MapData* mapData = MapData::instance();
	QString oldPath = mapData->path();

	if ( autoLoad ) {
		if ( !mapData->loaded() ) {
			directoryChanged( m_ui->directory->currentText() );
			int routingIndex = findModule( m_routingModules, m_lastRoutingModule );
			int renderingIndex = findModule( m_renderingModules, m_lastRenderingModule );
			int addressLookupIndex = findModule( m_addressLookupModules, m_lastAddressLookupModule );
			if ( routingIndex != -1 &&
				  renderingIndex != -1 &&
				  addressLookupIndex != -1 &&
				  mapData->containsMapData() &&
				  mapData->informationLoaded() &&
				  load() )
				return QDialog::Accepted;
		}
	}
	directoryChanged( m_ui->directory->currentText() );
	int result = QDialog::exec();
	if ( result != QDialog::Accepted || !mapData->loaded() ) {
		mapData->setPath( oldPath );
		if ( mapData->containsMapData() && mapData->loadInformation() )
			load();
	}
	return result;
}

int MapDataWidget::findModule( const QVector< MapData::Module >& modules, QString path )
{
	for ( int i = 0; i < modules.size(); i++ ) {
		if ( modules[i].path == path )
			return i;
	}
	return -1;
}

void MapDataWidget::directoryChanged( QString dir )
{
	MapData* mapData = MapData::instance();
	mapData->setPath( dir );
	QPalette normalPalette;
	QPalette failPalette;
	QPalette successPalette;
	failPalette.setColor( QPalette::Text, Qt::red );
	failPalette.setColor( QPalette::WindowText, Qt::red );
	successPalette.setColor( QPalette::Text, Qt::green );
	successPalette.setColor( QPalette::WindowText, Qt::green );
	if ( !QFile::exists( dir ) || !mapData->containsMapData() || !mapData->loadInformation() ) {
		m_ui->directory->setPalette( failPalette );
		m_ui->load->setDisabled( true );
		m_ui->name->hide();
		return;
	}

	m_ui->directory->setPalette( normalPalette );
	m_ui->name->setWindowTitle( mapData->name() );
	m_ui->image->setPixmap( QPixmap::fromImage( mapData->image().scaled( QSize( m_ui->image->size() ), Qt::KeepAspectRatio ) ) );

	m_routingModules = mapData->modules( MapData::Routing );
	m_renderingModules = mapData->modules( MapData::Rendering );
	m_addressLookupModules = mapData->modules( MapData::AddressLookup );

	m_ui->routing->clear();
	m_ui->rendering->clear();
	m_ui->addressLookup->clear();

	foreach( const MapData::Module& module, m_routingModules )
		m_ui->routing->addItem( module.name );
	foreach( const MapData::Module& module, m_renderingModules )
		m_ui->rendering->addItem( module.name );
	foreach( const MapData::Module& module, m_addressLookupModules )
		m_ui->addressLookup->addItem( module.name );

	int routingIndex = findModule( m_routingModules, m_lastRoutingModule );
	int renderingIndex = findModule( m_renderingModules, m_lastRoutingModule );
	int addressLookupIndex = findModule( m_addressLookupModules, m_lastRoutingModule );

	m_ui->routing->setCurrentIndex( std::max( routingIndex, 0 ) );
	m_ui->rendering->setCurrentIndex( std::max( renderingIndex, 0 ) );
	m_ui->addressLookup->setCurrentIndex( std::max( addressLookupIndex, 0 ) );

	m_ui->name->show();
}

void MapDataWidget::modulesChanged()
{
	if ( m_ui->routing->currentIndex() != -1 &&
		  m_ui->rendering->currentIndex() != -1 &&
		  m_ui->addressLookup->currentIndex() != -1 )
		m_ui->load->setEnabled( true );
	else {
		m_ui->load->setEnabled( false );
	}
}

void MapDataWidget::showEvent( QShowEvent* /*event*/ )
{
	directoryChanged( MapData::instance()->path() );
}

void MapDataWidget::resizeEvent( QResizeEvent* /*event*/  )
{
	if ( MapData::instance()->informationLoaded() )
		m_ui->image->setPixmap( QPixmap::fromImage( MapData::instance()->image().scaled( QSize( m_ui->image->size() ), Qt::KeepAspectRatio ) ) );
}

void MapDataWidget::browse()
{
	QString dir = m_ui->directory->currentText();
	dir = QFileDialog::getExistingDirectory( this, tr( "Open Map Data Directory" ), dir );
	if ( !dir.isEmpty() )
		m_ui->directory->lineEdit()->setText( dir );
}

bool MapDataWidget::load()
{
	bool success = MapData::instance()->load(
			m_routingModules[m_ui->routing->currentIndex()],
			m_renderingModules[m_ui->rendering->currentIndex()],
			m_addressLookupModules[m_ui->addressLookup->currentIndex()] );
	if ( !success ) {
		QMessageBox::warning( this, "Loading Map Data", "Could Not Load Map Data" );
		return false;
	}
	m_lastRoutingModule = m_routingModules[m_ui->routing->currentIndex()].path;
	m_lastRenderingModule = m_renderingModules[m_ui->rendering->currentIndex()].path;
	m_lastAddressLookupModule = m_addressLookupModules[m_ui->addressLookup->currentIndex()].path;
	QString path = MapData::instance()->path();
	if ( !m_directories.contains( path ) ) {
		m_directories.push_back( path );
		m_directories.sort();
	}
	accept();
	return true;
}

