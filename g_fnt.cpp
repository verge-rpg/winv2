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
	
	imagedata = new byte[width*height*96*subsets];
	vread(imagedata, width*height*96*subsets, f);
	totalframes = 96 * subsets;
	vclose(f);	
}

FNT::~FNT()
{
	delete[] imagedata;

}

int FNT::uncompress(byte* dest, int len, char *buf)
{
    byte run, w;
    do
    {
        run = 1;
        w = *buf++;
        if (0xFF == w)
        {
            run = *buf++;
            w = *buf++;
        }
        len -= run;

        if (len < 0)
			return 1;
        while (run--)
            *dest++ = w;
    } while (len);
    return 0;
}

void FNT::Render(int x, int y, int frame, image *dest)
{
	if (frame >= totalframes)
		err("FNT::Render(), char requested is undefined");

	byte *c = (byte *) imagedata + ((frame + (selected*96)) * width * height);
	TCopySprite8(x, y, width, height, c);
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
		byte c = *str++;
		switch (c)
		{
			case 126: selected=0; continue;
			case 128: selected=1; continue;
			case 129: selected=2; continue;
			case 130: selected=3; continue;
		}
		Render(x, y, c-32, myscreen);
		x += width;
		
	}
}

void FNT::PrintRaw(int x, int y, char *str)
{
	while (*str)
	{
		byte c = *str++;
		switch (c)
		{
			case 126: selected=0; continue;
			case 128: selected=1; continue;
			case 129: selected=2; continue;
			case 130: selected=3; continue;
		}
		Render(x, y, c-32, myscreen);
		x += width;		
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
	fnts[font]->PrintRaw(my_x, my_y, str);
	my_x += strlen(str)*fnts[font]->width;
}