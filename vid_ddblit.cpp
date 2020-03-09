/****************************************************************
	xerxes engine
	vid_ddblit.cpp
 ****************************************************************/

#include "xerxes.h"

/***************************** data *****************************/


/***************************** code *****************************/

void dd_Line(int x, int y, int xe, int ye, int color, image *dest)
{
	int dx = xe - x, dy = ye - y,
		xg = sgn(dx), yg = sgn(dy),
		i = 0;
	float slope = 0;

	if (abs(dx) >= abs(dy))
	{
		slope = (float) dy / (float) dx;
		for (i=0; i!=dx; i+=xg)
			PutPixel(x+i, y+(int)(slope*i), color, dest);
	}
	else
	{
		slope = (float) dx / (float) dy;
		for (i=0; i!=dy; i+=yg)
			PutPixel(x+(int)(slope*i), y+i, color, dest);
	}
	PutPixel(xe, ye, color, dest);
}


void dd_Box(int x, int y, int x2, int y2, int color, image *dest)
{
	if (x2<x) SWAP(x,x2);
	if (y2<y) SWAP(y,y2);
	HLine(x, y, x2, color, dest);
	HLine(x, y2, x2, color, dest);
	VLine(x, y+1, y2-1, color, dest);
	VLine(x2, y+1, y2-1, color, dest);
}


void dd_Rect(int x, int y, int x2, int y2, int color, image *dest)
{
	if (y2<y) SWAP(y,y2);
	for (; y<=y2; y++)
		HLine(x, y, x2, color, dest);
}


void dd_Oval(int x, int y, int xe, int ye, int color, int Fill, image *dest)
{
	int m=xe-x, n=ye-y,
		mi=m/2, ni=n/2,
		dx=4*m*m,
		dy=4*n*n,
		r=m*n*n,
		rx=2*r,
		ry=0,
		xx=m;

	y+=ni;
	if (Fill)
		HLine(x, y, x+xx-1, color, dest);
	else {
		PutPixel(x, y, color, dest);
		PutPixel(x+xx, y, color, dest);
	}

	xe=x, ye=y;
	if (ni+ni==n)
	{
		ry=-2*m*m;
	}
	else
	{
		ry=2*m*m;
		ye++;

		if (Fill)
			HLine(xe, ye, xe+xx-1, color, dest);
		else {
			PutPixel(xe, ye, color, dest);
			PutPixel(xe+xx, ye, color, dest);
		}
	}

	while (xx>0)
	{
		if (r<=0)
		{
			xx-=2;
			x++, xe++;
			rx-=dy;
			r+=rx;
		}
		if (r>0)
		{
			y--, ye++;
			ry+=dx;
			r-=ry;
		}

		if (Fill)
		{
			HLine(x, y, x+xx-1, color, dest);
			HLine(xe, ye, xe+xx-1, color, dest);
		}
		else {
			PutPixel(x, y, color, dest);
			PutPixel(x+xx, y, color, dest);
			PutPixel(xe, ye, color, dest);
			PutPixel(xe+xx, ye, color, dest);
		}
	}
}


void dd_Sphere(int x, int y, int xradius, int yradius, int color, image *dest)
{
	dd_Oval(x-xradius, y-yradius, x+xradius-1, y+yradius-1, color, 1, dest);
}


void dd_Circle(int x, int y, int xradius, int yradius, int color, image *dest)
{
	dd_Oval(x-xradius, y-yradius, x+xradius-1, y+yradius-1, color, 0, dest);
}


/************** 2xSAI **************/


static quad ColorMask;
static quad LowPixelMask;
static quad QColorMask;
static quad QLowPixelMask;
static quad RedBlueMask;
static quad GreenMask;
static int PixelsPerMask;

static byte *src_line[4];
static byte *dst_line[2];

void Init_2xSAI(int bpp)
{
	int min_r=0, min_g=0, min_b=0;

	/* Get lowest color bit */	
	for (int i=0; i<255; i++) 
	{
		if (!min_r) min_r = MakeColor(i, 0, 0);
		if (!min_g) min_g = MakeColor(0, i, 0);
		if (!min_b)	min_b = MakeColor(0, 0, i);
	}
	ColorMask = (MakeColor(255,0,0)-min_r) | (MakeColor(0,255,0)-min_g) | (MakeColor(0,0,255)-min_b);
	LowPixelMask = min_r | min_g | min_b;
	QColorMask = (MakeColor(255,0,0)-3*min_r) | (MakeColor(0,255,0)-3*min_g) | (MakeColor(0,0,255)-3*min_b);
	QLowPixelMask = (min_r*3) | (min_g*3) | (min_b*3);
	RedBlueMask = MakeColor(255, 0, 255);
	GreenMask = MakeColor(0, 255, 0);

	PixelsPerMask = (bpp <= 16) ? 2 : 1;

	if (PixelsPerMask == 2) 
	{
		ColorMask |= (ColorMask << 16);
		QColorMask |= (QColorMask << 16);
		LowPixelMask |= (LowPixelMask << 16);
		QLowPixelMask |= (QLowPixelMask << 16);
	}
}

#define GET_RESULT(A, B, C, D) ((A != C || A != D) - (B != C || B != D))
#define INTERPOLATE(A, B) (((A & ColorMask) >> 1) + ((B & ColorMask) >> 1) + (A & B & LowPixelMask))
#define Q_INTERPOLATE(A, B, C, D) ((A & QColorMask) >> 2) + ((B & QColorMask) >> 2) + ((C & QColorMask) >> 2) + ((D & QColorMask) >> 2) \
	+ ((((A & QLowPixelMask) + (B & QLowPixelMask) + (C & QLowPixelMask) + (D & QLowPixelMask)) >> 2) & QLowPixelMask)


