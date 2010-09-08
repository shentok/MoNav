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

#include "oisettingsdialog.h"
#include "ui_oisettingsdialog.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QTextStream>
#include <QSettings>
#include <QtDebug>
#include <cassert>
#include "utils/qthelpers.h"

OISettingsDialog::OISettingsDialog(QWidget *parent) :
	 QDialog(parent),
	 m_ui(new Ui::OISettingsDialog)
{
	m_ui->setupUi(this);
	m_ui->speedTable->resizeColumnsToContents();

	QSettings settings( "MoNav" );
	settings.beginGroup( "OSM Importer" );
	m_ui->inputEdit->setText( settings.value( "inputFile" ).toString() );
	m_lastFilename = settings.value( "lastProfile" ).toString();
	m_speedProfiles = settings.value( "SpeedProfiles" ).toStringList();

	QDir speedProfilesDir( ":/speed profiles" );
	QStringList includedSpeedProfiles = speedProfilesDir.entryList();
	for ( int i = 0; i < includedSpeedProfiles.size(); i++ ) {
		QString filename = speedProfilesDir.filePath( includedSpeedProfiles[i] );
		QString name = load( filename, true );
		assert( !name.isEmpty() );
		m_ui->speedProfileChooser->addItem( name, filename );
	}

	QStringList validSpeedProfiles;
	for ( int i = 0; i < m_speedProfiles.size(); i++ ) {
		QString name = load( m_speedProfiles[i], true );
		if( name.isEmpty() )
			continue;
		m_ui->speedProfileChooser->addItem( "File: " + name, m_speedProfiles[i] );
		validSpeedProfiles.push_back( m_speedProfiles[i] );
	}
	m_speedProfiles = validSpeedProfiles;

	QString lastName = load( m_lastFilename, true );
	if ( !lastName.isEmpty() )
		load( m_lastFilename );
	else
		load( speedProfilesDir.filePath( includedSpeedProfiles.first() ) );

	m_ui->speedProfileChooser->setCurrentIndex( m_ui->speedProfileChooser->findData( m_lastFilename ) );

	connectSlots();
}

void OISettingsDialog::connectSlots()
{
	connect( m_ui->browseButton, SIGNAL(clicked()), this, SLOT(browse()) );
	connect( m_ui->addRowButton, SIGNAL(clicked()), this, SLOT(addSpeed()) );
	connect( m_ui->deleteEntryButton, SIGNAL(clicked()), this, SLOT(removeSpeed()) );
	connect( m_ui->saveButton, SIGNAL(clicked()), this, SLOT(save()) );
	connect( m_ui->loadButton, SIGNAL(clicked()), this, SLOT(load()) );
	connect( m_ui->speedProfileChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)) );
}

OISettingsDialog::~OISettingsDialog()
{
	QSettings settings( "MoNav" );
	settings.beginGroup( "OSM Importer" );
	settings.setValue( "inputFile", m_ui->inputEdit->text()  );
	settings.setValue( "lastProfile", m_lastFilename );
	settings.setValue( "SpeedProfiles", m_speedProfiles );
	delete m_ui;
}

void OISettingsDialog::changeEvent(QEvent *e)
{
	 QDialog::changeEvent(e);
	 switch (e->type()) {
	 case QEvent::LanguageChange:
		  m_ui->retranslateUi(this);
		  break;
	 default:
		  break;
	 }
}

void OISettingsDialog::addSpeed() {
	int rows = m_ui->speedTable->rowCount();
	m_ui->speedTable->setRowCount( rows + 1 );
}

void OISettingsDialog::removeSpeed() {
	if ( m_ui->speedTable->rowCount() == 0 )
		return;

	int row = m_ui->speedTable->currentRow();
	m_ui->speedTable->removeRow( row );
}

