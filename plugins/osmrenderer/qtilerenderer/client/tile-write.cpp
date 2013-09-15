/* tile-write.cpp. Writes openstreetmap map tiles using a series of rules
(defined near the top) and a pre-processed database file of roads, paths,
and areas arranged as a quad-tree in the same fashion as the tiles.

Current issues:
    No text support - would be easy to add.
*/

#include "../../utils/qthelpers.h"

#include <string>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <list>
#include <vector>
#include <map>
#include <algorithm>
#ifdef TILE_WRITE_MAIN
#include <iostream>
#define qCritical() std::cout
#else
#include <QDebug>
#endif

#include "tile-write.h"

#include "img_writer.h"
#include "quadtile.h"

#define DB_VERSION "org.hollo.quadtile.pqdb.03"

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
    Way() :
        type(),
        coordi(),
        ncoords(0)
    {}
    Way(osm_type_t type, int coordi, int ncoords) :
        type(type),
        coordi(coordi),
        ncoords(ncoords)
    {}
    bool isEmpty() const { return ncoords == 0; }
    void print();
    bool draw(ImgWriter &img, DrawingRules & rules,
              unsigned long tilex, unsigned long tiley,
              int zoom, int magnification, int pass, std::vector<coord> &allcoords);
    static bool sort_by_type(Way w1, Way w2) {return w1.type<w2.type;};
    osm_type_t type;

  private:
    int coordi, ncoords;
};

static int g_nways=0, g_ndrawnways=0;
//Draw this way to an img tile.
bool Way::draw(ImgWriter &img, DrawingRules & rules,
              unsigned long tilex, unsigned long tiley,
              int zoom, int magnification, int pass, std::vector<coord> &allcoords)
{
    g_nways++;
    int r, g, b;
    double width;
    bool polygon;
    if(!rules.get_rule(type, zoom, pass, &r, &g, &b, &width, &polygon)) return false;
    img.SetPen(r, g, b, width * magnification);
    g_ndrawnways++;

    int oldx=0, oldy=0;
    
    ImgWriter::coord *c=NULL;
    if(polygon) {
        c = new ImgWriter::coord[ncoords];
    }
    int j=0;
    for(std::vector<coord>::iterator i=allcoords.begin() + coordi;
        i!=allcoords.begin() + coordi + ncoords; i++){
        int newx, newy;
        if(zoom<10 && i!=allcoords.begin() + coordi)
            i = allcoords.begin() + coordi + ncoords -1;
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
		  newx *= magnification;
		  newy *= magnification;
        if(!polygon && i!=allcoords.begin() + coordi) {
            img.DrawLine(oldx, oldy, newx, newy);
        }
        if(polygon) {c[j].x = newx; c[j++].y = newy;}
        oldx=newx; oldy=newy;
    }
    if(polygon) {
        img.FillPoly(c, ncoords);
        delete [] c;
    }
    return true;
}

/* DrawingRules - load from rendering.qrr */
DrawingRules::DrawingRules()
{
}

bool DrawingRules::get_rule(osm_type_t type, int zoom, int pass,
                  int *r, int *g, int *b, double *width, bool *polygon)
{
    if(pass<0 || pass>1) return false;

    for(std::vector<struct DrawingRule>::iterator rule = drawing_rules.begin();
       rule!=drawing_rules.end(); rule++) {
       if(type >= rule->type_from && type <= rule->type_to && 
           zoom>=rule->zoom_from && zoom<=rule->zoom_to) {
            if(rule->red[pass]==-1) return false;
            *r = rule->red[pass];
            *g = rule->green[pass];
            *b = rule->blue[pass];
            *width = rule->width[pass];
            *polygon = rule->polygon;
            return true;
        }
    }

    return false;
}