void run2xSAI_engine(byte *src, quad src_pitch, image *dest, quad width, quad height);
void run2xSAI(image *src, image *dest)
{
	/* are both images non-null? */
	if (!src || !dest)
		return;

	/* image must be at least 4x4 */
	if (src->width < 4 || src->height < 4)
		return;
	
	/* make sure the destination image is at least 2x as big as the source. */
	if (dest->width < src->width*2 || dest->height < src->height*2)
		return;

	run2xSAI_engine((byte*)src->data, src->pitch*vid_bytesperpixel, dest, src->width, src->height);
}

void run2xSAI_engine(byte *src, quad src_pitch, image *dest, quad width, quad height) 
{
	quad x, y;
	int sbpp = vid_bytesperpixel;
	unsigned long color[16];

	/* Point to the first 3 lines. */
	src_line[0] = src;
	src_line[1] = src;
	src_line[2] = src + src_pitch;
	src_line[3] = src + src_pitch * 2;
	
	/* Can we write the results directly? */
	dst_line[0] = &((byte*)dest->data)[0];
	dst_line[1] = &((byte*)dest->data)[dest->pitch*vid_bytesperpixel];
		
	x = 0, y = 0;
	
	if (PixelsPerMask == 2) 
	{
		word *sbp;
		sbp = (word*) src_line[0];
		color[0] = *sbp;       color[1] = color[0];   color[2] = color[0];    color[3] = color[0];
		color[4] = color[0];   color[5] = color[0];   color[6] = *(sbp + 1);  color[7] = *(sbp + 2);
		sbp = (word*)src_line[2];
		color[8] = *sbp;     color[9] = color[8];     color[10] = *(sbp + 1); color[11] = *(sbp + 2);
		sbp = (word*)src_line[3];
		color[12] = *sbp;    color[13] = color[12];   color[14] = *(sbp + 1); color[15] = *(sbp + 2);
	}
	else 
	{
		unsigned long *lbp;
		lbp = (unsigned long*) src_line[0];
		color[0] = *lbp;       color[1] = color[0];   color[2] = color[0];    color[3] = color[0];
		color[4] = color[0];   color[5] = color[0];   color[6] = *(lbp + 1);  color[7] = *(lbp + 2);
		lbp = (unsigned long*) src_line[2];
		color[8] = *lbp;     color[9] = color[8];     color[10] = *(lbp + 1); color[11] = *(lbp + 2);
		lbp = (unsigned long*) src_line[3];
		color[12] = *lbp;    color[13] = color[12];   color[14] = *(lbp + 1); color[15] = *(lbp + 2);
	}

	for (y = 0; y < height; y++) 
	{
		/* Todo: x = width - 2, x = width - 1 */
		for (x = 0; x < width; x++) {
			unsigned long product1a, product1b, product2a, product2b;

//---------------------------------------  B0 B1 B2 B3    0  1  2  3
//                                         4  5* 6  S2 -> 4  5* 6  7
//                                         1  2  3  S1    8  9 10 11
//                                         A0 A1 A2 A3   12 13 14 15
//--------------------------------------
			if (color[9] == color[6] && color[5] != color[10]) 
			{
				product2b = color[9];
				product1b = product2b;
			}
			else if (color[5] == color[10] && color[9] != color[6]) 
			{
				product2b = color[5];
				product1b = product2b;
			}
			else if (color[5] == color[10] && color[9] == color[6]) 
			{
				int r = 0;

				r += GET_RESULT(color[6], color[5], color[8], color[13]);
				r += GET_RESULT(color[6], color[5], color[4], color[1]);
				r += GET_RESULT(color[6], color[5], color[14], color[11]);
				r += GET_RESULT(color[6], color[5], color[2], color[7]);

				if (r > 0)
					product1b = color[6];
				else if (r < 0)
					product1b = color[5];
				else
					product1b = INTERPOLATE(color[5], color[6]);
					
				product2b = product1b;

			}
			else 
			{
				if (color[6] == color[10] && color[10] == color[13] && color[9] != color[14] && color[10] != color[12])
					product2b = Q_INTERPOLATE(color[10], color[10], color[10], color[9]);
				else if (color[5] == color[9] && color[9] == color[14] && color[13] != color[10] && color[9] != color[15])
					product2b = Q_INTERPOLATE(color[9], color[9], color[9], color[10]);
				else
					product2b = INTERPOLATE(color[9], color[10]);

				if (color[6] == color[10] && color[6] == color[1] && color[5] != color[2] && color[6] != color[0])
					product1b = Q_INTERPOLATE(color[6], color[6], color[6], color[5]);
				else if (color[5] == color[9] && color[5] == color[2] && color[1] != color[6] && color[5] != color[3])
					product1b = Q_INTERPOLATE(color[6], color[5], color[5], color[5]);
				else
					product1b = INTERPOLATE(color[5], color[6]);
			}

			if (color[5] == color[10] && color[9] != color[6] && color[4] == color[5] && color[5] != color[14])
				product2a = INTERPOLATE(color[9], color[5]);
			else if (color[5] == color[8] && color[6] == color[5] && color[4] != color[9] && color[5] != color[12])
				product2a = INTERPOLATE(color[9], color[5]);
			else
				product2a = color[9];

			if (color[9] == color[6] && color[5] != color[10] && color[8] == color[9] && color[9] != color[2])
				product1a = INTERPOLATE(color[9], color[5]);
			else if (color[4] == color[9] && color[10] == color[9] && color[8] != color[5] && color[9] != color[0])
				product1a = INTERPOLATE(color[9], color[5]);
			else
				product1a = color[5];
	
			if (PixelsPerMask == 2) 
			{
				*((unsigned long *) (&dst_line[0][x * 4])) = product1a | (product1b << 16);
				*((unsigned long *) (&dst_line[1][x * 4])) = product2a | (product2b << 16);
			}
			else 
			{
				*((unsigned long *) (&dst_line[0][x * 8])) = product1a;
				*((unsigned long *) (&dst_line[0][x * 8 + 4])) = product1b;
				*((unsigned long *) (&dst_line[1][x * 8])) = product2a;
				*((unsigned long *) (&dst_line[1][x * 8 + 4])) = product2b;
			}
			
			/* Move color matrix forward */
			color[0] = color[1]; color[4] = color[5]; color[8] = color[9];   color[12] = color[13];
			color[1] = color[2]; color[5] = color[6]; color[9] = color[10];  color[13] = color[14];
			color[2] = color[3]; color[6] = color[7]; color[10] = color[11]; color[14] = color[15];
			
			if (x < width - 3) 
			{
				x += 3;
				if (PixelsPerMask == 2) 
				{
					color[3] = *(((unsigned short*)src_line[0]) + x);					
					color[7] = *(((unsigned short*)src_line[1]) + x);
					color[11] = *(((unsigned short*)src_line[2]) + x);
					color[15] = *(((unsigned short*)src_line[3]) + x);
				}
				else 
				{
					color[3] = *(((unsigned long*)src_line[0]) + x);
					color[7] = *(((unsigned long*)src_line[1]) + x);
					color[11] = *(((unsigned long*)src_line[2]) + x);
					color[15] = *(((unsigned long*)src_line[3]) + x);
				}
				x -= 3;
			}
		}

		/* We're done with one line, so we shift the source lines up */
		src_line[0] = src_line[1];
		src_line[1] = src_line[2];
		src_line[2] = src_line[3];		

		/* Read next line */
		if (y + 3 >= height)
			src_line[3] = src_line[2];
		else
			src_line[3] = src_line[2] + src_pitch;
			
		/* Then shift the color matrix up */
		if (PixelsPerMask == 2) 
		{
			unsigned short *sbp;
			sbp = (unsigned short*)src_line[0];
			color[0] = *sbp;     color[1] = color[0];    color[2] = *(sbp + 1);  color[3] = *(sbp + 2);
			sbp = (unsigned short*)src_line[1];
			color[4] = *sbp;     color[5] = color[4];    color[6] = *(sbp + 1);  color[7] = *(sbp + 2);
			sbp = (unsigned short*)src_line[2];
			color[8] = *sbp;     color[9] = color[9];    color[10] = *(sbp + 1); color[11] = *(sbp + 2);
			sbp = (unsigned short*)src_line[3];
			color[12] = *sbp;    color[13] = color[12];  color[14] = *(sbp + 1); color[15] = *(sbp + 2);
		}
		else 
		{
			unsigned long *lbp;
			lbp = (unsigned long*)src_line[0];
			color[0] = *lbp;     color[1] = color[0];    color[2] = *(lbp + 1);  color[3] = *(lbp + 2);
			lbp = (unsigned long*)src_line[1];
			color[4] = *lbp;     color[5] = color[4];    color[6] = *(lbp + 1);  color[7] = *(lbp + 2);
			lbp = (unsigned long*)src_line[2];
			color[8] = *lbp;     color[9] = color[9];    color[10] = *(lbp + 1); color[11] = *(lbp + 2);
			lbp = (unsigned long*)src_line[3];
			color[12] = *lbp;    color[13] = color[12];  color[14] = *(lbp + 1); color[15] = *(lbp + 2);
		}
		
		/* Write the 2 lines, if not already done so */
		if (y < height - 1) 
		{
			dst_line[0] = &((byte*)dest->data)[dest->pitch*vid_bytesperpixel*(y * 2 + 2)];
			dst_line[1] = &((byte*)dest->data)[dest->pitch*vid_bytesperpixel*(y * 2 + 3)];
		}
	}
}


