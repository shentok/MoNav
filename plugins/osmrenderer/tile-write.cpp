/* tile-write.cpp. Writes openstreetmap map tiles using a series of rules
(defined near the top) and a pre-processed database file of roads, paths,
and areas arranged as a quad-tree in the same fashion as the tiles.

Current issues:
    No text support - would be easy to add.
*/

#include <string>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <list>
#include <vector>
#include <algorithm>

#include "types.h"
#include "tile-write.h"

#include "img_writer.h"
#include "quadtile.h"

#define DB_VERSION "org.hollo.quadtile.pqdb.03"

struct WriteRule {
    osm_type_t type_from; //List of types the rule
    osm_type_t type_to;   //applies to
    char flags; //0=LINE, 1=POLYGON
    int zoom_from;
    int zoom_to;
    int r1, g1, b1; //Red, Green, Blue, width of first line
    double w1;
    int r2, g2, b2; //Red, Green, Blue, width of second line
    double w2;
};
#define ALL_ZOOM 0, 20
#define NO_PASS -1, -1, -1, -1
#define WHITE 0xff, 0xff, 0xff
#define BLACK 0, 0, 0
#define C_ROAD_YELLOW 253, 253, 179
#define B_ROAD_ORANGE 253, 214, 164
#define A_ROAD_RED 236, 152, 154
#define A_ROAD_GREEN 168, 218, 168
#define MOTORWAY_BLUE 126, 126, 200
#define WATER_BLUE 0xb5, 0xd0, 0xd0
#define RAILWAY_BLACK 100, 100, 100
#define POLY 1

