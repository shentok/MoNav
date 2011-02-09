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

#include "mapdata.h"

#include "mapmoduleswidget.h"
#include "ui_mapmoduleswidget.h"

struct MapModulesWidget::PrivateImplementation
{
	QVector< MapData::Module > rendering;
	QVector< MapData::Module > routing;
	QVector< MapData::Module > addressLookup;
};

MapModulesWidget::MapModulesWidget( QWidget* parent ) :
		QWidget( parent ),
		m_ui( new Ui::MapModulesWidget )
{
	m_ui->setupUi( this );
	d = new PrivateImplementation;

	connect( m_ui->select, SIGNAL(clicked()), this, SLOT(select()) );
	connect( m_ui->cancel, SIGNAL(clicked()), this, SIGNAL(cancelled()) );
	populateData();
}

MapModulesWidget::~MapModulesWidget()
{
	delete d;
	delete m_ui;
}

void MapModulesWidget::populateData()
{
	MapData* mapData = MapData::instance();
	m_ui->routing->clear();
	m_ui->rendering->clear();
	m_ui->addressLookup->clear();

	if ( !mapData->informationLoaded() )
		return;

	d->routing = mapData->modules( MapData::Routing );
	d->rendering = mapData->modules( MapData::Rendering );
	d->addressLookup = mapData->modules( MapData::AddressLookup );

	for ( int i = 0; i < d->routing.size(); i++ )
		m_ui->routing->addItem( d->routing[i].name );
	for ( int i = 0; i < d->rendering.size(); i++ )
		m_ui->rendering->addItem( d->rendering[i].name );
	for ( int i = 0; i < d->addressLookup.size(); i++ )
		m_ui->addressLookup->addItem( d->addressLookup[i].name );
}

void MapModulesWidget::select()
{
	int routingIndex = m_ui->routing->currentIndex();
	int renderingIndex = m_ui->rendering->currentIndex();
	int addressLookupIndex = m_ui->addressLookup->currentIndex();
	if ( routingIndex == -1 )
		return;
	if ( renderingIndex == -1 )
		return;
	if ( addressLookupIndex == -1 )
		return;

	MapData* mapData = MapData::instance();
	if ( mapData->load( d->routing[routingIndex], d->rendering[renderingIndex], d->addressLookup[addressLookupIndex] ) )
		emit selected();

	return;
}