void runSuperEagle_engine(byte *src, quad src_pitch, image *dest, quad width, quad height);
void runSuperEagle(image *src, image *dest)
{
	/* are both images non-null? */
	if (!src || !dest)
		return;

	/* image must be at least 4x4 */
	if (src->width < 4 || src->height < 4)
		return;
	
	/* make sure the destination image is at least 2x as big as the source. */
	if (dest->width < src->width*2 || dest->height < src->height*2)
		return;

	runSuperEagle_engine((byte*)src->data, src->pitch*vid_bytesperpixel, dest, src->width, src->height);
}

void runSuperEagle_engine(byte *src, quad src_pitch, image *dest, quad width, quad height) 
{
	unsigned int x, y;
	int sbpp = vid_bytesperpixel;
	unsigned long color[12];

	/* Point to the first 3 lines. */
	src_line[0] = src;
	src_line[1] = src;
	src_line[2] = src + src_pitch;
	src_line[3] = src + src_pitch * 2;
	
	/* Can we write the results directly? */
	dst_line[0] = &((byte*)dest->data)[0];
	dst_line[1] = &((byte*)dest->data)[dest->pitch*vid_bytesperpixel];
	
	x = 0, y = 0;
	
	if (PixelsPerMask == 2) 
	{
		unsigned short *sbp;
		sbp = (word*)src_line[0];
		color[0] = *sbp;       color[1] = color[0];   color[2] = color[0];    color[3] = color[0];
		color[4] = *(sbp + 1); color[5] = *(sbp + 2);
		sbp = (word*)src_line[2];
		color[6] = *sbp;     color[7] = color[6];     color[8] = *(sbp + 1); color[9] = *(sbp + 2);
		sbp = (word*)src_line[3];
		color[10] = *sbp;    color[11] = *(sbp + 1); 
	}
	else 
	{
		unsigned long *lbp;
		lbp = (unsigned long*)src_line[0];
		color[0] = *lbp;       color[1] = color[0];   color[2] = color[0];    color[3] = color[0];
		color[4] = *(lbp + 1); color[5] = *(lbp + 2);
		lbp = (unsigned long*)src_line[2];
		color[6] = *lbp;     color[7] = color[6];     color[8] = *(lbp + 1); color[9] = *(lbp + 2);
		lbp = (unsigned long*)src_line[3];
		color[10] = *lbp;    color[11] = *(lbp + 1);
	}

	for (y = 0; y < height; y++) 
	{
		/* Todo: x = width - 2, x = width - 1 */
		for (x = 0; x < width; x++) 
		{
			unsigned long product1a, product1b, product2a, product2b;

//---------------------------------------     B1 B2           0  1
//                                         4  5  6  S2 ->  2  3  4  5
//                                         1  2  3  S1     6  7  8  9
//                                            A1 A2          10 11

			if (color[7] == color[4] && color[3] != color[8]) 
			{
				product1b = product2a = color[7];

				if ((color[6] == color[7]) || (color[4] == color[1]))
					product1a = INTERPOLATE(color[7], INTERPOLATE(color[7], color[3]));
				else
					product1a = INTERPOLATE(color[3], color[4]);

				if ((color[4] == color[5]) || (color[7] == color[10]))
					product2b = INTERPOLATE(color[7], INTERPOLATE(color[7], color[8]));
				else
					product2b = INTERPOLATE(color[7], color[8]);
			}
			else if (color[3] == color[8] && color[7] != color[4]) 
			{
				product2b = product1a = color[3];

				if ((color[0] == color[3]) || (color[5] == color[9]))
					product1b = INTERPOLATE(color[3], INTERPOLATE(color[3], color[4]));
				else
					product1b = INTERPOLATE(color[3], color[1]);

				if ((color[8] == color[11]) || (color[2] == color[3]))
					product2a = INTERPOLATE(color[3], INTERPOLATE(color[3], color[2]));
				else
					product2a = INTERPOLATE(color[7], color[8]);

			}
			else if (color[3] == color[8] && color[7] == color[4]) 
			{
				register int r = 0;

				r += GET_RESULT(color[4], color[3], color[6], color[10]);
				r += GET_RESULT(color[4], color[3], color[2], color[0]);
				r += GET_RESULT(color[4], color[3], color[11], color[9]);
				r += GET_RESULT(color[4], color[3], color[1], color[5]);

				if (r > 0) 
				{
					product1b = product2a = color[7];
					product1a = product2b = INTERPOLATE(color[3], color[4]);
				}
				else if (r < 0) 
				{
					product2b = product1a = color[3];
					product1b = product2a = INTERPOLATE(color[3], color[4]);
				}
				else 
				{
					product2b = product1a = color[3];
					product1b = product2a = color[7];
				}
			}
			else 
			{
				product2b = product1a = INTERPOLATE(color[7], color[4]);
				product2b = Q_INTERPOLATE(color[8], color[8], color[8], product2b);
				product1a = Q_INTERPOLATE(color[3], color[3], color[3], product1a);

				product2a = product1b = INTERPOLATE(color[3], color[8]);
				product2a = Q_INTERPOLATE(color[7], color[7], color[7], product2a);
				product1b = Q_INTERPOLATE(color[4], color[4], color[4], product1b);
			}

			if (PixelsPerMask == 2) 
			{
				*((unsigned long *) (&dst_line[0][x * 4])) = product1a | (product1b << 16);
				*((unsigned long *) (&dst_line[1][x * 4])) = product2a | (product2b << 16);
			}
			else 
			{
				*((unsigned long *) (&dst_line[0][x * 8])) = product1a;
				*((unsigned long *) (&dst_line[0][x * 8 + 4])) = product1b;
				*((unsigned long *) (&dst_line[1][x * 8])) = product2a;
				*((unsigned long *) (&dst_line[1][x * 8 + 4])) = product2b;
			}
			
			/* Move color matrix forward */
			color[0] = color[1]; 
			color[2] = color[3]; color[3] = color[4]; color[4] = color[5];
			color[6] = color[7]; color[7] = color[8]; color[8] = color[9]; 
			color[10] = color[11];
			
			if (x < width - 2) {
				x += 2;
				if (PixelsPerMask == 2) {
					color[1] = *(((unsigned short*)src_line[0]) + x);
					if (x < width) {
						color[5] = *(((unsigned short*)src_line[1]) + x + 1);
						color[9] = *(((unsigned short*)src_line[2]) + x + 1);
					}
					color[11] = *(((unsigned short*)src_line[3]) + x);
				}
				else {
					color[1] = *(((unsigned long*)src_line[0]) + x);
					if (x < width) {
						color[5] = *(((unsigned long*)src_line[1]) + x + 1);
						color[9] = *(((unsigned long*)src_line[2]) + x + 1);
					}
					color[11] = *(((unsigned long*)src_line[3]) + x);
				}
				x -= 2;
			}
		}

		/* We're done with one line, so we shift the source lines up */
		src_line[0] = src_line[1];
		src_line[1] = src_line[2];
		src_line[2] = src_line[3];		

		/* Read next line */
		if (y + 3 >= height)
			src_line[3] = src_line[2];
		else
			src_line[3] = src_line[2] + src_pitch;
			
		/* Then shift the color matrix up */
		if (PixelsPerMask == 2) 
		{
			unsigned short *sbp;
			sbp = (unsigned short*)src_line[0];
			color[0] = *sbp;     color[1] = *(sbp + 1);
			sbp = (unsigned short*)src_line[1];
			color[2] = *sbp;     color[3] = color[2];    color[4] = *(sbp + 1);  color[5] = *(sbp + 2);
			sbp = (unsigned short*)src_line[2];
			color[6] = *sbp;     color[7] = color[6];    color[8] = *(sbp + 1);  color[9] = *(sbp + 2);
			sbp = (unsigned short*)src_line[3];
			color[10] = *sbp;    color[11] = *(sbp + 1);
		}
		else 
		{
			unsigned long *lbp;
			lbp = (unsigned long*)src_line[0];
			color[0] = *lbp;     color[1] = *(lbp + 1);
			lbp = (unsigned long*)src_line[1];
			color[2] = *lbp;     color[3] = color[2];    color[4] = *(lbp + 1);  color[5] = *(lbp + 2);
			lbp = (unsigned long*)src_line[2];
			color[6] = *lbp;     color[7] = color[6];    color[8] = *(lbp + 1);  color[9] = *(lbp + 2);
			lbp = (unsigned long*)src_line[3];
			color[10] = *lbp;    color[11] = *(lbp + 1);
		}


		/* Write the 2 lines, if not already done so */
		if (y < height - 1) 
		{
			dst_line[0] = &((byte*)dest->data)[dest->pitch*vid_bytesperpixel*(y * 2 + 2)];
			dst_line[1] = &((byte*)dest->data)[dest->pitch*vid_bytesperpixel*(y * 2 + 3)];
		}
	}
}