struct WriteRule write_rules[] = {
//Low zoom
{HW_PRIMARY,      HW_PRIMARY,    0, 6, 11, A_ROAD_RED, 2,    NO_PASS},
{HW_TRUNK,        HW_TRUNK,      0, 6, 11, A_ROAD_GREEN, 3,  NO_PASS},
{HW_MOTORWAY,     HW_MOTORWAY,   0, 6, 11, MOTORWAY_BLUE, 6, NO_PASS},
//Higher level zooms
{HW_PEDESTRIAN,   HW_UNSURFACED, 0, 14, 20, 80, 80, 80, 1,    NO_PASS},
{HW_UNCLASSIFIED, HW_SERVICE,    0, 13, 13, BLACK, 2,         WHITE, 1.5},
{HW_UNCLASSIFIED, HW_SERVICE,    0, 14, 14, BLACK, 3.5,       WHITE, 2.5},
{HW_UNCLASSIFIED, HW_SERVICE,    0, 15, 20, BLACK, 5,         WHITE, 4},
{HW_TERTIARY,     HW_TERTIARY,   0, 13, 13, C_ROAD_YELLOW, 3, NO_PASS},
//Big roads without lines
{HW_SECONDARY,    HW_SECONDARY,  0, 12, 13, B_ROAD_ORANGE, 3, NO_PASS},
{HW_PRIMARY,      HW_PRIMARY,    0, 12, 13, A_ROAD_RED, 4,    NO_PASS},
{HW_TRUNK,        HW_TRUNK,      0, 12, 13, A_ROAD_GREEN, 5,  NO_PASS},
{HW_MOTORWAY,     HW_MOTORWAY,   0, 12, 13, MOTORWAY_BLUE, 5, NO_PASS},
//Big roads at medium zoom levels
{HW_TERTIARY,     HW_TERTIARY,   0, 14, 20, BLACK, 5,        C_ROAD_YELLOW, 4},
{HW_SECONDARY,    HW_SECONDARY,  0, 14, 20, BLACK, 6,        B_ROAD_ORANGE, 5},
{HW_PRIMARY,      HW_PRIMARY,    0, 14, 15, BLACK, 8,        A_ROAD_RED, 7},
{HW_TRUNK,        HW_TRUNK,      0, 14, 15, BLACK, 10,        A_ROAD_GREEN, 9},
{HW_MOTORWAY,     HW_MOTORWAY,   0, 14, 15, BLACK, 10,        MOTORWAY_BLUE, 9},
//Big roads with lines at high zoom levels.
{HW_PRIMARY,      HW_PRIMARY,    0, 16, 20, A_ROAD_RED, 7,    BLACK, 1},
{HW_TRUNK,        HW_TRUNK,      0, 16, 20, A_ROAD_GREEN, 9,  BLACK, 1},
{HW_MOTORWAY,     HW_MOTORWAY,   0, 16, 20, MOTORWAY_BLUE, 9, BLACK, 1},
//Water/rail
{WATERWAY,        WATERWAY,      0, 14, 20, WATER_BLUE, 4,    NO_PASS},
{RW_RAIL,         RW_RAIL,       0, 12, 13, RAILWAY_BLACK, 1, NO_PASS},
{RW_RAIL,         RW_RAIL,       0, 14, 20, RAILWAY_BLACK, 2, NO_PASS},
//Areas
{AREA_PARK,        AREA_PARK,        POLY, 6, 20, 0xb6, 0xfd, 0xb6, -1,NO_PASS},
{AREA_CAMPSITE,    AREA_CAMPSITE,    POLY, 6, 20, 0xcc, 0xff, 0x99, -1,NO_PASS},
{AREA_NATURE,      AREA_NATURE,      POLY, 6, 20, 0xab, 0xdf, 0x96, -1,NO_PASS},
{AREA_CEMETERY,    AREA_CEMETERY,    POLY, 6, 20, 0xaa, 0xcb, 0xaf, -1,NO_PASS},
{AREA_RESIDENTIAL, AREA_RESIDENTIAL, POLY, 6, 20,  220,  220,  220, -1,NO_PASS},
{AREA_BARRACKS,    AREA_BARRACKS,    POLY, 6, 20, 0xff, 0x8f, 0x8f, -1,NO_PASS},
{AREA_MILITARY,    AREA_MILITARY,    POLY, 6, 20, 0xff, 0xa8, 0xa8, -1,NO_PASS},
{AREA_FIELD,       AREA_FIELD,       POLY, 6, 20, 0x66, 0x66, 0x00, -1,NO_PASS},
{AREA_DANGER_AREA, AREA_DANGER_AREA, POLY, 6, 20, 0xf6, 0xa8, 0xb6, -1,NO_PASS},
{AREA_MEADOW,      AREA_MEADOW,      POLY, 6, 20, 0xcd, 0xf6, 0xc9, -1,NO_PASS},
{AREA_COMMON,      AREA_COMMON,      POLY, 6, 20, 0xcd, 0xf6, 0xc9, -1,NO_PASS},
{AREA_FOREST,      AREA_FOREST,      POLY, 6, 20, 0x8d, 0xc5, 0x6c, -1,NO_PASS},
{AREA_WOOD,        AREA_WOOD,        POLY, 6, 20, 0xae, 0xd1, 0xa0, -1,NO_PASS},
{AREA_WATER,       AREA_WATER,       POLY, 6, 20, WATER_BLUE,       -1,NO_PASS},
{AREA_RETAIL,      AREA_RETAIL,      POLY, 6, 20, 0xde, 0xd1, 0xd5, -1,NO_PASS},
{AREA_PARKING,     AREA_PARKING,     POLY, 6, 20, 0xf7, 0xef, 0xb7, -1,NO_PASS},
{AREA_INDUSTRIAL,  AREA_INDUSTRIAL,  POLY, 6, 20, 0xdf, 0xd1, 0xd6, -1,NO_PASS},
{DONE, DONE, 0, -1, -1, NO_PASS, NO_PASS }
};

#define PROFILELOGGING
#ifdef PROFILELOGGING
#include <sys/time.h>
static char g_timelog_str[256];
static struct timeval g_timelog_tz, g_timelog_tz2;
#define TIMELOGINIT(A) {strncpy(g_timelog_str, (A), 256);\
                        gettimeofday(&g_timelog_tz, 0);}
