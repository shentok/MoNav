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
#include <QPluginLoader>
#include "interfaces/irenderer.h"
#include "mapview.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
	void browseMap();
	void sourceMode();
	void targetMode();
	void about();
	void routeView();

	void targetBookmarks();
	void targetCityCenter();
	void targetAddress();
	void targetMap();
	void targetGPS();

	void settingsSystem();
	void settingsRenderer();
	void settingsGPSLookup();
	void settingsAddressLookup();
	void settingsGPS();
	void settingsDataDirectory();

protected:
    void changeEvent(QEvent *e);
	void connectSlots();
	bool loadPlugins();
	void unloadPlugins();

	QString dataDirectory;
	QList< QPluginLoader* > plugins;
	IRenderer* renderer;

	MapView* mapView;

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
