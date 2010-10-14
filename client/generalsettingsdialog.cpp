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

#include "generalsettingsdialog.h"
#include "ui_generalsettingsdialog.h"

GeneralSettingsDialog::GeneralSettingsDialog( QWidget* parent ) :
		QDialog( parent ),
		m_ui( new Ui::GeneralSettingsDialog )
{
	m_ui->setupUi( this );
}

void GeneralSettingsDialog::setIconSize( int size )
{
	m_ui->iconSize->setValue( size );
}

int GeneralSettingsDialog::iconSize()
{
	return m_ui->iconSize->value();
}
void GeneralSettingsDialog::setCustomIconSize( bool custom )
{
	m_ui->customIconSize->setChecked( custom );
}
bool GeneralSettingsDialog::customIconSize()
{
	return m_ui->customIconSize->isChecked();
}
void GeneralSettingsDialog::setMenuMode( MenuMode mode )
{
	if ( mode == MenuPopup )
		m_ui->popup->setChecked( true );
	else
		m_ui->overlay->setChecked( true );
}
GeneralSettingsDialog::MenuMode GeneralSettingsDialog::menuMode()
{
	if ( m_ui->popup->isChecked() )
		return MenuPopup;
	return MenuOverlay;
}

GeneralSettingsDialog::~GeneralSettingsDialog()
{
	delete m_ui;
}
