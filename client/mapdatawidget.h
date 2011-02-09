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

#ifndef MAPDATAWIDGET_H
#define MAPDATAWIDGET_H

#include <QDialog>
#include "mapdata.h"

namespace Ui {
	class MapDataWidget;
}

class MapDataWidget : public QDialog
{
	Q_OBJECT

public:

	explicit MapDataWidget( QWidget *parent = 0 );
	~MapDataWidget();
	int exec( bool autoLoad = false );

protected slots:

	void directoryChanged( QString dir );
	void modulesChanged();
	void browse();
	bool load();

protected:

	int findModule( const QVector< MapData::Module >& modules, QString path );
	void connectSlots();
	virtual void showEvent( QShowEvent* event );

	QString m_lastRoutingModule;
	QString m_lastRenderingModule;
	QString m_lastAddressLookupModule;

	QVector< MapData::Module > m_routingModules;
	QVector< MapData::Module > m_renderingModules;
	QVector< MapData::Module > m_addressLookupModules;

	Ui::MapDataWidget *m_ui;
	QStringList m_directories;
};

#endif // MAPDATAWIDGET_H
