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
		QDialog( parent ),
		m_ui( new Ui::BookmarksDialog )
{
	m_ui->setupUi(this);
#ifdef Q_WS_MAEMO_5
	setAttribute( Qt::WA_Maemo5StackedWindow );
#endif

	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Bookmarks" );
	m_names = settings.value( "names" ).toStringList();

	m_ui->bookmarkList->addItems( m_names );
	for ( int i = 0; i < m_names.size(); i++ ) {
		UnsignedCoordinate pos;
		pos.x = settings.value( QString( "%1.coordinates.x" ).arg( i ), 0 ).toUInt();
		pos.y = settings.value( QString( "%1.coordinates.y" ).arg( i ), 0 ).toUInt();
		m_coordinates.push_back( pos );
	}

	connectSlots();
	itemSelectionChanged();
	m_chosen = -1;
}

void BookmarksDialog::connectSlots()
{
	connect( m_ui->chooseButton, SIGNAL(clicked()), this, SLOT(chooseBookmark()) );
	connect( m_ui->sourceButton, SIGNAL(clicked()), this, SLOT(addSourceBookmark()) );
	connect( m_ui->targetButton, SIGNAL(clicked()), this, SLOT(addTargetBookmark()) );
	connect( m_ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteBookmark()) );
	connect( m_ui->bookmarkList, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
}

BookmarksDialog::~BookmarksDialog()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Bookmarks" );
	settings.setValue( "names", m_names );
	for ( int i = 0; i < m_names.size(); i++ ) {
		UnsignedCoordinate pos = m_coordinates[i];
		settings.setValue( QString( "%1.coordinates.x" ).arg( i ), pos.x );
		settings.setValue( QString( "%1.coordinates.y" ).arg( i ), pos.y );
	}
	delete m_ui;
}

void BookmarksDialog::deleteBookmark()
{
	int index = m_ui->bookmarkList->currentRow();
	if ( index == -1 )
		return;
	QListWidgetItem* item = m_ui->bookmarkList->takeItem( index );
	if ( item != NULL )
		delete item;
	m_coordinates.remove( index );
	m_names.removeAt( index );
}

void BookmarksDialog::chooseBookmark()
{
	int index = m_ui->bookmarkList->currentRow();

	if ( index == -1 )
		return;

	m_chosen = index;
	accept();
}

void BookmarksDialog::addTargetBookmark()
{
	if ( m_target.x == 0 && m_target.x == 0 )
		return;

	bool ok = false;
	QString name = QInputDialog::getText( this, "Enter Bookmark Name", "Bookmark Name", QLineEdit::Normal, "New Bookmark", &ok );

	if ( !ok )
		return;

	m_ui->bookmarkList->addItem( name );
	m_coordinates.push_back( m_target );
	m_names.push_back( name );
}

void BookmarksDialog::addSourceBookmark()
{
	if ( m_source.x == 0 && m_source.x == 0 )
		return;

	bool ok = false;
	QString name = QInputDialog::getText( this, "Enter Bookmark Name", "Bookmark Name", QLineEdit::Normal, "New Bookmark", &ok );

	if ( !ok )
		return;

	m_ui->bookmarkList->addItem( name );
	m_coordinates.push_back( m_source );
	m_names.push_back( name );
}

void BookmarksDialog::itemSelectionChanged()
{
	bool none = m_ui->bookmarkList->selectedItems().size() == 0;
	m_ui->chooseButton->setDisabled( none );
	m_ui->deleteButton->setDisabled( none );
}

bool BookmarksDialog::showBookmarks( UnsignedCoordinate* result, QWidget* p, UnsignedCoordinate source, UnsignedCoordinate target )
{
	if ( result == NULL )
		return false;

	BookmarksDialog* window = new BookmarksDialog( p );

	window->m_target = target;
	window->m_ui->targetButton->setDisabled( !target.IsValid() );
	window->m_source = source;
	window->m_ui->sourceButton->setDisabled( !source.IsValid() );

	window->exec();

	int value = window->result();
	if ( window->m_chosen != -1 )
		*result = window->m_coordinates[window->m_chosen];
	delete window;

	return value == Accepted;
}
