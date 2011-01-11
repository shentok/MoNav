#ifndef TILE_WRITE_H
#define TILE_WRITE_H

#include <string>
#include <stdio.h>

class TileWriter {
  public:
    TileWriter(const std::string &_filename1, const std::string &_filename2);
	 bool draw_image(const std::string &_imgname, int x, int y, int zoom, int magnification);
    const unsigned char * get_img_data();
  private:
    static bool need_next_pass(int type1, int type2);
    class ImgWriter *img;
    std::string filename[2];
    class qindex *qidx[2];
    FILE *db[2];
};

#endif