bool DrawingRules::load_rules(const std::string &filename)
{
    std::map<std::string, osm_type_t> type_table;
    type_table["AREA_PARK"] = AREA_PARK;
    type_table["AREA_CAMPSITE"] = AREA_CAMPSITE;
    type_table["AREA_NATURE"] = AREA_NATURE;
    type_table["AREA_CEMETERY"] = AREA_CEMETERY;
    type_table["AREA_RESIDENTIAL"] = AREA_RESIDENTIAL;
    type_table["AREA_BARRACKS"] = AREA_BARRACKS;
    type_table["AREA_MILITARY"] = AREA_MILITARY;
    type_table["AREA_FIELD"] = AREA_FIELD;
    type_table["AREA_DANGER_AREA"] = AREA_DANGER_AREA;
    type_table["AREA_MEADOW"] = AREA_MEADOW;
    type_table["AREA_COMMON"] = AREA_COMMON;
    type_table["AREA_FOREST"] = AREA_FOREST;
    type_table["AREA_WATER"] = AREA_WATER;
    type_table["AREA_WOOD"] = AREA_WOOD;
    type_table["AREA_RETAIL"] = AREA_RETAIL;
    type_table["AREA_INDUSTRIAL"] = AREA_INDUSTRIAL;
    type_table["AREA_PARKING"] = AREA_PARKING;
    type_table["AREA_BUILDING"] = AREA_BUILDING;
    type_table["HW_PEDESTRIAN"] = HW_PEDESTRIAN;
    type_table["HW_PATH"] = HW_PATH;
    type_table["HW_FOOTWAY"] = HW_FOOTWAY;
    type_table["HW_STEPS"] = HW_STEPS;
    type_table["HW_BRIDLEWAY"] = HW_BRIDLEWAY;
    type_table["HW_CYCLEWAY"] = HW_CYCLEWAY;
    type_table["HW_PRIVATE"] = HW_PRIVATE;
    type_table["HW_UNSURFACED"] = HW_UNSURFACED;
    type_table["HW_UNCLASSIFIED"] = HW_UNCLASSIFIED;
    type_table["HW_RESIDENTIAL"] = HW_RESIDENTIAL;
    type_table["HW_LIVING_STREET"] = HW_LIVING_STREET;
    type_table["HW_SERVICE"] = HW_SERVICE;
    type_table["HW_TERTIARY"] = HW_TERTIARY;
    type_table["HW_SECONDARY"] = HW_SECONDARY;
    type_table["HW_PRIMARY"] = HW_PRIMARY;
    type_table["HW_TRUNK"] = HW_TRUNK;
    type_table["HW_MOTORWAY"] = HW_MOTORWAY;
    type_table["RW_RAIL"] = RW_RAIL;
    type_table["WATERWAY"] = WATERWAY;
    type_table["PLACE_TOWN"] = PLACE_TOWN;

    drawing_rules.clear();
    std::map<std::string, long> colourMap;
    char buf[4096];
	 FILE *fp = fopen(filename.c_str(), "r");
    if(!fp) {
		  qCritical() << "Can't find rendering rules:" << filename.c_str();
        return false;
    }
    while(fgets(buf, 4095, fp)) {
        if(strlen(buf)<1) return false;
        buf[strlen(buf)-1]=0;
        std::vector<std::string> tokens;
        tokenise(buf, tokens);
        if(tokens.size()<5) continue;
        int i=1;
        if(tokens[0] == "Colour:") {
            if(tokens.size()!=5) continue;
            int red, green, blue;
            i++;
            if(!sscanf(tokens[i++].c_str(), "%i", &red)) continue;
            if(!sscanf(tokens[i++].c_str(), "%i", &green)) continue;
            if(!sscanf(tokens[i++].c_str(), "%i", &blue)) continue;
            printf("%s: %d, %d, %d\n", tokens[1].c_str(), red, green, blue);
            colourMap[tokens[1]] = (red << 16) | (green << 8)  | (blue + 1);
        }
        else if(tokens[0]=="Line:") {
            if(tokens.size()<7) continue;
            DrawingRule r;
            memset(&r, 0, sizeof(DrawingRule));
            r.red[1] = -1; //Disable 2nd pass by default.
            r.type_from = type_table[tokens[i++]];
            r.type_to   = type_table[tokens[i++]];
            r.polygon = false;
            r.zoom_from = atoi(tokens[i++].c_str());
            r.zoom_to = atoi(tokens[i++].c_str());
            std::string tmp = tokens[i++];
            if(colourMap[tmp]) {
                //Colour is stored +1 so we can store black (0) and find it.
                long col = colourMap[tmp]-1; 
                r.red[0] = col >> 16;
                r.green[0] = (col >> 8) & 0xFF;
                r.blue[0] = (col & 0xFF);
            } else {
                if(!sscanf(tmp.c_str(), "%i", &r.red[0])) continue;
                if(!sscanf(tokens[i++].c_str(), "%i", &r.green[0])) continue;
                if(!sscanf(tokens[i++].c_str(), "%i", &r.blue[0])) continue;
            }
            r.width[0] = atof(tokens[i++].c_str());
            if(i== (int) tokens.size()) {
                drawing_rules.push_back(r);
                continue;
            }
            tmp = tokens[i++];
            if(colourMap[tmp]) {
                long col = colourMap[tmp]-1; 
                r.red[1] = col >> 16;
                r.green[1] = (col >> 8) & 0xFF;
                r.blue[1] = (col & 0xFF);
            } else {
                if(!sscanf(tmp.c_str(), "%i", &r.red[1])) continue;
                if(!sscanf(tokens[i++].c_str(), "%i", &r.green[1])) continue;
                if(!sscanf(tokens[i++].c_str(), "%i", &r.blue[1])) continue;
            }
            r.width[1] = atof(tokens[i++].c_str());
            drawing_rules.push_back(r);
        }
        else if(tokens[0]=="Area:") {
            if(tokens.size()<5) continue;
            DrawingRule r;
            memset(&r, 0, sizeof(DrawingRule));
            r.type_from = type_table[tokens[i++]];
            r.type_to   = r.type_from;
            r.polygon = true;
            r.zoom_from = atoi(tokens[i++].c_str());
            r.zoom_to = atoi(tokens[i++].c_str());
            std::string tmp = tokens[i++];
            if(colourMap[tmp]) {
                //Colour is stored +1 so we can store black (0) and find it.
                long col = colourMap[tmp]-1; 
                r.red[0] = col >> 16;
                r.green[0] = (col >> 8) & 0xFF;
                r.blue[0] = (col & 0xFF);
            } else {
                if(!sscanf(tmp.c_str(), "%i", &r.red[0])) continue;
                if(!sscanf(tokens[i++].c_str(), "%i", &r.green[0])) continue;
                if(!sscanf(tokens[i++].c_str(), "%i", &r.blue[0])) continue;
            }
            r.width[0] = -1;
            r.red[1] = -1;
            drawing_rules.push_back(r);
        }
    }
    fclose(fp);
    return true;
}

