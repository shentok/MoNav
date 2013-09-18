/*
 * Copyright 2010, 2011, 2012 mapsforge.org
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAPSFORGE_READBUFFER_H
#define MAPSFORGE_READBUFFER_H

#include <QString>

class QIODevice;

/**
 * Reads from a {@link RandomAccessFile} into a buffer and decodes the data.
 */
class ReadBuffer
{
public:
	ReadBuffer(QIODevice *inputFile);

	int fileSize() const;

	/**
	 * Reads the given amount of bytes from the file into the read buffer and resets the internal buffer position. If
	 * the capacity of the read buffer is too small, a larger one is created automatically.
	 * 
	 * @param length
	 *            the amount of bytes to read from the file.
	 * @return true if the whole data was read successfully, false otherwise.
	 */
	bool readFromFile(int length);

	void setError( const QString &errorMessage );

	bool hasError() const { return m_hasError; }

	/**
	 * Returns one signed byte from the read buffer.
	 *
	 * @return the byte value.
	 */
	qint8 readByte();

	/**
	 * Converts two bytes from the read buffer to a signed int.
	 * <p>
	 * The byte order is big-endian.
	 *
	 * @return the int value.
	 */
	qint16 readShort();

	/**
	 * Converts four bytes from the read buffer to a signed int.
	 * <p>
	 * The byte order is big-endian.
	 * 
	 * @return the int value.
	 */
	qint32 readInt();

	/**
	 * Converts eight bytes from the read buffer to a signed long.
	 * <p>
	 * The byte order is big-endian.
	 * 
	 * @return the long value.
	 */
	qint64 readLong();

	/**
	 * Converts a variable amount of bytes from the read buffer to a signed int.
	 * <p>
	 * The first bit is for continuation info, the other six (last byte) or seven (all other bytes) bits are for data.
	 * The second bit in the last byte indicates the sign of the number.
	 * 
	 * @return the int value.
	 */
	qint32 readSignedInt();

	/**
	 * Converts a variable amount of bytes from the read buffer to an unsigned int.
	 * <p>
	 * The first bit is for continuation info, the other seven bits are for data.
	 * 
	 * @return the int value.
	 */
	quint32 readUnsignedInt();

	/**
	 * Decodes a variable amount of bytes from the read buffer to a string.
	 * 
	 * @return the UTF-8 decoded string (may be null).
	 */
	QString readUTF8EncodedString();

	/**
	 * Decodes the given amount of bytes from the read buffer to a string.
	 * 
	 * @param stringLength
	 *            the length of the string in bytes.
	 * @return the UTF-8 decoded string (may be null).
	 */
	QString readUTF8EncodedString(quint32 stringLength);

	/**
	 * @return the current buffer position.
	 */
	int getBufferPosition() const;

	/**
	 * @return the current size of the read buffer.
	 */
	int getBufferSize() const;

	/**
	 * Sets the buffer position to the given offset.
	 * 
	 * @param bufferPosition
	 *            the buffer position.
	 */
	void setBufferPosition(int bufferPosition);

	/**
	 * Skips the given number of bytes in the read buffer.
	 * 
	 * @param bytes
	 *            the number of bytes to skip.
	 */
	void skipBytes(int bytes);

	/**
	 * Maximum buffer size which is supported by this implementation.
	 */
	static const int MAXIMUM_BUFFER_SIZE = 2500000;

private:
	QByteArray m_bufferData;
	int m_bufferPosition;
	QIODevice *const m_inputFile;
	bool m_hasError;
	QString m_errorMessage;
};

#endif
