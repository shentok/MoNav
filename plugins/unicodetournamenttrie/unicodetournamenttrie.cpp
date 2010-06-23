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

#include "unicodetournamenttrie.h"
#include <algorithm>
#include <QDir>
#include <QHash>

UnicodeTournamentTrie::UnicodeTournamentTrie()
{

}

UnicodeTournamentTrie::~UnicodeTournamentTrie()
{

}

QString UnicodeTournamentTrie::GetName()
{
	return "Unicode Tournament Trie";
}

UnicodeTournamentTrie::Type UnicodeTournamentTrie::GetType()
{
	return AddressLookup;
}

void UnicodeTournamentTrie::SetOutputDirectory( const QString& dir )
{
	outputDirectory = dir;
}

void UnicodeTournamentTrie::ShowSettings()
{

}

bool UnicodeTournamentTrie::Preprocess( IImporter* importer )
{
	QDir dir( outputDirectory );
	QString filename = dir.filePath( "Unicode Tournament Trie" );

	QFile subTrieFile( filename + "_sub" );
	QFile mainTrieFile( filename + "_main" );
	QFile wayFile( filename + "_ways" );

	subTrieFile.open( QIODevice::WriteOnly );
	mainTrieFile.open( QIODevice::WriteOnly );
	wayFile.open( QIODevice::WriteOnly );

	std::vector< IImporter::Place > inputPlaces;
	std::vector< IImporter::Address > inputAddress;
	std::vector< UnsignedCoordinate > inputWayBuffer;
	if ( !importer->GetAddressData( &inputPlaces, &inputAddress, &inputWayBuffer ) )
		return false;

	std::vector< PlaceImportance > importanceOrder;
	importanceOrder.reserve( inputPlaces.size() );
	for ( std::vector< IImporter::Place >::const_iterator i = inputPlaces.begin(), e = inputPlaces.end(); i != e; ++i ) {
		PlaceImportance temp;
		temp.population  = i->population;
		temp.name = i->name;
		switch ( i->type ) {
		case IImporter::Place::City:
			{
				temp.type = 6;
				break;
			}
		case IImporter::Place::Town:
			{
				temp.type = 5;
				break;
			}
		case IImporter::Place::Suburb:
			{
				temp.type = 0;
				break;
			}
		case IImporter::Place::Village:
			{
				temp.type = 4;
				break;
			}
		case IImporter::Place::Hamlet:
			{
				temp.type = 3;
				break;
			}
		case IImporter::Place::None:
			{
				temp.type = 0;
				break;
			}
		}
		importanceOrder.push_back( temp );
	}
	std::sort( importanceOrder.begin(), importanceOrder.end() );
	QHash< QString, unsigned > importance;
	for ( int i = 0; i < ( int ) inputPlaces.size(); i++ ) {
		importance[inputPlaces[i].name] = i;
	}
	std::vector< PlaceImportance >().swap( importanceOrder );

	std::sort( inputAddress.begin(), inputAddress.end() );

	std::vector< utt::Node > trie( 1 );

	std::vector< IImporter::Address >::const_iterator address = inputAddress.begin();
	for ( std::vector< IImporter::Place >::const_iterator i = inputPlaces.begin(), e = inputPlaces.end(); i != e; ++i ) {
		if ( i->type != IImporter::Place::Suburb ) {
			utt::Data data;
			data.start = subTrieFile.pos();

			utt::CityData cityData;
			cityData.coordinate = i->coordinate;
			char* buffer = new char[cityData.GetSize()];
			cityData.Write( buffer );
			subTrieFile.write( buffer, cityData.GetSize() );
			delete buffer;

			QHash< QString, double > wayLength;
			for ( std::vector< IImporter::Address >::const_iterator nextAddress = address; nextAddress != inputAddress.end() && nextAddress->nearPlace == i - inputPlaces.begin(); ++nextAddress ) {
				double distance = 0;
				for ( unsigned coord = address->wayStart; coord + 1 < address->wayEnd; ++coord )
					distance += inputWayBuffer[coord].ToProjectedCoordinate().ToGPSCoordinate().ApproximateDistance( inputWayBuffer[coord + 1].ToProjectedCoordinate().ToGPSCoordinate() );
				if ( !wayLength.contains( nextAddress->name ) )
					wayLength[nextAddress->name] = distance;
				else
					wayLength[nextAddress->name] += distance;
			}

			std::vector< WayImportance > wayImportanceOrder;
			wayImportanceOrder.reserve( wayLength.size() );
			for ( QHash< QString, double >::const_iterator length = wayLength.begin(), lengthEnd = wayLength.end(); length != lengthEnd; ++length ) {
				WayImportance temp;
				temp.name = length.key();
				temp.distance = length.value();
				wayImportanceOrder.push_back( temp );
			}
			wayLength.clear();

			std::sort( wayImportanceOrder.begin(), wayImportanceOrder.end() );
			QHash< QString, unsigned > wayImportance;
			for ( std::vector< WayImportance >::const_iterator way = wayImportanceOrder.begin(), wayEnd = wayImportanceOrder.end(); way != wayEnd; ++way )
				wayImportance[way->name] = way - wayImportanceOrder.begin();
			std::vector< WayImportance >().swap( wayImportanceOrder );

			std::vector< utt::Node > subTrie( 1 );
			for ( ;address != inputAddress.end() && address->nearPlace == i - inputPlaces.begin(); ++address ) {
				utt::Data subEntry;
				subEntry.start = address->wayStart;
				subEntry.end = address->wayEnd;
				assert ( wayImportance.contains( address->name ) );
				insert( &subTrie, wayImportance[address->name], address->name, subEntry );
			}

			utt::Data cityCenterData;
			cityCenterData.start = inputWayBuffer.size();
			inputWayBuffer.push_back( i->coordinate );
			inputWayBuffer.push_back( i->coordinate );
			cityCenterData.end = inputWayBuffer.size();
			insert( &subTrie, 0, tr( "City Center" ), cityCenterData );

			writeTrie( &subTrie, subTrieFile );

			data.end = subTrieFile.pos();

			assert( importance.contains( i->name ) );
			insert( &trie, importance[i->name], i->name, data );
		}
		else {
			while ( address != inputAddress.end() && address->nearPlace == i - inputPlaces.begin() )
				++address;
		}
	}
	assert( address == inputAddress.end() );

	writeTrie( &trie, mainTrieFile );

	for ( std::vector< UnsignedCoordinate >::const_iterator i = inputWayBuffer.begin(), e = inputWayBuffer.end(); i != e; ++i ) {
		wayFile.write( ( char* ) &i->x, sizeof( i->x ) );
		wayFile.write( ( char* ) &i->y, sizeof( i->y ) );
	}

	return true;
}

