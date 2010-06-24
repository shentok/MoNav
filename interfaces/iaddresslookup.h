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

#ifndef IADDRESSLOOKUP_H
#define IADDRESSLOOKUP_H

#include "utils/coordinates.h"
#include <QtPlugin>
#include <QVector>

class IAddressLookup
{
public:
	virtual ~IAddressLookup() {}

	virtual QString GetName() = 0;
	virtual void SetInputDirectory( const QString& dir ) = 0;
	virtual void ShowSettings() = 0;
	virtual bool LoadData() = 0;
	virtual bool GetPlaceSuggestions( const QString& input, int amount, QStringList* suggestions, QStringList* inputSuggestions ) = 0;
	virtual bool GetStreetSuggestions( const QString& input, int amount, QStringList* suggestions, QStringList* inputSuggestions ) = 0;
	virtual bool GetPlaceData( QString input, QVector< int >* placeIDs, QVector< UnsignedCoordinate >* placeCoordinates ) = 0;
	virtual bool SelectPlace( int placeID ) = 0;
	virtual bool GetStreetData( QString input, QVector< int >* segmentLength, QVector< UnsignedCoordinate >* coordinates ) = 0;
};

Q_DECLARE_INTERFACE( IAddressLookup, "monav.IAddressLookup/1.1" )

#endif // IADDRESSLOOKUP_H
