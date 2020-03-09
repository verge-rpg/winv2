/****************************************************************
	xerxes engine
	g_8bpp.cpp
 ****************************************************************/

#include "xerxes.h"
#include <math.h>
/***************************** data *****************************/

byte *screen8 = 0, *realscreen = 0;
quad pal[256], base_pal[256];
byte pal8[256*3], base_pal8[256*3];

int clip_x, clip_y, clip_xend, clip_yend;
int screen_width, screen_height;

byte transtbl[256][256];

/***************************** code *****************************/

void Init8bpp()
{
	if (screen8) delete[] screen8;
	screen8 = new byte[v2_xres * v2_yres];
	realscreen = screen8;
	clip_x = 0;
	clip_y = 0;
	clip_xend = v2_xres-1;
	clip_yend = v2_yres-1;
	screen_width = v2_xres;
	screen_height = v2_yres;
}

void PutPixel8(int x, int y, int c)
{
	if (x<clip_x || x>clip_xend || y<clip_y || y>clip_yend)
		return;
	screen8[(y * screen_width) + x] = c;
}

byte ReadPixel8(int x, int y)
{
	if (x<clip_x || x>clip_xend || y<clip_y || y>clip_yend)
		return 0;
	return screen8[(y * screen_width) + x];
}

void HLine8(int x, int y, int xe, int color)
{
	byte *d = screen8;	
	if (xe<x) SWAP(x,xe);	
	if (x>clip_xend || y>clip_yend || xe<clip_x || y<clip_y)
		return;
	if (xe>clip_xend) xe=clip_xend;
	if (x<clip_x) x=clip_x;

	d += (y * screen_width) + x;
	for (; x<=xe; x++)
		*d++ = color;
}

void VLine8(int x, int y, int ye, int color)
{
	byte *d = screen8;
	if (ye<y) SWAP(y,ye);
	if (x>clip_xend || y>clip_yend || x<clip_x || ye<clip_y)
		return;
	if (ye>clip_yend) ye=clip_yend;
	if (y<clip_y)  y =clip_y;

	d += (y * screen_width) + x;
	for (; y<=ye; y++, d+=screen_width)
		*d = color;		
}

void Line8(int x, int y, int xe, int ye, int color)
{
	int dx = xe - x, dy = ye - y,
		xg = sgn(dx), yg = sgn(dy),
		i = 0;
	float slope = 0;

	if (abs(dx) >= abs(dy))
	{
		slope = (float) dy / (float) dx;
		for (i=0; i!=dx; i+=xg)
			PutPixel8(x+i, y+(int)(slope*i), color);
	}
	else
	{
		slope = (float) dx / (float) dy;
		for (i=0; i!=dy; i+=yg)
			PutPixel8(x+(int)(slope*i), y+i, color);
	}
	PutPixel8(xe, ye, color);
}

void Box8(int x, int y, int x2, int y2, int color)
{
	if (x2<x) SWAP(x,x2);
	if (y2<y) SWAP(y,y2);
	HLine8(x, y, x2, color);
	HLine8(x, y2, x2, color);
	VLine8(x, y+1, y2-1, color);
	VLine8(x2, y+1, y2-1, color);
}

void Rect8(int x, int y, int x2, int y2, int color)
{
	if (y2<y) SWAP(y,y2);
	for (; y<=y2; y++)
		HLine8(x, y, x2, color);
}

void Oval8(int x, int y, int xe, int ye, int color, int Fill)
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
		HLine8(x, y, x+xx-1, color);
	else {
		PutPixel8(x, y, color);
		PutPixel8(x+xx, y, color);
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
			HLine8(xe, ye, xe+xx-1, color);
		else {
			PutPixel8(xe, ye, color);
			PutPixel8(xe+xx, ye, color);
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
			HLine8(x, y, x+xx-1, color);
			HLine8(xe, ye, xe+xx-1, color);
		}
		else {
			PutPixel8(x, y, color);
			PutPixel8(x+xx, y, color);
			PutPixel8(xe, ye, color);
			PutPixel8(xe+xx, ye, color);
		}
	}
}