/********************** 15bpp blitter code **********************/

int dd15_MakeColor(int r, int g, int b)
{
	return (((r>>3)<<10)|((g>>3)<<5)|(b>>3));
}

/********************** 16bpp blitter code **********************/


int dd16_MakeColor(int r, int g, int b)
{
	return (((r>>3)<<11)|((g>>2)<<5)|(b>>3));
}


void dd16_Clear(int color, image *dest)
{	
	word *d = (word *)dest->data;
	int bytes = dest->pitch * dest->height;
	while (bytes--)
		*d++ = color;
}


void dd16_PutPixel(int x, int y, int color, image *dest)
{
	word *ptr = (word *)dest->data;
	if (x<dest->cx1 || x>dest->cx2 || y<dest->cy1 || y>dest->cy2)
		return;
	ptr[(y * dest->pitch) + x] = color;
}


void dd16_HLine(int x, int y, int xe, int color, image *dest)
{
	word *d = (word *) dest->data;
	int cx1=0, cy1=0, cx2=0, cy2=0;
	if (xe<x) SWAP(x,xe);
	dest->GetClip(&cx1, &cy1, &cx2, &cy2);
	if (x>cx2 || y>cy2 || xe<cx1 || y<cy1)
		return;
	if (xe>cx2) xe=cx2;
	if (x<cx1)  x =cx1;

	d += (y * dest->pitch) + x;
	for (; x<=xe; x++)
		*d++ = color;
}


