/*
Copyright 2010 Christoph Eckert ce@christeck.de

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

#include "logger.h"
#include "routinglogic.h"
#include "utils/qthelpers.h"

#include <QDesktopServices>

Logger* Logger::instance()
{
	static Logger logger;
	return &logger;
}

Logger::Logger( QObject* parent ) :
	QObject( parent )
{
	initialize();
}

Logger::~Logger()
{
	flushLogfile();
	convertLogToGpx();
}

void Logger::initialize()
{
	// Clean up in case this method gets invoked during runtime
	if ( m_loggingEnabled && !m_logBuffer.isEmpty() ){
		flushLogfile();
		convertLogToGpx();
	}

	m_lastFlushTime = QDateTime::currentDateTime();

	QSettings settings( "MoNavClient" );
	m_loggingEnabled = settings.value( "LoggingEnabled", false ).toBool();
	m_flushInterval = settings.value( "LogFileFlushInterval", 60 ).toInt();
	m_tracklogPath = settings.value( "LogFilePath", QDesktopServices::StandardLocation( QDesktopServices::DocumentsLocation ) ).toString();

	m_tracklogPrefix = "MoNav Tracklog ";
	QString tracklogFilename = m_tracklogPrefix;

	QDateTime currentDateTime = QDateTime::currentDateTime();
	tracklogFilename.append( currentDateTime.toString( "yyyy-MM-dd hh.mm.ss" ) );
	tracklogFilename.append( ".txt" );
	m_logFile.setFileName( fileInDirectory( m_tracklogPath, tracklogFilename ) );
}

void Logger::positionChanged()
{
	if ( !m_loggingEnabled )
		return;

	const RoutingLogic::GPSInfo& gpsInfo = RoutingLogic::instance()->gpsInfo();
	if ( !gpsInfo.position.IsValid() )
		return;

	QString separator = ";";
	QString newline = "\n";
	GPSCoordinate gps = gpsInfo.position.ToGPSCoordinate();

	m_logBuffer.append( QString::number( gps.latitude, 'f', 6 ) );
	m_logBuffer.append( separator );
	m_logBuffer.append( QString::number( gps.longitude, 'f', 6 ) );
	m_logBuffer.append( separator );
	// If unavailable, a "nan" string is returned
	m_logBuffer.append( QString::number( gpsInfo.altitude, 'f', 6 ) );
	m_logBuffer.append( separator );
	m_logBuffer.append( gpsInfo.timestamp.toString( "yyyy-MM-ddThh:mm:ss" ) );
	m_logBuffer.append( newline );

	int flushSecondsPassed = m_lastFlushTime.secsTo( QDateTime::currentDateTime() );
	if ( flushSecondsPassed >= m_flushInterval )
		flushLogfile();
}

bool Logger::flushLogfile()
{
	if ( m_logBuffer.isEmpty() ){
		qDebug() << "Logger: Nothing to flush.";
		return true;
	}

	QString backupFilename = m_logFile.fileName().remove( m_logFile.fileName().size() -4, 4 ).append( "-bck.txt" );

	if ( m_logFile.exists() && !m_logFile.copy( backupFilename ) )
		qDebug() << "Logger: Cannot create logfile backup " << backupFilename;

	if ( !m_logFile.open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append ) ){
		m_loggingEnabled = false;
		qDebug() << "Logger: Cannot write " << m_logFile.fileName() << ". Logging disabled.";
		return false;
	}

	m_logFile.write( m_logBuffer.toUtf8() );
	m_logBuffer.clear();
	qDebug() << "Logger: Logfile written: " << m_logFile.fileName();
	if ( m_logFile.exists( backupFilename ) && !m_logFile.remove( backupFilename ) )
		qDebug() << "Logger: Cannot remove logfile backup " << backupFilename;
	m_lastFlushTime = QDateTime::currentDateTime();
	return true;
}

bool Logger::convertLogToGpx()
{
	QString gpxFileName =  m_logFile.fileName();
	gpxFileName.remove( gpxFileName.size() -4, 4 );
	gpxFileName.append( ".gpx" );

	if ( QFile::exists( gpxFileName ) )
		return false;

	QFile gpxFile( gpxFileName );
	if ( !gpxFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
		return false;

	if ( !m_logFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
	return false;

	QString header;
	QString trackpoint;
	QString footer;
	QString lineBuffer;

	QStringList xmlElements;
	QTextStream xmlStream( &gpxFile );

	header.append( "<?xml version=\"1.0\"?>\n" );
	header.append( "<gpx version=\"1.0\" creator=\"MoNav Tracklogger\" xmlns=\"http://www.topografix.com/GPX/1/0\">\n" );
	header.append( "  <trk>\n" );
	header.append( "    <trkseg>\n" );

	xmlStream << header;

	while ( !m_logFile.atEnd() ) {
		lineBuffer.clear();
		xmlElements.clear();
		lineBuffer.append( m_logFile.readLine() );
		if ( lineBuffer.endsWith( "\n" ) )
			lineBuffer.remove( lineBuffer.size() -1, 1 );

		xmlElements = lineBuffer.split( ";" );

		trackpoint.append( "      <trkpt lat=\"");
		trackpoint.append( xmlElements.at( 0 ) ).append("\" lon=\"");
		trackpoint.append( xmlElements.at( 1 ) ).append( "\">\n" );

		if ( xmlElements.at( 2 ) != "nan" )
			trackpoint.append( "        <ele>" ).append( xmlElements.at( 2 ) ).append("</ele>\n" );

		trackpoint.append( "        <time>" ).append( xmlElements.at( 3 ) ).append( "</time>\n" );
		trackpoint.append( "      </trkpt>\n" );
		xmlStream << trackpoint;
		trackpoint.clear();
	}

	footer.append( "    </trkseg>\n  </trk>\n</gpx>\n" );
	xmlStream << footer;
	gpxFile.close();
	m_logFile.close();
	return true;
}