void Sphere8(int x, int y, int xradius, int yradius, int color)
{
	Oval8(x-xradius, y-yradius, x+xradius-1, y+yradius-1, color, 1);
}

void Circle8(int x, int y, int xradius, int yradius, int color)
{
	Oval8(x-xradius, y-yradius, x+xradius-1, y+yradius-1, color, 0);
}

void CopySprite8(int x, int y, int width, int height, byte *source)
{
	int xend = x + width - 1,
		yend = y + height - 1;

	if (x > clip_xend || xend < clip_x || y > clip_yend || yend < clip_y)
		return;

	if (x < clip_x)
	{
		source += (clip_x - x);
		x = clip_x;
	}
	if (y < clip_y)
	{
		source += width*(clip_y - y);
		y = clip_y;
	}
	if (xend > clip_xend)
		xend = clip_xend;
	if (yend > clip_yend)
		yend = clip_yend;

	byte *dest = screen8 + y*screen_width + x;
	xend = xend - x + 1;
	y = yend - y + 1;

	do
	{
		memcpy(dest, source, xend);
		source += width;
		dest += screen_width;
	} while (--y);
}

void TCopySprite8(int x, int y, int width, int height, byte *source)
{
	int xend = x + width - 1,
		yend = y + height - 1;	

	if (x > clip_xend || xend < clip_x || y > clip_yend || yend < clip_y)
		return;

	if (x < clip_x)
	{
		source += (clip_x - x);
		x = clip_x;
	}
	if (y < clip_y)
	{
		source += width*(clip_y - y);
		y = clip_y;
	}
	if (xend > clip_xend)
		xend = clip_xend;
	if (yend > clip_yend)
		yend = clip_yend;

	byte *dest = screen8 + y*screen_width + x;
	xend = xend - x + 1;
	y = yend - y + 1;

	do 
	{
		for (yend=0; yend < xend; yend++)
		{
			byte c = source[yend];
			if (c) dest[yend] = c;
		}
		source += width;
		dest += screen_width;
	} while (--y);
}

void ScaleSprite8(int x, int y, int sw, int sh, int dw, int dh, byte *s)
{
	int i, j;
	int xerr, yerr;
	int xerr_start, yerr_start;
	int xadj, yadj;
	byte *d;
	int xl, yl, xs, ys;

	if (dw < 1 || dh < 1)
		return;

	xl = dw;
	yl = dh;
	xs = ys = 0;

	if (x > clip_xend || y > clip_yend
	|| x + xl < clip_x || y + yl < clip_y)
		return;
	if (x + xl > clip_xend)
		xl = clip_xend - x + 1;
	if (y + yl > clip_yend)
		yl = clip_yend - y + 1;
	if (x < clip_x) { xs = clip_x - x; xl -= xs; x = clip_x; }
	if (y < clip_y) { ys = clip_y - y; yl -= ys; y = clip_y; }

	xadj = (sw << 16) / dw;
	yadj = (sh << 16) / dh;
	xerr_start = xadj * xs;
	yerr_start = yadj * ys;

	
	s += ((yerr_start >> 16) * sw);
	d = screen8 + (y * screen_width) + x;
	yerr = yerr_start & 0xffff;

	for (i = 0; i < yl; i += 1)
	{
		xerr = xerr_start;
		for (j = 0; j < xl; j += 1)
		{
			d[j] = s[(xerr >> 16)];
			xerr += xadj;
		}
		d    += screen_width;
		yerr += yadj;
		s    += (yerr >> 16) * sw;
		yerr &= 0xffff;
	}
}

