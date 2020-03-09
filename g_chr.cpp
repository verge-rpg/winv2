/****************************************************************
	xerxes engine
	g_chr.cpp
 ****************************************************************/

#include "xerxes.h"

/***************************** code *****************************/

CHR::CHR(char *fname)
{
	VFILE *f = vopen(fname);
	if (!f) err("CHR::CHR(), could not open %s", fname);

	fxsize=fysize=hx=hy=hw=hh=totalframes=0;

	byte ver;
	vread(&ver, 1, f);
	
	if (ver == 2)
	{
		vread(&fxsize, 2, f);
		vread(&fysize, 2, f);
		vread(&hx, 2, f);
		vread(&hy, 2, f);
		vread(&hw, 2, f);
		vread(&hh, 2, f);
    
		int n;
        vread(&totalframes, 2, f);
        vread(&n, 4, f);
        char *ptr = new char[n];
        vread(ptr, n, f);
        imagedata = new byte[fxsize * fysize * totalframes]; 
        
        n = uncompress(imagedata, fxsize * fysize * totalframes, ptr);
        if (n) err("LoadCHR: %s: bogus compressed image data", fname);
                
        delete[] ptr;

        vread(&idle[3], 4, f);
        vread(&idle[2], 4, f);
        vread(&idle[1], 4, f);
        vread(&idle[0], 4, f);

        for (int b=0; b<4; b++)
        {
			switch (b)
			{
				case 0: ptr = lanim; break;
				case 1: ptr = ranim; break;
				case 2: ptr = uanim; break;
				case 3: ptr = danim; break;
			}
			vread(&n, 4, f);
			if (n>99)
					err("Animation strand too long. %d", n);
			vread(ptr, n, f);
		}

	}
	else
		err("%s incorrect CHR version.", fname);
	vclose(f);

}

CHR::~CHR()
{
	delete[] imagedata;
}

int CHR::uncompress(byte* dest, int len, char *buf)
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

void CHR::Render(int x, int y, int frame, image *dest)
{
	x -= hx;
	y -= hy;

	if (frame >= totalframes)
		err("CHR::Render(), frame requested is undefined");

	byte *c = (byte *) imagedata + (frame * fxsize * fysize);
	TCopySprite8(x, y, fxsize, fysize, c);
}