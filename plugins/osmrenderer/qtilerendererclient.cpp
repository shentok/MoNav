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

#include <QDir>
#include <QDebug>
#include <QDesktopServices>
#include <QPaintEngine>

#include "qtilerendererclient.h"
#include "utils/qthelpers.h"
#include "tile-write.h"

QtileRendererClient::QtileRendererClient()
{
        twriter = NULL;
}

QtileRendererClient::~QtileRendererClient()
{
	unload();
}

void QtileRendererClient::unload()
{
        delete twriter;
        twriter = NULL;
}

QString QtileRendererClient::GetName()
{
	return "Qtile Renderer";
}

bool QtileRendererClient::IsCompatible( int fileFormatVersion )
{
        if ( fileFormatVersion == 1 )
                return true;
        return false;
}

bool QtileRendererClient::load()
{
	tileSize = 256;
        for(int i=0;i<=18;i++) m_zoomLevels.push_back(i);

        std::string dir = m_directory.toStdString();
        twriter = new TileWriter(dir+ "/ways.all.pqdb",
                                 dir+ "/ways.motorway.pqdb");
	return true;
}

int QtileRendererClient::GetMaxZoom()
{
	return 18;
}

bool QtileRendererClient::loadTile( int x, int y, int zoom, QPixmap** tile )
{
        twriter->draw_image("", x, y, zoom);
        QImage img(twriter->get_img_data(), tileSize, tileSize,
                   QImage::Format_RGB888);
        *tile = new QPixmap(QPixmap::fromImage(img));
        return true;
}

Q_EXPORT_PLUGIN2( qtilerendererclient, QtileRendererClient )
