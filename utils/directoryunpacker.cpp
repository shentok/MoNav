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

#include "directoryunpacker.h"

#include "utils/qthelpers.h"
#include "utils/lzma/LzmaDec.h"

#include <QFile>
#include <QByteArray>
#include <QtEndian>
#include <QBuffer>
#include <QVector>
#include <QDir>
#include <algorithm>

#include <string.h>

struct DirectoryUnpacker::PrivateImplementation {

	static void* SzAlloc( void* /*p*/, size_t size )
	{
		return ( void* ) new char[size];
	}

	static void SzFree( void* /*p*/, void *address )
	{
		delete ( char* ) address;
	}

	QString filename;
	QFile inputFile;
	QFile outputFile;

	QByteArray block;
	int blockPosition;

	unsigned char* inputBuffer;
	unsigned char* outputBuffer;
	int bufferSize;

	struct File {
		QString name;
		qint64 size;
	};
	QVector< File > fileList;

	bool readHeader()
	{
		QByteArray id = inputFile.read( strlen( "MoNav Map Module" ) );
		if ( id != "MoNav Map Module" ) {
			qCritical() << "Wrong file type";
			return false;
		}
		return true;
	}

	bool readFileInfo()
	{
		if ( !readBlock() )
			return false;

		QBuffer buffer( &block );
		buffer.open( QIODevice::ReadOnly );

		int numFiles;
		if ( buffer.read( ( char * ) &numFiles, sizeof( int ) ) != sizeof( int ) )
			return false;
		numFiles = qFromLittleEndian( numFiles );

		for ( int i = 0; i < numFiles; i++ ) {
			File file;
			int stringLength;
			if ( buffer.read( ( char* ) &stringLength, sizeof( int ) ) != sizeof( int ) )
				return false;
			stringLength = qFromLittleEndian( stringLength );
			QByteArray data = buffer.read( stringLength );
			if ( data.size() != stringLength )
				return false;
			file.name = QString::fromUtf8( data.constData(), stringLength );
			if ( buffer.read( ( char* ) &file.size, sizeof( qint64 ) ) != sizeof( qint64 ) )
				return false;
			file.size = qFromLittleEndian( file.size );
			fileList.push_back( file );
		}

		return true;
	}

	bool readBlock()
	{
		int size;
		if ( inputFile.read( ( char* ) &size, sizeof( int ) ) != sizeof( int ) )
			return false;
		size = qFromLittleEndian( size );
		quint16 checksum;
		if ( inputFile.read( ( char* ) &checksum, sizeof( quint16 ) ) != sizeof( quint16 ) )
			return false;
		checksum = qFromLittleEndian( checksum );

		//qDebug() << "reading block" << size;
		block = inputFile.read( size );
		if ( block.size() != size )
			return false;
		int readChecksum = qChecksum( block.constData(), block.size() );
		if ( checksum != readChecksum ) {
			qCritical() << "Checksum not correct, file corrupted";
			return false;
		}
		blockPosition = 0;
		//qDebug() << "read block:" << block.size();
		//qDebug() << block.toBase64();
		return true;
	}

	int readFromBlock()
	{
		if ( blockPosition == block.size() ) {
			if ( !readBlock() )
				return 0;
			blockPosition = 0;
		}

		int read = std::min( block.size() - blockPosition, bufferSize );
		//qDebug() << "reading" << blockPosition << "->" << read << block.size();
		memcpy( inputBuffer, block.constData() + blockPosition, read );
		blockPosition += read;
		return read;
	}

