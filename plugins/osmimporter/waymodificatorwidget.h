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

#ifndef WAYMODIFICATORWIDGET_H
#define WAYMODIFICATORWIDGET_H

#include <QFrame>
#include <QVariant>

namespace Ui {
	class WayModificatorWidget;
}

class WayModificatorWidget : public QFrame
{
	Q_OBJECT

public:

	enum ModificatorType {
		WayModifyFixed = 0,
		WayModifyPercentage = 1,
		WayAccess = 2,
		WayOneway = 3

	};

	struct Modificator {
		QString key;
		bool checkValue;
		QString value;
		bool invert;
		QVariant modificatorValue;
		ModificatorType type;
	};

	explicit WayModificatorWidget( QWidget* parent = 0 );
	~WayModificatorWidget();

	void setModificator( const Modificator& modificator );
	Modificator modificator();

private:
	Ui::WayModificatorWidget* m_ui;
};

#endif // WAYMODIFICATORWIDGET_H