void UnicodeTournamentTrie::insert( std::vector< utt::Node >* trie, unsigned importance, const QString& name, utt::Data data )
{
	unsigned node = 0;
	QString lowerName = name.toLower();
	int position = 0;
	while ( position < lowerName.length() ) {
		bool found = false;
		for ( int c = 0; c < ( int ) trie->at( node ).labelList.size(); c++ ) {
			utt::Label& label = (*trie)[node].labelList[c];
			if ( label.string[0] == lowerName[position] )
			{
				int diffPos = 0;
				for ( ; diffPos < label.string.length(); diffPos++ )
					if ( label.string[diffPos] != lowerName[position + diffPos] )
						break;

				if ( diffPos != label.string.length() )
				{
					utt::Label newEdge;
					newEdge.importance = label.importance;
					newEdge.index = label.index;
					newEdge.string = label.string.mid( diffPos );

					label.string = label.string.left( diffPos );
					label.index = trie->size();
					node = label.index;

					if ( label.importance < importance )
						label.importance = importance;

					trie->push_back( utt::Node() ); //invalidates label reference!!!
					trie->back().labelList.push_back( newEdge );
				}
				else
				{
					node = label.index;
					if ( label.importance < importance )
						label.importance = importance;
				}

				position += diffPos;
				found = true;
				break;
			}
		}

		if ( !found ) {
			utt::Label label;
			label.string = lowerName.mid( position );
			label.index = trie->size();
			label.importance = importance;
			(*trie)[node].labelList.push_back( label );
			trie->push_back( utt::Node() );
			break;
		}
	}

	(*trie)[node].dataList.push_back( data );
}

void UnicodeTournamentTrie::writeTrie( std::vector< utt::Node >* trie, QFile& file )
{
	size_t position = 0;
	std::vector< unsigned > index( trie->size() );
	for ( int i = 0; i < ( int ) trie->size(); i++ ) {
		std::sort( (*trie)[i].labelList.begin(), (*trie)[i].labelList.end() );
		index[i] = position;
		position += (*trie)[i].GetSize();
	}

	for ( int i = 0; i < ( int ) trie->size(); i++ ) {
		for ( int c = 0; c < ( int ) (*trie)[i].labelList.size(); c++ ) {
			(*trie)[i].labelList[c].index = index[(*trie)[i].labelList[c].index];
		}
	}

	char* buffer = new char[position];

	position = 0;
	for ( int i = 0; i < ( int ) trie->size(); i++ ) {
		(*trie)[i].Write( buffer + position );
		utt::Node testElement;
		testElement.Read( buffer + position );
		assert( testElement == (*trie)[i] );
		position += (*trie)[i].GetSize();
	}
	file.write( buffer, position );

	delete buffer;
}

Q_EXPORT_PLUGIN2(unicodetournamenttrie, UnicodeTournamentTrie)
