/*********************************************************
 *   File: g_map.cpp									 *
 * Author: vecna (Benjamin J. Eirich)					 *
 *   Desc: VSP loading, converting, displaying class.    *
 *********************************************************/

#include "xerxes.h"
 
// ***************************** Code *****************************

MAP::MAP(char *fname)
{
	int i, bufsize;
	char strbuf[100];
	byte *cb, nents;
	
	VFILE *f = vopen(fname);
	if (!f) err("MAP::MAP, Could not open %s.",fname);

	vread(strbuf, 6, f);

	vread(&i, 4, f);
	vread(strbuf, 60, f);
	vsp = new VSP(strbuf);  // load VSP
	vread(strbuf, 60, f);
	PlayMusic(strbuf, false);    // play music
	vread(rstring, 20, f);
    vread(strbuf, 55, f);
	vread(&numlayers, 1, f);

	for (i = 0; i < numlayers; i++)
		vread(&layer[i], 12, f);

	// Map layer data
	for (i = 0; i < numlayers; i++)
	{ 
		vread(&bufsize, 4, f);
		cb = new byte[bufsize];
		layers[i] = new word[layer[i].sizex * layer[i].sizey];		
		vread(cb, bufsize, f);
		DecodeWordCompression(layers[i],(layer[i].sizex * layer[i].sizey), (word *) cb);
		delete [] cb;		
	}

	// Obstruct layer
	vread(&bufsize, 4, f);
	cb = new byte[bufsize];
	Obstruct = new byte[layer[0].sizex * layer[0].sizey];		
	vread(cb, bufsize, f);
	DecodeByteCompression((byte *) Obstruct,(layer[0].sizex * layer[0].sizey), cb);
	delete [] cb;		

	// Zone layer
	vread(&bufsize, 4, f);
	cb = new byte[bufsize];
	Zone = new byte[layer[0].sizex * layer[0].sizey];
	vread(cb, bufsize, f);
	DecodeByteCompression((byte *) Zone,(layer[0].sizex * layer[0].sizey), cb);
	delete [] cb;		

	// Load zone records
	memset(zones, 0, sizeof zones);
	vread(&i, 4, f);
	vread(zones, 50 * i, f);

	// Load CHR list
	memset(&chrlist, 0, sizeof chrlist);
	vread(&nmchr, 1, f);
	vread(&chrlist, 60*nmchr, f);
	
	// # entities..
	vread(&nents, 1, f);
	
	for (i=0; i<nents; i++)
	{
		entity_r e;
		vread(&e, sizeof entity_r, f);
		int z = AllocateEntity(e.x * 16, e.y * 16, chrlist[e.chrindex].t);
		switch (e.speed)
		{
			case 0: entity[z]->setspeed(0); break;
			case 1: entity[z]->setspeed(25); break;
			case 2: entity[z]->setspeed(33); break;
			case 3: entity[z]->setspeed(50); break;
			case 4: entity[z]->setspeed(100); break;
			case 5: entity[z]->setspeed(200); break;
			case 6: entity[z]->setspeed(300); break;
			case 7: entity[z]->setspeed(400); break;
			default: err("MAP::MAP() - entity %d invalid speed %d", i, e.speed);
		}
	}

char nms;
char msbuf[1024];
	vread(&nms, 1, f);
	vread(&i, 4, f);
	vread(msbuf, nms*4, f);
	if (nms)
	{
		char *ms=(char *) malloc(i);
		vread(ms, i, f);
	}
	else
	{
		vseek(f, i, 0);
		char *ms=(char *) malloc(16);
	}
	vread(&i, 4, f); // # of things
	current_map = this;

	LoadMapVC(f);
	vclose(f);
	ExecuteEvent(0);
}

MAP::~MAP()
{
	delete vsp;
	FreeMapVC();
	for (int i = 0; i < numlayers; i++)
		delete [] layers[i];
	delete [] Obstruct;
	delete [] Zone;
}

int MAP::obstructed(int x, int y)
{
	while (x<0) x += layer[0].sizex;
	while (y<0) y += layer[0].sizey;
	x %= layer[0].sizex;
	y %= layer[0].sizey;
	return Obstruct[(y*layer[0].sizex) + x];
}

int MAP::zone(int x, int y)
{
	while (x<0) x += layer[0].sizex;
	while (y<0) y += layer[0].sizey;
	x %= layer[0].sizex;
	y %= layer[0].sizey;
	return Zone[(y*layer[0].sizex) + x];
}

void MAP::settile(int x, int y, int l, int t)
{
	if (l>numlayers) err("MAP::settile(), invalid layer index.");
	l--;
	while (x<0) x += layer[l].sizex;
	while (y<0) y += layer[l].sizey;
	x %= layer[l].sizex;
	y %= layer[l].sizey;
	layers[l][(y * layer[l].sizex) + x] = t;
}

