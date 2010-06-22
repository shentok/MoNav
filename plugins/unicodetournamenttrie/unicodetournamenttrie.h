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

#ifndef UNICODETOURNAMENTTRIE_H
#define UNICODETOURNAMENTTRIE_H

#include "interfaces/ipreprocessor.h"

class UnicodeTournamentTrie : public QObject, public IPreprocessor {
	Q_OBJECT
	Q_INTERFACES( IPreprocessor )

public:

			UnicodeTournamentTrie();
	virtual ~UnicodeTournamentTrie();

	virtual QString GetName();
	virtual Type GetType();
	virtual void SetOutputDirectory( const QString& dir );
	virtual void ShowSettings();
	virtual bool Preprocess( IImporter* importer );

protected:

	struct PlaceImportance {
		QString name;
		int population;
		unsigned type;
		bool operator<( const PlaceImportance& right ) const {
			if ( population != right.population )
				return population < right.population;
			if ( type != right.type )
				return type < right.type;
			return name > right.name;
		}
	};

	struct WayImportance {
		QString name;
		double distance;
		bool operator<( const WayImportance& right ) const {
			if ( distance != right.distance )
				return distance < right.distance;
			return name > right.name;
		}
	};

	QString outputDirectory;
};

#endif // UNICODETOURNAMENTTRIE_H
