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

#ifndef MAPSFORGERENDERER_H
#define MAPSFORGERENDERER_H

#include <QObject>
#include "rendererbase.h"

class QImage;
class QPixmap;
class QThread;

class MapsforgeRendererClient : public RendererBase
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "monav.IRenderer/1.2")
#endif

public:
	MapsforgeRendererClient();
	virtual ~MapsforgeRendererClient();
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

	class MapsforgeTileWriter *twriter;
	QThread* m_renderThread;
};

#endif // MAPSFORGERENDERER_H
