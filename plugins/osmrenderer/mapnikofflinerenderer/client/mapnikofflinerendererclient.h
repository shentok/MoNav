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

#ifndef MAPNIKOFFLINERENDERER_H
#define MAPNIKOFFLINERENDERER_H

#include <QObject>
#include <QThread>
#include "brsettingsdialog.h"
#include "rendererbase.h"
#include "interfaces/irenderer.h"
#include <map>

class MapnikOfflineRendererClient : public RendererBase
{
	Q_OBJECT
public:

	MapnikOfflineRendererClient();
	virtual ~MapnikOfflineRendererClient();
	virtual QString GetName();
	virtual bool IsCompatible( int fileFormatVersion );
	virtual int GetMaxZoom();

signals:
	void abort();
	void drawImage( int x, int y, int zoom, int magnification );

private slots:

	void tileLoaded( int x, int y, int zoom, int magnification, const QImage &img );

protected:

    virtual QPixmap* loadTile( int x, int y, int zoom, int magnification );
	virtual bool load();
	virtual void unload();

	int tileSize;
	class MapnikTileWriter *twriter;
	QThread* m_renderThread;
};

#endif // MAPNIKOFFLINERENDERER_H
