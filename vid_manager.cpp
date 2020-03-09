/****************************************************************
	xerxes engine
	vid_manager.cpp
 ****************************************************************/

#include "xerxes.h"

/***************************** data *****************************/

bool vid_initd = false;
bool vid_window = true;
int vid_bpp, vid_xres, vid_yres, vid_bytesperpixel;
int transColor;
image *screen;

/****************************************************************/

int    (*MakeColor) (int r, int g, int b);
void   (*Flip) (void);
void   (*Blit) (int x, int y, image *src, image *dest);
void   (*TBlit) (int x, int y, image *src, image *dest);
void   (*Blit8) (int x, int y, char *src, int w, int h, quad pal[], image *dest);
void   (*TBlit8) (int x, int y, char *src, int w, int h, quad pal[], image *dest);
void   (*Clear) (int color, image *dest);
void   (*PutPixel) (int x, int y, int color, image *dest);
int    (*ReadPixel) (int x, int y, image *dest);
void   (*Line) (int x, int y, int xe, int ye, int color, image *dest);
void   (*VLine) (int x, int y, int ye, int color, image *dest);
void   (*HLine) (int x, int y, int xe, int color, image *dest);
void   (*Box) (int x, int y, int xe, int ye, int color, image *dest);
void   (*Rect) (int x, int y, int xe, int ye, int color, image *dest);
void   (*Sphere) (int x, int y, int xradius, int yradius, int color, image *dest);
void   (*Circle) (int x, int y, int xradius, int yradius, int color, image *dest);
void   (*WrapBlit) (int x, int y, image *src, image *dst);
void   (*ScaleBlit8) (int x, int y, int dw, int dh, byte *src, int sw, int sh, quad pal[], image *dest);
void   (*SetLucent) (int percent);
image* (*ImageFrom8bpp) (byte *src, int width, int height, byte *pal);
image* (*ImageFrom24bpp) (byte *src, int width, int height);
void   (*vid_Close) (void);

/***************************** code *****************************/


int vid_SetMode(int xres, int yres, int bpp, int window, int mode)
{
	switch (mode)
	{
		case MODE_SOFTWARE:
			if (dd_SetMode(xres, yres, bpp, window?1:0))
			{
				vid_initd = true;
				return 1;
			}
			return 0;
		default:
			return 0;
	}
	return 0;
}


image::image()
{
}


image::image(int xres, int yres)
{

	width = pitch = xres;
	height = yres;
	cx1 = 0;
	cy1 = 0;
	cx2 = width - 1;
	cy2 = height - 1;
	alphamap = 0;
	bpp = vid_bpp;
	switch (vid_bpp)
	{
		case 15:
		case 16: data = new word[width * height];
				 break;
		case 32: data = new quad[width * height];
				 break;
	}
}


image::~image()
{
	delete[] data;
}


void image::SetClip(int x1, int y1, int x2, int y2)
{
	cx1 = x1 >= 0 ? x1 : 0;
	cy1 = y1 >= 0 ? y1 : 0;
	cx2 = x2 < width ? x2 : width-1;
	cy2 = y2 < height ? y2 : height-1;
}


void image::GetClip(int *x1, int *y1, int *x2, int *y2)
{	
	*x1 = cx1;
	*y1 = cy1;
	*x2 = cx2;
	*y2 = cy2;
}
