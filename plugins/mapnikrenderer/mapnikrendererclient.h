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

#ifndef MAPNIKRENDERER_H
#define MAPNIKRENDERER_H

#include <QObject>
#include <QDataStream>
#include <QFile>
#include <QCache>
#include "interfaces/irenderer.h"

class MapnikRendererClient : public QObject, IRenderer
{
	Q_OBJECT
	Q_INTERFACES( IRenderer )

public:

	 MapnikRendererClient();
	~MapnikRendererClient();
	virtual QString GetName();
	virtual void SetInputDirectory( const QString& dir );
	virtual void ShowSettings();
	virtual bool LoadData();
	virtual int GetMaxZoom();
	virtual ProjectedCoordinate Move( ProjectedCoordinate center, int shiftX, int shiftY, int zoom );
	virtual ProjectedCoordinate PointToCoordinate( ProjectedCoordinate center, int shiftX, int shiftY, int zoom );
	virtual ProjectedCoordinate ZoomInOn( ProjectedCoordinate center, ProjectedCoordinate zoomPoint, int zoom );
	virtual ProjectedCoordinate ZoomOutOn( ProjectedCoordinate center, ProjectedCoordinate zoomPoint, int zoom );
	virtual bool SetPoints( QVector< UnsignedCoordinate > points );
	virtual bool SetEdges( QVector< int > segmentLengths, QVector< UnsignedCoordinate > edges );
	virtual bool SetPosition( UnsignedCoordinate coordinate, double heading ) ;
	virtual bool Paint( QPainter* painter, ProjectedCoordinate center, int zoomLevel, double rotation, double virtualZoom );

protected:

	void unload();

	struct Box
	{
		int minX;
		int maxX;
		int minY;
		int maxY;
	};

	QString directory;
	bool loaded;
	int tileSize;
	int maxZoom;
	QVector< UnsignedCoordinate > points;
	QVector< int > segmentLengths;
	QVector< UnsignedCoordinate > edges;
	std::vector< Box > boxes;
	QCache< long long, QPixmap > cache;
};

#endif // MAPNIKRENDERER_H
