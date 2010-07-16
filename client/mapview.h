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
	void setGPSLookup( IGPSLookup* g );
	void setAddressLookup( IAddressLookup* al );
	void setMenu( Menu m );
	void setMode( Mode m );
	void setFixed( bool fixed );

	bool exitedToMapview();

	static int selectPlaces( QVector< UnsignedCoordinate > places, IRenderer* renderer, QWidget* p = NULL );
	static bool selectStreet( UnsignedCoordinate* result, QVector< int >segmentLength, QVector< UnsignedCoordinate > coordinates, IRenderer* renderer, IGPSLookup* gpsLookup, QWidget* p = NULL );

public slots:
	void mouseClicked( ProjectedCoordinate clickPos );
	void setCenter( ProjectedCoordinate center );
	void setSource( UnsignedCoordinate source, double heading );
	void setTarget( UnsignedCoordinate target );
	void nextPlace();
	void previousPlace();
	void showContextMenu( QPoint globalPos );
	void showContextMenu();
	void gotoSource();
	void gotoGPS();
	void gotoTarget();
	void gotoAddress();
	void setRoute( QVector< UnsignedCoordinate > path );
	void addZoom();
	void substractZoom();
	void bookmarks();
	void magnify();
	void gotoMapview();

signals:
	void coordinateChosen( UnsignedCoordinate coordinate );
	void sourceChanged( UnsignedCoordinate pos, double heading );
	void targetChanged( UnsignedCoordinate pos );

protected:
	void connectSlots();
	void setupMenu();
	void setPlaces( QVector< UnsignedCoordinate > p );
	void setEdges( QVector< int > segmentLength, QVector< UnsignedCoordinate > coordinates );

private:

	Ui::MapView *ui;

	IRenderer* renderer;
	IGPSLookup* gpsLookup;
	IAddressLookup* addressLookup;

	int maxZoom;
	QVector< UnsignedCoordinate > places;
	int place;
	UnsignedCoordinate selected;
	UnsignedCoordinate target;
	UnsignedCoordinate source;
	double heading;
	int virtualZoom;

	bool fixed;
	bool toMapview;

	Menu menu;
	Mode mode;
	QMenu* contextMenu;
	QMenu* routeMenu;
	QActionGroup* modeGroup;
	QAction* gotoSourceAction;
	QAction* gotoTargetAction;
	QAction* gotoGPSAction;
	QAction* gotoAddressAction;
	QAction* bookmarkAction;
	QAction* magnifyAction;
	QAction* modeSourceAction;
	QAction* modeTargetAction;
	QAction* mapViewAction;
};

#endif // MAPVIEW_H
