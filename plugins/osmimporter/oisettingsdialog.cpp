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
#include "utils/qthelpers.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QTextStream>
#include <QSettings>
#include <QtDebug>
#include <cassert>

OISettingsDialog::OISettingsDialog(QWidget *parent) :
	QWidget(parent),
	m_ui(new Ui::OISettingsDialog)
{
	m_ui->setupUi(this);
	m_ui->speedTable->resizeColumnsToContents();
	m_ui->toolBox->widget( 2 )->setEnabled( false );
	m_ui->toolBox->widget( 3 )->setEnabled( false );
	m_ui->toolBox->widget( 4 )->setEnabled( false );
	m_ui->toolBox->widget( 5 )->setEnabled( false );
	m_ui->toolBox->widget( 6 )->setEnabled( false );

	connectSlots();
}

void OISettingsDialog::connectSlots()
{
	connect( m_ui->addWayType, SIGNAL(clicked()), this, SLOT(addSpeed()) );
	connect( m_ui->deleteWayType, SIGNAL(clicked()), this, SLOT(removeSpeed()) );
	connect( m_ui->saveButton, SIGNAL(clicked()), this, SLOT(save()) );
	connect( m_ui->loadButton, SIGNAL(clicked()), this, SLOT(load()) );
	connect( m_ui->speedProfileChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged()) );
	connect( m_ui->customProfile, SIGNAL(toggled(bool)), this, SLOT(currentIndexChanged()) );
	connect( m_ui->customProfile, SIGNAL(toggled(bool)), m_ui->toolBox->widget( 2 ), SLOT(setEnabled(bool)) );
	connect( m_ui->customProfile, SIGNAL(toggled(bool)), m_ui->toolBox->widget( 3 ), SLOT(setEnabled(bool)) );
	connect( m_ui->customProfile, SIGNAL(toggled(bool)), m_ui->toolBox->widget( 4 ), SLOT(setEnabled(bool)) );
	connect( m_ui->customProfile, SIGNAL(toggled(bool)), m_ui->toolBox->widget( 5 ), SLOT(setEnabled(bool)) );
	connect( m_ui->customProfile, SIGNAL(toggled(bool)), m_ui->toolBox->widget( 6 ), SLOT(setEnabled(bool)) );
	connect( m_ui->languagePriorities, SIGNAL(currentRowChanged(int)), this, SLOT(currentLanguageChanged(int)) );
	connect( m_ui->speedTable, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentWayTypeChanged(int,int,int,int)) );
	connect( m_ui->addLanguage, SIGNAL(clicked()), this, SLOT(addLanguage()) );
	connect( m_ui->deleteLanguage, SIGNAL(clicked()), this, SLOT(deleteLanguage()) );

	connect( m_ui->addWayModificator, SIGNAL(clicked()), this, SLOT(addWayModificator()) );
	connect( m_ui->addNodeModificator, SIGNAL(clicked()), this, SLOT(addNodeModificator()) );
}

OISettingsDialog::~OISettingsDialog()
{
	delete m_ui;
}

void OISettingsDialog::addWayModificator()
{
	WayModificatorWidget* widget = new WayModificatorWidget( this );
	m_ui->wayModificators->widget()->layout()->addWidget( widget );
	if ( isVisible() )
		widget->show();
}

void OISettingsDialog::addNodeModificator()
{
	NodeModificatorWidget* widget = new NodeModificatorWidget( this );
	m_ui->nodeModificators->widget()->layout()->addWidget( widget );
	if ( isVisible() )
		widget->show();
}

void OISettingsDialog::currentLanguageChanged ( int currentRow )
{
	m_ui->deleteLanguage->setEnabled( currentRow != -1 );
}

void OISettingsDialog::addLanguage()
{
	QListWidgetItem* item = new QListWidgetItem( "name", m_ui->languagePriorities, 0 );
	item->setFlags( item->flags () | Qt::ItemIsEditable );
}

void OISettingsDialog::deleteLanguage()
{
	delete m_ui->languagePriorities->takeItem( m_ui->languagePriorities->currentRow() );
}

void OISettingsDialog::currentWayTypeChanged( int currentRow, int /*currentCol*/, int /*lastRow*/, int /*lastCol*/ )
{
	m_ui->deleteWayType->setEnabled( currentRow != -1 );
}

void OISettingsDialog::addSpeed()
{
	int rows = m_ui->speedTable->rowCount();
	m_ui->speedTable->setRowCount( rows + 1 );
}

