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

#ifndef MAPDATA_H
#define MAPDATA_H

#include "interfaces/iaddresslookup.h"
#include "interfaces/igpslookup.h"
#include "interfaces/irenderer.h"
#include "interfaces/irouter.h"
#include <QObject>
#include <QString>
#include <QImage>

class MapData : public QObject
{

	Q_OBJECT

public:

	enum PluginType {
		AddressLookup = 0, GPSLookup = 1, Renderer = 2, Router = 3
	};

	~MapData();

	// returns the instance of MapData
	static MapData* instance();
	// the path of the current directory
	QString path() const;
	void setPath( QString path );
	// does the current directory contain MapData?
	bool containsMapData() const;
	// is a certain plugin type necessary for the application?
	// default value is true for all types
	// changes require that load() or loadInformation() have to be called again
	bool pluginRequired( PluginType plugin ) const;
	void setPluginRequired( PluginType plugin, bool required );
	// is a map loaded?
	bool loaded() const;
	// tries to load the current directory
	// automatically calls loadInformation()
	bool load();
	// tries to unload the map data
	bool unload();
	// is the map information loaded?
	bool informationLoaded() const;
	// loads information about the map data
	bool loadInformation();

	// Information about the current directory
	// only valid after loadInformation() or load() was called successfully

	// can the required plugins load the present file formats?
	bool canBeLoaded() const;
	// the name of the MapData
	QString name() const;
	// the description of the MapData
	QString description() const;
	// an representative image of the MapData
	QImage image() const;
	// returns the name of required plugins
	QString pluginName( PluginType plugin ) const;
	// could the plugin be found?
	bool pluginPresent( PluginType plugin ) const;
	// returns the file format version of MapData for certain plugins
	int fileFormatVersion( PluginType plugin ) const;
	// is the file format version compatible with the currently loaded plugin?
	bool fileFormatCompatible( PluginType plugin ) const;

	// returns an instance of a required plugin
	// only available after a successfull call to load()
	IAddressLookup* addressLookup();
	IGPSLookup* gpsLookup();
	IRenderer* renderer();
	IRouter* router();

signals:

	void dataLoaded();
	void dataUnloaded();
	void informationChanged();

public slots:

private:

	explicit MapData();
	struct PrivateImplementation;
	PrivateImplementation* const d;

};

#endif // MAPDATA_H
