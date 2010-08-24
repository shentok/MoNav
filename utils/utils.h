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

#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

struct DoublePoint {
	DoublePoint(){
		x = y = 0;
	}
	DoublePoint( double xVal, double yVal ) {
		x = xVal;
		y = yVal;
	}
	double x;
	double y;
};

static inline bool pointInPolygon( int numPoints, const DoublePoint* points, DoublePoint testPoint ) {
	bool inside = false;
	for ( int i = 0, j = numPoints - 1; i < numPoints; j = i++ ) {
		if ( ((  points[i].y > testPoint.y ) != ( points[j].y > testPoint.y ) ) && ( testPoint.x < ( points[j].x - points[i].x ) * ( testPoint.y - points[i].y ) / ( points[j].y - points[i].y ) + points[i].x ) )
			inside = !inside;
	}
	return inside;
}

static inline bool clipHelper( double directedProjection, double directedDistance, double* tMinimum, double* tMaximum ) {
	if ( directedProjection == 0 ) {
		if ( directedDistance < 0 )
			return false;
	} else {
		double amount = directedDistance / directedProjection;
		if ( directedProjection < 0 ) {
			if ( amount > *tMaximum )
				return false;
			else if ( amount > *tMinimum )
				*tMinimum = amount;
		} else {
			if ( amount < *tMinimum )
				return false;
			else if ( amount < *tMaximum )
				*tMaximum = amount;
		}
	}
	return true;
}

static inline bool clipEdge( ProjectedCoordinate* start, ProjectedCoordinate* end, ProjectedCoordinate min, ProjectedCoordinate max ) {
	const double xDiff = ( double ) end->x - start->x;
	const double yDiff = ( double ) end->y - start->y;
	double tMinimum = 0, tMaximum = 1;

	if ( clipHelper( -xDiff, ( double ) start->x - min.x, &tMinimum, &tMaximum ) ) {
		if ( clipHelper( xDiff, ( double ) max.x - start->x, &tMinimum, &tMaximum ) ) {
			if ( clipHelper( -yDiff, ( double ) start->y - min.y, &tMinimum, &tMaximum ) ) {
				if ( clipHelper( yDiff, ( double ) max.y - start->y, &tMinimum, &tMaximum ) ) {
					if ( tMaximum < 1 ) {
						end->x = start->x + tMaximum * xDiff;
						end->y = start->y + tMaximum * yDiff;
					}
					if ( tMinimum > 0 ) {
						start->x += tMinimum * xDiff;
						start->y += tMinimum * yDiff;
					}
					if ( start->x < min.x )
						start->x = min.x;
					if ( start->y < min.y )
						start->y = min.y;
					if ( start->x > max.x )
						start->x = max.x;
					if ( start->y > max.y )
						start->y = max.y;
						
					if ( end->x < min.x )
						end->x = min.x;
					if ( end->y < min.y )
						end->y = min.y;
					if ( end->x > max.x )
						end->x = max.x;
					if ( end->y > max.y )
						end->y = max.y;
					
					return true;
				}
			}
		}
	}
	return false;

}

template< class T >
static inline T readUnaligned( const char* buffer ) {
	T temp;
	memcpy( &temp, buffer, sizeof( T ) );
	return temp;
}

// reads first bits to a max of 31 bits ( 31 because 1u << 32 is undefined )
// offset has to be <8
// safe with unaligned memory access
static inline unsigned read_unaligned_unsigned( const unsigned char* buffer, int offset ){
	assert ( offset <= 7 );

	const int diff = ( ( size_t ) buffer ) & 3;
	buffer -= diff;
	offset += 8 * diff;

	unsigned temp = * ( unsigned * ) buffer;
	if ( offset == 0 )
		return temp;
	unsigned temp2 = * ( ( ( unsigned * ) buffer ) + 1);
	return ( temp >> offset ) | ( temp2 << ( 32 - offset ) );
}

static inline unsigned read_unaligned_unsigned( const unsigned char** buffer, int bits, int* offset ){
	assert ( *offset <= 7 );

	const int diff = ( ( size_t ) *buffer ) & 3;
	const unsigned char* alignedBuffer = *buffer - diff;
	int alignedOffset = *offset + 8 * diff;
	unsigned temp = * ( unsigned * ) alignedBuffer;
	unsigned temp2 = * ( ( ( unsigned * ) alignedBuffer ) + 1);
	unsigned result;
	if ( alignedOffset == 0 )
		result =  temp;
	else
		result =  ( temp >> alignedOffset ) | ( temp2 << ( 32 - alignedOffset ) );
		
	*offset += bits;
	*buffer += ( *offset ) >> 3;
	*offset &= 7;
	
	if ( bits == 32 )
		return result;
	
	return result & ( ( 1u << bits ) - 1 );
}

static inline unsigned read_unaligned_unsigned( const unsigned char* buffer, int bits, int offset ){
	assert ( offset <= 7 );

	const int diff = ( ( size_t ) buffer ) & 3;
	const unsigned char* alignedBuffer = buffer - diff;
	int alignedOffset = offset + 8 * diff;
	unsigned temp = * ( unsigned * ) alignedBuffer;
	unsigned temp2 = * ( ( ( unsigned * ) alignedBuffer ) + 1);
	unsigned result;
	if ( alignedOffset == 0 )
		result =  temp;
	else
		result =  ( temp >> alignedOffset ) | ( temp2 << ( 32 - alignedOffset ) );

	if ( bits == 32 )
		return result;

	return result & ( ( 1u << bits ) - 1 );
}

// writes #bits bits of data into the buffer at the offset
// offset has to be <8, **buffer has to be zeroed
// modifies buffer and offset to point after the inserted data
static inline void write_unaligned_unsigned( unsigned char** buffer, unsigned data, int bits, int* offset ) {
	( ( unsigned* ) *buffer )[0] |= ( data << ( *offset ) );
	( ( unsigned* ) ( *buffer + 1 ) )[0] |= ( data >> ( 8 - *offset ) );
	
#ifndef NDEBUG
	const unsigned char* tempBuffer = *buffer;
	int tempOffset = *offset;
	unsigned tempData = read_unaligned_unsigned( &tempBuffer, bits, &tempOffset );
	assert( tempData == data );
#endif
	
	*offset += bits;
	*buffer += ( *offset ) >> 3;
	*offset &= 7;
}

static inline unsigned read_bits ( unsigned data, char bits ) {
	if ( bits == 32 )
		return data;

	return data & ( ( 1u << bits ) - 1 );
}

static inline unsigned log2_rounded ( unsigned x ) {
	static const unsigned bit_position[32] = {
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};

	//round up
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	++x;

	return bit_position[ ( x * 0x077CB531u ) >> 27];
}

// computes log2_rounded( x + 1 ), works even up to x = 2^32 - 1
static inline unsigned bits_needed( unsigned x )
{
	static const unsigned bit_position[32] = {
		32, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};
	//slower, maybe think of a better workaround
	if ( x == 0 )
		return 0;

	//+1 and round up
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	++x;

	return bit_position[ ( x * 0x077CB531u ) >> 27];
}

#endif // UTILS_H_INCLUDED