#define TIMELOG(A)     {gettimeofday(&g_timelog_tz2,0); \
fprintf(stderr, "%s: %-25s %10ldms\n", g_timelog_str, (A), (g_timelog_tz2.tv_sec-g_timelog_tz.tv_sec)*1000 + g_timelog_tz2.tv_usec/1000 - g_timelog_tz.tv_usec/1000); g_timelog_tz = g_timelog_tz2;}
#else
#define TIMELOGINIT(A)
#define TIMELOG(A)
#endif
/* Logging stuff - integrate to MoNav*/
enum logLevel {LOG_VERBOSE, LOG_DEBUG, LOG_ERROR, LOG_CRITICAL};
static void Log(enum logLevel lvl, const char *fmt, ...)
{
    if(lvl < LOG_ERROR) return;
    va_list argp;

    va_start(argp, fmt);
    if(lvl>=LOG_VERBOSE) vfprintf(stdout, fmt, argp);
    va_end(argp);
}

/*Way. FIXME this class has been ported across
  from another application in which it needed far more functionality.
  Needs pruning and lots of clean up */
struct coord {
    unsigned long x, y;
};
class Way {
  public:
    Way() {};
    bool init(FILE *fp);
    void print();
    bool draw(ImgWriter &img, unsigned long tilex, unsigned long tiley,
              int zoom, int pass);
  //private:
    std::vector<coord> coords;
    osm_type_t type;

    static bool sort_by_type(Way *w1, Way *w2) {return w1->type<w2->type;};
  private:
    unsigned long namep;
};

//Initialise a way from the database. fp has been seeked to the start of the
//way we wish to read.
bool Way::init(FILE *fp)
{
    unsigned char buf[18];

    if(fread(buf, 11, 1, fp)!=1) {
            Log(LOG_ERROR, "Failure to read any of the way\n");
            return false;
    }
    type = (osm_type_t) buf[0];
    int nqtiles = (((int) buf[1]) << 8) | buf[2];

    coords.clear();
    coord c;
    c.x = buf2l(buf+3);
    c.y = buf2l(buf+7);
    coords.push_back(c);

    //Add the cordinates
    for(int i=1;i<nqtiles;i++) {
        if(fread(buf, 4, 1, fp)!=1) {
            Log(LOG_ERROR, "Failed to read remainder of route\n");
            return false;
        }
        long dx = ((buf[0]<<8)  | buf[1]) << 4;
        long dy = ((buf[2]<<8)  | buf[3]) << 4;
        dx -= 1<<19; dy -= 1<<19;
        c.x += (dx);
        c.y += (dy);
        coords.push_back(c);
    }
    return true;
}

static int g_nways=0, g_ndrawnways=0;
//Draw this way to an img tile.
bool Way::draw(ImgWriter &img, unsigned long tilex, unsigned long tiley,
                                 int zoom, int pass)
{
    g_nways++;
    char flags;
    for(struct WriteRule *w = write_rules; w->type_from!=DONE; w++) {
        if(type>=w->type_from && type<=w->type_to && 
           zoom>=w->zoom_from && zoom<=w->zoom_to) {
            if((pass==0 && w->r1==-1) || (pass==1 && w->r2==-1)) return false;
            if(pass==0) img.SetPen(w->r1, w->g1, w->b1, w->w1);
            else img.SetPen(w->r2, w->g2, w->b2, w->w2);
            flags = w->flags;
            goto pen_set;
        }
    }
    //printf("Can't find %d\n", type);
    return false;

    pen_set:
    g_ndrawnways++;


    int oldx=0, oldy=0;
    
    ImgWriter::coord *c=NULL;
    if(flags==POLY) {
        c = new ImgWriter::coord[coords.size()];
    }
    int j=0;
    for(std::vector<coord>::iterator i=coords.begin(); i!=coords.end();i++){
        int newx, newy;
        if(zoom<10 && i!=coords.begin()) i = coords.end()-1;
        //The coords are in the range 0 to 1ULL<<31
        //We want to divide by 2^31 and multiply by 2^zoom and multiply
        //by 2^8 (=256 - the no of pixels in a tile) to get to pixel
        //scale at current zoom. We do this with a shift as it's faster
        //and performance sensitive on profiling. Note we can't do
        //(i->x - tilex) >> (31-8-zoom) as if(tilex>i->x) we'll be shifting
        //a negative number. We shift each part separately, and then cast to a 
        //signed number before subtracting.
        newx = (long) ((i->x) >> (31-8-zoom)) - (long) (tilex >> (31-8-zoom));
        newy = (long) ((i->y) >> (31-8-zoom)) - (long) (tiley >> (31-8-zoom));
        if(flags!=POLY && i!=coords.begin()) {
            img.DrawLine(oldx, oldy, newx, newy);
        }
        if(flags==POLY) {c[j].x = newx; c[j++].y = newy;}
        oldx=newx; oldy=newy;
    }
    if(flags==POLY) {
        img.FillPoly(c, coords.size());
        delete [] c;
    }
    return true;
}

