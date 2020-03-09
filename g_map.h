extern int obszone;
struct layer_r
{
  char pmultx,pdivx;                 // parallax multiplier/divisor for X
  char pmulty,pdivy;                 // parallax multiplier/divisor for Y
  unsigned short sizex, sizey;       // layer dimensions.
  unsigned char trans, hline;        // transparency flag | hline (raster fx)
};

struct zoneinfo
{
  char name[40];
  word script;
  word percent;
  word delay;
  word aaa;
  word entityscript;
};

struct chrlist_r
{
  char t[60];
};

struct entity_r
{
	int x, y;                            // xwc, ywx position
	word tx, ty;                         // xtc, ytc position
	byte facing;                         // direction entity is facing
	byte moving, movecnt;                // direction entity is moving
	byte frame;                          // bottom-line frame to display
	byte specframe;                      // special-frame set thingo
	byte chrindex, reset;                // CHR index | Reset animation
	byte obsmode1, obsmode2;             // can be obstructed | Is an obstruction
	byte speed, speedct;                 // entity speed, speedcount :)
	byte delayct;                        // animation frame-delay
	char *animofs, *scriptofs;           // anim script | move script
	byte face, actm;                     // auto-face | activation mode
	byte movecode, movescript;           // movement type | movement script
	byte ctr, mode;                      // sub-tile move ctr, mode flag (internal)
	word step, delay;                    // step, delay
	word stepctr, delayctr;              // internal use counters
	word data1, data2, data3;            //
	word data4, data5, data6;            //
	int  actscript;                      // activation script
	byte on, visible;
	char expand1[2];
	int  expand2;               //
	int  expand3, expand4;               //
	char desc[20];                       // Entity description.
};

class MAP
{
public:
	VSP *vsp;
	char rstring[30];
	layer_r layer[6];
	zoneinfo zones[256];
	byte *Obstruct, *Zone;

	MAP(char *fname);
	~MAP();
	void Render(int x, int y, image *dest);
	int obstructed(int x, int y);
	int zone(int x, int y);
	void settile(int x, int y, int l, int t);
	void RestoreCamera();
	int mapwidth();
	int mapheight();

	char numlayers;
	word *layers[6];
	chrlist_r chrlist[100];
	byte nmchr;
	char curlayer;

	void BlitLayer(byte c, image *dest, int tx, int ty, int xwin, int ywin);
	void BlitBackLayer(byte l, image *dest, int tx, int ty, int xwin, int ywin);
	void BlitTransLayer(byte l, image *dest, int tx, int ty, int xwin, int ywin);
	void BlitObs(image *dest, int tx, int ty, int xwin, int ywin);
	void BlitZone(image *dest, int tx, int ty, int xwin, int ywin);
	void DecodeByteCompression(byte *dest, int len, byte *buf);
	void DecodeWordCompression(word *dest, int len, word *buf);
};