void OISettingsDialog::removeSpeed()
{
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

	QList< WayModificatorWidget* > wayModificators = m_ui->wayModificators->findChildren< WayModificatorWidget* >();
	settings.setValue( "wayModificatorsCount", wayModificators.size() );
	for ( int i = 0; i < wayModificators.size(); i++ ) {
		MoNav::WayModificator mod = wayModificators[i]->modificator();
		settings.setValue( QString( "wayModificator.%1.key" ).arg( i ), mod.key );
		settings.setValue( QString( "wayModificator.%1.checkValue" ).arg( i ), mod.checkValue );
		if ( mod.checkValue )
			settings.setValue( QString( "wayModificator.%1.value" ).arg( i ), mod.value );
		settings.setValue( QString( "wayModificator.%1.invert" ).arg( i ), mod.invert );
		settings.setValue( QString( "wayModificator.%1.type" ).arg( i ), ( int ) mod.type );
		settings.setValue( QString( "wayModificator.%1.modificatorValue" ).arg( i ), mod.modificatorValue );
	}

	QList< NodeModificatorWidget* > nodeModificators = m_ui->nodeModificators->findChildren< NodeModificatorWidget* >();
	settings.setValue( "nodeModificatorsCount", nodeModificators.size() );
	for ( int i = 0; i < nodeModificators.size(); i++ ) {
		MoNav::NodeModificator mod = nodeModificators[i]->modificator();
		settings.setValue( QString( "nodeModificator.%1.key" ).arg( i ), mod.key );
		settings.setValue( QString( "nodeModificator.%1.checkValue" ).arg( i ), mod.checkValue );
		if ( mod.checkValue )
			settings.setValue( QString( "nodeModificator.%1.value" ).arg( i ), mod.value );
		settings.setValue( QString( "nodeModificator.%1.invert" ).arg( i ), mod.invert );
		settings.setValue( QString( "nodeModificator.%1.type" ).arg( i ), ( int ) mod.type );
		settings.setValue( QString( "nodeModificator.%1.modificatorValue" ).arg( i ), mod.modificatorValue );
	}

	if ( !settings.status() == QSettings::NoError ) {
		qCritical() << "error accessing file:" << filename;
		return;
	}

	m_ui->customProfile->setChecked( false );
	m_ui->toolBox->setCurrentIndex( 0 );

	load( filename );
}

