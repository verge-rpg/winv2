#ifndef VIDMANAGER_H
#define VIDMANAGER_H

#define MODE_NULL 0
#define MODE_SOFTWARE 1
#define MODE_D3D 2

class image
{
public:
	int width, height, pitch;
	int cx1, cy1, cx2, cy2;
	int bpp;
	byte *alphamap;
	void *data;

	image();
	image(int xres, int yres);
	~image();
	void SetClip(int x1, int y1, int x2, int y2);
	void GetClip(int *x1, int *y1, int *x2, int *y2);
};

extern int vid_bpp, vid_xres, vid_yres, vid_bytesperpixel, transColor;
extern bool vid_window;
extern image *screen;
extern int alpha, ialpha;

int dd_SetMode(int xres, int yres, int bpp, bool windowflag);
int vid_SetMode(int xres, int yres, int bpp, int window, int mode);
int SetLucent(int percent);

void run2xSAI(image *src, image *dest);
void runSuperEagle(image *src, image *dest);

extern int    (*MakeColor) (int r, int g, int b);
extern bool   (*GetColor) (int c, int *r, int *g, int *b);
extern void   (*Flip) (void);
extern void   (*Blit) (int x, int y, image *src, image *dest);
extern void   (*TBlit) (int x, int y, image *src, image *dest);
extern void   (*Blit8) (int x, int y, char *src, int w, int h, quad pal[], image *dest);
extern void   (*TBlit8) (int x, int y, char *src, int w, int h, quad pal[], image *dest);
extern void   (*AdditiveBlit) (int x, int y, image *src, image *dest);
extern void   (*TAdditiveBlit) (int x, int y, image *src, image *dest);
extern void   (*SubtractiveBlit) (int x, int y, image *src, image *dest);
extern void   (*TSubtractiveBlit) (int x, int y, image *src, image *dest);
extern void   (*BlitTile) (int x, int y, char *src, image *dest);
extern void   (*TBlitTile) (int x, int y, char *src, image *dest);
extern void   (*Clear) (int color, image *dest);
extern void   (*PutPixel) (int x, int y, int color, image *dest);
extern int    (*ReadPixel) (int x, int y, image *dest);
extern void   (*Line) (int x, int y, int xe, int ye, int color, image *dest);
extern void   (*VLine) (int x, int y, int ye, int color, image *dest);
extern void   (*HLine) (int x, int y, int xe, int color, image *dest);
extern void   (*Box) (int x, int y, int xe, int ye, int color, image *dest);
extern void   (*Rect) (int x, int y, int xe, int ye, int color, image *dest);
extern void   (*Sphere) (int x, int y, int xradius, int yradius, int color, image *dest);
extern void   (*Circle) (int x, int y, int xradius, int yradius, int color, image *dest);
extern void   (*ScaleBlit) (int x, int y, int dw, int dh, image *src, image *dest);
extern void   (*ScaleBlit8) (int x, int y, int dw, int dh, byte *src, int sw, int sh, quad pal[], image *dest);
extern void   (*WrapBlit) (int x, int y, image *src, image *dst);
extern image* (*ImageFrom8bpp) (byte *src, int width, int height, byte *pal);
extern image* (*ImageFrom24bpp) (byte *src, int width, int height);
extern void   (*vid_Close) (void);

#endif