void dd16_VLine(int x, int y, int ye, int color, image *dest)
{
	word *d = (word *) dest->data;
	int cx1=0, cy1=0, cx2=0, cy2=0;
	if (ye<y) SWAP(y,ye);
	dest->GetClip(&cx1, &cy1, &cx2, &cy2);
	if (x>cx2 || y>cy2 || x<cx1 || ye<cy1)
		return;
	if (ye>cy2) ye=cy2;
	if (y<cy1)  y =cy1;

	d += (y * dest->pitch) + x;
	for (; y<=ye; y++, d+=dest->pitch)
		*d = color;		
}


void dd16_Blit(int x, int y, image *src, image *dest)
{
	word *s=(word *)src->data,
		 *d=(word *)dest->data;
	int spitch=src->pitch,
		dpitch=dest->pitch;
	int xlen=src->width,
		ylen=src->height;
	int cx1=0, cy1=0,
		cx2=0, cy2=0;

	dest->GetClip(&cx1, &cy1, &cx2, &cy2);
	if (x>cx2 || y>cy2 || x+xlen<cx1 || y+ylen<cy1)
		return;

	if (x+xlen>cx2) xlen = cx2-x+1;
	if (y+ylen>cy2) ylen = cy2-y+1;
	if (x<cx1) 	{
		s +=(cx1-x);
		xlen-=(cx1-x);
		x  =cx1;
	}
	if (y<cy1) {
		s +=(cy1-y)*spitch;
		ylen-=(cy1-y);
		y  =cy1;
	}

	d += (y * dpitch) + x;
	for (xlen *= 2; ylen--; s+=spitch, d+=dpitch)
		memcpy(d, s, xlen);
}


void dd16_TBlit(int x, int y, image *src, image *dest)
{
	word *s=(word *)src->data,c,
		 *d=(word *)dest->data;
	int spitch=src->pitch,
		dpitch=dest->pitch;
	int xlen=src->width,
		ylen=src->height;
	int cx1=0, cy1=0,
		cx2=0, cy2=0;

	dest->GetClip(&cx1, &cy1, &cx2, &cy2);
	if (x>cx2 || y>cy2 || x+xlen<cx1 || y+ylen<cy1)
		return;

	if (x+xlen > cx2) xlen=cx2-x+1;
	if (y+ylen > cy2) ylen=cy2-y+1;
	if (x<cx1) {
		s +=(cx1-x);
		xlen-=(cx1-x);
		x  =cx1;
	}
	if (y<cy1) {
		s +=(cy1-y)*spitch;
		ylen-=(cy1-y);
		y  =cy1;
	}
	d+=y*dpitch+x;
	for (; ylen; ylen--)
	{
		for (x=0; x<xlen; x++)
		{
			c=s[x];
			if (c != transColor) d[x]=c;
		}
		s+=spitch;
		d+=dpitch;
	}
}


