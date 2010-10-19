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
#include "mapdata.h"

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

	m_ui->table->hide();

	connectSlots();
}

MapDataWidget::~MapDataWidget()
{
	QSettings settings( "MoNavClient" );
	settings.setValue( "directories", m_directories );
	settings.setValue( "dataDirectory", MapData::instance()->path() );
	delete m_ui;
}

void MapDataWidget::connectSlots()
{
	connect( m_ui->directory, SIGNAL(editTextChanged(QString)), this, SLOT(directoryChanged(QString)) );
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
			if ( mapData->containsMapData() && mapData->canBeLoaded() && mapData->load() )
						return QDialog::Accepted;
		}
	}
	directoryChanged( m_ui->directory->currentText() );
	int result = QDialog::exec();
	if ( result != QDialog::Accepted || !mapData->loaded() ) {
		mapData->setPath( oldPath );
		if ( mapData->containsMapData() && mapData->canBeLoaded() )
			mapData->load();
	}
	return result;
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
		m_ui->details->setEnabled( false );
		m_ui->load->setDisabled( true );
		return;
	}

	m_ui->details->setEnabled( true );

	m_ui->directory->setPalette( normalPalette );
	m_ui->name->setText( mapData->name() );
	m_ui->description->setText( mapData->description() );
	m_ui->image->setPixmap( QPixmap::fromImage( mapData->image().scaled( QSize( m_ui->image->size() ), Qt::KeepAspectRatio ) ) );

	m_ui->rendererName->setText( mapData->pluginName( MapData::Renderer ) );
	m_ui->routerName->setText( mapData->pluginName( MapData::Router ) );
	m_ui->addressLookupName->setText( mapData->pluginName( MapData::AddressLookup ) );
	m_ui->gpsLookupName->setText( mapData->pluginName( MapData::GPSLookup ) );

	if ( mapData->pluginPresent( MapData::Renderer ) )
		m_ui->rendererName->setPalette( successPalette );
	else
		m_ui->rendererName->setPalette( failPalette );
	if ( mapData->pluginPresent( MapData::Router ) )
		m_ui->routerName->setPalette( successPalette );
	else
		m_ui->routerName->setPalette( failPalette );
	if ( mapData->pluginPresent( MapData::AddressLookup ) )
		m_ui->addressLookupName->setPalette( successPalette );
	else
		m_ui->addressLookupName->setPalette( failPalette );
	if ( mapData->pluginPresent( MapData::GPSLookup ) )
		m_ui->gpsLookupName->setPalette( successPalette );
	else
		m_ui->gpsLookupName->setPalette( failPalette );

	m_ui->rendererFileFormatVersion->setText( QString::number( mapData->fileFormatVersion( MapData::Renderer ) ) );
	m_ui->routerFileFormatVersion->setText( QString::number( mapData->fileFormatVersion( MapData::Router ) ) );
	m_ui->addressLookupFileFormatVersion->setText( QString::number( mapData->fileFormatVersion( MapData::AddressLookup ) ) );
	m_ui->gpsLookupFileFormatVersion->setText( QString::number( mapData->fileFormatVersion( MapData::GPSLookup ) ) );

	if ( mapData->fileFormatCompatible( MapData::Renderer ) )
		m_ui->rendererFileFormatVersion->setPalette( successPalette );
	else
		m_ui->rendererFileFormatVersion->setPalette( failPalette );
	if ( mapData->fileFormatCompatible( MapData::Router ) )
		m_ui->routerFileFormatVersion->setPalette( successPalette );
	else
		m_ui->routerFileFormatVersion->setPalette( failPalette );
	if ( mapData->fileFormatCompatible( MapData::AddressLookup ) )
		m_ui->addressLookupFileFormatVersion->setPalette( successPalette );
	else
		m_ui->addressLookupFileFormatVersion->setPalette( failPalette );
	if ( mapData->fileFormatCompatible( MapData::GPSLookup ) )
		m_ui->gpsLookupFileFormatVersion->setPalette( successPalette );
	else
		m_ui->gpsLookupFileFormatVersion->setPalette( failPalette );

	m_ui->load->setEnabled( true );
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

void MapDataWidget::load()
{
	if ( !MapData::instance()->load() ) {
		QMessageBox::warning( this, "Loading Map Data", "Could Not Load Map Data" );
		return;
	}
	QString path = MapData::instance()->path();
	if ( !m_directories.contains( path ) ) {
		m_directories.push_back( path );
		m_directories.sort();
	}
	accept();
}

