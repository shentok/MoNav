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
	void importerPreprocessing();
	void rendererSettings();
	void rendererPreprocessing();
	void routerSettings();
	void routerPreprocessing();
	void gpsLookupSettings();
	void gpsLookupPreprocessing();
	void addressLookupSettings();
	void addressLookupPreprocessing();
	void preprocessAll();
	void writeConfig();
	void deleteTemporary();

protected:
	void changeEvent(QEvent *e);
	void connectSlots();
	void loadPlugins();
	void unloadPlugins();

	AboutDialog* aboutDialog;

	QList< IImporter* > importerPlugins;
	QList< IPreprocessor* > rendererPlugins;
	QList< IPreprocessor* > routerPlugins;
	QList< IPreprocessor* > gpsLookupPlugins;
	QList< IPreprocessor* > addressLookupPlugins;

	QList< QPluginLoader* > plugins;

private:
    Ui::PreprocessingWindow *ui;
};

#endif // PREPROCESSINGWINDOW_H