QString OISettingsDialog::load( const QString& filename, bool nameOnly )
{

	QSettings settings( filename, QSettings::IniFormat );

	QString name = settings.value( "name" ).toString();

	if ( nameOnly )
		return name;

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

	foreach( WayModificatorWidget* widget, m_ui->wayModificators->findChildren< WayModificatorWidget* >() )
		widget->deleteLater();

	int wayModificatorCount = settings.value( "wayModificatorsCount" ).toInt();
	for ( int i = 0; i < wayModificatorCount; i++ ) {
		MoNav::WayModificator mod;
		mod.key = settings.value( QString( "wayModificator.%1.key" ).arg( i ) ).toString();
		mod.checkValue = settings.value( QString( "wayModificator.%1.checkValue" ).arg( i ) ).toBool();
		if ( mod.checkValue )
			mod.value = settings.value( QString( "wayModificator.%1.value" ).arg( i ) ).toString();
		mod.invert = settings.value( QString( "wayModificator.%1.invert" ).arg( i ) ).toBool();
		mod.type = ( MoNav::WayModificatorType ) settings.value( QString( "wayModificator.%1.type" ).arg( i ) ).toInt();
		mod.modificatorValue = settings.value( QString( "wayModificator.%1.modificatorValue" ).arg( i ) );

		WayModificatorWidget* widget = new WayModificatorWidget( this );
		widget->setModificator( mod );
		m_ui->wayModificators->widget()->layout()->addWidget( widget );
		if ( isVisible() )
			widget->show();
	}

	foreach( NodeModificatorWidget* widget, m_ui->nodeModificators->findChildren< NodeModificatorWidget* >() )
		widget->deleteLater();

	int nodeModificatorCount = settings.value( "nodeModificatorsCount" ).toInt();
	for ( int i = 0; i < nodeModificatorCount; i++ ) {
		MoNav::NodeModificator mod;
		mod.key = settings.value( QString( "nodeModificator.%1.key" ).arg( i ) ).toString();
		mod.checkValue = settings.value( QString( "nodeModificator.%1.checkValue" ).arg( i ) ).toBool();
		if ( mod.checkValue )
			mod.value = settings.value( QString( "nodeModificator.%1.value" ).arg( i ) ).toString();
		mod.invert = settings.value( QString( "nodeModificator.%1.invert" ).arg( i ) ).toBool();
		mod.type = ( MoNav::NodeModificatorType ) settings.value( QString( "nodeModificator.%1.type" ).arg( i ) ).toInt();
		mod.modificatorValue = settings.value( QString( "nodeModificator.%1.modificatorValue" ).arg( i ) );

		NodeModificatorWidget* widget = new NodeModificatorWidget( this );
		widget->setModificator( mod );
		m_ui->nodeModificators->widget()->layout()->addWidget( widget );
		if ( isVisible() )
			widget->show();
	}

	if ( !settings.status() == QSettings::NoError ) {
		qCritical() << "error accessing file:" << filename << settings.status();
		return "";
	}

	int existingIndex = m_ui->speedProfileChooser->findData( filename );
	if ( existingIndex != -1 ) {
		if ( filename.startsWith( ":/" ) )
			m_ui->speedProfileChooser->setItemText( existingIndex, name );
		else
			m_ui->speedProfileChooser->setItemText( existingIndex, "File: " + name );
		m_ui->speedProfileChooser->setCurrentIndex( existingIndex );
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

	if ( !name.endsWith( ".spp" ) )
		name += ".spp";

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

bool OISettingsDialog::getSettings( Settings* settings )
{
	if ( settings == NULL )
		return false;

	settings->accessList.clear();
	settings->defaultCitySpeed = m_ui->setDefaultCitySpeed->isChecked();
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

	for ( int item = 0; item < m_ui->languagePriorities->count(); item++ ) {
		qDebug() << "OSM Importer: language list:" << settings->languageSettings.size() << ":" << m_ui->languagePriorities->item( item )->text();
		if ( !m_ui->languagePriorities->item( item )->text().startsWith( "name" ) ) {
			qDebug() << "language tags have to begin with 'name'";
			return false;
		}
		settings->languageSettings.append( m_ui->languagePriorities->item( item )->text() );
	}

	settings->nodeModificators.clear();
	QList< NodeModificatorWidget* > nodeModificators = m_ui->nodeModificators->findChildren< NodeModificatorWidget* >();
	for ( int i = 0; i < nodeModificators.size(); i++ )
		settings->nodeModificators.push_back( nodeModificators[i]->modificator() );

	settings->wayModificators.clear();
	QList< WayModificatorWidget* > wayModificators = m_ui->wayModificators->findChildren< WayModificatorWidget* >();
	for ( int i = 0; i < wayModificators.size(); i++ )
		settings->wayModificators.push_back( wayModificators[i]->modificator() );

	return true;
}

void OISettingsDialog::currentIndexChanged()
{
	int index = m_ui->speedProfileChooser->currentIndex();
	if ( index == -1 )
		return;
	QString filename = m_ui->speedProfileChooser->itemData( index ).toString();
	load( filename );
}

bool OISettingsDialog::loadSettings( QSettings* settings )
{
	settings->beginGroup( "OSM Importer" );
	m_speedProfiles = settings->value( "SpeedProfiles" ).toStringList();
	QStringList languageSettings = settings->value( "languageSettings", QStringList( "name:en" ) + QStringList( "name" ) ).toStringList();
	for ( int language = 0; language < languageSettings.size(); language++ ) {
		QListWidgetItem* item = new QListWidgetItem( languageSettings[language], m_ui->languagePriorities, 0 );
		item->setFlags( item->flags () | Qt::ItemIsEditable );
	}

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

	m_lastFilename = settings->value( "lastProfile" ).toString();
	QString lastName = load( m_lastFilename, true );
	if ( !lastName.isEmpty() )
		load( m_lastFilename );
	else
		load( speedProfilesDir.filePath( includedSpeedProfiles.last() ) );

	m_ui->speedProfileChooser->setCurrentIndex( m_ui->speedProfileChooser->findData( m_lastFilename ) );

	settings->endGroup();
	return true;
}

bool OISettingsDialog::saveSettings( QSettings* settings )
{
	settings->beginGroup( "OSM Importer" );
	settings->setValue( "lastProfile", m_lastFilename );
	settings->setValue( "SpeedProfiles", m_speedProfiles );
	QStringList languageSettings;
	for ( int item = 0; item < m_ui->languagePriorities->count(); item++ )
		languageSettings.append( m_ui->languagePriorities->item( item )->text() );
	settings->setValue( "languageSettings", languageSettings );
	settings->endGroup();

	if ( m_ui->customProfile->isChecked() ) {
		qCritical() << "Unsafed Custom Speed Profile";
		return false;
	}
	return true;
}