/* Static helper function */
void DrawingRules::tokenise(const std::string &input, std::vector<std::string> &output)
{
	 const char *s = input.c_str();
	 char* buffer = new char[strlen( s ) + 1];
	 strcpy( buffer, s );
	 const char *p = strtok(buffer, " ");
    while(p) {
        output.push_back(p);
        p = strtok(NULL, " ");
    }
	 delete buffer;
}

/* A recursive index class into the quadtile way database. */
class Database::QuadIndex
{
 public:
    static QuadIndex *load(QIODevice &fp);
    long toOffset(int x, int y, int _level, int *_nways=NULL) const;

 private:
    QuadIndex(){level = -1;}
    long get_index(quadtile q, int _level, int *_nways=NULL) const;

    int max_safe_zoom;
    QuadIndex *child[4];
    quadtile q;
    quadtile qmask;
    long offset;
    int nways;
    int level;
};

Database::Database( const QString &fileName ) :
    db(new QFile( fileName )),
    qidx(QuadIndex::load(*db)),
    m_count(0)
{
}

Database::~Database()
{
    delete [] qidx;
    delete db;
}

//Static function to load the index from the start of the DB
Database::QuadIndex *Database::QuadIndex::load(QIODevice &fp)
{
    Log(LOG_DEBUG, "Load index\n");
    if(!fp.open(QFile::ReadOnly))
        return NULL;

    char tmp[100];
    if(fp.readLine(tmp,100)==-1)
        return NULL;

    if(strncmp(tmp, DB_VERSION, strlen(DB_VERSION))) {
        Log(LOG_ERROR, "Not a DB file, or wrong version\n");
        return NULL;
    }

    char *s=strstr(tmp, "depth=");
    int max_safe_zoom;
    if(s) max_safe_zoom = atoi(s+6);
    else {
        Log(LOG_ERROR, "Can't read maximum safe zoom\n");
        return NULL;
    }

    unsigned char buf[8];
    memset(buf, 0, 8);
    if(fp.read(reinterpret_cast<char*>(buf+5), 3)!=3) {
        Log(LOG_ERROR, "Failed to read index file\n");
        return NULL;
    }
    int nidx = (int) buf2ll(buf);
    Log(LOG_DEBUG, "Load %d index items\n", nidx);

    QuadIndex *result = new QuadIndex[nidx];
    result[0].level = 0;
    result[0].qmask = 0x3FFFFFFFFFFFFFFFLL;

    for(int i=0; i<nidx;i++) {
        result[i].max_safe_zoom = max_safe_zoom;
        if(result[i].level==-1) {
            Log(LOG_ERROR, "Inconsistent index file - orphaned child\n");
            delete [] result;
            return NULL;
        }
        unsigned char buf[28];
        if(fp.read(reinterpret_cast<char *>(buf), 28)!=28) {
            Log(LOG_ERROR, "Failed read of index file\n");
            delete [] result;
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
            if(child_offset>=0xFFFFFE) {
                result[i].child[j]=NULL;
            }
            else if(child_offset >= nidx) {
                Log(LOG_ERROR, "Inconsistent index file - child too big %x %d\n", (int)child_offset, i);
                delete [] result;
                return NULL;
            }
            else {
                result[child_offset].level = result[i].level + 1;
                result[child_offset].qmask = result[i].qmask >> 2;
                result[i].child[j] = &result[child_offset];
            }
        }
    }
    return result;
}

