#ifndef MAPNIKTILEWRITE_H
#define MAPNIKTILEWRITE_H

#include <QObject>
#include <QString>

#include <mapnik/map.hpp>
#include <mapnik/projection.hpp>

class QImage;

class MapnikTileWriter : public QObject
{
	Q_OBJECT

public slots:
	void draw_image( int x, int y, int zoom, int magnification );

signals:
	void image_finished( int x, int y, int zoom, int magnification, const QImage &image );

public:
    MapnikTileWriter( const QString &dir );
    ~MapnikTileWriter();

private:
	const mapnik::projection m_projection;
	mapnik::Map m_map;
	const QString m_theme;
	const int m_tileSize;
	const int m_margin;
};

#endif
