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
#include "interfaces/iaddresslookup.h"
#include "interfaces/igpslookup.h"
#include "interfaces/irouter.h"
#include "addressdialog.h"

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
	void routeView();
	void settingsMenu();

	void targetBookmarks();
	void targetAddress();
	void targetGPS();

	void back();

	void settingsSystem();
	void settingsRenderer();
	void settingsGPSLookup();
	void settingsAddressLookup();
	void settingsGPS();
	void settingsDataDirectory();

	void setSource( UnsignedCoordinate source, double heading );
	void setTarget( UnsignedCoordinate target );

	void computeRoute();

signals:
	void routeChanged( QVector< UnsignedCoordinate > path );

protected:
	void connectSlots();
	bool loadPlugins();
	bool testPlugin( QObject* plugin, QString rendererName, QString routerName, QString gpsLookupName, QString addressLookupName );
	void unloadPlugins();

	QString dataDirectory;
	QList< QPluginLoader* > plugins;
	IRenderer* renderer;
	IAddressLookup* addressLookup;
	IGPSLookup* gpsLookup;
	IRouter* router;

	UnsignedCoordinate source;
	double heading;
	UnsignedCoordinate target;
	QVector< UnsignedCoordinate > path;
	IGPSLookup::Result sourcePos;
	bool sourceSet;
	IGPSLookup::Result targetPos;
	bool targetSet;

	enum {
		Source = 0, Target = 1
	} mode;

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
