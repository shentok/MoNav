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

#include "waymodificatorwidget.h"
#include "ui_waymodificatorwidget.h"

WayModificatorWidget::WayModificatorWidget( QWidget* parent ) :
		QFrame( parent ),
		m_ui( new Ui::WayModificatorWidget )
{
	m_ui->setupUi( this );
}

WayModificatorWidget::~WayModificatorWidget()
{
	delete m_ui;
}

void WayModificatorWidget::setModificator( const Modificator& modificator )
{
	m_ui->type->setCurrentIndex( modificator.invert ? 1 : 0 );
	m_ui->key->setText( modificator.key );
	m_ui->useValue->setChecked( modificator.checkValue );
	if ( modificator.checkValue )
		m_ui->value->setText( modificator.value );
	m_ui->action->setCurrentIndex( ( int ) modificator.type );
	switch ( modificator.type ) {
	case WayModifyFixed:
		m_ui->fixed->setValue( modificator.modificatorValue.toInt() );
		break;
	case WayModifyPercentage:
		m_ui->percentage->setValue( modificator.modificatorValue.toInt() );
		break;
	case WayAccess:
		m_ui->access->setCurrentIndex( modificator.modificatorValue.toInt() );
		break;
	case WayOneway:
		m_ui->oneway->setCurrentIndex( modificator.modificatorValue.toInt() );
		break;
	}
}

WayModificatorWidget::Modificator WayModificatorWidget::modificator()
{
	Modificator result;
	result.invert = m_ui->type->currentIndex() == 1;
	result.key = m_ui->key->text();
	result.checkValue = m_ui->useValue->isChecked();
	if ( result.checkValue )
		result.value = m_ui->value->text();
	result.type = ( ModificatorType ) m_ui->action->currentIndex();
	switch ( result.type ) {
	case WayModifyFixed:
		result.modificatorValue = m_ui->fixed->value();
		break;
	case WayModifyPercentage:
		result.modificatorValue = m_ui->percentage->value();
		break;
	case WayAccess:
		result.modificatorValue = m_ui->access->currentIndex();
		break;
	case WayOneway:
		result.modificatorValue = m_ui->oneway->currentIndex();
		break;
	}
	return result;
}
