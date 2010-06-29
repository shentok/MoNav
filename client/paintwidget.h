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

#ifndef PAINTWIDGET_H
#define PAINTWIDGET_H

#include <QWidget>
#include "interfaces/irenderer.h"

namespace Ui {
    class PaintWidget;
}

class PaintWidget : public QWidget {
    Q_OBJECT
public:
    PaintWidget(QWidget *parent = 0);
    ~PaintWidget();

public slots:

	void setZoom( int z );
	void setMaxZoom( int z );
	void setRenderer( IRenderer* r );
	void setCenter( const ProjectedCoordinate c );
	void setPosition( const UnsignedCoordinate p, double heading );
	void setPOIs( QVector< UnsignedCoordinate > p );
	void setRoute( QVector< UnsignedCoordinate > r );
	void setEdges( QVector< int > edgeSegments, QVector< UnsignedCoordinate > edges );

signals:

	void zoomChanged( int z );
	void centerChanged( ProjectedCoordinate c );
	void mouseClicked( ProjectedCoordinate clickPos );

protected:
	void paintEvent( QPaintEvent* );
	void mouseMoveEvent( QMouseEvent * event );
	void mousePressEvent( QMouseEvent * event );
	void mouseReleaseEvent( QMouseEvent* event );
	void wheelEvent( QWheelEvent * event );

	IRenderer* renderer;
	IRenderer::PaintRequest request;

	int maxZoom;
	int lastMouseX;
	int lastMouseY;
	int startMouseX;
	int startMouseY;
	bool drag;
	int wheelDelta;

private:
    Ui::PaintWidget *ui;
};

#endif // PAINTWIDGET_H
