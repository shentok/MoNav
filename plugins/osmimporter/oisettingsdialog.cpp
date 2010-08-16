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
#include <QMessageBox>
#include <QTextStream>
#include <QSettings>
#include <QtDebug>

OISettingsDialog::OISettingsDialog(QWidget *parent) :
    QDialog(parent),
	 ui(new Ui::OISettingsDialog)
{
	ui->setupUi(this);
	ui->speedTable->resizeColumnsToContents();

	connectSlots();

	QDir appDir( QApplication::applicationDirPath() );
	QString defaultFilename = appDir.filePath( "default.spp" );
	load( defaultFilename );

	QSettings settings( "MoNav" );
	settings.beginGroup( "OSM Importer" );
	ui->inputEdit->setText( settings.value( "inputFile" ).toString() );
	ui->trafficLightPenalty->setValue( settings.value( "trafficLightPenalty", 1 ).toInt() );
	ui->setDefaultCitySpeed->setChecked( settings.value( "defaultCitySpeed", true ).toBool() );
	ui->ignoreOneway->setChecked( settings.value( "ignoreOneway", false ).toBool() );
	ui->ignoreMaxspeed->setChecked( settings.value( "ignoreMaxspeed", false ).toBool() );
	QString accessType = settings.value( "accessType", "motorcar" ).toString();
	QList< QTreeWidgetItem* > items = ui->accessTree->findItems( accessType, Qt::MatchFixedString | Qt::MatchRecursive );
	if ( items.size() > 0 )
		items.first()->setSelected( true );
	ui->accessTree->expandAll();
}

void OISettingsDialog::connectSlots()
{
	connect( ui->browseButton, SIGNAL(clicked()), this, SLOT(browse()) );
	connect( ui->addRowButton, SIGNAL(clicked()), this, SLOT(addSpeed()) );
	connect( ui->deleteEntryButton, SIGNAL(clicked()), this, SLOT(removeSpeed()) );
	connect( ui->defaultButton, SIGNAL(clicked()), this, SLOT(setDefaultSpeed()) );
	connect( ui->saveButton, SIGNAL(clicked()), this, SLOT(saveSpeed()) );
	connect( ui->loadButton, SIGNAL(clicked()), this, SLOT(loadSpeed()) );
}

OISettingsDialog::~OISettingsDialog()
{
	QSettings settings( "MoNav" );
	settings.beginGroup( "OSM Importer" );
	settings.setValue( "inputFile", ui->inputEdit->text()  );
	settings.setValue( "trafficLightPenalty", ui->trafficLightPenalty->value() );
	settings.setValue( "defaultCitySpeed", ui->setDefaultCitySpeed->isChecked() );
	settings.setValue( "ignoreOneway", ui->ignoreOneway->isChecked() );
	settings.setValue( "ignoreMaxspeed", ui->ignoreMaxspeed->isChecked() );
	QList< QTreeWidgetItem* > items = ui->accessTree->selectedItems();
	if ( items.size() == 1 )
		settings.setValue( "accessType", items.first()->text( 0 ) );
	delete ui;
}

void OISettingsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void OISettingsDialog::setDefaultSpeed() {
}

void OISettingsDialog::addSpeed() {
	int rows = ui->speedTable->rowCount();
	ui->speedTable->setRowCount( rows + 1 );
}

void OISettingsDialog::removeSpeed() {
	if ( ui->speedTable->rowCount() == 0 )
		return;

	int row = ui->speedTable->currentRow();
	ui->speedTable->removeRow( row );
}

void OISettingsDialog::save( const QString& filename )
{
	QFile file( filename );
	if ( !file.open( QIODevice::WriteOnly ) ) {
		QMessageBox::warning( this, tr( "Save Speed Profile" ), tr( "Cannot write to file %1:\n%2" ).arg( file.fileName() ).arg( file.errorString() ) );
		return;
	}

	QTextStream out( &file );
	int rowCount = ui->speedTable->rowCount();
	int colCount = ui->speedTable->columnCount();
	for ( int row = 0; row < rowCount; ++row ) {
		bool valid = true;
		for ( int i = 0; i < colCount; i++ )
		{
			if ( ui->speedTable->item( row, i ) == NULL )
				valid = false;
		}
		if ( !valid )
			continue;
		for ( int i = 0; i < colCount - 1; i++ )
			out << ui->speedTable->item( row, i )->text() << "\t";

		out << ui->speedTable->item( row, colCount - 1 )->text() << "\n";
	}
}

