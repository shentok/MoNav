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

#ifndef BITHELPERS_H
#define BITHELPERS_H

#include <cstring>

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

#endif // BITHELPERS_H