void dd16_Blit8(int x, int y, char *src, int w, int h, quad pal[], image *dest)
{
	byte *s = (byte *) src;
	word *d = (word *) dest->data;
	int spitch=w,
		dpitch=dest->pitch;
	int xlen=w,
		ylen=h;
	int cx1=0, cy1=0,
		cx2=0, cy2=0;

	dest->GetClip(&cx1, &cy1, &cx2, &cy2);
	if (x>cx2 || y>cy2 || x+xlen<cx1 || y+ylen<cy1)
		return;

	if (x+xlen > cx2) xlen=cx2-x+1;
	if (y+ylen > cy2) ylen=cy2-y+1;
	if (x<cx1) {
		s +=(cx1-x);
		xlen-=(cx1-x);
		x  =cx1;
	}
	if (y<cy1) {
		s +=(cy1-y)*spitch;
		ylen-=(cy1-y);
		y  =cy1;
	}

	d+=(y*dest->pitch)+x;
	for (; ylen; ylen--)
	{
		for (x=0; x<xlen; x++)			
			d[x]=pal[s[x]];
		s+=spitch;
		d+=dpitch;
	}
}


void dd16_TBlit8(int x, int y, char *src, int w, int h, quad pal[], image *dest)
{
	byte *s = (byte *) src;
	word *d = (word *) dest->data;
	int spitch=w,
		dpitch=dest->pitch;
	int xlen=w,
		ylen=h;
	int cx1=0, cy1=0,
		cx2=0, cy2=0;
	byte c;

	dest->GetClip(&cx1, &cy1, &cx2, &cy2);
	if (x>cx2 || y>cy2 || x+xlen<cx1 || y+ylen<cy1)
		return;

	if (x+xlen > cx2) xlen=cx2-x+1;
	if (y+ylen > cy2) ylen=cy2-y+1;

	if (x<cx1) {
		s +=(cx1-x);
		xlen-=(cx1-x);
		x = cx1;
	}
	if (y<cy1) {
		s +=(cy1-y)*spitch;
		ylen-=(cy1-y);
		y=cy1;
	}
	d+=y*dpitch+x;
	for (; ylen; ylen--)
	{
		for (x=0; x<xlen; x++)
		{
			c=s[x];
			if (c) d[x]=pal[c];
		}
		s+=spitch;
		d+=dpitch;
	}
}


void dd16_WrapBlit(int x, int y, image *src, image *dst)
{
	int i;
	int cliph, clipw;
	int curx, cury;
	int spanx, spany;
	word *source, *dest;

	cliph = dst->cy2 - dst->cy1 + 1;
	clipw = dst->cx2 - dst->cx1 + 1;
	y %= src->height;
	curx = 0;

	do
	{
		x %= curx ? 1 : src->width;
		spanx = src->width - x;
		if (curx + spanx >= clipw)
			spanx = clipw - curx;
		source = (word *) src -> data + (y * src->width) + x;
		dest = (word *) dst -> data + (dst->cy1 * dst->pitch) + dst->cx1 + curx;
		cury = 0;

		do
		{
			spany = src->height - (cury ? 0 : y);
			if (cury + spany >= cliph)
				spany = cliph - cury;

			for (i = 0; i < spany; i++, source += src->width, dest += dst->pitch)
				memcpy(dest, source, spanx*2);

			source = (word *) src->data + x;
			cury += spany;
		} while (cury < cliph);
		curx +=	spanx;
	} while (curx < clipw);
}  


void dd16_ScaleBlit8(int x, int y, int dw, int dh, byte *src, int sw, int sh, quad pal[],  image *dest)
{
	int i, j;
	int xerr, yerr;
	int xerr_start, yerr_start;
	int xadj, yadj;
	word *d;
	byte *s;
	int xl, yl, xs, ys;
	int cx1=0, cy1=0,
		cx2=0, cy2=0;

	if (dw < 1 || dh < 1)
		return;

	xl = dw;
	yl = dh;
	xs = ys = 0;
	dest->GetClip(&cx1, &cy1, &cx2, &cy2);
	if (x > cx2 || y > cy2
	|| x + xl < cx1 || y + yl < cy1)
		return;
	if (x + xl > cx2)
		xl = cx2 - x + 1;
	if (y + yl > cy2)
		yl = cy2 - y + 1;
	if (x < cx1) { xs = cx1 - x; xl -= xs; x = cx1; }
	if (y < cy1) { ys = cy1 - y; yl -= ys; y = cy1; }

	xadj = (sw << 16)/dw;
	yadj = (sh << 16)/dh;
	xerr_start = xadj * xs;
	yerr_start = yadj * ys;

	s = (byte *) src;
	s += ((yerr_start >> 16) * sw);
	d = ((word *) dest->data) + (y * dest->pitch) + x;
	yerr = yerr_start & 0xffff;

	for (i = 0; i < yl; i += 1)
	{
		xerr = xerr_start;
		for (j = 0; j < xl; j += 1)
		{
			d[j] = pal[s[(xerr >> 16)]];
			xerr += xadj;
		}
		d    += dest->pitch;
		yerr += yadj;
		s    += (yerr>>16)*sw;
		yerr &= 0xffff;
	}
}


image *dd16_ImageFrom8bpp(byte *src, int width, int height, byte *pal)
{
	word palconv[256], *p;
	image *b;
	int i;

	b = new image(width, height);
	p = (word *) b->data;
	for (i=0; i<256; i++)
		palconv[i] = MakeColor(pal[i*3], pal[(i*3)+1], pal[(i*3)+2]);
	for (i=0; i<width*height; i++)
		p[i] = palconv[src[i]];
	return b;
}


image *dd16_ImageFrom24bpp(byte *src, int width, int height)
{
	word *dest;
	image *img;
	int i;
	byte r, g, b;

	img = new image(width, height);
	dest = (word *) img->data;
	for (i=0; i<width*height; i++)
	{
		r = *src++;
		g = *src++;
		b = *src++;
		dest[i] = MakeColor(r,g,b);
	}
	return img;
}

