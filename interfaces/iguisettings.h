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

#ifndef IGUISETTINGS_H
#define IGUISETTINGS_H

#include <QtPlugin>

class IImporter;
class QWidget;

// plugins can support this interface to provide the user
// with a graphical interface for its settings
class IGUISettings
{

public:

	// has to return a valid pointer to a settings window
	// the window can be displayed modal and nonmodal
	// the implementation should be able to cope with this
	// if the window feature settings it should display the following buttons:
	// "Ok", "Apply" and "Cancel"
	virtual bool GetSettingsWindow( QWidget** window ) = 0;

	virtual ~IGUISettings() {}
};

Q_DECLARE_INTERFACE( IGuiSettings, "monav.IGUISettings/1.0" )

#endif // IGUISETTINGS_H
