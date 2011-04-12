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

#include "mappackageswidget.h"
#include "ui_mappackageswidget.h"
#include "mapdata.h"

#include <QResizeEvent>
#include <QShowEvent>
#include <QSettings>
#include <QStringList>
#include <QDir>
#include <QFileDialog>
#include <QtDebug>
#include <QMessageBox>

struct MapPackagesWidget::PrivateImplementation {
	struct Server {
		QString name;
		QString url;
		int id;
	};

	int selected;

	QString path;
	QVector< MapData::MapPackage > maps;
	QVector< Server > servers;

	void populateInstalled( QListWidget* list );
	void highlightButton( QPushButton* button, bool highlight );
};

MapPackagesWidget::MapPackagesWidget( QWidget* parent ) :
	QWidget( parent ),
	m_ui( new Ui::MapPackagesWidget )
{
	m_ui->setupUi( this );
	d = new PrivateImplementation;

	// Remove non-functional pages
	// TODO remove this when functionality is available
	m_ui->updatable->deleteLater();
	m_ui->downloadable->deleteLater();

	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapPackages" );
	d->path = settings.value( "path" ).toString();
	bool worldMap = settings.value( "worldmap", true ).toBool();
	m_ui->installedList->setVisible( !worldMap );
	m_ui->worldMap->setVisible( worldMap );
	m_ui->switchSelection->setChecked( worldMap );

	int entries = settings.beginReadArray( "server" );
	for ( int i = 0; i < entries; i++ ) {
		settings.setArrayIndex( i );
		PrivateImplementation::Server server;
		server.name = settings.value( "name" ).toString();
		server.url = settings.value( "url" ).toString();
		server.id = settings.value( "id" ).toInt();
	}
	settings.endArray();
	// TODO: INSERT DEFAULT SERVER

	connect( m_ui->changeDirectory, SIGNAL(clicked()), this, SLOT(directory()) );
	connect( m_ui->load, SIGNAL(clicked()), this, SLOT(load()) );
	connect( m_ui->check, SIGNAL(clicked()), this, SLOT(check()) );
	connect( m_ui->update, SIGNAL(clicked()), this, SLOT(check()) );
	connect( m_ui->download, SIGNAL(clicked()), this, SLOT(download()) );
	// ADD SERVER
	// BACK
	// CLICK
	connect( m_ui->installedList, SIGNAL(itemSelectionChanged()), this, SLOT(mapSelectionChanged()) );
	connect( m_ui->updateList, SIGNAL(itemSelectionChanged()), this, SLOT(updateSelectionChanged()) );
	connect( m_ui->downloadList, SIGNAL(itemSelectionChanged()), this, SLOT(downloadSelectionChanged()) );
	connect( m_ui->worldMap, SIGNAL(clicked(int)), this, SLOT(selected(int)) );

	d->populateInstalled( m_ui->installedList );
	d->highlightButton( m_ui->changeDirectory, m_ui->installedList->count() == 0 );
	m_ui->worldMap->setMaps( d->maps );
	m_ui->worldMap->setHighlight( d->selected );
}

MapPackagesWidget::~MapPackagesWidget()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapPackages" );
	settings.setValue( "path", d->path );
	settings.setValue( "worldmap", m_ui->switchSelection->isChecked() );
	settings.beginWriteArray( "server", d->servers.size() );
	for ( int i = 0; i < d->servers.size(); i++ ) {
		settings.setArrayIndex( i );
		settings.setValue( "name", d->servers[i].name );
		settings.setValue( "url", d->servers[i].url );
		settings.setValue( "id", d->servers[i].id );
	}
	settings.endArray();

	delete d;
	delete m_ui;
}

void MapPackagesWidget::resizeEvent ( QResizeEvent* /*event*/ )
{
	// TODO CHANGE ORIENTATION
}

void MapPackagesWidget::showEvent( QShowEvent* /*event*/ )
{
	if ( !QFile::exists( d->path ) ) {
		//QMessageBox::information( this, "Data Directory", "Before proceeding be sure to select a valid data directory", "Ok" );
	}
}

void MapPackagesWidget::selected( int id )
{
	m_ui->installedList->item( id )->setSelected( true );
}

void MapPackagesWidget::mapSelectionChanged()
{
	bool selected = m_ui->installedList->selectedItems().size() == 1;
	if ( selected )
		m_ui->worldMap->setHighlight( m_ui->installedList->selectedItems().first()->data( Qt::UserRole ).toInt() );
	m_ui->load->setEnabled( selected );
	m_ui->deleteMap->setEnabled( selected );
}

void MapPackagesWidget::updateSelectionChanged()
{
	bool selected = m_ui->updateList->selectedItems().size() > 0;
	m_ui->update->setEnabled( selected );
}

void MapPackagesWidget::downloadSelectionChanged()
{
	bool selected = m_ui->downloadList->selectedItems().size() > 0;
	m_ui->download->setEnabled( selected );
}

void MapPackagesWidget::load()
{
	QList< QListWidgetItem* > items = m_ui->installedList->selectedItems();
	if ( items.size() != 1 ) {
		qDebug() << "Error: only one map should be selected";
		return;
	}

	int index = items.first()->data( Qt::UserRole ).toInt();

	MapData* mapData = MapData::instance();
	mapData->setPath( d->maps[index].path );
	if ( !mapData->loadInformation() )
		return;

	emit mapChanged();
}

void MapPackagesWidget::directory()
{
	QString newDir = QFileDialog::getExistingDirectory( this, "MoNav Data Directory", d->path );
	if ( newDir.isEmpty() || newDir == d->path )
		return;

	d->path = newDir;
	d->populateInstalled( m_ui->installedList );
	d->highlightButton( m_ui->changeDirectory, m_ui->installedList->count() == 0 );
	m_ui->worldMap->setMaps( d->maps );
	m_ui->worldMap->setHighlight( d->selected );
}

void MapPackagesWidget::check()
{

}

void MapPackagesWidget::update()
{

}

void MapPackagesWidget::download()
{

}

void MapPackagesWidget::PrivateImplementation::populateInstalled( QListWidget* list )
{
	list->clear();
	maps.clear();
	selected = -1;

	MapData* mapData = MapData::instance();
	if ( !mapData->searchForMapPackages( path, &maps, 2 ) )
		return;

	for ( int i = 0; i < maps.size(); i++ ) {
		QListWidgetItem* item = new QListWidgetItem( maps[i].name );
		item->setData( Qt::UserRole, i );
		list->addItem( item );
		if ( maps[i].path == mapData->path() ) {
			item->setSelected( true );
			selected = i;
		}
	}
}

void MapPackagesWidget::PrivateImplementation::highlightButton( QPushButton* button, bool highlight )
{
	QFont font = button->font();
	font.setBold( highlight );
	font.setUnderline( highlight );
	button->setFont( font );
}
