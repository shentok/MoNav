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

#include "osmrenderer.h"
#include "utils/qthelpers.h"
#include <QFile>

OSMRenderer::OSMRenderer()
{
	m_settingsDialog = NULL;
}

OSMRenderer::~OSMRenderer()
{
	if ( m_settingsDialog != NULL )
		delete m_settingsDialog;
}

QString OSMRenderer::GetName()
{
	return "OSM Renderer";
}

OSMRenderer::Type OSMRenderer::GetType()
{
	return Renderer;
}

void OSMRenderer::SetOutputDirectory( const QString& directory )
{
	m_directory = directory;
}

void OSMRenderer::ShowSettings()
{
	if ( m_settingsDialog == NULL )
		m_settingsDialog = new ORSettingsDialog();
	m_settingsDialog->exec();
}

bool OSMRenderer::Preprocess( IImporter* )
{
	if ( m_settingsDialog == NULL )
		m_settingsDialog = new ORSettingsDialog;
	ORSettingsDialog::Settings settings;
	if ( !m_settingsDialog->getSettings( &settings ) )
		return false;

	QFile settingsFile( fileInDirectory( m_directory, "OSM Renderer" ) + "_settings" );
	if ( !openQFile( &settingsFile, QIODevice::WriteOnly ) )
		return false;

	for ( int zoom = 0; zoom < ( int ) settings.zoomLevels.size(); zoom ++ ) {
		int zoomLevel = settings.zoomLevels[zoom];
		settingsFile.write( ( const char* ) &zoomLevel, sizeof( zoomLevel ) );
	}
	return true;
}

Q_EXPORT_PLUGIN2( osmrenderer, OSMRenderer )