void TScaleSprite8(int x, int y, int sw, int sh, int dw, int dh, byte *s)
{
	int i, j;
	int xerr, yerr;
	int xerr_start, yerr_start;
	int xadj, yadj;
	byte *d, c;
	int xl, yl, xs, ys;

	if (dw < 1 || dh < 1)
		return;

	xl = dw;
	yl = dh;
	xs = ys = 0;

	if (x > clip_xend || y > clip_yend
	|| x + xl < clip_x || y + yl < clip_y)
		return;
	if (x + xl > clip_xend)
		xl = clip_xend - x + 1;
	if (y + yl > clip_yend)
		yl = clip_yend - y + 1;
	if (x < clip_x) { xs = clip_x - x; xl -= xs; x = clip_x; }
	if (y < clip_y) { ys = clip_y - y; yl -= ys; y = clip_y; }

	xadj = (sw << 16) / dw;
	yadj = (sh << 16) / dh;
	xerr_start = xadj * xs;
	yerr_start = yadj * ys;

	
	s += ((yerr_start >> 16) * sw);
	d = screen8 + (y * screen_width) + x;
	yerr = yerr_start & 0xffff;

	for (i = 0; i < yl; i += 1)
	{
		xerr = xerr_start;
		for (j = 0; j < xl; j += 1)
		{
			c = s[(xerr >> 16)];
			if (c) d[j] = c;
			xerr += xadj;
		}
		d    += screen_width;
		yerr += yadj;
		s    += (yerr >> 16) * sw;
		yerr &= 0xffff;
	}
}

void WrapBlit8(int x, int y, int sw, int sh, byte *src)
{
	int cur_x, sign_y;

	if (sw < 1 || sh < 1)
		return;

	x %= sw, y %= sh;
	sign_y = 0 - y;

	for (; sign_y < screen_height; sign_y += sh)
		for (cur_x = 0 - x; cur_x < screen_width; cur_x += sw)
			CopySprite8(cur_x, sign_y, sw, sh, src);
}

void TWrapBlit8(int x, int y, int sw, int sh, byte *src)
{
	int cur_x, sign_y;

	if (sw < 1 || sh < 1)
		return;

	x %= sw, y %= sh;
	sign_y = 0 - y;

	for (; sign_y < screen_height; sign_y += sh)
		for (cur_x = 0 - x; cur_x < screen_width; cur_x += sw)
			TCopySprite8(cur_x, sign_y, sw, sh, src);
}

void Silhouette8(int x, int y, int width, int height, byte color, byte* source)
{
	byte* dest;
	int	clip_width;

	if (x > clip_xend || x + width - 1 < clip_x	||	y > clip_yend || y + height - 1 < clip_y)
		return;
	
	clip_width = width;
	if (x + clip_width - 1 > clip_xend)
		clip_width = clip_xend - x + 1;
	if (y + height - 1 > clip_yend)
		height = clip_yend - y + 1;
	
	if (x < clip_x)
	{
		source += (clip_x - x);
		clip_width -= (clip_x - x);
		x = clip_x;
	}
	if (y < clip_y)
	{
		source += (clip_y - y)*width;
		height -= (clip_y - y);
		y = clip_y;
	}

	dest = screen8 + y*screen_width + x;

	do {
		x = clip_width;
		do
		{
			if (*source)
				*dest = color;
			source += 1;
			dest += 1;
			x -= 1;
		} while (x);

		source += (width - clip_width);
		dest += (screen_width - clip_width);
		height -= 1;
	} while (height);
}

