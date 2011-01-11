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

#ifndef FORMATTEDOUTPUT_H
#define FORMATTEDOUTPUT_H

#include <QStringList>
#include <QTextStream>
#include <QtDebug>
#include <vector>
#include <cassert>
#include <algorithm>

QString printStringTable( QStringList table, int width = 2 )
{
	if ( table.size() % width != 0 ) {
		qWarning() << "printTable: table size does not match width";
		return false;
	}
	assert( width > 0 );

	std::vector< int > size( width );
	for ( int string = 0; string < table.size(); string++ ) {
		int column = string % width;
		size[column] = std::max( size[column], table[string].size() );
	}

	QString result;
	QTextStream stream( &result );
	for ( int string = 0; string < table.size(); string++ ) {
		if ( string != 0 && ( string % width ) == 0 )
			stream << "\n";
		QString entry = table[string];
		stream << entry.leftJustified( size[string % width], ' ' );
	}
}

#endif // FORMATTEDOUTPUT_H