/* A recursive index class into the quadtile way database. */
class qindex {
 public:
   static qindex *load(FILE *fp);
   long get_index(quadtile q, int _level, int *_nways=NULL);

   int max_safe_zoom;
 private:
   qindex(){level = -1;};

   qindex *child[4];
   long long q, qmask;
   long offset;
   int nways;
   int level;
};

//Static function to load the index from the start of the DB
qindex *qindex::load(FILE *fp)
{
    Log(LOG_DEBUG, "Load index\n");
    if(!fp) return false;
    char tmp[100];
    if(!fgets(tmp, 100, fp)) return false;
    if(strncmp(tmp, DB_VERSION, strlen(DB_VERSION))) {
        Log(LOG_ERROR, "Not a DB file, or wrong version\n");
        return false;
    }
    char *s=strstr(tmp, "depth=");
    int max_safe_zoom;
    if(s) max_safe_zoom = atoi(s+6);
    else {
        Log(LOG_ERROR, "Can't read maximum safe zoom\n");
        return false;
    }

    unsigned char buf[8];
    memset(buf, 0, 8);
    if(fread(buf+5, 3, 1, fp)!=1) {
        Log(LOG_ERROR, "Failed to read index file\n");
        return NULL;
    }
    int nidx = (int) buf2ll(buf);
    Log(LOG_DEBUG, "Load %d index items\n", nidx);

    qindex *result;
    result = new qindex[nidx];
    result->max_safe_zoom = max_safe_zoom;
    result[0].level = 0;
    result[0].qmask = 0x3FFFFFFFFFFFFFFFLL;

    for(int i=0; i<nidx;i++) {
        result[i].max_safe_zoom = max_safe_zoom;
        if(result[i].level==-1) {
            Log(LOG_ERROR, "Inconsistent index file - orphaned child\n");
            delete result;
            return NULL;
        }
        unsigned char buf[28];
        if(fread(buf, 28, 1, fp)!=1) {
            Log(LOG_ERROR, "Failed read of index file\n");
            delete result;
            return NULL;
        }
        result[i].q = buf2ll(buf);
        result[i].offset = (buf[8] << 24) |
                           (buf[9] << 16) |
                           (buf[10]<< 8) |
                            buf[11];
        result[i].nways  = (buf[12] << 24) |
                           (buf[13] << 16) |
                           (buf[14]<< 8) |
                            buf[15];

        //nways = buf[12];
        for(int j=0;j<4;j++) {
            long child_offset = (buf[16+j*3] << 16) |
                                (buf[17+j*3] << 8) |
                                 buf[18+j*3];
            if(child_offset>=0xFFFFFE) result[i].child[j]=NULL;
            else {
                result[i].child[j] = result + child_offset;
                if(child_offset > nidx) {
                    Log(LOG_ERROR, "Inconsistent index file - child too big %x %d\n", (int)child_offset, i);
                    delete result;
                    return NULL;
                }
                result[i].child[j]->level = result[i].level+1;
                result[i].child[j]->qmask = result[i].qmask >> 2;
            }
        }
    }
    return result;
}

