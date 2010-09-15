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

#ifndef NOQTMOBILE
	#include <QGeoPositionInfoSource>
	QTM_USE_NAMESPACE
#endif

		namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

public slots:
	void setSource( UnsignedCoordinate source, double heading );
	void setTarget( UnsignedCoordinate target );

	void computeRoute();

#ifndef NOQTMOBILE
	void positionUpdated( const QGeoPositionInfo & update );
#endif

signals:
	void routeChanged( QVector< IRouter::Node > path, QStringList icon, QStringList label );
	void sourceChanged( UnsignedCoordinate source, double heading );
	void targetChanged( UnsignedCoordinate target );

protected slots:

	void routeDescription();

	void browseMap();
	void sourceMode();
	void targetMode();
	void routeView();
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
	bool testPlugin( QObject* plugin, QString rendererName, QString routerName, QString gpsLookupName, QString addressLookupName );
	void unloadPlugins();

	// stores the current data directory
	QString m_dataDirectory;
	// stores pointers to all dynamically loaded plugins
	QList< QPluginLoader* > m_plugins;
	// stores a pointer to the current renderer plugin
	IRenderer* m_renderer;
	// stores a pointer to the current address lookup plugin
	IAddressLookup* m_addressLookup;
	// stores a pointer to the current GPS lookup plugin
	IGPSLookup* m_gpsLookup;
	// stores a pointer to the current router plugin
	IRouter* m_router;

	// the current source and its heading
	UnsignedCoordinate m_source;
	double m_heading;
	// the current target
	UnsignedCoordinate m_target;
	// the last computed route
	QVector< IRouter::Node > m_pathNodes;
	QVector< IRouter::Edge > m_pathEdges;
	QStringList m_descriptionIcons;
	QStringList m_descriptionLabels;
	// the result of the GPS lookup of source
	IGPSLookup::Result m_sourcePos;
	bool m_sourceSet;
	// the result of the GPS lookup of target
	IGPSLookup::Result m_targetPos;
	bool m_targetSet;

	// the current menu mode [ source selection, target selection ]
	enum {
		Source = 0, Target = 1, Map, Route, Address
	} mode;

#ifndef NOQTMOBILE
	// the current GPS source
	QGeoPositionInfoSource* m_gpsSource;
#endif

	// target updated via GPS source?
	bool m_updateTarget;
	// source updated via GPS source?
	bool m_updateSource;

	Ui::MainWindow* m_ui;
};

#endif // MAINWINDOW_H