/********************** 32bpp blitter code **********************/

int dd32_MakeColor(int r, int g, int b)
{
	return ((r<<16)|(g<<8)|b);
}


void dd32_Clear(int color, image *dest)
{	
	int *d = (int *)dest->data;
	int bytes = dest->pitch * dest->height;
	while (bytes--)
		*d++ = color;
}


void dd32_PutPixel(int x, int y, int color, image *dest)
{
	int *ptr = (int *)dest->data;
	if (x<dest->cx1 || x>dest->cx2 || y<dest->cy1 || y>dest->cy2)
		return;
	ptr[(y * dest->pitch) + x] = color;
}


void dd32_HLine(int x, int y, int xe, int color, image *dest)
{
	int *d = (int *) dest->data;
	int cx1=0, cy1=0, cx2=0, cy2=0;
	if (xe<x) SWAP(x,xe);
	dest->GetClip(&cx1, &cy1, &cx2, &cy2);
	if (x>cx2 || y>cy2 || xe<cx1 || y<cy1)
		return;
	if (xe>cx2) xe=cx2;
	if (x<cx1)  x =cx1;

	d += (y * dest->pitch) + x;
	for (; x<=xe; x++)
		*d++ = color;
}


void dd32_VLine(int x, int y, int ye, int color, image *dest)
{
	int *d = (int *) dest->data;
	int cx1=0, cy1=0, cx2=0, cy2=0;
	if (ye<y) SWAP(y,ye);
	dest->GetClip(&cx1, &cy1, &cx2, &cy2);
	if (x>cx2 || y>cy2 || x<cx1 || ye<cy1)
		return;
	if (ye>cy2) ye=cy2;
	if (y<cy1)  y =cy1;

	d += (y * dest->pitch) + x;
	for (; y<=ye; y++, d+=dest->pitch)
		*d = color;		
}


void dd32_Blit(int x, int y, image *src, image *dest)
{
	int *s=(int *)src->data,
		*d=(int *)dest->data;
	int spitch=src->pitch,
		dpitch=dest->pitch;
	int xlen=src->width,
		ylen=src->height;
	int cx1=0, cy1=0,
		cx2=0, cy2=0;

	dest->GetClip(&cx1, &cy1, &cx2, &cy2);
	if (x>cx2 || y>cy2 || x+xlen<cx1 || y+ylen<cy1)
		return;

	if (x+xlen>cx2) xlen = cx2-x+1;
	if (y+ylen>cy2) ylen = cy2-y+1;
	if (x<cx1) 	{
		s +=(cx1-x);
		xlen-=(cx1-x);
		x  =cx1;
	}
	if (y<cy1) {
		s +=(cy1-y)*spitch;
		ylen-=(cy1-y);
		y  =cy1;
	}

	d += (y * dpitch) + x;
	for (xlen *= 4; ylen--; s+=spitch, d+=dpitch)
		memcpy(d, s, xlen);
}


void dd32_TBlit(int x, int y, image *src, image *dest)
{
	int *s=(int *)src->data,c,
		*d=(int *)dest->data;
	int spitch=src->pitch,
		dpitch=dest->pitch;
	int xlen=src->width,
		ylen=src->height;
	int cx1=0, cy1=0,
		cx2=0, cy2=0;

	dest->GetClip(&cx1, &cy1, &cx2, &cy2);
	if (x>cx2 || y>cy2 || x+xlen<cx1 || y+ylen<cy1)
		return;

	if (x+xlen > cx2) xlen=cx2-x+1;
	if (y+ylen > cy2) ylen=cy2-y+1;
	if (x<cx1) {
		s +=(cx1-x);
		xlen-=(cx1-x);
		x  =cx1;
	}
	if (y<cy1) {
		s +=(cy1-y)*spitch;
		ylen-=(cy1-y);
		y  =cy1;
	}
	d+=y*dpitch+x;
	for (; ylen; ylen--)
	{
		for (x=0; x<xlen; x++)
		{
			c=s[x];
			if (c != transColor) d[x]=c;
		}
		s+=spitch;
		d+=dpitch;
	}
}


void dd32_Blit8(int x, int y, char *src, int w, int h, quad pal[], image *dest)
{
	byte *s=(byte *) src;
	quad *d=(quad *)dest->data;
	int spitch=w,
		dpitch=dest->pitch;
	int xlen=w,
		ylen=h;
	int cx1=0, cy1=0,
		cx2=0, cy2=0;

	dest->GetClip(&cx1, &cy1, &cx2, &cy2);
	if (x>cx2 || y>cy2 || x+xlen<cx1 || y+ylen<cy1)
		return;

	if (x+xlen > cx2) xlen=cx2-x+1;
	if (y+ylen > cy2) ylen=cy2-y+1;
	if (x<cx1) {
		s +=(cx1-x);
		xlen-=(cx1-x);
		x  =cx1;
	}
	if (y<cy1) {
		s +=(cy1-y)*spitch;
		ylen-=(cy1-y);
		y  =cy1;
	}

	d+=(y*dest->pitch)+x;
	for (; ylen; ylen--)
	{
		for (x=0; x<xlen; x++)
			d[x]=pal[s[x]];
		s+=spitch;
		d+=dpitch;
	}
}