//Get the offset into the DB file where we can read the first way from
//quadtile _q (masked with level - ie. the first way in the same level
//_level map tile as _q
long qindex::get_index(quadtile _q, int _level, int *_nways)
{
    if(_level > max_safe_zoom) _level = max_safe_zoom;
    //for(int j=0;j<level;j++) printf(" ");
    //printf("Trying qindex. Looking for %llx in %llx:%llx\n", _q, q, qmask);
    if((_q & (~qmask))!=q) return(-1); //We don't contain _q
    if(_level==level) {
        *_nways = nways;
        return offset;
    }
    long result=-1;
    for(int i=0;i<4;i++) {
        if(child[i]) result = child[i]->get_index(_q, _level, _nways);
        if(result!=-1) return result;
    }
    *_nways = nways;
    //printf("  GOT IT: %ld with %d ways\n", offset, nways);
    return offset;
}

TileWriter::TileWriter(const std::string &_filename, const std::string &_filename2)
{
    img = new ImgWriter;
    filename[0] = _filename; filename[1]=_filename2;
    for(int i=0;i<2;i++) {
        db[i] = fopen(filename[i].c_str(), "rb");
        qidx[i] = qindex::load(db[i]);
    }
}

const unsigned char * TileWriter::get_img_data()
{
    return img->get_img_data();
}

/*Static function. Way types that are drawn similarly (eg.
HIGHWAY_RESIDENTIAL / HIGHWAY_LIVING_STREET) should have their first passes
drawn together then second passes to make the pics look right. */
bool TileWriter::need_next_pass(int type1, int type2)
{
    if(type1==type2) return false;
    if(type1 > HW_UNSURFACED && type1<HW_SECONDARY &&
       type2 > HW_UNSURFACED && type2<HW_SECONDARY) return false;
    return true;
}

//The main entry point into the class. Draw the map tile described by x, y
//and zoom, and save it into _imgname if it is not an empty string. The tile
//is held in memory and the raw image data can be accessed by get_img_data()
bool TileWriter::draw_image(const std::string &_imgname, int x, int y, int zoom)
{
    TIMELOGINIT("Draw_image");
    //FILE *fp = fopen(filename.c_str(), "rb");
    int current_db = zoom>12 ? 0 : 1;
    if(!db[current_db]) return false;
    Log(LOG_DEBUG, "Write %s, %d, %d, %d\n", _imgname.c_str(), x, y, zoom);

    //Work out which quadtile to load.
    long z = 1UL << zoom;
    double xf = ((double)x)/z;
    double yf = ((double)y)/z;
    //unsigned long itilex = xf * (1UL<<31), itiley = yf * (1UL<<31);
    unsigned long itilex = x << (31-zoom), itiley = y << (31-zoom);
    quadtile qmask, q;
    qmask = 0x3FFFFFFFFFFFFFFFLL >> ((zoom>qidx[current_db]->max_safe_zoom ? qidx[current_db]->max_safe_zoom : zoom)*2);
    //binary_printf(qmask); binary_printf(q);
    q = xy2q(xf, yf);

    TIMELOG("Setup");
    Log(LOG_VERBOSE, "Finding seek point\n");
    //Seek to the relevant point in the db.
    int nways;
    long offset = qidx[current_db]->get_index(q & ~qmask, zoom, &nways);
    if(offset==-1 || fseek(db[current_db], offset, SEEK_SET)==-1) {
        Log(LOG_ERROR, "Failed to find index\n");
        return false;
    }
    TIMELOG("DB seeking");

    //Initialise the image.
    Log(LOG_VERBOSE, "Seek point %ld (%d ways)\nInitialising image\n", offset, nways);
    img->NewImage(256, 256, _imgname);
    img->SetBG(242, 238, 232);
    TIMELOG("Image initialisation");

    //load the ways.
    Log(LOG_VERBOSE, "Loading ways\n");
    Way s;
    typedef std::vector<Way *> WayList;
    WayList waylist;

    for(int i=0; i<nways;i++) {
        Way *s = new Way;
        if(!s->init(db[current_db])) {
            delete s;
            break;
        }
        waylist.push_back(s);
    }
    TIMELOG("Loading ways");

    if(waylist.size()==0) { //We de-reference waylist.begin() later.
        Log(LOG_VERBOSE, "Empty tile\n");
        if(_imgname.size()) img->Save();
        return true;
    }

    //Sort by way-type, with least important roads first. This stops minor roads
    //intruding on the colour of major roads that they join.
    Log(LOG_VERBOSE, "Sorting ways\n");
    std::sort(waylist.begin(), waylist.end(), Way::sort_by_type);
    TIMELOG("Sorting ways");
    
    //This is a pain. We have to do both passes of one type before moving on to
    //the next type.
    Log(LOG_VERBOSE, "Drawing ways\n");
    int current_type = (*waylist.begin())->type;
    std::vector<Way *>::iterator cur_type_start, i, j;
    cur_type_start = waylist.begin();
    for(i=waylist.begin(); i!=waylist.end(); i++) {
        if(need_next_pass((*i)->type, current_type)) { //Do the second pass.
            for(j=cur_type_start; j!=i; j++) 
                if(!(*j)->draw(*img, itilex, itiley, zoom, 1)) break;
            current_type = (*i)->type;
            cur_type_start = i;
        }
        (*i)->draw(*img, itilex, itiley, zoom, 0);
    }
    //Do second pass for the last type.
    for(j=cur_type_start; j!=waylist.end(); j++)
        if(!(*j)->draw(*img, itilex, itiley, zoom, 1)) break;;
    TIMELOG("Drawing");
    
    Log(LOG_VERBOSE, "Saving img\n");
    if(_imgname.size()) img->Save();
    Log(LOG_VERBOSE, "Done\n");
    TIMELOG("Saving");
    
    for(i=waylist.begin();i!=waylist.end();i++) delete *i;
    TIMELOG("Cleanup");
    printf("Drew %d/%d ways\n", g_ndrawnways, g_nways);
    return true;
}

