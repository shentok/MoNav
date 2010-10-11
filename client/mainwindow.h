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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

protected slots:

	void routeDescription();

	void browseMap();
	void sourceMode();
	void targetMode();
	void settingsMenu();

	void targetBookmarks();
	void targetAddress();
	void targetGPS();

	void settingsSystem();
	void settingsRenderer();
	void settingsGPSLookup();
	void settingsAddressLookup();
	void settingsGPS();
	void settingsDataDirectory();

protected:
	void connectSlots();
	bool loadPlugins();

	// the current menu mode [ source selection, target selection ]
	enum {
		Source = 0, Target = 1, Map, Route, Address
	} mode;

	Ui::MainWindow* m_ui;
};

#endif // MAINWINDOW_H
