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

#ifndef STATICKDTREE_H_INCLUDED
#define STATICKDTREE_H_INCLUDED

#include <vector>
#include <algorithm>
#include <stack>
#include <limits>

namespace KDTree {

template< unsigned k, typename T >
class BoundingBox {
	public:
		BoundingBox() {
			for ( unsigned dim = 0; dim < k; ++dim ) {
				min[dim] = std::numeric_limits< T >::min();
				max[dim] = std::numeric_limits< T >::max();
			}
		}

		T min[k];
		T max[k];
};

struct NoData {};

template< unsigned k, typename T >
class EuclidianMetric {
	public:
		double operator() ( T left[k], T right[k] ) {
			double result = 0;
			for ( unsigned i = 0; i < k; ++i ) {
				double temp = left[i] - right[i];
				result += temp * temp;
			}
			return result;
		}
		
		double operator() ( const BoundingBox< k, T > &box, T point[k] ) {
			T nearest[k];
			for ( unsigned dim = 0; dim < k; ++dim ) {
				if ( point[dim] < box.min[dim] )
					nearest[dim] = box.min[dim];
				else if ( point[dim] > box.max[dim] )
					nearest[dim] = box.max[dim];
				else
					nearest[dim] = point[dim];
			}
			return operator() ( point, nearest );
		}
};

template < unsigned k, typename T, typename Data = NoData, typename Metric = EuclidianMetric< k, T > >
class StaticKDTree {
	public:

		struct InputPoint {
			T coordinates[k];
			Data data;
		};

		StaticKDTree( std::vector< InputPoint > points ){
			assert( k > 0 );
			assert ( points.size() > 0 );
			size = points.size();
			kdtree = new InputPoint[size];
			for ( Iterator i = 0; i != size; ++i ) {
				kdtree[i] = points[i];
				for ( unsigned dim = 0; dim < k; ++dim ) {
					if ( kdtree[i].coordinates[dim] < boundingBox.min[dim] )
						boundingBox.min[dim] = kdtree[i].coordinates[dim];
					if ( kdtree[i].coordinates[dim] > boundingBox.max[dim] )
						boundingBox.max[dim] = kdtree[i].coordinates[dim];
				}
			}
			std::stack< Tree > s;
			s.push ( Tree ( 0, size, 0 ) );
			while ( !s.empty() ) {
				Tree tree = s.top();
				s.pop();
				
				if ( tree.left == tree.right )
					continue;
					
				Iterator middle = tree.left + ( tree.right - tree.left ) / 2;
				std::nth_element( kdtree + tree.left, kdtree + middle, kdtree + tree.right, Less( tree.dimension ) );
				s.push( Tree( tree.left, middle, ( tree.dimension + 1 ) % k ) );
				s.push( Tree( middle + 1, tree.right, ( tree.dimension + 1 ) % k ) );
			}
		}
		
		~StaticKDTree(){
			delete[] kdtree;
		}
		
		bool Find ( InputPoint& point ){
			std::stack< Tree > s;
			s.push ( Tree ( kdtree.begin(), kdtree.end() ), 0 );
			while ( !s.empty() ) {
				Tree tree = s.top();
				s.pop();
				
				if ( tree.left == tree.right )
					continue;
					
				Iterator middle = tree.left + ( tree.right - tree.left ) / 2;
				
				bool found = true;
				for ( int i = 0; i < k; ++i ) {
					if ( point.coordinates[i] != kdtree[middle].coordinates[i] ) {
						found = false;
						break;
					}
				}
				if ( found ) {
					point.data = kdtree[middle].data;
					return true;
				}
				
				Less comperator( tree.dimension );
				if ( !comperator( point, kdtree[middle] ) )
					s.push( Tree( middle + 1, tree.right, ( tree.dimension + 1 ) % k ) );
				if ( !comperator( kdtree[middle], point ) )
					s.push( Tree( tree.left, middle, ( tree.dimension + 1 ) % k ) );
			}
			return false;
		}
		
