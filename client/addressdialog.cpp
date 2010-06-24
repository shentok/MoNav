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

#include "addressdialog.h"
#include "ui_addressdialog.h"

AddressDialog::AddressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddressDialog)
{
    ui->setupUi(this);
	 addressLookup = NULL;
	 resetCity();
	 connectSlots();
}

AddressDialog::~AddressDialog()
{
    delete ui;
}

void AddressDialog::setAddressLookup( IAddressLookup* al )
{
	addressLookup = al;
	resetCity();
}

void AddressDialog::setPlaceID( int p )
{
	placeID = p;
}

void AddressDialog::connectSlots()
{
	connect( ui->cityEdit, SIGNAL(textChanged(QString)), this, SLOT(cityTextChanged(QString)) );
	connect( ui->streetEdit, SIGNAL(textChanged(QString)), this, SLOT(streetTextChanged(QString)) );
	connect( ui->suggestionList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(suggestionClicked(QListWidgetItem*)) );
	connect( ui->characterList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(characterClicked(QListWidgetItem*)) );
	connect( ui->resetCity, SIGNAL(clicked()), this, SLOT(resetCity()) );
	connect( ui->resetStreet, SIGNAL(clicked()), this, SLOT(resetStreet()) );
}

void AddressDialog::characterClicked( QListWidgetItem * item )
{
	QString text = item->text();
	if ( mode == City )
		ui->cityEdit->setText( text );
	else if ( mode == Street )
		ui->streetEdit->setText( text );
}

void AddressDialog::suggestionClicked( QListWidgetItem * item )
{
	QString text = item->text();
	if ( mode == City ) {
		if ( addressLookup->GetPlaceData( text, &placeIDs, &placeCoordinates ) )
		{
			ui->cityEdit->setText( text );
			ui->cityEdit->setDisabled( true );
			ui->streetEdit->setEnabled( true );
			ui->resetStreet->setEnabled( true );
			mode = Street;
			placeID = placeIDs[0];
			streetTextChanged( ui->streetEdit->text() );
		}
	}
	else {
		ui->streetEdit->setText( text );
		chosen = true;
		close();
	}
}

void AddressDialog::cityTextChanged( QString text )
{
	if ( addressLookup == NULL )
		return;
	ui->suggestionList->clear();
	ui->characterList->clear();
	QStringList suggestions;
	QStringList characters;
	if ( !addressLookup->GetPlaceSuggestions( text, 16, &suggestions, &characters ) )
		return;
	bool parity = false;
	foreach( QString label, suggestions ) {
		QListWidgetItem* item = new QListWidgetItem( label );
		QFont font = item->font();
		font.setPointSize( font.pointSize() * 1.5 );
		item->setFont( font );
		if ( ( parity = !parity ) )
			item->setBackgroundColor( QColor::fromHsv( 50, 10, 255 ) );
		else
			item->setBackgroundColor( QColor::fromHsv( 50, 25, 255 ) );
		ui->suggestionList->addItem( item );
	}
	parity = false;
	foreach( QString label, characters )
	{
		QListWidgetItem* item = new QListWidgetItem( label );
		QFont font = item->font();
		font.setPointSize( font.pointSize() * 1.5 );
		item->setFont( font );
		if ( ( parity = !parity ) )
			item->setBackgroundColor( QColor::fromHsv( 100, 10, 255 ) );
		else
			item->setBackgroundColor( QColor::fromHsv( 100, 25, 255 ) );
		ui->characterList->addItem( item );
	}
}

void AddressDialog::streetTextChanged( QString text)
{
	if ( addressLookup == NULL )
		return;
	if ( mode != Street )
		return;
	ui->suggestionList->clear();
	ui->characterList->clear();
	QStringList suggestions;
	QStringList characters;
	if ( !addressLookup->GetStreetSuggestions( text, 16, &suggestions, &characters ) )
		return;
	bool parity = false;
	foreach( QString label, suggestions ) {
		QListWidgetItem* item = new QListWidgetItem( label );
		QFont font = item->font();
		font.setPointSize( font.pointSize() * 1.5 );
		item->setFont( font );
		if ( ( parity = !parity ) )
			item->setBackgroundColor( QColor::fromHsv( 50, 10, 255 ) );
		else
			item->setBackgroundColor( QColor::fromHsv( 50, 25, 255 ) );
		ui->suggestionList->addItem( item );
	}
	parity = false;
	foreach( QString label, characters )
	{
		QListWidgetItem* item = new QListWidgetItem( label );
		QFont font = item->font();
		font.setPointSize( font.pointSize() * 1.5 );
		item->setFont( font );
		if ( ( parity = !parity ) )
			item->setBackgroundColor( QColor::fromHsv( 100, 10, 255 ) );
		else
			item->setBackgroundColor( QColor::fromHsv( 100, 25, 255 ) );
		ui->characterList->addItem( item );
	}
}

void AddressDialog::resetCity()
{
	ui->cityEdit->setEnabled( true );
	ui->resetCity->setEnabled( true );
	ui->streetEdit->setDisabled( true );
	ui->resetStreet->setDisabled( true );
	ui->streetEdit->setText( "" );
	ui->cityEdit->setText( "" );
	mode = City;
	chosen = false;
	cityTextChanged( "" );
}

void AddressDialog::resetStreet()
{
	ui->streetEdit->setText( "" );
	streetTextChanged( "" );
	chosen = false;
}

bool AddressDialog::wasSuccessfull( std::vector< int >* segmentLengths, std::vector< UnsignedCoordinate >* coordinates )
{
	if ( chosen ) {
		return true;
	}
	return false;
}

