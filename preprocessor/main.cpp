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

#include "preprocessingwindow.h"
#include <QtPlugin>
#include <QtGui/QApplication>
#include <QMessageBox>
#include <QThread>
#include <cstdio>

Q_IMPORT_PLUGIN( mapnikrenderer );
Q_IMPORT_PLUGIN( contractionhierarchies );
Q_IMPORT_PLUGIN( gpsgrid );
Q_IMPORT_PLUGIN( unicodetournamenttrie );
Q_IMPORT_PLUGIN( osmrenderer );
Q_IMPORT_PLUGIN( qtilerenderer );
Q_IMPORT_PLUGIN( osmimporter );

bool isSilent = false;

void MessageBoxHandler(QtMsgType type, const char *msg)
{
	if ( !isSilent && QApplication::instance() != NULL ) {
		const bool isGuiThread = QThread::currentThread() == QApplication::instance()->thread();
		if ( isGuiThread ) {
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
		}
	}
	printf( "%s\n", msg );
}

void help()
{
	qDebug( "MoNav Preprocessor Usage:" );
	qDebug( "\t--silent             No GUI will be used. A preprocessing action has to be specified." );
	qDebug( "\t--all                Action: All preprocessing steps will be computed." );
	qDebug( "\t--daemon             Action: All preprocessing steps for the routing daemon will be computed." );
	qDebug( "\t--config=<filename>  Specifies a config file." );
	qDebug( "\t--name=<string>      Overwrite the name specified in the settings." );
	qDebug( "\t--description=<text> Overwrite the description specified in the settings." );
	qDebug( "\t--image=<filename>   Overwrite the image specified in the settings." );
	qDebug( "\t--input=<filename>   Overwrite the input filename specified in the settings." );
	qDebug( "\t--ouput=<directory>  Overwrite the output directory specified in the settings." );
	qDebug( "\t--threads=<number>   Overwrite the number of threads specified in the settings." );
}

int main(int argc, char *argv[])
{
	qInstallMsgHandler( MessageBoxHandler );
	QApplication a(argc, argv);

	QStringList args = QApplication::arguments();
	if ( args.contains( "--help" ) || args.contains( "-h" ) || args.contains( "/?" ) ) {
		help();
		return 0;
	}
	if ( args.contains( "--silent" ) )
		isSilent = true;

	if ( isSilent ) {
		QString configFile;
		int index = args.indexOf( QRegExp( "--config=.*" ) );
		if ( index != -1 )
			configFile = args[index].section( '=', 1 );
		PreprocessingWindow w;
		if ( args.contains( "--all") )
			return w.preprocessAll() ? 0 : 1;
		if ( args.contains( "--daemon" ) )
			return w.preprocessDaemon() ? 0 : 1;
		help();
	} else {
		PreprocessingWindow w;
		w.show();
		return a.exec();
	}
}