void OISettingsDialog::load( const QString&filename )
{
	QFile file( filename );
	if ( !file.open( QIODevice::ReadOnly ) ) {
		QMessageBox::warning( this, tr( "Load Speed Profile" ), tr( "Cannot read from file %1:\n%2" ).arg( file.fileName() ).arg( file.errorString() ) );
		return;
	}

	QTextStream in( &file );
	int rowCount = 0;
	int colCount = ui->speedTable->columnCount();
	while ( !in.atEnd() ) {
		rowCount++;
		ui->speedTable->setRowCount( rowCount );
		QString text = in.readLine();
		QStringList entries = text.split( '\t' );
		if ( entries.size() != colCount )
			continue;
		for ( int i = 0; i < colCount; i++ )
		{
			if ( ui->speedTable->item( rowCount - 1, i ) == NULL )
				ui->speedTable->setItem( rowCount - 1, i, new QTableWidgetItem );
		}
		for ( int i = 0; i < colCount; i++ )
		{
			ui->speedTable->item( rowCount - 1, i )->setText( entries[i] );
		}
	}
}

void OISettingsDialog::saveSpeed()
{
	QString filename = QFileDialog::getSaveFileName( this, tr( "Enter Speed Filename" ), "", "*.spp" );
	save( filename );
}

void OISettingsDialog::loadSpeed()
{
	QString filename = QFileDialog::getOpenFileName( this, tr( "Enter Speed Filename" ), "", "*.spp" );
	load( filename );
}

void OISettingsDialog::browse() {
	QString file = ui->inputEdit->text();
	file = QFileDialog::getOpenFileName( this, tr("Enter OSM XML Filename"), file, "*.osm *osm.bz2" );
	if ( file != "" )
		ui->inputEdit->setText( file );
}

bool OISettingsDialog::getSettings( Settings* settings )
{
	if ( settings == NULL )
		return false;
	settings->accessList.clear();
	settings->defaultCitySpeed = ui->setDefaultCitySpeed->isChecked();
	settings->trafficLightPenalty = ui->trafficLightPenalty->value();
	settings->input = ui->inputEdit->text();
	settings->ignoreOneway = ui->ignoreOneway->isChecked();
	settings->ignoreMaxspeed = ui->ignoreMaxspeed->isChecked();

	int rowCount = ui->speedTable->rowCount();
	int colCount = ui->speedTable->columnCount();

	if ( colCount != 4 )
		return false;

	settings->speedProfile.names.clear();
	settings->speedProfile.speed.clear();
	settings->speedProfile.speedInCity.clear();
	settings->speedProfile.averagePercentage.clear();

	for ( int row = 0; row < rowCount; ++row ) {
		for ( int i = 0; i < colCount; i++ )
		{
			if ( ui->speedTable->item( row, i ) == NULL ) {
				qCritical() << tr( "Missing entry in speed profile table" );
				return false;
			}
		}
		settings->speedProfile.names.push_back( ui->speedTable->item( row, 0 )->text() );
		settings->speedProfile.speed.push_back( ui->speedTable->item( row, 1 )->text().toInt() );
		settings->speedProfile.speedInCity.push_back( ui->speedTable->item( row, 2 )->text().toInt() );
		settings->speedProfile.averagePercentage.push_back( ui->speedTable->item( row, 3 )->text().toInt() );
	}

	QList< QTreeWidgetItem* > items = ui->accessTree->selectedItems();
	if ( items.size() != 1 ) {
		qCritical() << tr( "No Access Type selected" );
		return false;
	}
	QTreeWidgetItem* item = items.first();
	do {
		qDebug() << settings->accessList.size() << ": " << item->text( 0 );
		settings->accessList.push_back( item->text( 0 ) );
		item = item->parent();
	} while ( item != NULL );

	return true;
}