void RotScale8(int posx, int posy, quad width, quad height, float angle, float scale, byte* src)
{
	int xs,ys,xl,yl;
	int sinas,cosas,xc,yc,srcx,srcy,x,y,tempx,tempy,T_WIDTH_CENTER,T_HEIGHT_CENTER,W_WIDTH_CENTER,W_HEIGHT_CENTER,W_HEIGHT,W_WIDTH;
	byte *dest;	
	float ft;

	ft=atan2((float)width,(float)height);

	T_WIDTH_CENTER=width>>1;
	T_HEIGHT_CENTER=height>>1;
	W_WIDTH=((float)width/scale*sin(ft) + (float)height/scale*cos(ft));
	W_HEIGHT=W_WIDTH;
	W_HEIGHT_CENTER=W_HEIGHT>>1;
	W_WIDTH_CENTER=W_HEIGHT_CENTER;

	sinas=sin(-angle)*65536*scale;
	cosas=cos(-angle)*65536*scale;

	xc=T_WIDTH_CENTER*65536 - (W_HEIGHT_CENTER*(cosas+sinas));
	yc=T_HEIGHT_CENTER*65536 - (W_WIDTH_CENTER*(cosas-sinas));
	posx-=W_WIDTH_CENTER;
	posy-=W_HEIGHT_CENTER;

	// clipping
	if (W_WIDTH<2 || W_HEIGHT<2) return;
	xl=W_WIDTH;
	yl=W_HEIGHT;
	xs=ys=0;
	if (posx>clip_xend || posy>clip_yend || posx+xl<clip_x || posy+yl<clip_y)
		return;
	if (posx+xl > clip_xend) xl=clip_xend-posx+1;
	if (posy+yl > clip_yend) yl=clip_yend-posy+1;
	if (posx<clip_x)
	{
		xs=clip_x-posx;
		xl-=xs;
		posx=clip_x;

		xc+=cosas*xs;
		yc-=sinas*xs;
	}
	if (posy<clip_y)
	{
		ys=clip_y-posy;
		yl-=ys;
		posy=clip_y;

		xc+=sinas*ys;
		yc+=cosas*ys;
	}

	dest=screen8+posx+posy*screen_width;
	for (y=0; y<yl; y++)
	{
		srcx=xc;
		srcy=yc;

		for (x=0; x<xl; x++)
		{
			tempx=(srcx>>16);
			tempy=(srcy>>16);

			if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
			{
				dest[x]=src[tempx+tempy*width];
			}

			srcx+=cosas;
			srcy-=sinas;
		}

		dest+=screen_width;

		xc+=sinas;
		yc+=cosas;
	}
}

/****************************** lucent blitters ******************************/

void PutPixel8_Lucent(int x, int y, int c)
{
	if (x<clip_x || x>clip_xend || y<clip_y || y>clip_yend)
		return;
	screen8[(y * screen_width) + x] = transtbl[screen8[(y * screen_width) + x]][c];
}

void CopySprite8_Lucent(int x, int y, int width, int height, byte *source)
{
	int xend = x + width - 1,
		yend = y + height - 1;

	if (x > clip_xend || xend < clip_x || y > clip_yend || yend < clip_y)
		return;

	if (x < clip_x)
	{
		source += (clip_x - x);
		x = clip_x;
	}
	if (y < clip_y)
	{
		source += width*(clip_y - y);
		y = clip_y;
	}
	if (xend > clip_xend)
		xend = clip_xend;
	if (yend > clip_yend)
		yend = clip_yend;

	byte *dest = screen8 + y*screen_width + x;
	xend = xend - x + 1;
	y = yend - y + 1;

	do
	{
		for (yend=0; yend < xend; yend++)
			dest[yend] = transtbl[source[yend]][dest[yend]];
		source += width;
		dest += screen_width;
	} while (--y);
}

void TCopySprite8_Lucent(int x, int y, int width, int height, byte *source)
{
	int xend = x + width - 1,
		yend = y + height - 1;

	if (x > clip_xend || xend < clip_x || y > clip_yend || yend < clip_y)
		return;

	if (x < clip_x)
	{
		source += (clip_x - x);
		x = clip_x;
	}
	if (y < clip_y)
	{
		source += width*(clip_y - y);
		y = clip_y;
	}
	if (xend > clip_xend)
		xend = clip_xend;
	if (yend > clip_yend)
		yend = clip_yend;

	byte *dest = screen8 + y*screen_width + x;
	xend = xend - x + 1;
	y = yend - y + 1;

	do
	{
		for (yend=0; yend < xend; yend++)
		{
			byte c = source[yend];
			if (c) dest[yend] = transtbl[c][dest[yend]];
		}
		source += width;
		dest += screen_width;
	} while (--y);
}

