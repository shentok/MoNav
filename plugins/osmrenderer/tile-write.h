#ifndef TILE_WRITE_H
#define TILE_WRITE_H

#include <string>
#include <stdio.h>

struct placename {
    double tilex, tiley; // fractional x/y tile coords.
    char type;
    std::string name;
};

class TileWriter {
  public:
    TileWriter(const std::string &_filename1, const std::string &_filename2,
               const std::string &_filename3);
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
};

#endif
