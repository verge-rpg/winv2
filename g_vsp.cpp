/****************************************************************
	xerxes engine
	g_vsp.cpp
 ****************************************************************/

#include "xerxes.h"
#define  VSP_8BPP_UNCOMPRESSED 2
#define  VSP_8BPP_BYTECOMPRESSED 3

VSP::VSP(char *fname)
{
	numtiles = 0;
	vspspace = 0;
	LoadVSP(fname);
	memcpy(base_pal, pal, sizeof pal);
	memcpy(base_pal8, pal8, sizeof pal8);
	mytimer = systemtime;	
}

VSP::~VSP(void)
{
	delete[] vspspace;
	delete[] tileidx;
	delete[] flipped;
}

void VSP::LoadVSP(char *fname)
{
	VFILE *f;
	word ver;	
	byte *cb;
	int i;
	
	f = vopen(fname);
	if (!f) err("VSP::LoadVSP, could not open file %s.",fname);

	vread(&ver, 2, f);
	switch (ver)
	{
		case VSP_8BPP_UNCOMPRESSED: /* uncompressed 8bpp V1/V2 VSP */
			vread(pal8, 768, f);
			vread(&numtiles, 2, f);
			vspspace = new byte[numtiles * 256];
			vread(vspspace, numtiles * 256, f);
			LoadAnimation(f);
			vclose(f);			
			for (i = 0; i < 768; i++)
				pal8[i] = pal8[i] * 255 / 63;
			for (i = 0; i < 256; i++)
				pal[i] = MakeColor(pal8[i * 3], pal8[(i * 3) + 1], pal8[(i * 3) + 2]);
			break;

		case VSP_8BPP_BYTECOMPRESSED: /* compressed 8bpp V2 VSP */
			vread(&pal8, 768, f);
			vread(&numtiles, 2, f);
			vread(&i, 4, f);
			vspspace = new byte[numtiles * 256];
			cb = new byte[numtiles * 256];
			vread(cb, i, f);
			DecodeByteCompression((byte *) vspspace, 256 * numtiles, (byte *) cb);
			delete cb;
			for (i = 0; i < 768; i++)
				pal8[i] = pal8[i] * 255 / 63;
			for (i = 0; i < 256; i++)
				pal[i] = MakeColor(pal8[i * 3], pal8[(i * 3) + 1], pal8[(i * 3) + 2]);		
			LoadAnimation(f);
			vclose(f);
			break;
		default: err("VSP::LoadVSP, unsupported VSP format");
	}
}


void VSP::LoadAnimation(VFILE *f)
{
	vread(vspanim, sizeof vspanim, f);
	for (int i=0; i<100; i++)
		vadelay[i]=0;
	tileidx = new int[numtiles];
	flipped = new int[numtiles];
	for (i=0; i<numtiles; i++)
	{
		flipped[i] = 0;
		tileidx[i] = i;
	}
}

void VSP::blit(int x, int y, int tile, image *dest)
{
	byte *c;
	while (mytimer < systemtime)
	{
		CheckTileAnimation();
		mytimer++;
	}
	tile = tileidx[tile];
	if (tile >= numtiles) err("VSP::Blit(), tile %d exceeds (%d)", tile, numtiles);
	c = (byte *) vspspace + (tile * 256);
	if (alpha==50)
		CopySprite8_Lucent(x, y, 16, 16, c);
	else
		CopySprite8(x, y, 16, 16, c);
}

void VSP::tblit(int x, int y, int tile, image *dest)
{
	byte *c;
	while (mytimer < systemtime)
	{
		CheckTileAnimation();
		mytimer++;
	}
	tile = tileidx[tile];
	if (tile >= numtiles) err("VSP::TBlit(), tile %d exceeds (%d)", tile, numtiles);
	c = (byte *) vspspace + (tile * 256);
	if (alpha==50)
		TCopySprite8_Lucent(x, y, 16, 16, c);
	else
		TCopySprite8(x, y, 16, 16, c);
}

void VSP::DecodeByteCompression(byte *dest, int len, byte *buf)
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
			return;
        while (run--)
            *dest++ = w;
    } while (len);
    return;
}

void VSP::AnimateTile(byte i, int l)
{
	switch (vspanim[i].mode)
	{
	    case 0:
			if (tileidx[l]<vspanim[i].finish) tileidx[l]++;
            else tileidx[l]=vspanim[i].start;
            break;
		case 1:
			if (tileidx[l]>vspanim[i].start) tileidx[l]--;
            else tileidx[l]=vspanim[i].finish;
            break;
		case 2:
			tileidx[l]=rnd(vspanim[i].start,vspanim[i].finish);
            break;
		case 3: 
			if (flipped[l])
            {
				if (tileidx[l]!=vspanim[i].start) tileidx[l]--;
				else { tileidx[l]++; flipped[l]=0; }
            }
            else
            {
				if (tileidx[l]!=vspanim[i].finish) tileidx[l]++;
				else { tileidx[l]--; flipped[l]=1; }
            }
			break;
	}
}

void VSP::Animate(byte i)
{
	int l;

	vadelay[i]=0;
	for (l = vspanim[i].start; l <= vspanim[i].finish; l++)
	    AnimateTile(i,l);
}

void VSP::CheckTileAnimation()
{
	byte i;
		
	for (i = 0; i<100; i++)
	{
		if ((vspanim[i].delay) && (vspanim[i].delay<vadelay[i]))
			Animate(i);
		vadelay[i]++;
	}
}
