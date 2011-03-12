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

#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <QObject>
#include <QDebug>
#include <QSettings>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QFile>

class Logger : public QObject
{

Q_OBJECT

public:

	static Logger* instance();
	~Logger();
	bool loggingEnabled();
	QString directory();
	int flushInterval();
	void setLoggingEnabled(bool);
	void setDirectory(QString);
	void setFlushInterval(int);

public slots:

	void positionChanged();
	void initialize();

protected:

	explicit Logger( QObject* parent = 0 );

	bool convertLogToGpx();
	bool flushLogfile();

	QFile m_logFile;
	int m_flushInterval;
	QDateTime m_lastFlushTime;
	bool m_loggingEnabled;
	QString m_logBuffer;
	QString m_tracklogPath;
	QString m_tracklogPrefix;

};

#endif // LOGGER_H
