#ifndef TILE_WRITE_H
#define TILE_WRITE_H

#include <string>
#include <stdio.h>
#include <vector>
#include <QObject>
#include <QString>
#include "types.h"

class QIODevice;

class Database
{
public:
    Database( const QString &fileName );
    ~Database();
    bool query_index(int x, int y, int zoom);
    int count() const;

protected:
    QIODevice *const db;

private:
    class QuadIndex;
    const QuadIndex *const qidx;
    int m_count;
};

struct placename {
    double tilex, tiley; // fractional x/y tile coords.
    char type;
    std::string name;
};

class PlaceDatabase : public Database
{
public:
    PlaceDatabase( const QString &fileName );
    void get_placenames(int x, int y, int zoom, int drawzoom, std::vector<struct placename> &result);
};

class DrawingRules {
  public:
    struct DrawingRule {
        osm_type_t type_from; //List of types the rule
        osm_type_t type_to;   //applies to
        bool polygon;
        int zoom_from, zoom_to;
        int red[2], green[2], blue[2];
        double width[2];
    };
    DrawingRules();
    bool load_rules(const std::string &filename);
    bool get_rule(osm_type_t type, int zoom, int pass,
                  int *r, int *g, int *b, double *width, bool *polygon);
  private:
    static void tokenise(const std::string &input, std::vector<std::string> &output);

    std::vector<struct DrawingRule> drawing_rules;
};

class TileWriter : public QObject {
	Q_OBJECT

public slots:

	bool draw_image( QString filename, int x, int y, int zoom, int magnification);

signals:

	void image_finished( int x, int y, int zoom, int magnification, QByteArray data );

public:
    TileWriter( const QString &dir );
    ~TileWriter();

private:
	static bool need_next_pass(int type1, int type2);
    class WayDatabase;
    WayDatabase *const allWays;
    WayDatabase *const motorWays;
	DrawingRules dr;
};

#endif
