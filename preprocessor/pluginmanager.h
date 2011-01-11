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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QVector>
#include <QObject>
#include <QStringList>

/*class PluginManager : public QObject
{
	Q_OBJECT

public:

	// returns the single instance of this class
	static PluginManager* instance();

	// loads dynamic plugins and initializes dynamic and static plugins
	bool loadPlugins();

	// returns a list of plugins
	// plugins are uniquely identified by their name
	QStringList importerPlugins();
	QStringList routerPlugins();
	QStringList gpsLookupPlugins();
	QStringList rendererPlugins();
	QStringList addressLookupPlugins();

	// returns pointers to the plugins
	QVector< QObject* > plugins();

	// input filename for processing
	QString inputFile();
	// name of the data set to be created
	QString name();
	// description of the data set to be created
	QString description();
	// image filename of the data set to be created
	QString image();

	// waits until all current processing steps are finished
	void waitForFinish();
	// clears the current processing queue
	void abort();

public slots:

	void setInputFile( QString filename );
	void setName( QString name );
	void setDescription( QString description );
	void setImage( QString image );

	// process plugins asynchronously
	void processImporter( QString plugin );
	void processRouter( QString plugin );
	void processGPSLookup( QString plugin );
	void processRenderer( QString plugin );
	void processAddressLookup( QString plugin );

	// writes config file
	void writeConfig();
	// deletes temporary files
	void deleteTemporaryFiles();

signals:

	// finished a preprocessing item
	void finishedProcessingItem( bool success );
	// log a debug / warning / error message
	void log( QtMsgType type, QString text );

private:

	PluginManager();
	~PluginManager();
};*/

#endif // PLUGINMANAGER_H
