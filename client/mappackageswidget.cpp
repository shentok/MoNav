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

	QString path;
	QVector< MapData::MapPackage > maps;
	QVector< Server > servers;

	void populateInstalled( QListWidget* list );
};

MapPackagesWidget::MapPackagesWidget( QWidget* parent ) :
	QWidget( parent ),
	m_ui( new Ui::MapPackagesWidget )
{
	m_ui->setupUi( this );
	d = new PrivateImplementation;

	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapPackages" );
	d->path = settings.value( "path" ).toString();

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

	d->populateInstalled( m_ui->installedList );
}

MapPackagesWidget::~MapPackagesWidget()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapPackages" );
	settings.setValue( "path", d->path );
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

void MapPackagesWidget::resizeEvent ( QResizeEvent* event )
{
	// TODO CHANGE ORIENTATION
}

void MapPackagesWidget::showEvent( QShowEvent* /*event*/ )
{
	if ( !QFile::exists( d->path ) ) {
		QMessageBox::information( this, "Data Directory", "Before proceeding be sure to select a valid data directory", "Ok" );
	}
}

void MapPackagesWidget::mapSelectionChanged()
{
	bool selected = m_ui->installedList->selectedItems().size() == 1;
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

	MapData* mapData = MapData::instance();
	if ( !mapData->searchForMapPackages( path, &maps, 2 ) )
		return;

	for ( int i = 0; i < maps.size(); i++ ) {
		QListWidgetItem* item = new QListWidgetItem( maps[i].name );
		item->setData( Qt::UserRole, i );
		list->addItem( item );
		if ( maps[i].path == mapData->path() )
			item->setSelected( true );
	}
}
