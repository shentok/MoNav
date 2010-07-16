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

#include "brsettingsdialog.h"
#include "ui_brsettingsdialog.h"
#include <QSettings>

BRSettingsDialog::BRSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BRSettingsDialog)
{
	ui->setupUi(this);
	 QSettings settings( "MoNavClient" );
	 settings.beginGroup( "Renderer Base" );
	 ui->antiAliasing->setChecked( settings.value( "antiAliasing", true ).toBool() );
	 ui->hqAntiAliasing->setChecked( settings.value( "hqAntiAliasing", false ).toBool() );
	 ui->filtering->setChecked( settings.value( "filtering", false ).toBool() );
	 ui->cacheSize->setValue( settings.value( "cacheSize", 1 ).toInt() );
}

BRSettingsDialog::~BRSettingsDialog()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Renderer Base" );
	settings.setValue( "antiAliasing", ui->antiAliasing->isChecked() );
	settings.setValue( "hqAntiAliasing", ui->hqAntiAliasing->isChecked() );
	settings.setValue( "filtering", ui->filtering->isChecked() );
	settings.setValue( "cacheSize", ui->cacheSize->value() );
	delete ui;
}

bool BRSettingsDialog::getSettings( Settings* settings )
{
	if ( settings == NULL )
		return false;
	settings->antiAliasing = ui->antiAliasing->isChecked();
	settings->hqAntiAliasing = ui->hqAntiAliasing->isChecked();
	settings->filter = ui->filtering->isChecked();
	settings->cacheSize = ui->cacheSize->value();
	return true;
}
