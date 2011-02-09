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

#ifndef MAPPACKAGESWIDGET_H
#define MAPPACKAGESWIDGET_H

#include <QWidget>

namespace Ui {
	class MapPackagesWidget;
}

class MapPackagesWidget : public QWidget
{
	Q_OBJECT

public:

	explicit MapPackagesWidget( QWidget* parent = 0 );
	~MapPackagesWidget();

public slots:

	void mapSelectionChanged();
	void updateSelectionChanged();
	void downloadSelectionChanged();

signals:

	void mapChanged();
	void closed();

protected slots:

	void load();
	void directory();
	void check();
	void update();
	void download();

protected:

	virtual void resizeEvent ( QResizeEvent* event );

	struct PrivateImplementation;
	PrivateImplementation* d;
	Ui::MapPackagesWidget* m_ui;
};

#endif // MAPPACKAGESWIDGET_H
