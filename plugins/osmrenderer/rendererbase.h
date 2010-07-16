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

#ifndef RENDERERBASE_H
#define RENDERERBASE_H

#include <QObject>
#include <QCache>
#include "interfaces/irenderer.h"
#include "utils/coordinates.h"
#include "brsettingsdialog.h"

class RendererBase : public QObject, IRenderer
{
	Q_OBJECT
	Q_INTERFACES( IRenderer )

public:

	RendererBase();
	virtual ~RendererBase();
	virtual QString GetName() = 0;
	virtual void SetInputDirectory( const QString& dir );
	virtual void ShowSettings();
	virtual bool LoadData();
	virtual int GetMaxZoom() = 0;
	virtual ProjectedCoordinate Move( int shiftX, int shiftY, const PaintRequest& request );
	virtual ProjectedCoordinate PointToCoordinate( int shiftX, int shiftY, const PaintRequest& request );
	virtual bool Paint( QPainter* painter, const PaintRequest& request );
	virtual void setSlot( QObject* obj, const char* slot );

signals:
	void changed();

protected:

	virtual bool loadTile( int x, int y, int zoom, QPixmap** tile ) = 0;
	virtual bool load() = 0;
	virtual void unload() = 0;

	long long tileID( int x, int y, int zoom );
	void reset();
	void setupPolygons();
	void drawArrow( QPainter* painter, int x, int y, double rotation, QColor outer, QColor inner );
	void drawIndicator( QPainter* painter, const QTransform& transform, const QTransform& inverseTransform, int x, int y, int sizeX, int sizeY, int virtualZoom, QColor outer, QColor inner );
	void drawPolyline( QPainter* painter, const QRect& boundingBox, QVector< ProjectedCoordinate > line, QColor color );

	QString directory;
	QCache< long long, QPixmap > cache;
	bool loaded;
	int tileSize;
	QPolygonF arrow;

private:
	BRSettingsDialog::Settings settings;
	BRSettingsDialog* settingsDialog;
};

#endif // RENDERERBASE_H
