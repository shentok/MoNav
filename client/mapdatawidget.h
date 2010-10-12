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

namespace Ui {
	class MapDataWidget;
}

class MapDataWidget : public QDialog
{
	Q_OBJECT

public:

	explicit MapDataWidget( QWidget *parent = 0 );
	~MapDataWidget();

signals:

	void loaded();
	void quit();

protected slots:

	void directoryChanged( QString dir );
	void browse();
	void load();

protected:

	void connectSlots();
	virtual void resizeEvent( QResizeEvent * event );
	virtual void showEvent( QShowEvent* event );

	Ui::MapDataWidget *m_ui;
	QStringList m_directories;
};

#endif // MAPDATAWIDGET_H