void OISettingsDialog::save( const QString& filename, QString name )
{
	QSettings settings( filename, QSettings::IniFormat );
	settings.clear();

	settings.setValue( "name", name );
	settings.setValue( "trafficLightPenalty", m_ui->trafficLightPenalty->value() );
	settings.setValue( "defaultCitySpeed", m_ui->setDefaultCitySpeed->isChecked() );
	settings.setValue( "ignoreOneway", m_ui->ignoreOneway->isChecked() );
	settings.setValue( "ignoreMaxspeed", m_ui->ignoreMaxspeed->isChecked() );

	QList< QTreeWidgetItem* > items = m_ui->accessTree->selectedItems();

	if ( items.size() != 1 ) {
		qCritical() << "no access type selected";
		return;
	}

	settings.setValue( "accessType", items.first()->text( 0 ) );

	int rowCount = m_ui->speedTable->rowCount();
	int colCount = m_ui->speedTable->columnCount();
	settings.setValue( "entries", rowCount );
	for ( int row = 0; row < rowCount; ++row ) {
		QStringList entry;
		for ( int col = 0; col < colCount; col++ ) {
			if ( m_ui->speedTable->item( row, col ) == NULL ) {
				qCritical() << "speed profile table is missing some entries";
				return;
			}
			entry.push_back( m_ui->speedTable->item( row, col )->text() );
		}
		settings.setValue( QString( "entry.%1" ).arg( row ), entry );
	}

	if ( !settings.status() == QSettings::NoError ) {
		qCritical() << "error accessing file:" << filename;
		return;
	}

	load( filename );
}

QString OISettingsDialog::load( const QString& filename, bool nameOnly)
{

	QSettings settings( filename, QSettings::IniFormat );

	QString name = settings.value( "name" ).toString();

	if ( nameOnly )
		return name;

	m_ui->trafficLightPenalty->setValue( settings.value( "trafficLightPenalty" ).toInt() );
	m_ui->setDefaultCitySpeed->setChecked( settings.value( "defaultCitySpeed" ).toBool() );
	m_ui->ignoreOneway->setChecked( settings.value( "ignoreOneway" ).toBool() );
	m_ui->ignoreMaxspeed->setChecked( settings.value( "ignoreMaxspeed" ).toBool() );

	QString accessType = settings.value( "accessType" ).toString();
	QList< QTreeWidgetItem* > items = m_ui->accessTree->findItems( accessType, Qt::MatchFixedString | Qt::MatchRecursive );

	if ( items.size() < 1 ) {
		qCritical() << "invalid access type found:" << accessType;
		return "";
	}

	foreach( QTreeWidgetItem* item, m_ui->accessTree->selectedItems() )
		item->setSelected( false );
	items.first()->setSelected( true );
	m_ui->accessTree->expandAll();

	int rowCount = settings.value( "entries" ).toInt();
	int colCount = m_ui->speedTable->columnCount();
	m_ui->speedTable->setRowCount( 0 );
	m_ui->speedTable->setRowCount( rowCount );
	for ( int row = 0; row < rowCount; ++row ) {
		QStringList entry = settings.value( QString( "entry.%1" ).arg( row ) ).toStringList();

		if ( entry.size() != colCount ) {
			qCritical() << "invalid speed profile entry:" << entry;
			return "";
		}

		for ( int col = 0; col < colCount; col++ ) {
			m_ui->speedTable->setItem( row, col, new QTableWidgetItem );
			m_ui->speedTable->item( row, col )->setText( entry[col] );
		}
	}

	if ( !settings.status() == QSettings::NoError ) {
		qCritical() << "error accessing file:" << filename << settings.status();
		return "";
	}

	if ( !filename.startsWith( ":/" ) && !m_speedProfiles.contains( filename ) ) {
		m_speedProfiles.push_back( filename );
		m_ui->speedProfileChooser->addItem( "File: " + name, filename );
		m_ui->speedProfileChooser->setCurrentIndex( m_ui->speedProfileChooser->findData( filename ) );
	}
	qDebug() << "OSM Importer:: loaded speed profile:" << name << "," << filename;
	m_lastFilename = filename;
	return name;
}

