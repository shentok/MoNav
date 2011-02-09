#ifndef TILE_WRITE_H
#define TILE_WRITE_H

#include <string>
#include <stdio.h>
#include <vector>
#include "types.h"

struct placename {
    double tilex, tiley; // fractional x/y tile coords.
    char type;
    std::string name;
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
    DrawingRules(const std::string &_dir);
    bool get_rule(osm_type_t type, int zoom, int pass,
                  int *r, int *g, int *b, double *width, bool *polygon);
  private:
    static void tokenise(const std::string &input, std::vector<std::string> &output);
    bool load_rules();

    std::string dir;
    std::vector<struct DrawingRule> drawing_rules;
};

class TileWriter {
  public:
    TileWriter(const std::string &_dir);
    bool query_index(int x, int y, int zoom, int cur_db, int *nways);
    bool draw_image(const std::string &_imgname, int x, int y, int zoom, int magnification);
    const unsigned char * get_img_data();
    void get_placenames(int x, int y, int zoom, int drawzoom,
         std::vector<struct placename> &result);
  private:
    static bool need_next_pass(int type1, int type2);
    class ImgWriter *img;
    std::string filename[3];
    class qindex *qidx[3];
    FILE *db[3];
    DrawingRules dr;
};

#endif
