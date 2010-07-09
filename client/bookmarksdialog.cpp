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

#include "bookmarksdialog.h"
#include "ui_bookmarksdialog.h"
#include <QSettings>
#include <QInputDialog>

BookmarksDialog::BookmarksDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BookmarksDialog)
{
    ui->setupUi(this);
	 QSettings settings( "MoNavClient" );
	 settings.beginGroup( "Bookmarks" );
	 names = settings.value( "names" ).toStringList();
	 ui->bookmarkList->addItems( names );
	 for ( int i = 0; i < names.size(); i++ ) {
		 UnsignedCoordinate pos;
		 pos.x = settings.value( QString( "%1.coordinates.x" ).arg( i ), 0 ).toUInt();
		 pos.y = settings.value( QString( "%1.coordinates.y" ).arg( i ), 0 ).toUInt();
		 coordinates.push_back( pos );
	 }
	 connectSlots();
	 chosen = -1;
}

void BookmarksDialog::connectSlots()
{
	connect( ui->chooseButton, SIGNAL(clicked()), this, SLOT(chooseBookmark()) );
	connect( ui->sourceButton, SIGNAL(clicked()), this, SLOT(addSourceBookmark()) );
	connect( ui->targetButton, SIGNAL(clicked()), this, SLOT(addTargetBookmark()) );
	connect( ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteBookmark()) );
}

BookmarksDialog::~BookmarksDialog()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Bookmarks" );
	settings.setValue( "names", names );
	for ( int i = 0; i < names.size(); i++ ) {
		UnsignedCoordinate pos = coordinates[i];
		settings.setValue( QString( "%1.coordinates.x" ).arg( i ), pos.x );
		settings.setValue( QString( "%1.coordinates.y" ).arg( i ), pos.y );
	}
	delete ui;
}

void BookmarksDialog::deleteBookmark()
{
	int index = ui->bookmarkList->currentRow();
	if ( index == -1 )
		return;
	QListWidgetItem* item = ui->bookmarkList->takeItem( index );
	if ( item != NULL )
		delete item;
	coordinates.remove( index );
	names.removeAt( index );
}

void BookmarksDialog::chooseBookmark()
{
	int index = ui->bookmarkList->currentRow();
	if ( index == -1 )
		return;
	chosen = index;
	accept();
}

void BookmarksDialog::addTargetBookmark()
{
	if ( target.x == 0 && target.x == 0 )
		return;
	bool ok = false;
	QString name = QInputDialog::getText( this, "Enter Bookmark Name", "Bookmark Name", QLineEdit::Normal, "New Bookmark", &ok );
	if ( !ok )
		return;
	ui->bookmarkList->addItem( name );
	coordinates.push_back( target );
	names.push_back( name );
}

void BookmarksDialog::addSourceBookmark()
{
	if ( source.x == 0 && source.x == 0 )
		return;
	bool ok = false;
	QString name = QInputDialog::getText( this, "Enter Bookmark Name", "Bookmark Name", QLineEdit::Normal, "New Bookmark", &ok );
	if ( !ok )
		return;
	ui->bookmarkList->addItem( name );
	coordinates.push_back( source );
	names.push_back( name );
}

bool BookmarksDialog::showBookmarks( UnsignedCoordinate* result, QWidget* p, UnsignedCoordinate source, UnsignedCoordinate target )
{
	if ( result == NULL )
		return false;
	BookmarksDialog* window = new BookmarksDialog( p );
	window->target = target;
	if ( target.x == 0 && target.y == 0 )
		window->ui->targetButton->setDisabled( true );
	window->source = source;
	if ( source.x == 0 && source.y == 0 )
		window->ui->sourceButton->setDisabled( true );

	window->exec();

	int value = window->result();
	if ( window->chosen != -1 )
		*result = window->coordinates[window->chosen];
	delete window;
	return value == Accepted;
}
