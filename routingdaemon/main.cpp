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

#include <QtDebug>

#include "routinddaemon.h"

Q_IMPORT_PLUGIN( contractionhierarchiesclient );
Q_IMPORT_PLUGIN( gpsgridclient );

QtMsgHandler oldHandler = NULL;
RoutingDaemon* servicePointer = NULL;

void MessageBoxHandler( QtMsgType type, const char *msg )
{
	if ( servicePointer == NULL )
		return;
	switch (type) {
	case QtDebugMsg:
		servicePointer->logMessage( msg, QtServiceBase::Information );
		break;
	case QtWarningMsg:
		servicePointer->logMessage( msg, QtServiceBase::Warning );
		break;
	case QtCriticalMsg:
		servicePointer->logMessage( msg, QtServiceBase::Error );
		break;
	case QtFatalMsg:
		servicePointer->logMessage( msg, QtServiceBase::Error );
		exit( -1 );
		break;
	}
	if ( oldHandler != NULL )
		oldHandler( type, msg );
}

int main( int argc, char** argv )
{
	oldHandler = qInstallMsgHandler( MessageBoxHandler );
	RoutingDaemon service( argc, argv );
	servicePointer = &service;
	return service.exec();
}

