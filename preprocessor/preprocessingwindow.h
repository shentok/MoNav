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

#ifndef PREPROCESSINGWINDOW_H
#define PREPROCESSINGWINDOW_H

#include <QMainWindow>
#include <QPluginLoader>
#include "aboutdialog.h"
#include "interfaces/iimporter.h"
#include "interfaces/ipreprocessor.h"

namespace Ui {
	class PreprocessingWindow;
}

class PreprocessingWindow : public QMainWindow {

	Q_OBJECT

public:

	PreprocessingWindow(QWidget *parent = 0);
	~PreprocessingWindow();

public slots:

	void about();
	void browse();
	void importerSettings();
	bool importerPreprocessing();
	void rendererSettings();
	bool rendererPreprocessing();
	void routerSettings();
	bool routerPreprocessing();
	void gpsLookupSettings();
	bool gpsLookupPreprocessing();
	void addressLookupSettings();
	bool addressLookupPreprocessing();
	void preprocessAll();
	void writeConfig();
	void deleteTemporary();
	void manual();

protected:

	void connectSlots();
	void loadPlugins();
	bool testPlugin( QObject* plugin );
	void unloadPlugins();

	AboutDialog* m_aboutDialog;

	QList< IImporter* > m_importerPlugins;
	QList< IPreprocessor* > m_rendererPlugins;
	QList< IPreprocessor* > m_routerPlugins;
	QList< IPreprocessor* > m_gpsLookupPlugins;
	QList< IPreprocessor* > m_addressLookupPlugins;

	QList< QPluginLoader* > m_plugins;

	Ui::PreprocessingWindow* m_ui;
};

#endif // PREPROCESSINGWINDOW_H
