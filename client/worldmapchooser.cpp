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

#include "worldmapchooser.h"
#include "ui_worldmapchooser.h"

#include <algorithm>
#include <QPixmap>
#include <QPainter>
#include <QResizeEvent>
#include <QDebug>
#include <QMouseEvent>
#include <QDir>

struct WorldMapChooser::PrivateImplementation {
	QVector< MapData::MapPackage > maps;
	double minX;
	double minY;
	double maxX;
	double maxY;
	int highlight;
	QVector< QRect > rects;

	QString background;
};

WorldMapChooser::WorldMapChooser( QWidget* parent ) :
		QWidget( parent ),
		m_ui(new Ui::WorldMapChooser)
{
	m_ui->setupUi(this);
	d = new PrivateImplementation;
	d->minX = 0;
	d->minY = 0;
	d->maxX = 1;
	d->maxY = 1;

	QDir dir( ":/images/world" );
	// crude heuristic: the largest image will be the most accurate one
	// in praxis only one image should exist as an appropiate one shouldbe selected during
	// the build process
	QStringList candidates = dir.entryList( QDir::Files, QDir::Size );
	if ( !candidates.isEmpty() )
		d->background = dir.filePath( candidates.first() );
}

WorldMapChooser::~WorldMapChooser()
{
	delete d;
	delete m_ui;
}

void WorldMapChooser::showEvent( QShowEvent* )
{
	QResizeEvent event( size(), size() );
	resizeEvent( &event );
}

void WorldMapChooser::hideEvent( QHideEvent* )
{
	// save memory
	m_ui->label->setPixmap( QPixmap() );
}

void WorldMapChooser::setHighlight( int id )
{
	d->highlight = id;
	update();
}

void WorldMapChooser::mouseReleaseEvent( QMouseEvent* event )
{
	if ( event->button() != Qt::LeftButton ) {
		event->ignore();
		return;
	}

	QVector< int > result;
	for ( int i = 0; i < d->rects.size(); i++ ) {
		if ( d->rects[i].contains( event->pos() ) ) {
			result.push_back( i );
		}
	}
	if ( result.size() > 0 ) {
		int next = 0;
		if ( result.contains( d->highlight ) ) {
			next = result.indexOf( d->highlight );
			next = ( next + 1 ) % result.size();
		}
		d->highlight = result[next];
		emit clicked( d->highlight );
		QResizeEvent event( size(), size() );
		resizeEvent( &event );
	}

	event->accept();
}

void WorldMapChooser::setMaps( QVector<MapData::MapPackage> maps )
{
	d->maps = maps;
	if ( maps.size() == 0 ) {
		d->minX = 0;
		d->minY = 0;
		d->maxX = 1;
		d->maxY = 1;
	} else {
		d->minX = 1;
		d->minY = 1;
		d->maxX = 0;
		d->maxY = 0;

		for ( int i = 0; i < maps.size(); i++ ) {
			ProjectedCoordinate min = maps[i].min.ToProjectedCoordinate();
			ProjectedCoordinate max = maps[i].max.ToProjectedCoordinate();
			d->minX = std::min( min.x, d->minX );
			d->maxX = std::max( max.x, d->maxX );
			d->minY = std::min( min.y, d->minY );
			d->maxY = std::max( max.y, d->maxY );
		}

		double rangeX = d->maxX - d->minX;
		double rangeY = d->maxY - d->minY;
		if ( rangeX == 0 )
			rangeX = 1;
		if ( rangeY == 0 )
			rangeY = 1;

		d->minX = std::max( d->minX - rangeX * 0.1, 0.0 );
		d->minY = std::max( d->minY - rangeY * 0.1, 0.0 );
		d->maxX = std::min( d->maxX + rangeX * 0.1, 1.0 );
		d->maxY = std::min( d->maxY + rangeY * 0.1, 1.0 );
	}

	QResizeEvent event( size(), size() );
	resizeEvent( &event );
}

void WorldMapChooser::resizeEvent( QResizeEvent* event )
{
	int width = event->size().width();
	int height = event->size().height();

	double rangeX = d->maxX - d->minX;
	double rangeY = d->maxY - d->minY;
	double minX = d->minX;
	double maxX = d->maxX;
	double minY = d->minY;
	double maxY = d->maxY;

	if ( rangeX / rangeY > ( double ) width / height ) {
		double move = rangeX / width * height - rangeY;
		minY -= move / 2;
		maxY += move / 2;
		rangeY = rangeX / width * height;
	} else if ( rangeX / rangeY < ( double ) width / height ) {
		double move = rangeY * width / height - rangeX;
		minX -= move / 2;
		maxX += move / 2;
		rangeX = rangeY * width / height;
	}

	d->rects.clear();

	QPixmap image( width, height );
	if ( !image.isNull() )
	{
		image.fill( Qt::transparent );
		QPainter painter( &image );
		QPixmap background( d->background );
		painter.setRenderHint( QPainter::SmoothPixmapTransform );
		painter.drawPixmap( QRect( 0, 0, width, height ), background, QRect( minX * background.width(), minY * background.width() - 192 / 1024.0 * background.width(), rangeX * background.width(), rangeY * background.width() ) );
		for ( int i = 0; i < d->maps.size(); i++ ) {
			ProjectedCoordinate min = d->maps[i].min.ToProjectedCoordinate();
			ProjectedCoordinate max = d->maps[i].max.ToProjectedCoordinate();
			int left = ( min.x - minX ) * width / rangeX;
			int top = ( min.y - minY ) * height / rangeY;
			int right = ( max.x - minX ) * width / rangeX;
			int bottom = ( max.y - minY ) * height / rangeY;
			if ( d->highlight == i )
				painter.setBrush( QColor( 128, 128, 128, 128 ) );
			else
				painter.setBrush( Qt::NoBrush );
			QRect rect( left, top, right - left, bottom - top );
			painter.drawRect( rect );
			d->rects.push_back( rect );
		}
	}
	m_ui->label->setPixmap( image );
}
