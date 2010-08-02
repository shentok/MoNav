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

#ifndef BZ2INPUT_H
#define BZ2INPUT_H

#include <stdio.h>
#include <string.h>
#include <bzlib.h>
#include <libxml/xmlreader.h>

struct Context {
	FILE* file;
	BZFILE* bz2;
	bool error;
};

int readFile( void* pointer, char* buffer, int len )
{
	Context* context = (Context*) pointer;
	if ( len == 0 || context->error )
		return 0;

	int error = 0;
	int read = BZ2_bzRead( &error, context->bz2, buffer, len );

	if ( ( error != BZ_OK && error != BZ_STREAM_END ) || read < 0 ) {
		context->error = true;
		return 0;
	}

	return read;
}

int inputClose( void *pointer )
{
	Context* context = (Context*) pointer;
	BZ2_bzclose( context->bz2 );
	fclose( context->file );
	delete context;
	return 0;
}

xmlTextReaderPtr getBz2Reader( const char* name )
{
	Context* context = new Context;
	context->error = false;
	context->file = fopen( name, "r" );
	int error;
	context->bz2 = BZ2_bzReadOpen( &error, context->file, 0, 0, NULL, 0 );
	if ( context->bz2 == NULL || context->file == NULL ) {
		delete context;
		return NULL;
	}

	return xmlReaderForIO( readFile, inputClose, (void*) context, NULL, NULL, 0 );
}

#endif // BZ2INPUT_H
