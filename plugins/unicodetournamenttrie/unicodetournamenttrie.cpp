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
#include "trie.h"


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
	return true;
}

void UnicodeTournamentTrie::insert( QVector< utt::Node >* trie, unsigned importance, const QString& name, utt::Data )
{
	unsigned node = 0;
	const QVector< utt::Node >& constTrie = trie;
		QString lowerName = name.toLower();
		int position = 0;
		while ( position < lowerName.length() ) {
			bool found = false;
			for ( int c = 0; c < constTrie[node].labelList.size(); c++ ) {
				utt::Label& label = constTrie[node].labelList[c];
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
						trie->push_back( utt::Node );
						trie->last().labelList.push_back( newEdge );

						label.string = label.string.left( diffPos );
						label.index = trie->size() - 1;
					}

					position += diffPos;
					node = label.index;
					found = true;
					break;
				}
			}

			if ( !found ) {
				utt::Label label;
				label.string = lowerName.mid( position );
				label.index = trie.size();
				label.importance = importance;
				trie[node].characterList.push_back( label );
				trie.push_back( utt::Node() );
				break;
			}
		}

		trie[node].finishedList.push_back( data );
}

void UnicodeTournamentTrie::writeTrie( const QVector< utt::Node >& trie, QFile& file )
{

}

Q_EXPORT_PLUGIN2(unicodetournamenttrie, UnicodeTournamentTrie)