void HLine8_Lucent(int x, int y, int xe, int color)
{
	byte *d = screen8;	
	if (xe<x) SWAP(x,xe);	
	if (x>clip_xend || y>clip_yend || xe<clip_x || y<clip_y)
		return;
	if (xe>clip_xend) xe=clip_xend;
	if (x<clip_x) x=clip_x;

	d += (y * screen_width) + x;
	for (; x<=xe; x++, d++)
		*d = transtbl[*d][color];
}

void VLine8_Lucent(int x, int y, int ye, int color)
{
	byte *d = screen8;
	if (ye<y) SWAP(y,ye);
	if (x>clip_xend || y>clip_yend || x<clip_x || ye<clip_y)
		return;
	if (ye>clip_yend) ye=clip_yend;
	if (y<clip_y)  y =clip_y;

	d += (y * screen_width) + x;
	for (; y<=ye; y++, d+=screen_width)
		*d = transtbl[*d][color];
}

void Line8_Lucent(int x, int y, int xe, int ye, int color)
{
	int dx = xe - x, dy = ye - y,
		xg = sgn(dx), yg = sgn(dy),
		i = 0;
	float slope = 0;

	if (abs(dx) >= abs(dy))
	{
		slope = (float) dy / (float) dx;
		for (i=0; i!=dx; i+=xg)
			PutPixel8_Lucent(x+i, y+(int)(slope*i), color);
	}
	else
	{
		slope = (float) dx / (float) dy;
		for (i=0; i!=dy; i+=yg)
			PutPixel8_Lucent(x+(int)(slope*i), y+i, color);
	}
	PutPixel8_Lucent(xe, ye, color);
}


void Rect8_Lucent(int x, int y, int x2, int y2, int color)
{
	if (y2<y) SWAP(y,y2);
	for (; y<=y2; y++)
		HLine8_Lucent(x, y, x2, color);
}

void Box8_Lucent(int x, int y, int x2, int y2, int color)
{
	if (x2<x) SWAP(x,x2);
	if (y2<y) SWAP(y,y2);
	HLine8_Lucent(x, y, x2, color);
	HLine8_Lucent(x, y2, x2, color);
	VLine8_Lucent(x, y+1, y2-1, color);
	VLine8_Lucent(x2, y+1, y2-1, color);
}


void Oval8_Lucent(int x, int y, int xe, int ye, int color, int Fill)
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
		HLine8_Lucent(x, y, x+xx-1, color);
	else {
		PutPixel8_Lucent(x, y, color);
		PutPixel8_Lucent(x+xx, y, color);
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
			HLine8_Lucent(xe, ye, xe+xx-1, color);
		else {
			PutPixel8_Lucent(xe, ye, color);
			PutPixel8_Lucent(xe+xx, ye, color);
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
			HLine8_Lucent(x, y, x+xx-1, color);
			HLine8_Lucent(xe, ye, xe+xx-1, color);
		}
		else {
			PutPixel8_Lucent(x, y, color);
			PutPixel8_Lucent(x+xx, y, color);
			PutPixel8_Lucent(xe, ye, color);
			PutPixel8_Lucent(xe+xx, ye, color);
		}
	}
}

void Sphere8_Lucent(int x, int y, int xradius, int yradius, int color)
{
	Oval8_Lucent(x-xradius, y-yradius, x+xradius-1, y+yradius-1, color, 1);
}

void Circle8_Lucent(int x, int y, int xradius, int yradius, int color)
{
	Oval8_Lucent(x-xradius, y-yradius, x+xradius-1, y+yradius-1, color, 0);
}