void MAP::RestoreCamera()
{
/*	umap = 0;
	lmap = 0;
	rmap = (layer[0].sizex * 16) - 1;
	dmap = (layer[0].sizey * 16) - 1;*/
}

void MAP::DecodeByteCompression(byte *dest, int len, byte *buf)
{
	int j,n;
	byte run, w;

	n = 0;
	do
	{
		w = *buf;
		buf++;
		if (w == 0xFF)
		{
			run = *buf;
			buf++;
			w = *buf;
			buf++;
			for (j = 0; j < run; j++)
				dest[n + j] = w;
			n += run;
		}
		else
		{
			dest[n] = w;
			n++;
		}
	} while (n < len);
}

void MAP::DecodeWordCompression(word *dest, int len, word *buf)
{
	int j,n;
	byte run;
	word w;

	n = 0;
	do
	{
		w = *buf;
		buf++;
		if ((w & 0xFF00) == 0xFF00)
		{
			run = (w & 0x00FF);
			w = *buf;
			buf++;
			for (j = 0; j < run; j ++)
				dest[n + j] = w;
			n += run;
		}
		else
		{
			dest[n] = w;
			n++;
		}
	} while (n < len);
}

void MAP::BlitBackLayer(byte l, image *dest, int tx, int ty, int xwin, int ywin)
{
	int i, j, c;
	int oxw, oyw, xofs, yofs, xtc, ytc;

	oxw = xwin * layer[l].pmultx / layer[l].pdivx;
	oyw = ywin * layer[l].pmulty / layer[l].pdivy;
	xofs =- (oxw & 15);
	yofs =- (oyw & 15);
	xtc = oxw >> 4;
	ytc = oyw >> 4;
  
    for (i = 0; i < ty; i++)
		for (j = 0; j < tx; j++)
		{
			if (ytc + i >= layer[l].sizey) 
				c = 0;
			else if (xtc + j >= layer[l].sizex)
				c = 0;
			else
				c = layers[l][(((ytc + i) * layer[l].sizex) + xtc + j)];
			vsp -> Blit((j * 16) + xofs, (i * 16) + yofs, c, dest);
		}
	curlayer++;
}

void MAP::BlitTransLayer(byte l, image *dest, int tx, int ty, int xwin, int ywin)
{
	int i, j, c;
	int oxw, oyw, xofs, yofs, xtc, ytc;

	oxw = xwin * layer[l].pmultx / layer[l].pdivx;
	oyw = ywin * layer[l].pmulty / layer[l].pdivy;
	xofs =- (oxw & 15);
	yofs =- (oyw & 15);
	xtc = oxw >> 4;
	ytc = oyw >> 4;
	
	for (i = 0; i < ty; i++)
		for (j = 0; j< tx; j++)
		{
			if (ytc + i >= layer[l].sizey) 
				c = 0;
			else if (xtc + j >= layer[l].sizex)
				c = 0;
			else
				c = layers[l][(((ytc + i) * layer[l].sizex) + xtc + j)];
			if (c) vsp -> TBlit((j * 16) + xofs, (i * 16) + yofs, c, dest);
		}
	curlayer++;
}

void MAP::BlitLayer(byte c, image *dest, int tx, int ty, int xwin, int ywin)
{
  if (curlayer)  { BlitTransLayer(c, dest, tx, ty, xwin, ywin); return; }
  if (!curlayer) { BlitBackLayer(c, dest, tx, ty, xwin, ywin);  return; }
}

void MAP::BlitObs(image *dest, int tx, int ty, int xwin, int ywin)
{
	int i, j;
	int oxw, oyw, xofs, yofs, xtc, ytc;

	oxw = xwin;
	oyw = ywin;
	xofs =- (oxw & 15);
	yofs =- (oyw & 15);
	xtc = oxw >> 4;
	ytc = oyw >> 4;
	
	SetLucent(50);
	for (i = 0; i < ty; i++)
		for (j = 0; j< tx; j++)
		{
		
		}
	SetLucent(0);
	curlayer++;
}

int MAP::mapheight()
{
	return layer[0].sizey;
}

int MAP::mapwidth()
{
	return layer[0].sizex;
}

/**************************************/

void MAP::Render(int x, int y, image *dest)
{
	char *src;
	int tx, ty;
  
	curlayer = 0;
	src = rstring;
	tx = dest -> width / 16;
	ty = dest -> height / 16;
	tx++; ty++;

	while (*src)
	{
		switch (*src)
		{
			case '1': BlitLayer(0, dest, tx, ty, x, y); break;
			case '2': BlitLayer(1, dest, tx, ty, x, y); break;
			case '3': BlitLayer(2, dest, tx, ty, x, y); break;
			case '4': BlitLayer(3, dest, tx, ty, x, y); break;
			case '5': BlitLayer(4, dest, tx, ty, x, y); break;
			case '6': BlitLayer(5, dest, tx, ty, x, y); break;
			case 'E': RenderEntities();
					  break;
			case 'R': HookRetrace();
					  break;
	    }
		src++;
	}
}