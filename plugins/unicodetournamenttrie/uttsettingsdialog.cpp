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

#include "uttsettingsdialog.h"
#include "ui_uttsettingsdialog.h"

#include <QSettings>

UTTSettingsDialog::UTTSettingsDialog( QWidget *parent ) :
		QWidget( parent ),
		ui( new Ui::UTTSettingsDialog )
{
	ui->setupUi(this);
}

UTTSettingsDialog::~UTTSettingsDialog()
{
	delete ui;
}

bool UTTSettingsDialog::getSettings( Settings* /*settings*/ )
{
	return true;
}

bool UTTSettingsDialog::loadSettings( QSettings* settings )
{
	settings->beginGroup( "UnicodeTournamentTree" );
	settings->endGroup();
	return true;
}

bool UTTSettingsDialog::saveSettings( QSettings* settings )
{
	settings->beginGroup( "UnicodeTournamentTree" );
	settings->endGroup();
	return true;
}
