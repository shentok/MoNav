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

#ifndef MRSettingsDialog_H
#define MRSettingsDialog_H

#include <QDialog>

namespace Ui {
	 class MRSettingsDialog;
}

class MRSettingsDialog : public QDialog {
    Q_OBJECT
public:
	 MRSettingsDialog(QWidget *parent = 0);
	 ~MRSettingsDialog();

	struct Settings {
		QString plugins;
		QString fonts;
		QString theme;
		int minZoom;
		int maxZoom;
		int tileSize;
		int metaTileSize;
		int margin;
		int tileMargin;
		bool reduceColors;
		bool deleteTiles;
		bool pngcrush;
	};

	bool getSettings( Settings* settings );

public slots:
	void browseFont();
	void browseTheme();
	void browsePlugins();

protected:
    void changeEvent(QEvent *e);
	void connectSlots();

private:
	 Ui::MRSettingsDialog *ui;
};

#endif // MRSettingsDialog_H