//Stuff for ease of profiling
//Compile with:
//   g++ -DPROFILELOGGING -DTILE_WRITE_MAIN -O3 -pg -o tile-write tile-write.cpp -lagg
//
//   expects data files in current dir. Either run with no args to time
//   building the compiled in area, or with 3 args of x, y, z (to display
//   tile) or 4 args: x,y,z,i to draw the tile i times for profiling.
#ifdef TILE_WRITE_MAIN
#include <math.h>
#include <stdlib.h>

void get_tile(double lat, double lon, int zoom, int *x, int *y)
{
    double mlat = (( 1-log(tan(lat*M_PI/180) + (1/cos(lat*M_PI/180)))/M_PI)/2);
    double mlon = (lon+180.0)/360.0;
    *x = mlon * pow(2, zoom);
    *y = mlat * pow(2, zoom);
}

int main(int argc, char *argv[])
{
    TileWriter t("ways.all.pqdb", "ways.motorway.pqdb");

    if(argc==1) {
        int total_tiles=0, max_tiles=10000000, i;
        char buf[1024];
        for(int z=6; z<=16; z++) {
            sprintf(buf, "mkdir %d >/dev/null 2>&1", z); i=system(buf);
            int x1, y1, x2, y2;
            //get_tile(58, -6, z, &x1, &y1);
            //get_tile(50, 2, z, &x2, &y2);
            get_tile(51.6, -0.4, z, &x1, &y1);
            get_tile(51.2, 0.1, z, &x2, &y2);
            for(int x=x1; x<=x2; x++) {
                sprintf(buf, "mkdir %d/%d >/dev/null 2>&1", z, x); i=system(buf);
                for(int y=y1; y<=y2; y++) {
                    sprintf(buf, "%d/%d/%d.png", z, x, y);
                    printf("%s\n", buf);
                    t.draw_image(buf, x, y, z);
                    if(++total_tiles > max_tiles) return 0;
                }
            }
        }
    } else {
        int x = atoi(argv[2]);
        int y = atoi(argv[3]);
        int z = atoi(argv[1]);

        if(argc==4) {
            t.draw_image("test.png", x, y, z);
            int i = system("eog test.png");
        } else {
            int count = atoi(argv[4]);
            for(int i=0;i<count;i++) {
                t.draw_image("", x, y, z);
            }
        }
    }
    return 0;
}
#endif /* TILE_WRITE_MAIN */