void dd32_TBlit8(int x, int y, char *src, int w, int h, quad pal[], image *dest)
{
	byte *s=(byte *) src;
	quad *d=(quad *) dest->data;
	int spitch=w,
		dpitch=dest->pitch;
	int xlen=w,
		ylen=h;
	int cx1=0, cy1=0,
		cx2=0, cy2=0;
	byte c;

	dest->GetClip(&cx1, &cy1, &cx2, &cy2);
	if (x>cx2 || y>cy2 || x+xlen<cx1 || y+ylen<cy1)
		return;

	if (x+xlen > cx2) xlen=cx2-x+1;
	if (y+ylen > cy2) ylen=cy2-y+1;

	if (x<cx1) {
		s +=(cx1-x);
		xlen-=(cx1-x);
		x = cx1;
	}
	if (y<cy1) {
		s +=(cy1-y)*spitch;
		ylen-=(cy1-y);
		y=cy1;
	}
	d+=y*dpitch+x;
	for (; ylen; ylen--)
	{
		for (x=0; x<xlen; x++)
		{
			c=s[x];
			if (c) d[x]=pal[c];
		}
		s+=spitch;
		d+=dpitch;
	}
}


void dd32_WrapBlit(int x, int y, image *src, image *dst)
{
	int i;
	int cliph, clipw;
	int curx, cury;
	int spanx, spany;
	quad *source, *dest;

	cliph = dst->cy2 - dst->cy1 + 1;
	clipw = dst->cx2 - dst->cx1 + 1;
	y %= src->height;
	curx = 0;

	do
	{
		x %= curx ? 1 : src->width;
		spanx = src->width - x;
		if (curx + spanx >= clipw)
			spanx = clipw - curx;
		source = (quad *) src -> data + (y * src->width) + x;
		dest = (quad *) dst -> data + (dst->cy1 * dst->pitch) + dst->cx1 + curx;
		cury = 0;

		do
		{
			spany = src->height - (cury ? 0 : y);
			if (cury + spany >= cliph)
				spany = cliph - cury;

			for (i = 0; i < spany; i++, source += src->width, dest += dst->pitch)
				memcpy(dest, source, spanx*4);

			source = (quad *) src->data + x;
			cury += spany;
		} while (cury < cliph);
		curx +=	spanx;
	} while (curx < clipw);
}  


image *dd32_ImageFrom8bpp(byte *src, int width, int height, byte *pal)
{
	quad palconv[256], *p;
	image *b;
	int i;

	b = new image(width, height);
	p = (quad *) b->data;
	for (i=0; i<256; i++)
		palconv[i] = MakeColor(pal[i*3], pal[(i*3)+1], pal[(i*3)+2]);
	for (i=0; i<width*height; i++)
		p[i] = palconv[src[i]];
	return b;
}


image *dd32_ImageFrom24bpp(byte *src, int width, int height)
{
	quad *dest;
	image *img;
	int i;
	byte r, g, b;

	img = new image(width, height);
	dest = (quad *) img->data;
	for (i=0; i<width*height; i++)
	{
		r = *src++;
		g = *src++;
		b = *src++;
		dest[i] = MakeColor(r,g,b);
	}
	return img;
}


/*********************** blitter managment **********************/

void dd_RegisterBlitters()
{
	switch (vid_bpp)
	{
		case 15:
			Flip			= vid_window ? ddwin_Flip : dd_Flip;
			MakeColor		= dd15_MakeColor;
			Clear			= dd16_Clear;
			PutPixel		= dd16_PutPixel;
			HLine			= dd16_HLine;
			VLine			= dd16_VLine;
			Line			= dd_Line;
			Box				= dd_Box;
			Rect            = dd_Rect;
			Sphere          = dd_Sphere;
			Circle          = dd_Circle;
			Blit            = dd16_Blit;
			TBlit           = dd16_TBlit;
			Blit8           = dd16_Blit8;
			TBlit8			= dd16_TBlit8;
			WrapBlit        = dd16_WrapBlit;
			ScaleBlit8      = dd16_ScaleBlit8;
			ImageFrom8bpp	= dd16_ImageFrom8bpp;
			ImageFrom24bpp	= dd16_ImageFrom24bpp;
			break;
		case 16:
			Flip			= vid_window ? ddwin_Flip : dd_Flip;
			MakeColor		= dd16_MakeColor;
			Clear			= dd16_Clear;
			PutPixel		= dd16_PutPixel;
			HLine			= dd16_HLine;
			VLine			= dd16_VLine;
			Line			= dd_Line;
			Box				= dd_Box;
			Rect            = dd_Rect;
			Sphere          = dd_Sphere;
			Circle          = dd_Circle;
			Blit            = dd16_Blit;
			TBlit           = dd16_TBlit;
			Blit8           = dd16_Blit8;
			TBlit8			= dd16_TBlit8;
			WrapBlit        = dd16_WrapBlit;
			ScaleBlit8      = dd16_ScaleBlit8;
			ImageFrom8bpp	= dd16_ImageFrom8bpp;
			ImageFrom24bpp	= dd16_ImageFrom24bpp;
			break;
		case 32:
			Flip			= vid_window ? ddwin_Flip : dd_Flip;
			MakeColor		= dd32_MakeColor;
			Clear			= dd32_Clear;
			PutPixel		= dd32_PutPixel;
			HLine			= dd32_HLine;
			VLine			= dd32_VLine;
			Line			= dd_Line;
			Box				= dd_Box;
			Rect            = dd_Rect;
			Sphere          = dd_Sphere;
			Circle          = dd_Circle;
			Blit            = dd32_Blit;
			TBlit           = dd32_TBlit;
			Blit8	        = dd32_Blit8;
			TBlit8			= dd32_TBlit8;
			WrapBlit        = dd32_WrapBlit;
			ImageFrom8bpp	= dd32_ImageFrom8bpp;
			ImageFrom24bpp	= dd32_ImageFrom24bpp;
			break;
		default:
			err("vid_bpp (%d) not a standard value", vid_bpp);
	}
	Init_2xSAI(vid_bpp);
	transColor = MakeColor(255, 0, 255);
}