void WrapBlit8_Lucent(int x, int y, int sw, int sh, byte *src)
{
	int cur_x, sign_y;

	if (sw < 1 || sh < 1)
		return;

	x %= sw, y %= sh;
	sign_y = 0 - y;

	for (; sign_y < screen_height; sign_y += sh)
		for (cur_x = 0 - x; cur_x < screen_width; cur_x += sw)
			CopySprite8_Lucent(cur_x, sign_y, sw, sh, src);
}

void TWrapBlit8_Lucent(int x, int y, int sw, int sh, byte *src)
{
	int cur_x, sign_y;

	if (sw < 1 || sh < 1)
		return;

	x %= sw, y %= sh;
	sign_y = 0 - y;

	for (; sign_y < screen_height; sign_y += sh)
		for (cur_x = 0 - x; cur_x < screen_width; cur_x += sw)
			TCopySprite8_Lucent(cur_x, sign_y, sw, sh, src);
}

void ScaleSprite8_Lucent(int x, int y, int sw, int sh, int dw, int dh, byte *s)
{
	int i, j;
	int xerr, yerr;
	int xerr_start, yerr_start;
	int xadj, yadj;
	byte *d;
	int xl, yl, xs, ys;

	if (dw < 1 || dh < 1)
		return;

	xl = dw;
	yl = dh;
	xs = ys = 0;

	if (x > clip_xend || y > clip_yend
	|| x + xl < clip_x || y + yl < clip_y)
		return;
	if (x + xl > clip_xend)
		xl = clip_xend - x + 1;
	if (y + yl > clip_yend)
		yl = clip_yend - y + 1;
	if (x < clip_x) { xs = clip_x - x; xl -= xs; x = clip_x; }
	if (y < clip_y) { ys = clip_y - y; yl -= ys; y = clip_y; }

	xadj = (sw << 16) / dw;
	yadj = (sh << 16) / dh;
	xerr_start = xadj * xs;
	yerr_start = yadj * ys;

	
	s += ((yerr_start >> 16) * sw);
	d = screen8 + (y * screen_width) + x;
	yerr = yerr_start & 0xffff;

	for (i = 0; i < yl; i += 1)
	{
		xerr = xerr_start;
		for (j = 0; j < xl; j += 1)
		{
			d[j] = transtbl[d[j]][s[(xerr >> 16)]];
			xerr += xadj;
		}
		d    += screen_width;
		yerr += yadj;
		s    += (yerr >> 16) * sw;
		yerr &= 0xffff;
	}
}

void TScaleSprite8_Lucent(int x, int y, int sw, int sh, int dw, int dh, byte *s)
{
	int i, j;
	int xerr, yerr;
	int xerr_start, yerr_start;
	int xadj, yadj;
	byte *d, c;
	int xl, yl, xs, ys;

	if (dw < 1 || dh < 1)
		return;

	xl = dw;
	yl = dh;
	xs = ys = 0;

	if (x > clip_xend || y > clip_yend
	|| x + xl < clip_x || y + yl < clip_y)
		return;
	if (x + xl > clip_xend)
		xl = clip_xend - x + 1;
	if (y + yl > clip_yend)
		yl = clip_yend - y + 1;
	if (x < clip_x) { xs = clip_x - x; xl -= xs; x = clip_x; }
	if (y < clip_y) { ys = clip_y - y; yl -= ys; y = clip_y; }

	xadj = (sw << 16) / dw;
	yadj = (sh << 16) / dh;
	xerr_start = xadj * xs;
	yerr_start = yadj * ys;

	
	s += ((yerr_start >> 16) * sw);
	d = screen8 + (y * screen_width) + x;
	yerr = yerr_start & 0xffff;

	for (i = 0; i < yl; i += 1)
	{
		xerr = xerr_start;
		for (j = 0; j < xl; j += 1)
		{
			c = s[(xerr >> 16)];
			if (c) d[j] = transtbl[d[j]][c];
			xerr += xadj;
		}
		d    += screen_width;
		yerr += yadj;
		s    += (yerr >> 16) * sw;
		yerr &= 0xffff;
	}
}

