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
#include "mapview.h"

AddressDialog::AddressDialog(QWidget *parent) :
		QDialog(parent),
		ui(new Ui::AddressDialog)
{
	ui->setupUi(this);
	addressLookup = NULL;
	renderer = NULL;
	gpsLookup = NULL;
	skipStreetPosition = false;
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

void AddressDialog::setRenderer( IRenderer* r )
{
	renderer = r;
}

void AddressDialog::setGPSLookup( IGPSLookup* g )
{
	gpsLookup = g;
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
		QVector< int > placeIDs;
		QVector< UnsignedCoordinate > placeCoordinates;
		if ( !addressLookup->GetPlaceData( text, &placeIDs, &placeCoordinates ) )
			return;

		placeID = placeIDs.front();
		if ( placeIDs.size() > 1 )
		{
			int id = MapView::selectPlaces( placeCoordinates, renderer, this );
			if ( id >= 0 && id < placeIDs.size() )
				placeID = placeIDs[id];
			else
				return;
		}
		ui->cityEdit->setText( text );
		ui->cityEdit->setDisabled( true );
		ui->streetEdit->setEnabled( true );
		ui->resetStreet->setEnabled( true );
		mode = Street;
		addressLookup->SelectPlace( placeID );
		streetTextChanged( ui->streetEdit->text() );
	}
	else {
		QVector< int > segmentLength;
		QVector< UnsignedCoordinate > coordinates;
		if ( !addressLookup->GetStreetData( text, &segmentLength, &coordinates ) )
			return;
		if ( coordinates.size() == 0 )
			return;
		if ( skipStreetPosition ) {
			result = coordinates.first();
			accept();
			return;
		}
		if( !MapView::selectStreet( &result, segmentLength, coordinates, renderer, gpsLookup, this ) )
			return;
		ui->streetEdit->setText( text );
		accept();
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
	cityTextChanged( "" );
}

void AddressDialog::resetStreet()
{
	ui->streetEdit->setText( "" );
	streetTextChanged( "" );
}

bool AddressDialog::getAddress( UnsignedCoordinate* result, IAddressLookup* addressLookup, IRenderer* renderer, IGPSLookup* gpsLookup, QWidget* p, bool cityOnly )
{
	if ( result == NULL )
		return false;
	if ( addressLookup == NULL )
		return false;
	if ( gpsLookup == NULL )
		return false;
	if ( renderer == NULL )
		return false;

	AddressDialog* window = new AddressDialog( p );
	window->setAddressLookup( addressLookup );
	window->setRenderer( renderer );
	window->setGPSLookup( gpsLookup );
	window->skipStreetPosition = cityOnly;

	int value = window->exec();
	if ( value == Accepted )
		*result = window->result;

	delete window;
	return value == Accepted;
}

