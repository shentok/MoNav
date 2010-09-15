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

#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QDialog>
#include <QMenu>
#include "interfaces/irenderer.h"
#include "interfaces/igpslookup.h"
#include "interfaces/iaddresslookup.h"
#include "interfaces/irouter.h"

namespace Ui {
	 class MapView;
}

class MapView : public QDialog {
	Q_OBJECT
public:
	MapView( QWidget *parent = 0 );
	~MapView();

	enum Mode {
		Source, Target, POI, None
	};

	enum Menu {
		NoMenu, ContextMenu, RouteMenu
	};

	void setRender( IRenderer* r );
	void setAddressLookup( IAddressLookup* al );
	void setMenu( Menu m );
	void setMode( Mode m );
	void setFixed( bool fixed );

	bool exitedToMapview();

	static int selectPlaces( QVector< UnsignedCoordinate > places, IRenderer* renderer, QWidget* p = NULL );
	static bool selectStreet( UnsignedCoordinate* result, QVector< int >segmentLength, QVector< UnsignedCoordinate > coordinates, IRenderer* renderer, QWidget* p = NULL );

public slots:

	void setCenter( ProjectedCoordinate center );
	void setSource( UnsignedCoordinate source, double heading );
	void setTarget( UnsignedCoordinate target );
	void setRoute( QVector< IRouter::Node > path, QStringList icon, QStringList label );
	void magnify();

signals:
	void sourceChanged( UnsignedCoordinate pos, double heading );
	void targetChanged( UnsignedCoordinate pos );
	void infoClicked();

protected slots:
	void mouseClicked( ProjectedCoordinate clickPos );
	void nextPlace();
	void previousPlace();
	void showContextMenu( QPoint globalPos );
	void showContextMenu();
	void gotoSource();
	void gotoGPS();
	void gotoTarget();
	void gotoAddress();
	void gotoMapview();
	void addZoom();
	void substractZoom();
	void bookmarks();

protected:
	void connectSlots();
	void setupMenu();
	void setPlaces( QVector< UnsignedCoordinate > p );
	void setEdges( QVector< int > segmentLength, QVector< UnsignedCoordinate > coordinates );

	Ui::MapView *m_ui;

	IRenderer* m_renderer;
	IAddressLookup* m_addressLookup;

	int m_maxZoom;
	QVector< UnsignedCoordinate > m_places;
	int m_place;
	UnsignedCoordinate m_selected;
	UnsignedCoordinate m_target;
	UnsignedCoordinate m_source;
	double m_heading;
	int m_virtualZoom;

	bool m_fixed;
	bool m_toMapview;

	Menu m_menu;
	Mode m_mode;
	QMenu* m_contextMenu;
	QMenu* m_routeMenu;
	QActionGroup* m_modeGroup;
	QAction* m_gotoSourceAction;
	QAction* m_gotoTargetAction;
	QAction* m_gotoGPSAction;
	QAction* m_gotoAddressAction;
	QAction* m_bookmarkAction;
	QAction* m_magnifyAction;
	QAction* m_modeSourceAction;
	QAction* m_modeTargetAction;
	QAction* m_mapViewAction;
};

#endif // MAPVIEW_H
