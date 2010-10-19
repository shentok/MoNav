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

#include "mrsettingsdialog.h"
#include "ui_mrsettingsdialog.h"
#include <QFileDialog>
#include <QFile>
#include <cassert>
#include <QtDebug>
#include <QSettings>

MRSettingsDialog::MRSettingsDialog( QWidget *parent ) :
	 QWidget( parent ),
	 ui( new Ui::MRSettingsDialog )
{
	 ui->setupUi(this);
	connectSlots();
}

MRSettingsDialog::~MRSettingsDialog()
{
	delete ui;
}

void MRSettingsDialog::connectSlots()
{
	connect( ui->fontBrowse, SIGNAL(clicked()), this, SLOT(browseFont()) );
	connect( ui->themeBrowse, SIGNAL(clicked()), this, SLOT(browseTheme()) );
	connect( ui->modulesBrowse, SIGNAL(clicked()), this, SLOT(browsePlugins()) );
}

void MRSettingsDialog::changeEvent(QEvent *e)
{
	 QWidget::changeEvent(e);
	 switch (e->type()) {
	 case QEvent::LanguageChange:
		  ui->retranslateUi(this);
		  break;
	 default:
		  break;
	 }
}

void MRSettingsDialog::browseFont()
{
	QString dir = ui->fontEdit->text();
	dir = QFileDialog::getExistingDirectory(this, tr( "Open Font Directory" ), dir );
	if ( dir != "" )
		ui->fontEdit->setText( dir );
}

void MRSettingsDialog::browseTheme()
{
	QString file = ui->themeEdit->text();
	file = QFileDialog::getOpenFileName( this, tr("Enter OSM Theme Filename"), file, "*.xml" );
	if ( file != "" )
		ui->themeEdit->setText( file );
}

void MRSettingsDialog::browsePlugins()
{
	QString dir = ui->modulesEdit->text();
	dir = QFileDialog::getExistingDirectory(this, tr( "Open Mapnik Plugin Directory" ), dir );
	if ( dir != "" )
		ui->modulesEdit->setText( dir );
}

bool MRSettingsDialog::getSettings( Settings* settings ) {
	settings->fonts = ui->fontEdit->text();
	settings->theme = ui->themeEdit->text();
	settings->plugins = ui->modulesEdit->text();
	settings->tileSize = ui->tileSize->value();
	settings->metaTileSize = ui->metaTileSize->value();
	settings->fullZoom = ui->minZoom->value();
	settings->margin = ui->margin->value();
	settings->tileMargin = ui->tileMargin->value();
	settings->reduceColors = ui->colorReduction->isChecked();
	settings->deleteTiles = ui->removeTiles->isChecked();
	settings->pngcrush = ui->pngcrush->isChecked();

	for ( int zoom = 0; zoom < 19; zoom++ ) {
		QString name = QString( "zoom%1" ).arg( zoom );
		QCheckBox* checkbox = findChild< QCheckBox* >( name );
		assert( checkbox != NULL );
		if ( checkbox->isChecked() )
			settings->zoomLevels.push_back( zoom );
	}
	if ( settings->zoomLevels.size() == 0 ) {
		qCritical() << "No Zoom Level Selected";
		return false;
	}
	return true;
}

bool MRSettingsDialog::loadSettings( QSettings* settings )
{
	settings->beginGroup( "Mapnik Renderer" );
	ui->fontEdit->setText( settings->value( "fontDirectory" ).toString() );
	ui->themeEdit->setText( settings->value( "themeDirectory" ).toString() );
	ui->modulesEdit->setText( settings->value( "pluginDirectory" ).toString() );
	ui->tileSize->setValue( settings->value( "tileSize", 256 ).toInt() );
	ui->metaTileSize->setValue( settings->value( "metaTileSize", 8 ).toInt() );
	ui->minZoom->setValue( settings->value( "minZoom", 6 ).toInt() );
	ui->margin->setValue( settings->value( "margin", 128 ).toInt() );
	ui->tileMargin->setValue( settings->value( "tileMargin", 1 ).toInt() );
	ui->colorReduction->setChecked( settings->value( "colorReduction", true ).toBool() );
	ui->removeTiles->setChecked( settings->value( "removeTiles", false ).toBool() );
	ui->pngcrush->setChecked( settings->value( "pngcrush", false ).toBool() );

	for ( int zoom = 0; zoom < 19; zoom++ ) {
		QString name = QString( "zoom%1" ).arg( zoom );
		QCheckBox* checkbox = findChild< QCheckBox* >( name );
		assert( checkbox != NULL );
		checkbox->setChecked( settings->value( name, true ).toBool() );
	}

	settings->endGroup();
	return true;
}

bool MRSettingsDialog::saveSettings( QSettings* settings )
{
	settings->beginGroup( "Mapnik Renderer" );
	settings->setValue( "fontDirectory", ui->fontEdit->text() );
	settings->setValue( "themeDirectory", ui->themeEdit->text() );
	settings->setValue( "pluginDirectory", ui->modulesEdit->text() );
	settings->setValue( "tileSize", ui->tileSize->value() );
	settings->setValue( "metaTileSize", ui->metaTileSize->value() );
	settings->setValue( "minZoom", ui->minZoom->value() );
	settings->setValue( "margin", ui->margin->value() );
	settings->setValue( "tileMargin", ui->tileMargin->value() );
	settings->setValue( "colorReduction", ui->colorReduction->isChecked() );
	settings->setValue( "removeTiles", ui->removeTiles->isChecked() );
	settings->setValue( "pngcrush", ui->pngcrush->isChecked() );

	for ( int zoom = 0; zoom < 19; zoom++ ) {
		QString name = QString( "zoom%1" ).arg( zoom );
		QCheckBox* checkbox = findChild< QCheckBox* >( name );
		assert( checkbox != NULL );
		settings->setValue( name, checkbox->isChecked() );
	}

	settings->endGroup();
	return true;
}