void OISettingsDialog::save()
{
	QString filename = QFileDialog::getSaveFileName( this, tr( "Enter Speed Profile Filename" ), "", "*.spp" );
	if ( filename.isEmpty() )
		return;

	QString name = QInputDialog::getText( this, "Save Speed Profile", "Enter Speed Profile Name" );
	if ( name.isEmpty() )
		return;

	save( filename, name );
}

void OISettingsDialog::load()
{
	QString filename = QFileDialog::getOpenFileName( this, tr( "Enter Speed Filename" ), "", "*.spp" );
	if ( filename.isEmpty() )
		return;

	QString name = load( filename );
	if ( name.isEmpty() )
		return;
}

void OISettingsDialog::browse() {
	QString file = m_ui->inputEdit->text();
	file = QFileDialog::getOpenFileName( this, tr("Enter OSM XML Filename"), file, "*.osm *osm.bz2" );
	if ( file.isEmpty() )
		return;

	m_ui->inputEdit->setText( file );
}

bool OISettingsDialog::getSettings( Settings* settings )
{
	if ( settings == NULL )
		return false;

	settings->accessList.clear();
	settings->defaultCitySpeed = m_ui->setDefaultCitySpeed->isChecked();
	settings->trafficLightPenalty = m_ui->trafficLightPenalty->value();
	settings->input = m_ui->inputEdit->text();
	settings->ignoreOneway = m_ui->ignoreOneway->isChecked();
	settings->ignoreMaxspeed = m_ui->ignoreMaxspeed->isChecked();

	int rowCount = m_ui->speedTable->rowCount();
	int colCount = m_ui->speedTable->columnCount();

	if ( colCount != 4 )
		return false;

	settings->speedProfile.names.clear();
	settings->speedProfile.speed.clear();
	settings->speedProfile.speedInCity.clear();
	settings->speedProfile.averagePercentage.clear();

	for ( int row = 0; row < rowCount; ++row ) {
		for ( int i = 0; i < colCount; i++ ) {
			if ( m_ui->speedTable->item( row, i ) == NULL ) {
				qCritical() << tr( "Missing entry in speed profile table" );
				return false;
			}
		}
		bool ok = true;
		settings->speedProfile.names.push_back( m_ui->speedTable->item( row, 0 )->text() );
		settings->speedProfile.speed.push_back( m_ui->speedTable->item( row, 1 )->text().toInt( &ok ) );
		if ( !ok ) {
			qCritical() << "speed table contains invalid entries:" << m_ui->speedTable->item( row, 1 )->text();
			return false;
		}
		settings->speedProfile.speedInCity.push_back( m_ui->speedTable->item( row, 2 )->text().toInt( &ok ) );
		if ( !ok ) {
			qCritical() << "speed table contains invalid entries:" << m_ui->speedTable->item( row, 1 )->text();
			return false;
		}
		settings->speedProfile.averagePercentage.push_back( m_ui->speedTable->item( row, 3 )->text().toInt( &ok ) );
		if ( !ok ) {
			qCritical() << "speed table contains invalid entries:" << m_ui->speedTable->item( row, 1 )->text();
			return false;
		}
	}

	QList< QTreeWidgetItem* > items = m_ui->accessTree->selectedItems();
	if ( items.size() != 1 ) {
		qCritical() << tr( "no access type selected" );
		return false;
	}

	QTreeWidgetItem* item = items.first();
	do {
		qDebug() << "OSM Importer: access list:" << settings->accessList.size() << ":" << item->text( 0 );
		settings->accessList.push_back( item->text( 0 ) );
		item = item->parent();
	} while ( item != NULL );

	return true;
}

void OISettingsDialog::currentIndexChanged( int index )
{
	if ( index == -1 )
		return;
	QString filename = m_ui->speedProfileChooser->itemData( index ).toString();
	load( filename );
}