	bool decode( qint64 size )
	{
		if ( !readBlock() )
			return false;

		// header: 5 bytes of LZMA properties
		unsigned char header[LZMA_PROPS_SIZE];

		if ( block.size() < LZMA_PROPS_SIZE ) {
			qCritical() << "Error reading LZMA header";
			return false;
		}
		memcpy( header, block.constData(), LZMA_PROPS_SIZE );
		blockPosition = LZMA_PROPS_SIZE;

		//qDebug() << ( int ) header[0] << ( int ) header[1] << ( int ) header[2] << ( int ) header[3] << ( int ) header[4];

		CLzmaDec state;
		LzmaDec_Construct( &state );

		ISzAlloc g_Alloc = { SzAlloc, SzFree };
		if ( LzmaDec_Allocate( &state, header, LZMA_PROPS_SIZE, &g_Alloc ) != SZ_OK ) {
			qCritical() << "Error allocating LZMA data structures";
			return false;
		}
		LzmaDec_Init( &state );

		bool result = true;

		size_t inPos = 0;
		size_t inSize = 0;
		size_t outPos = 0;
		qint64 decoded = 0;

		while ( true ) {
			//qDebug() << "round" << inPos << inSize << outPos;
			if ( inPos == inSize ) {
				inPos = 0;
				inSize = readFromBlock();
			}
			//qDebug() << "read" << inPos << inSize << outPos;

			size_t inLeft = inSize - inPos;
			size_t outLeft = bufferSize - outPos;
			ELzmaFinishMode mode = LZMA_FINISH_ANY;
			if ( outLeft + decoded >= ( size_t ) size ) {
				mode = LZMA_FINISH_END;
				outLeft = size - decoded;
			}
			ELzmaStatus status;
			//qDebug() << inLeft << outLeft;
			SRes res = LzmaDec_DecodeToBuf( &state, outputBuffer + outPos, &outLeft, inputBuffer + inPos, &inLeft, mode, &status );
			if ( res != SZ_OK ) {
				//qDebug() << res;
				qCritical() << "Error decoding LZMA stream";
				result = false;
				break;
			}
			inPos += inLeft;
			outPos += outLeft;
			decoded += outLeft;

			if ( ( size_t ) outputFile.write( ( const char* ) outputBuffer, outPos ) != outPos ) {
				qCritical() << "Error writing to file:" << outputFile.fileName();
				result = false;
				break;
			}
			outPos = 0;

			if ( decoded == size )
				break;

			if ( inLeft == 0 && outLeft == 0) {
				qCritical() << "Internal LZMA error";
				result = false;
				break;
			}
		}

		LzmaDec_Free( &state, &g_Alloc );

		return result;
	}

	bool uncompress()
	{
		QString dirName = filename.replace( ".mmm", "" );
		QDir dir( dirName );
		if ( !dir.exists() )
			dir.mkdir( filename );
		dir.cd( filename );

		for ( int i = 0; i < fileList.size(); i++ ) {
			if ( outputFile.isOpen() )
				outputFile.close();
			outputFile.setFileName( dir.filePath( fileList[i].name ) );
			if ( !openQFile( &outputFile, QIODevice::WriteOnly ) )
				return false;
			qDebug() << "Decoding:" << fileList[i].name << ", size:" << fileList[i].size;
			if ( !decode( fileList[i].size ) )
				return false;
			qDebug() << "Finished decoding:" << fileList[i].name;
		}
		return true;
	}
};

DirectoryUnpacker::DirectoryUnpacker( QString filename, int bufferSize )
{
	d = new PrivateImplementation;
	d->filename = filename;
	d->inputBuffer = new unsigned char[bufferSize];
	d->outputBuffer = new unsigned char[bufferSize];
	d->bufferSize = bufferSize;
}

DirectoryUnpacker::~DirectoryUnpacker()
{
	delete d->inputBuffer;
	delete d->outputBuffer;
	delete d;
}

bool DirectoryUnpacker::decompress( bool deleteFile )
{
	Timer time;
	qDebug() << "Unpacking MoNav Map Module:" << d->filename;
	d->inputFile.setFileName( d->filename );
	if ( !openQFile( &d->inputFile, QIODevice::ReadOnly ) ) {
		qCritical() << "Failed to open file:" << d->filename;
		return false;
	}
	if ( !d->readHeader() ) {
		qCritical() << "Failed to read header";
		return false;
	}
	if ( !d->readFileInfo() ) {
		qCritical()  << "Failed to read file list";
		return false;
	}
	if ( !d->uncompress() ) {
		qCritical() << "Failed to decompress files";
		return false;
	}

	qint64 combinedSize = 0;
	foreach( const PrivateImplementation::File& info, d->fileList )
		combinedSize += info.size;
	qDebug() << "Finished unpacking:" << time.elapsed() / 1000 << "s" << combinedSize / 1024.0 / 1024.0 / ( time.elapsed() / 1000.0 ) << "MB/s";

	d->inputFile.close();
	if ( deleteFile ) {
		d->inputFile.remove();
	}
	return true;
}
