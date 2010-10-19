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
#include <QFile>
#include "interfaces/ipreprocessor.h"
#include "mrsettingsdialog.h"

class MapnikRenderer : public QObject, public IPreprocessor
{
	Q_OBJECT
	Q_INTERFACES( IPreprocessor )

public:
	 MapnikRenderer();
	virtual QString GetName();
	virtual int GetFileFormatVersion();
	virtual Type GetType();
	virtual QWidget* GetSettings();
	virtual bool LoadSettings( QSettings* settings );
	virtual bool SaveSettings( QSettings* settings );
	virtual bool Preprocess( IImporter* importer, QString dir );
	virtual ~MapnikRenderer();

signals:
	void changed();

protected:
	struct MetaTile {
		int zoom;
		int x;
		int y;
		int metaTileSizeX;
		int metaTileSizeY;
	};
	struct IndexElement {
		qint64 start;
		int size;
	};
	struct ZoomInfo {
		int minX;
		int maxX;
		int minY;
		int maxY;
		std::vector< IndexElement > index;
		QFile* tilesFile;
	};

	MRSettingsDialog* m_settingsDialog;
};

#endif // MAPNIKRENDERER_H
