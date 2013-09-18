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

#include "ReadBuffer.h"

#include <QDebug>
#include <QIODevice>

ReadBuffer::ReadBuffer(QIODevice *inputFile) :
	m_inputFile(inputFile),
	m_hasError(false),
	m_errorMessage()
{
}

int ReadBuffer::fileSize() const
{
	return m_inputFile->size();
}

bool ReadBuffer::readFromFile(int length)
{
	// ensure that the read buffer is not too large
	if (length > MAXIMUM_BUFFER_SIZE) {
		return false;
	}

	// reset the buffer position and read the data into the buffer
	m_bufferPosition = 0;
	m_bufferData.resize(length);
	return m_inputFile->read(m_bufferData.data(), length) == length;
}

void ReadBuffer::setError(const QString &errorMessage)
{
	if ( m_hasError )
		return;

	qWarning() << errorMessage;
	m_hasError = true;
	m_errorMessage = errorMessage;
}

qint8 ReadBuffer::readByte()
{
	return m_bufferData[m_bufferPosition++];
}

qint16 ReadBuffer::readShort()
{
	qint16 result = 0;
	for (int i = 0; i < 2; ++i ) {
		result = (result << 8) | (m_bufferData[m_bufferPosition++] & 0xff);
	}

	return result;
}

qint32 ReadBuffer::readInt()
{
	qint32 result = 0;
	for (int i = 0; i < 4; ++i ) {
		result = (result << 8) | (m_bufferData[m_bufferPosition++] & 0xff);
	}

	return result;
}

qint64 ReadBuffer::readLong()
{
	qint64 result = 0;
	for (int i = 0; i < 8; ++i ) {
		result = (result << 8) | (m_bufferData[m_bufferPosition++] & 0xff);
	}

	return result;
}

qint32 ReadBuffer::readSignedInt()
{
	qint32 variableByteDecode = 0;
	unsigned char variableByteShift = 0;

	// check if the continuation bit is set
	while ((m_bufferData[m_bufferPosition] & 0x80) != 0) {
		variableByteDecode |= (m_bufferData[m_bufferPosition++] & 0x7f) << variableByteShift;
		variableByteShift += 7;
	}

	// read the six data bits from the last byte
	if ((m_bufferData[m_bufferPosition] & 0x40) != 0) {
		// negative
		return -(variableByteDecode | ((m_bufferData[m_bufferPosition++] & 0x3f) << variableByteShift));
	}
	// positive
	return variableByteDecode | ((m_bufferData[m_bufferPosition++] & 0x3f) << variableByteShift);
}

quint32 ReadBuffer::readUnsignedInt()
{
	quint32 variableByteDecode = 0;
	unsigned char variableByteShift = 0;

	// check if the continuation bit is set
	while ((m_bufferData[m_bufferPosition] & 0x80) != 0) {
		variableByteDecode |= (m_bufferData[m_bufferPosition++] & 0x7f) << variableByteShift;
		variableByteShift += 7;
	}

	// read the seven data bits from the last byte
	return variableByteDecode | (m_bufferData[m_bufferPosition++] << variableByteShift);
}

QString ReadBuffer::readUTF8EncodedString()
{
	const quint32 length = readUnsignedInt();
	return readUTF8EncodedString(length);
}

QString ReadBuffer::readUTF8EncodedString(quint32 stringLength)
{
	if (stringLength <= 0)
		return QString();

	const QByteArray data = m_bufferData.mid(m_bufferPosition, stringLength);
	if (data.length() != stringLength)
		return QString();

	m_bufferPosition += stringLength;
	return QString::fromUtf8(data);
}

int ReadBuffer::getBufferPosition() const
{
	return m_bufferPosition;
}

int ReadBuffer::getBufferSize() const
{
	return m_bufferData.length();
}

void ReadBuffer::setBufferPosition(int bufferPosition)
{
	m_bufferPosition = bufferPosition;
}

void ReadBuffer::skipBytes(int bytes)
{
	m_bufferPosition += bytes;
}
