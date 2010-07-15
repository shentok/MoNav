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

#include <QtGui/QApplication>
#include "mainwindow.h"
#include <QMessageBox>
#include <cstdio>
#include <QtPlugin>

Q_IMPORT_PLUGIN( mapnikrendererclient );
Q_IMPORT_PLUGIN( contractionhierarchiesclient );
Q_IMPORT_PLUGIN( gpsgridclient );
Q_IMPORT_PLUGIN( unicodetournamenttrieclient );
Q_IMPORT_PLUGIN( osmrendererclient );

QtMsgHandler oldHandler = NULL;

void MessageBoxHandler(QtMsgType type, const char *msg)
{
	switch (type) {
	case QtDebugMsg:
		//QMessageBox::information(0, "Debug message", msg, QMessageBox::Ok);
		break;
	case QtWarningMsg:
		QMessageBox::warning(0, "Warning", msg, QMessageBox::Ok);
		break;
	case QtCriticalMsg:
		QMessageBox::critical(0, "Critical error", msg, QMessageBox::Ok);
		break;
	case QtFatalMsg:
		QMessageBox::critical(0, "Fatal error", msg, QMessageBox::Ok);
		abort();
	}
	printf( "%s\n", msg );
	if ( oldHandler != NULL )
		oldHandler( type, msg );
}

int main(int argc, char *argv[])
{
	oldHandler = qInstallMsgHandler( MessageBoxHandler );
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
