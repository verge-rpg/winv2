/****************************************************************
	xerxes engine
	vid_sysfont.cpp
 ****************************************************************/

#include "xerxes.h"
#include "fontdef.h"

/***************************** data *****************************/

int fontx;
int fonty;
int text_color = 0xFFFFFFFF;

char *smal_tbl[]=
{ sBlank,
    sYow,  sQuote,    sNum,   sBuck,sPercnt, sCarot, sQuotes, slParen,
 srParen,    star,    sPtr,  sComma, sMinus,sPeriod,  sSlash,      s0,
      s1,      s2,      s3,      s4,     s5,     s6,      s7,      s8,
      s9,  sColon,  ssemic,      ss,     ss,    sra,  sQuest,     sAT,
     sbA,     sbB,     sbC,     sbD,    sbE,    sbF,     sbG,     sbH,
     sbI,     sbJ,     sbK,     sbL,    sbM,    sbN,     sbO,     sbP,
     sbQ,     sbR,     sbS,     sbT,    sbU,    sbV,     sbW,     sbX,
     sbY,     sbZ,      ss, sbSlash,     ss, sCarot,     usc,     sch,
     ssA,     ssB,     ssC,     ssD,    ssE,    ssF,     ssG,     ssH,
     ssI,     ssJ,     ssK,     ssL,    ssM,    ssN,     ssO,     ssP,
     ssQ,     ssR,     ssS,     ssT,    ssU,    ssV,     ssW,     ssX,
     ssY,     ssZ,      ss,  target,  check,  sCopy,  sBlock,     ss};

/***************************** code *****************************/

void GotoXY(int x1, int y1)
{
	fontx = x1;
	fonty = y1;
}

void PrintRight(int x1, int y1, char *str)
{
	GotoXY(x1 - pixels(str), y1);
	PrintString(str);
}

void PrintCenter(int x1, int y1, char *str, ...)
{
	va_list argptr;
	char msg[256];

	va_start(argptr,str);
	vsprintf(msg,str,argptr);
	va_end(argptr);
	str=msg;

	GotoXY(x1 - pixels(str)/2, y1);
	PrintString(str);
}

void print_char(char c)
{
	char *img, w;
	int xc, yc;

	if (c < 32) return;
	c -= 32;
	if (c>96) c = 2;
	img = smal_tbl[c];
	w = *img; img++;
	for (yc=0; yc<7; yc++)
		for (xc=0; xc<w; xc++)
		{
			if (*img) PutPixel(xc + fontx, yc + fonty, text_color, screen);
			img++;
		}
	fontx += w + 1;
	if (c == '*' - 32) fontx -= 1;
}

void PrintString(char *str, ...)
{ 
	va_list argptr;
	char msg[256];

	va_start(argptr, str);
	vsprintf(msg, str, argptr);
	va_end(argptr);
	str = msg;

	for (; *str; ++str)
		print_char(*str);
}

int pixels(char *str)
{
	int pix;

	for (pix=0; *str; ++str)
		pix += *smal_tbl[*str-32]+1;
	return pix;
}

void TextColor(int newc)
{
	text_color = newc;
}
