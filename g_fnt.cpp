/****************************************************************
	xerxes engine
	g_chr.cpp
 ****************************************************************/

#include "xerxes.h"

/***************************** code *****************************/

FNT::FNT(char *fname)
{	
	VFILE *f = vopen(fname);
	if (!f) err("FNT::FNT(), could not open %s", fname);
	
	byte ver = vgetc(f);
	if (ver != 1)	
		err("FNT::FNT() %s: incorrect version number (reported %d).\n",
			fname, ver);

	width = vgetw(f);
	height = vgetw(f);
	if (width < 1 || width > 128 || height < 1 || height > 128)
		err("FNT::FNT(), bad font dimensions (%dx%d)", width, height);

	subsets = vgetw(f);
	selected = 0;
	if (subsets < 1 || subsets > 4)
		err("FNT::FNT(), illegal # of subsets");
	
	byte *imagedata = new byte[width*height*96*subsets];
	vread(imagedata, width*height*96*subsets, f);
	totalframes = 96 * subsets;
	vclose(f);

	rawdata = new image(width, height*96*subsets);
	for (int i=0; i<width*height*96*subsets; i++)
		PutPixel(i%width, i/width, imagedata[i] ? base_pal[imagedata[i]] : transColor, rawdata);
	container = new image(width, height);
	delete[] container->data;
	delete[] imagedata;
}

FNT::~FNT()
{	
	container->data = 0;
	delete container;
	delete rawdata;
}

void FNT::Render(int x, int y, int frame, image *dest)
{
	if (frame >= totalframes)
		err("FNT::Render(), char requested is undefined");

	container->data = (void *) ((int) rawdata->data + (frame*width*height*vid_bytesperpixel));
	TBlit(x, y, container, dest);
}

void FNT::Print(int x, int y, char *str, ...)
{
	va_list argptr;
	char msg[256];

	va_start(argptr, str);
	vsprintf(msg, str, argptr);
	va_end(argptr);
	str = msg;

	while (*str)
	{
		Render(x, y, (*str-32)+(selected*96), myscreen);
		x += width;
		str++;
	}

}

/********************* v2 fontbay crap *********************/

FNT *fnts[8];
int numfonts = 0;
int my_x, my_y;

void Font_GotoXY(int x, int y)
{
	my_x = x;
	my_y = y;
}

int LoadFont(char *fname)
{
	if (numfonts == 8) err("LoadFont: Too many fonts");
	fnts[numfonts] = new FNT(fname);
	return numfonts++;
}

void PrintText(int font, char *str)
{
	if (font > numfonts) err("PrintText: Requested font out of range.");	
	fnts[font]->Print(my_x, my_y, str);
}