long Database::QuadIndex::toOffset(int x, int y, int _level, int *_nways) const
{
    //Work out which quadtile to load.
    const long z = 1UL << _level;
    const double xf = ((double)x)/z;
    const double yf = ((double)y)/z;
    const quadtile qmask = 0x3FFFFFFFFFFFFFFFLL >> ((_level>max_safe_zoom ? max_safe_zoom : _level)*2);
    //binary_printf(qmask); binary_printf(q);

    /* Store a pair of doubles in the range (0.0->1.0) as integers with alternating
       binary bits */
    const quadtile q = mux((long long) (xf * (1ULL<<31)), (long long) (yf * (1ULL<<31)));

    return get_index(q & ~qmask, _level, _nways);
}

//Get the offset into the DB file where we can read the first way from
//quadtile _q (masked with level - ie. the first way in the same level
//_level map tile as _q
long Database::QuadIndex::get_index(quadtile _q, int _level, int *_nways) const
{
    if(_level > max_safe_zoom) _level = max_safe_zoom;
    if((_q & (~qmask))!=q) return(-1); //We don't contain _q
    //We contain _q.
    if(_level==level) {
        //If we are the same level as the request, we have the answer.
        *_nways = nways;
        return offset;
    }
    //See whether our children have the answer
    long result=-1;
    for(int i=0;i<4;i++) {
        if(child[i]) result = child[i]->get_index(_q, _level, _nways);
        if(result!=-1) return result;
    }
    //If execution reaches here, _q/_level refers to a tile within us
    //which none of our children claim. This is either because we are
	//nearly empty (and therefore didn't bother with children), or as
	//_q/_level refers to an empty corner of a high level tile.
	for(int i=0;i<4;i++) {
		if(child[i]) { //We have children. It's an empty corner.
			*_nways = 0;
			return 0;
		}
	}
	//We didn't have enough ways to bother with children.
	*_nways = nways;
	return offset;
}

class TileWriter::WayDatabase : public Database
{
public:
    WayDatabase( const QString &fileName );
    Way readWay(std::vector<coord> &allcoords);
};

TileWriter::WayDatabase::WayDatabase( const QString &fileName ) :
    Database( fileName )
{
}

//Initialise a way from the database. fp has been seeked to the start of the
//way we wish to read.
Way TileWriter::WayDatabase::readWay(std::vector<coord> &allcoords)
{
    const int coordi = allcoords.size();

    unsigned char buf[18];

    if(db->read(reinterpret_cast<char *>(buf), 11)!=11) {
            Log(LOG_ERROR, "Failure to read any of the way\n");
            return Way();
    }
    const osm_type_t type = (osm_type_t) buf[0];
    const int ncoords = (((int) buf[1]) << 8) | buf[2];

    coord c;
    c.x = buf2l(buf+3);
    c.y = buf2l(buf+7);
    allcoords.push_back(c);

    //Add the cordinates
    for(int i=1;i<ncoords;i++) {
        if(db->read(reinterpret_cast<char *>(buf), 4)!=4) {
            Log(LOG_ERROR, "Failed to read remainder of route\n");
            return Way();
        }
        const long dx = ((buf[0]<<8) | buf[1]) << 4;
        const long dy = ((buf[2]<<8) | buf[3]) << 4;
        c.x += dx - (1<<19);
        c.y += dy - (1<<19);
        allcoords.push_back(c);
    }

    Q_ASSERT( size_t(coordi + ncoords) == allcoords.size() );

    return Way(type, coordi, ncoords);
}