		bool NearestNeighbor( InputPoint* result, InputPoint& point, double radius = std::numeric_limits< T >::max() ) {
			Metric distance;
			bool found = false;
			double nearestDistance = radius;
			std::stack< NNTree > s;
			s.push ( NNTree ( 0, size, 0, boundingBox ) );
			while ( !s.empty() ) {
				NNTree tree = s.top();
				s.pop();
				
				if ( distance( tree.box, point.coordinates ) >= nearestDistance )
					continue;
				
				if ( tree.left == tree.right )
					continue;
					
				Iterator middle = tree.left + ( tree.right - tree.left ) / 2;
				
				double newDistance = distance( kdtree[middle].coordinates, point.coordinates );
				if ( newDistance < nearestDistance ) {
					nearestDistance = newDistance;
					*result = kdtree[middle];
					found = true;
				}
				
				Less comperator( tree.dimension );
				if ( !comperator( point, kdtree[middle] ) ) {
					NNTree first( middle + 1, tree.right, ( tree.dimension + 1 ) % k, tree.box );
					NNTree second( tree.left, middle, ( tree.dimension + 1 ) % k, tree.box );
					first.box.min[tree.dimension] = kdtree[middle].coordinates[tree.dimension];
					second.box.max[tree.dimension] = kdtree[middle].coordinates[tree.dimension];
					s.push( second );
					s.push( first );
				}
				else {
					NNTree first( middle + 1, tree.right, ( tree.dimension + 1 ) % k, tree.box );
					NNTree second( tree.left, middle, ( tree.dimension + 1 ) % k, tree.box );
					first.box.min[tree.dimension] = kdtree[middle].coordinates[tree.dimension];
					second.box.max[tree.dimension] = kdtree[middle].coordinates[tree.dimension];
					s.push( first );
					s.push( second );
				}
			}
			
			return found;
		}
		
		bool NearNeighbors( std::vector< InputPoint >* result, InputPoint& point, double radius = std::numeric_limits< T >::max() ) {
			Metric distance;
			bool found = false;
			std::stack< NNTree > s;
			s.push ( NNTree ( 0, size, 0, boundingBox ) );
			while ( !s.empty() ) {
				NNTree tree = s.top();
				s.pop();
				
				if ( distance( tree.box, point.coordinates ) >= radius )
					continue;
				
				if ( tree.left == tree.right )
					continue;
					
				Iterator middle = tree.left + ( tree.right - tree.left ) / 2;
				
				double newDistance = distance( kdtree[middle].coordinates, point.coordinates );
				if ( newDistance < radius ) {
					result->push_back( kdtree[middle] );
					found = true;
				}
				
				Less comperator( tree.dimension );
				if ( !comperator( point, kdtree[middle] ) ) {
					NNTree first( middle + 1, tree.right, ( tree.dimension + 1 ) % k, tree.box );
					NNTree second( tree.left, middle, ( tree.dimension + 1 ) % k, tree.box );
					first.box.min[tree.dimension] = kdtree[middle].coordinates[tree.dimension];
					second.box.max[tree.dimension] = kdtree[middle].coordinates[tree.dimension];
					s.push( second );
					s.push( first );
				}
				else {
					NNTree first( middle + 1, tree.right, ( tree.dimension + 1 ) % k, tree.box );
					NNTree second( tree.left, middle, ( tree.dimension + 1 ) % k, tree.box );
					first.box.min[tree.dimension] = kdtree[middle].coordinates[tree.dimension];
					second.box.max[tree.dimension] = kdtree[middle].coordinates[tree.dimension];
					s.push( first );
					s.push( second );
				}
			}
			
			return found;
		}

	private:
		typedef unsigned Iterator;
		struct Tree {
			Iterator left;
			Iterator right;
			unsigned dimension;
			Tree() {}
			Tree( Iterator l, Iterator r, unsigned d ): left( l ), right( r ), dimension( d ) {}
		};
		struct NNTree {
			Iterator left;
			Iterator right;
			unsigned dimension;
			BoundingBox< k, T > box;
			NNTree() {}
			NNTree( Iterator l, Iterator r, unsigned d, const BoundingBox< k, T >& b ): left( l ), right( r ), dimension( d ), box ( b ) {}
		};
		class Less {
			public:
				Less( unsigned d ) {
					dimension = d;
					assert( dimension < k );
				}
				
				bool operator() ( const InputPoint& left, const InputPoint& right ) {
					assert( dimension < k );
					return left.coordinates[dimension] < right.coordinates[dimension];
				}
			private:
				unsigned dimension;
		};

		BoundingBox< k, T > boundingBox;
		InputPoint* kdtree;
		Iterator size;
};

}

#endif // STATICKDTREE_H_INCLUDED
