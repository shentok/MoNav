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

#ifndef QTILERENDERER_H
#define QTILERENDERER_H

#include <QObject>
#include "interfaces/ipreprocessor.h"
#include "qrsettingsdialog.h"

class QtileRenderer : public QObject, public IPreprocessor
{
	Q_OBJECT
	Q_INTERFACES( IPreprocessor )

public:

	QtileRenderer();
	virtual QString GetName();
	virtual int GetFileFormatVersion();
	virtual Type GetType();
	virtual QWidget* GetSettings();
	virtual bool LoadSettings( QSettings* settings );
	virtual bool SaveSettings( QSettings* settings );
	virtual bool Preprocess( IImporter* importer, QString dir );
	virtual ~QtileRenderer();

protected:
        void write_ways(QString &dir, bool motorway);

        class OSMReader *m_osr;
	QString m_directory;
	QRSettingsDialog* m_settingsDialog;
};

#endif // QtileRENDERER_H
