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
#include "utils/qthelpers.h"
#include <algorithm>
#include <QHash>
#include <limits>
#ifndef NDEBUG
	#include <QTextStream>
#endif

UnicodeTournamentTrie::UnicodeTournamentTrie()
{
	settingsDialog = NULL;
}

UnicodeTournamentTrie::~UnicodeTournamentTrie()
{
	if ( settingsDialog != NULL )
		delete settingsDialog;
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
	if ( settingsDialog == NULL )
		settingsDialog = new UTTSettingsDialog();
	settingsDialog->exec();
}

bool UnicodeTournamentTrie::Preprocess( IImporter* importer )
{
	if ( settingsDialog == NULL )
		settingsDialog = new UTTSettingsDialog();

	UTTSettingsDialog::Settings settings;
	if ( !settingsDialog->getSettings( &settings ) )
		return false;
	QString filename = fileInDirectory( outputDirectory, "Unicode Tournament Trie" );

	QFile subTrieFile( filename + "_sub" );
	QFile mainTrieFile( filename + "_main" );
	QFile wayFile( filename + "_ways" );

	if ( !openQFile( &subTrieFile, QIODevice::WriteOnly ) )
		return false;
	if ( !openQFile( &mainTrieFile, QIODevice::WriteOnly ) )
		return false;
	if ( !openQFile( &wayFile, QIODevice::WriteOnly ) )
		return false;

	std::vector< IImporter::Place > inputPlaces;
	std::vector< IImporter::Address > inputAddress;
	std::vector< UnsignedCoordinate > inputWayBuffer;
	std::vector< QString > inputWayNames;
	if ( !importer->GetAddressData( &inputPlaces, &inputAddress, &inputWayBuffer ) )
		return false;
	if ( !importer->GetRoutingWayNames( &inputWayNames ) )
		return false;

	Timer time;

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
	for ( int i = 0; i < ( int ) importanceOrder.size(); i++ )
		importance[importanceOrder[i].name] = i;
	std::vector< PlaceImportance >().swap( importanceOrder );

	std::sort( inputAddress.begin(), inputAddress.end() );
	qDebug() << "Unicode Tournament Trie: sorted addresses by importance:" << time.restart() << "s";

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
			delete[] buffer;

			QHash< QString, double > wayLength;
			for ( std::vector< IImporter::Address >::const_iterator nextAddress = address; nextAddress != inputAddress.end() && nextAddress->nearPlace == i - inputPlaces.begin(); ++nextAddress ) {
				double distance = 0;
				for ( unsigned coord = address->pathID; coord + 1 < address->pathID + address->pathLength; ++coord )
					distance += inputWayBuffer[coord].ToProjectedCoordinate().ToGPSCoordinate().ApproximateDistance( inputWayBuffer[coord + 1].ToProjectedCoordinate().ToGPSCoordinate() );
				QString name = inputWayNames[nextAddress->name];
				if ( !wayLength.contains( name ) )
					wayLength[name] = distance;
				else
					wayLength[name] += distance;
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
				subEntry.start = address->pathID;
				subEntry.length = address->pathLength;
				QString name = inputWayNames[address->name];
				assert ( wayImportance.contains( name ) );
				insert( &subTrie, wayImportance[name], name, subEntry );
			}

			utt::Data cityCenterData;
			cityCenterData.start = inputWayBuffer.size();
			inputWayBuffer.push_back( i->coordinate );
			inputWayBuffer.push_back( i->coordinate );
			cityCenterData.length = 2;
			insert( &subTrie, std::numeric_limits< unsigned >::max(), tr( "City Center" ), cityCenterData );

			writeTrie( &subTrie, subTrieFile );

			data.length = subTrieFile.pos() - data.start;

			assert( importance.contains( i->name ) );
			insert( &trie, importance[i->name], i->name, data );
		}
		else {
			while ( address != inputAddress.end() && address->nearPlace == i - inputPlaces.begin() )
				++address;
		}
	}
	assert( address == inputAddress.end() );
	qDebug() << "Unicode Tournament Trie: build tries and tournament trees:" << time.restart() << "s";

	writeTrie( &trie, mainTrieFile );
	qDebug() << "Unicode Tournament Trie: wrote tries:" << time.restart() << "s";

	for ( std::vector< UnsignedCoordinate >::const_iterator i = inputWayBuffer.begin(), e = inputWayBuffer.end(); i != e; ++i ) {
		wayFile.write( ( char* ) &i->x, sizeof( i->x ) );
		wayFile.write( ( char* ) &i->y, sizeof( i->y ) );
	}
	qDebug() << "Unicode Tournament Trie: wrote ways:" << time.restart() << "s";

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

			node = trie->size();
			trie->push_back( utt::Node() );
			break;
		}
	}

	(*trie)[node].dataList.push_back( data );
}

void UnicodeTournamentTrie::writeTrie( std::vector< utt::Node >* trie, QFile& file )
{
	if ( trie->size() == 0 )
		return;

	size_t position = 0;
	std::vector< unsigned > index( trie->size() );
	std::vector< unsigned > stack;
	std::vector< unsigned > order;
	stack.push_back( 0 );
	while ( !stack.empty() ) {
		unsigned node = stack.back();
		stack.pop_back();
		order.push_back( node );

		index[node] = position;
		position += (*trie)[node].GetSize();

		std::sort( (*trie)[node].labelList.begin(), (*trie)[node].labelList.end() );
		for ( int i = (*trie)[node].labelList.size() - 1; i >= 0; i-- )
			stack.push_back( (*trie)[node].labelList[i].index );
	}

	for ( int i = 0; i < ( int ) trie->size(); i++ ) {
		for ( int c = 0; c < ( int ) (*trie)[i].labelList.size(); c++ ) {
			(*trie)[i].labelList[c].index = index[(*trie)[i].labelList[c].index];
		}
	}
	assert( order.size() == trie->size() );

	char* buffer = new char[position];

	position = 0;
	for ( int i = 0; i < ( int ) order.size(); i++ ) {
		unsigned node = order[i];
		(*trie)[node].Write( buffer + position );
		utt::Node testElement;
		testElement.Read( buffer + position );
		assert( testElement == (*trie)[node] );
		position += (*trie)[node].GetSize();
	}
	file.write( buffer, position );

	delete[] buffer;
}

Q_EXPORT_PLUGIN2(unicodetournamenttrie, UnicodeTournamentTrie)
