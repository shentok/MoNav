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

#ifndef CONTRACTIONHIERARCHIESCLIENT_H
#define CONTRACTIONHIERARCHIESCLIENT_H

#include <QObject>
#include "interfaces/irouter.h"
#include "binaryheap.h"
#include "compressedgraph.h"
#include <queue>

class ContractionHierarchiesClient : public QObject, public IRouter
{
	Q_OBJECT
	Q_INTERFACES( IRouter )
public:
	ContractionHierarchiesClient();
	virtual ~ContractionHierarchiesClient();

	virtual QString GetName();
	virtual void SetInputDirectory( const QString& dir );
	virtual void ShowSettings();
	virtual bool LoadData();
	virtual bool GetRoute( double* distance, QVector< UnsignedCoordinate>* path, const IGPSLookup::Result& source, const IGPSLookup::Result& target );

protected:
	struct _HeapData {
		CompressedGraph::NodeIterator parent;
		bool stalled: 1;
		_HeapData( CompressedGraph::NodeIterator p ) {
			parent = p;
			stalled = false;
		}
	};

	class AllowForwardEdge {
		public:
			bool operator()( bool forward, bool /*backward*/ ) const {
				return forward;
			}
	};

	class AllowBackwardEdge {
		public:
			bool operator()( bool /*forward*/, bool backward ) const {
				return backward;
			}
	};

	typedef CompressedGraph::NodeIterator Node;
	typedef CompressedGraph::EdgeIterator EdgeIterator;
	typedef BinaryHeap< Node, int, int, _HeapData, MapStorage< Node, unsigned > > _Heap;

	CompressedGraph graph;
	_Heap* heapForward;
	_Heap* heapBackward;
	std::queue< Node > stallQueue;

	void unload();
	template< class EdgeAllowed, class StallEdgeAllowed >
	void computeStep( _Heap* heapForward, _Heap* heapBackward, const EdgeAllowed& edgeAllowed, const StallEdgeAllowed& stallEdgeAllowed, Node* middle, int* targetDistance );
	int computeRoute( const IGPSLookup::Result& source, const IGPSLookup::Result& target, QVector< UnsignedCoordinate >* path );
	bool unpackEdge( const Node source, const Node target, bool forward, QVector< UnsignedCoordinate >* path );

	QString directory;

};

#endif // CONTRACTIONHIERARCHIESCLIENT_H