void Silhouette8_Lucent(int x, int y, int width, int height, byte color, byte* source)
{
	byte* dest;
	int	clip_width;

	if (x > clip_xend || x + width - 1 < clip_x	||	y > clip_yend || y + height - 1 < clip_y)
		return;
	
	clip_width = width;
	if (x + clip_width - 1 > clip_xend)
		clip_width = clip_xend - x + 1;
	if (y + height - 1 > clip_yend)
		height = clip_yend - y + 1;
	
	if (x < clip_x)
	{
		source += (clip_x - x);
		clip_width -= (clip_x - x);
		x = clip_x;
	}
	if (y < clip_y)
	{
		source += (clip_y - y)*width;
		height -= (clip_y - y);
		y = clip_y;
	}

	dest = screen8 + y*screen_width + x;

	do {
		x = clip_width;
		do
		{
			if (*source)
				*dest = transtbl[*dest][color];
			source += 1;
			dest += 1;
			x -= 1;
		} while (x);

		source += (width - clip_width);
		dest += (screen_width - clip_width);
		height -= 1;
	} while (height);
}

void RotScale8_Lucent(int posx, int posy, quad width, quad height, float angle, float scale, byte* src)
{
	int xs,ys,xl,yl;
	int sinas,cosas,xc,yc,srcx,srcy,x,y,tempx,tempy,T_WIDTH_CENTER,T_HEIGHT_CENTER,W_WIDTH_CENTER,W_HEIGHT_CENTER,W_HEIGHT,W_WIDTH;
	byte *dest;	
	float ft;

	ft=atan2((float)width,(float)height);

	T_WIDTH_CENTER=width>>1;
	T_HEIGHT_CENTER=height>>1;
	W_WIDTH=((float)width/scale*sin(ft) + (float)height/scale*cos(ft));
	W_HEIGHT=W_WIDTH;
	W_HEIGHT_CENTER=W_HEIGHT>>1;
	W_WIDTH_CENTER=W_HEIGHT_CENTER;

	sinas=sin(-angle)*65536*scale;
	cosas=cos(-angle)*65536*scale;

	xc=T_WIDTH_CENTER*65536 - (W_HEIGHT_CENTER*(cosas+sinas));
	yc=T_HEIGHT_CENTER*65536 - (W_WIDTH_CENTER*(cosas-sinas));
	posx-=W_WIDTH_CENTER;
	posy-=W_HEIGHT_CENTER;

	// clipping
	if (W_WIDTH<2 || W_HEIGHT<2) return;
	xl=W_WIDTH;
	yl=W_HEIGHT;
	xs=ys=0;
	if (posx>clip_xend || posy>clip_yend || posx+xl<clip_x || posy+yl<clip_y)
		return;
	if (posx+xl > clip_xend) xl=clip_xend-posx+1;
	if (posy+yl > clip_yend) yl=clip_yend-posy+1;
	if (posx<clip_x)
	{
		xs=clip_x-posx;
		xl-=xs;
		posx=clip_x;

		xc+=cosas*xs;
		yc-=sinas*xs;
	}
	if (posy<clip_y)
	{
		ys=clip_y-posy;
		yl-=ys;
		posy=clip_y;

		xc+=sinas*ys;
		yc+=cosas*ys;
	}

	dest=screen8+posx+posy*screen_width;
	for (y=0; y<yl; y++)
	{
		srcx=xc;
		srcy=yc;

		for (x=0; x<xl; x++)
		{
			tempx=(srcx>>16);
			tempy=(srcy>>16);

			if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
				dest[x]=transtbl[src[tempx+tempy*width]][dest[x]];

			srcx+=cosas;
			srcy-=sinas;
		}

		dest+=screen_width;

		xc+=sinas;
		yc+=cosas;
	}
}

void ShowPage8()
{
	Blit8(0, 0, (char *) realscreen, v2_xres, v2_yres, pal, myscreen);
	ShowPage();
}