TileWriter::TileWriter( const QString &dir ) :
    allWays(new WayDatabase(fileInDirectory( dir, "ways.all.pqdb" ))),
    motorWays(new WayDatabase(fileInDirectory( dir, "ways.motorway.pqdb" ))),
    dr()
{
    dr.load_rules(fileInDirectory( dir, "rendering.qrr" ).toLocal8Bit().constData());
}

TileWriter::~TileWriter()
{
    delete motorWays;
    delete allWays;
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

//For tile x/y/zoom use the index to seek to the relavent part of the db file,
//and return in nways the number of ways needed to load. Factored out from
//draw_image() as it's also needed for place names.
bool Database::query_index(int x, int y, int zoom)
{
    if(!db)
        return false;

    Log(LOG_VERBOSE, "Finding seek point\n");
    //Seek to the relevant point in the db.
    const long offset = qidx->toOffset(x, y, zoom, &m_count);
    if(m_count==0)
        return true;
    if(offset==-1 || !db->seek(offset)) {
        Log(LOG_ERROR, "Failed to find index\n");
        return false;
    }
    return true;
}

int Database::count() const
{
    return m_count;
}

//The main entry point into the class. Draw the map tile described by x, y
//and zoom, and save it into _imgname if it is not an empty string. The tile
//is held in memory and the raw image data can be accessed by get_img_data()
bool TileWriter::draw_image(QString _imgname, int x, int y, int zoom, int magnification)
{
    TIMELOGINIT("Draw_image");
    //FILE *fp = fopen(filename.c_str(), "rb");
    WayDatabase *const current_db = zoom>12 ? allWays : motorWays;
	 Log(LOG_DEBUG, "Write %s, %d, %d, %d\n", _imgname.toUtf8().constData(), x, y, zoom);

    if(!current_db->query_index(x, y, zoom))
        return false;

    //Initialise the image.
    Log(LOG_VERBOSE, "Initialising image with %d ways\n", current_db->count());
    ImgWriter img(256 * magnification, 256 * magnification, _imgname.toLocal8Bit().constData() );
    img.SetBG(242, 238, 232);
    TIMELOG("Image initialisation");

    //load the ways.
    std::vector<coord> allcoords; //Coords for all loaded ways.
    Log(LOG_VERBOSE, "Loading ways\n");
    typedef std::vector<Way> WayList;
    WayList waylist;

    for(int i=0; i<current_db->count();i++) {
        const Way s = current_db->readWay(allcoords);
        if (s.isEmpty())
            continue;
        waylist.push_back(s);
    }
    TIMELOG("Loading ways");

    if(waylist.size()==0) { //We de-reference waylist.begin() later.
        Log(LOG_VERBOSE, "Empty tile\n");
        if(_imgname.size())
            img.Save();
        return true;
    }

    //Sort by way-type, with least important roads first. This stops minor roads
    //intruding on the colour of major roads that they join.
    Log(LOG_VERBOSE, "Sorting ways\n");
    std::sort(waylist.begin(), waylist.end(), Way::sort_by_type);
    TIMELOG("Sorting ways");
    
    unsigned long itilex = x << (31-zoom), itiley = y << (31-zoom);
    //This is a pain. We have to do both passes of one type before moving on to
    //the next type.
    Log(LOG_VERBOSE, "Drawing ways\n");
    int current_type = waylist.begin()->type;
    std::vector<Way>::iterator cur_type_start, i, j;
    cur_type_start = waylist.begin();
    for(i=waylist.begin(); i!=waylist.end(); i++) {
        if(need_next_pass(i->type, current_type)) { //Do the second pass.
            for(j=cur_type_start; j!=i; j++) 
                if(!j->draw(img, dr, itilex, itiley, zoom, magnification, 1, allcoords)) break;
            current_type = i->type;
            cur_type_start = i;
        }
        i->draw(img, dr, itilex, itiley, zoom, magnification, 0, allcoords);
    }
    //Do second pass for the last type.
    for(j=cur_type_start; j!=waylist.end(); j++)
        if(!j->draw(img, dr, itilex, itiley, zoom, magnification, 1, allcoords)) break;
    TIMELOG("Drawing");
    
    Log(LOG_VERBOSE, "Saving img\n");
    if(_imgname.size())
        img.Save();
    Log(LOG_VERBOSE, "Done\n");
    TIMELOG("Saving");
    
    //for(i=waylist.begin();i!=waylist.end();i++) delete *i;
    TIMELOG("Cleanup");
    Log(LOG_VERBOSE, "Drew %d/%d ways\n", g_ndrawnways, g_nways);

    emit image_finished( x, y, zoom, magnification, QByteArray( ( const char* ) img.get_img_data(), 256 * magnification * 256 * magnification * 3 ) );
    return true;
}

PlaceDatabase::PlaceDatabase( const QString &fileName ) :
    Database( fileName )
{
}

//x,y,zoom refer to an osm map tile. Return all placenames within this
//tile. The data will be drawn at actualzoom, so adjust tilex/tiley and
//select which places to draw based on this.
void PlaceDatabase::get_placenames(int x, int y, int zoom, int actualzoom,
            std::vector<struct placename> &result)
{
    if(!query_index(x, y, zoom))
        return;

    for(int i=0; i<count();i++) {
        unsigned char ubuf[10];
        if(db->read(reinterpret_cast<char *>(ubuf), 10)!=10)
            return;

        struct placename p;
        p.type = ubuf[9];
        int namelen = ubuf[8];
        quadtile q = buf2ll(ubuf);
        quadtile x, y;
        demux(q, &x, &y);
        p.tilex = (double) x / (double) (1ULL<<(31-actualzoom));
        p.tiley = (double) y / (double) (1ULL<<(31-actualzoom));

        char buf[256];
        if(db->read(buf, namelen)!=namelen)
            return;

        buf[namelen]=0;
        if(p.type>=5 && actualzoom<13) continue; //ignore hamlets
        if(p.type>=4 && actualzoom<12) continue; //and suburbs
        if(p.type>=3 && actualzoom<13) continue; //and stations
        if(p.type>=2 && actualzoom<12) continue; //and villages
        if(p.type>=1 && actualzoom<9) continue;  //and towns

        p.name = std::string(buf);
        result.push_back(p);
    }
}

//Stuff for ease of profiling
//Compile with:
//   g++ -DPROFILELOGGING -DTILE_WRITE_MAIN -O3 -pg -o tile-write tile-write.cpp -lagg
//
//   expects data files in current dir. Either run with no args to time
//   building the compiled in area, 1 arg for building the whole area without any
//   time spent saving/converting image formats, or with 3 args of x, y, z (to display
//   tile) or 4 args: x,y,z,i to draw the tile i times for profiling.
#ifdef TILE_WRITE_MAIN
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>

void get_tile(double lat, double lon, int zoom, int *x, int *y)
{
    double mlat = (( 1-log(tan(lat*M_PI/180) + (1/cos(lat*M_PI/180)))/M_PI)/2);
    double mlon = (lon+180.0)/360.0;
    *x = mlon * pow(2, zoom);
    *y = mlat * pow(2, zoom);
}


int main(int argc, char *argv[])
{
    TileWriter t("./");

    struct timeval tv1, tv2;
    long totalmsecs[18] = {0};
    int maxmsecs[18] = {0}, ntiles[18] = {0};
    int total_tiles=0, max_tiles=10000000, i;
    if(argc==1 || argc==2) {
        char buf[1024];
        for(int z=5; z<=16; z++) {
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
                    gettimeofday(&tv1, 0);
                    t.draw_image(argc==1 ? buf : "", x, y, z, 1);
                    gettimeofday(&tv2, 0);
                    int msecs = (tv2.tv_sec-tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
                    if(msecs > maxmsecs[z]) maxmsecs[z] = msecs;
                    totalmsecs[z] += msecs;
                    ntiles[z] ++;
                    if(++total_tiles > max_tiles) goto stats;
                }
            }
        }
        goto stats;
    } else {
        int x = atoi(argv[2]);
        int y = atoi(argv[3]);
        int z = atoi(argv[1]);

        if(argc==4) {
            t.draw_image("test.png", x, y, z, 1);
            int i = system("eog test.png");
        } else {
            int count = atoi(argv[4]);
            for(int i=0;i<count;i++) {
                t.draw_image("", x, y, z, 1);
            }
        }
    }
    return 0;

    stats:
    for(int z=0;z<18;z++) {
        if(!ntiles[z]) continue;
        printf(" Zoom lvl: %2d, n =%5d, total = %5ldms, avg = %3ldms, max = %3dms\n",
               z, ntiles[z], totalmsecs[z], totalmsecs[z]/ntiles[z], maxmsecs[z]);
    }
    return 0;
}
#endif /* TILE_WRITE_MAIN */
