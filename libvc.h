#include <math.h>
#include "sincos.h"

int Lucent;

void EnforceNoDirectories(string s)
{
	int	n = 0;        

    if (s[0]=='/' || s[0]=='\\')
		err("vc does not allow accessing dir: %s", s.c_str());

    if (s[1]==':')
		err("vc does not allow accessing dir: %s", s.c_str());

    n=0;
    while (n<s.length()-1)
    {
		if (s[n]=='.' && s[n+1]=='.')
			err("vc does not allow accessing dir: %s", s.c_str());
		n++;
    }
}

void vc_Exit()
{
	string message = ResolveString();
	err(message.c_str());
}

void vc_Message()
{
	string message = ResolveString();
	ResolveOperand();
	log(message.c_str());
}

int malloced_crap[1024];
int num_malloced_crap = 0;

bool IsInThere(int ofs)
{
	for (int i=0; i<num_malloced_crap; i++)
		if (malloced_crap[i] == ofs)
			return true;
	return false;
}

void RemoveMallocedThing(int ofs)
{
	for (int i=0; i<num_malloced_crap; i++)
	{
		if (malloced_crap[i] == ofs)
		{
			for (; i<num_malloced_crap-1; i++)
				malloced_crap[i] = malloced_crap[i+1];
			num_malloced_crap--;
			return;
		}
	}
	err("RemoveMallocedThing: internal error!");
}

void vc_Malloc()
{
	int size = ResolveOperand();
	if (!size) err("vc_Malloc: zero-sized block requested");
	vcreturn = (int) malloc(size);
	memset((void *) vcreturn, 0, size);
	malloced_crap[num_malloced_crap++] = vcreturn;
	if (num_malloced_crap == 1024)
		err("vc_Malloc: malloced_crap[] overflow");
	log("vc allocated %d bytes, ptr %d", size, vcreturn);	
}

void vc_Free()
{
	int ptr = ResolveOperand();
	bool kok = IsInThere(ptr);
	if (!kok && vc_paranoid)
		err("vc attempt to free invalid ptr %d", ptr);
	if (!kok) return;	
	log("vc freed ptr at %d", ptr);
	free((void*) ptr);
	RemoveMallocedThing(ptr);
}

void vc_pow()
{
	int a = ResolveOperand();
	int b = ResolveOperand();
	vcreturn = pow(a, b);
}

void vc_LoadImage()
{
	string fname = ResolveString();
	vcreturn = (int) xLoadImage8bpp(fname.c_str());
	malloced_crap[num_malloced_crap++] = vcreturn;
}

void vc_CopySprite()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int width = ResolveOperand();
	int height = ResolveOperand();
	int ptr = ResolveOperand();
	if (Lucent)
		CopySprite8_Lucent(x, y, width, height, (byte *) ptr);
	else
		CopySprite8(x, y, width, height, (byte *) ptr);
}

void vc_TCopySprite()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int width = ResolveOperand();
	int height = ResolveOperand();
	int ptr = ResolveOperand();
	if (Lucent)
		TCopySprite8_Lucent(x, y, width, height, (byte *) ptr);
	else
		TCopySprite8(x, y, width, height, (byte *) ptr);

}

void vc_EntitySpawn()
{
	int tilex = ResolveOperand();
	int tiley = ResolveOperand();
	string chr_filename = ResolveString();
	vcreturn = AllocateEntity(tilex*16, tiley*16, chr_filename.c_str());
}

void vc_SetPlayer()
{
	player = ResolveOperand();
	myself = entity[player];
}

void vc_Map()
{
	hookretrace = 0;
	hooktimer = 0;
	kill = 1;
	done = true;
	strcpy(mapname, ResolveString().c_str());
}

void vc_LoadFont()
{
	string fntname = ResolveString();
	vcreturn = LoadFont(fntname.c_str());
}

void vc_GotoXY()
{
	int x, y;
	x = ResolveOperand();
	y = ResolveOperand();
	Font_GotoXY(x, y);
}

void vc_PrintString()
{
	int font_slot = ResolveOperand();
	string text = ResolveString();
	
	PrintText(font_slot, text.c_str());
}

void vc_LoadRaw()
{
	string raw_filename = ResolveString();
	EnforceNoDirectories(raw_filename);
	VFILE *vf = vopen(raw_filename.c_str());
	if (!vf)
		err("vc_LoadRaw: could not open file %s", raw_filename.c_str());
	
	int n = filesize(vf);
	char *ptr = (char *) malloc(n);
	if (!ptr)
		err("vc_LoadRaw: couldn't allocate memory");
	vread(ptr, n, vf);
	vclose(vf);
	vcreturn = (int)ptr;
}

void vc_SetTile()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int lay = ResolveOperand();
	int value = ResolveOperand();

	if (x<0 || y<0)
		return;
	if (lay==6 || lay==7)
	{
		if (x>=current_map->mapwidth() || y>=current_map->mapheight())
			return;
	}
	else
	{
		if ((lay>=0 && lay<6) && (x>=current_map->layer[lay].sizex || y>=current_map->layer[lay].sizey))
			return;
	}

	switch (lay)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			current_map->layers[lay][y*current_map->layer[lay].sizex+x] = (short)value;
			break;
		case 6:
			current_map->Obstruct[y*current_map->layer[0].sizex+x] = (byte)value;
			break;
		case 7:
			current_map->Zone[y*current_map->layer[0].sizex+x] = (byte)value;
			break;
		default:
			err("vc_SetTile: invalid layer");
	}
}

void vc_ScaleSprite()
{
	int x		= ResolveOperand();
	int y		= ResolveOperand();
	int swidth	= ResolveOperand();
	int sheight = ResolveOperand();
	int dwidth	= ResolveOperand();
	int dheight = ResolveOperand();
	int ptr		= ResolveOperand();
	if (Lucent)
		ScaleSprite8_Lucent(x, y, swidth, sheight, dwidth, dheight, (byte *) ptr);
	else
		ScaleSprite8(x, y, swidth, sheight, dwidth, dheight, (byte *) ptr);
}

void vc_Unpress()
{
	int n = ResolveOperand();
	switch (n)
	{
		case 0: if (b1) UnB1(); if (b2) UnB2(); if (b3) UnB3(); if (b4) UnB4();	break;
		case 1: if (b1) UnB1(); break;
		case 2: if (b2) UnB2(); break;
		case 3: if (b3) UnB3(); break;
		case 4: if (b4) UnB4(); break;
		case 5: if (up) UnUp(); break;
		case 6: if (down) UnDown(); break;
		case 7: if (left) UnLeft(); break;
		case 8: if (right) UnRight(); break;
	}
}

void vc_EntityMove()
{
	int ent_num = ResolveOperand();
	string movestr = ResolveString();
	if (ent_num >= entities && vc_paranoid)
		err("vc_EntityMove: No such entity, pal. (%d)", ent_num);
	entity[ent_num]->set_movescript(movestr.c_str());

}

void vc_HLine()
{
	int x1 = ResolveOperand();
	int y1 = ResolveOperand();
	int x2 = ResolveOperand();
	int c = ResolveOperand();
	if (Lucent)
		HLine8_Lucent(x1, y1, x2, c);
	else
		HLine8(x1, y1, x2, c);
}

void vc_VLine()
{
	int x1 = ResolveOperand();
	int y1 = ResolveOperand();
	int y2 = ResolveOperand();
	int c = ResolveOperand();
	if (Lucent)
		VLine8_Lucent(x1, y1, y2, c);
	else
		VLine8(x1, y1, y2, c);
}

void vc_Line()
{

	int x1 = ResolveOperand();
	int y1 = ResolveOperand();
	int x2 = ResolveOperand();
	int y2 = ResolveOperand();
	int c  = ResolveOperand();
	Line8(x1, y1, x2, y2, c);
}

void vc_Circle()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int radius = ResolveOperand();
	int color = ResolveOperand();
	if (Lucent)
		Circle8_Lucent(x, y, radius, radius, color);
	else
		Circle8(x, y, radius, radius, color);
}

void vc_CircleFill()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int radius = ResolveOperand();
	int color = ResolveOperand();
	if (Lucent)
		Sphere8_Lucent(x, y, radius, radius, color);
	else
		Sphere8(x, y, radius, radius, color);
}

void vc_Rect()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int xe = ResolveOperand();
	int ye = ResolveOperand();
	int color = ResolveOperand();
	Box8(x, y, xe, ye, color);
}

void vc_RectFill()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int xe = ResolveOperand();
	int ye = ResolveOperand();
	int color = ResolveOperand();
	if (Lucent)
		Rect8_Lucent(x, y, xe, ye, color);
	else
		Rect8(x, y, xe, ye, color);
}

void vc_strlen()
{
	string text = ResolveString();
	vcreturn = text.length();
}

void vc_strcmp()
{
	string a = ResolveString();
	string b = ResolveString();

	if (a < b) vcreturn = -1;
	else if (a > b) vcreturn = +1;
	else vcreturn=0;
}




void vc_FontWidth()
{
	int slot = ResolveOperand();
	vcreturn = fnts[slot]->width;
}

void vc_FontHeight()
{
	int slot = ResolveOperand();
	vcreturn = fnts[slot]->height;
}

void vc_SetPixel()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int c = ResolveOperand();
	PutPixel8(x, y, c);
}

void vc_GetPixel()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	vcreturn = (int) ReadPixel8(x, y);
}

void vc_EntityOnScreen()
{
	int find = ResolveOperand();
	err("implement entityonscreen!!");
	vcreturn = 0;
}

void vc_GetTile()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int l = ResolveOperand();
	vcreturn = 0;

	if (x<0 || y<0)
		return;
	if (l<6 && l >= current_map->numlayers && vc_paranoid)
		err("vc_GetTile(): passing an nonexistant layer");
	if (l<6 && (l >= current_map->numlayers || x >= current_map->layer[l].sizex || y >= current_map->layer[l].sizey))
 		return;
	if (l==6 || l==7)
	{
		if (x>=current_map->mapwidth() || y>=current_map->mapheight())
			return;
	}	
	
	switch (l)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			vcreturn = (int) current_map->layers[l][(y*current_map->layer[l].sizex)+x];
			break;
		case 6:
			vcreturn = (int) current_map->Obstruct[(y*current_map->layer[0].sizex)+x];
			break;
		case 7:
			vcreturn = (int) current_map->Zone[(y*current_map->layer[0].sizex)+x];
			break;

		default:
			err("vc_GetTile: no such layer");
	}
}

void vc_HookRetrace()
{
	int script = 0;
	switch (GrabC())
	{
		case 1:
			script = ResolveOperand();
			break;
		case 2:
			script = USERFUNC_MARKER + GrabD();
			break;
	}		
	hookretrace = script;
}

void vc_HookTimer()
{
	int script = 0;
	switch (GrabC())
	{
		case 1:
			script = ResolveOperand();
			break;
		case 2:
			script = USERFUNC_MARKER + GrabD();
			break;
	}		
	hooktimer = script;
}


void vc_SetResolution()
{	
	v2_xres = ResolveOperand();
	v2_yres = ResolveOperand();

	if (eagle)
	{
		delete myscreen;
		vid_SetMode(v2_xres*2, v2_yres*2, vid_bpp, vid_window, MODE_SOFTWARE);
		myscreen = new image(v2_xres, v2_yres);
	}
	else
	{
		vid_SetMode(v2_xres, v2_yres, vid_bpp, vid_window, MODE_SOFTWARE);
		myscreen = screen;
	}
	Init8bpp();
}

void vc_SetRString()
{
	string rstring = ResolveString();
	strcpy(current_map->rstring, rstring.c_str());
}

void vc_SetClipRect()
{
	clip_x = ResolveOperand();
	clip_y = ResolveOperand();
	clip_xend = ResolveOperand();
	clip_yend = ResolveOperand();

	if (clip_x < 0)
		clip_x = 0;
	else if (clip_x >= screen_width)
		clip_x = screen_width - 1;
	if (clip_y < 0)
		clip_y = 0;
	else if (clip_y >= screen_height)
		clip_y = screen_height - 1;

	if (clip_xend < 0)
		clip_xend = 0;
	else if (clip_xend >= screen_width)
		clip_xend = screen_width - 1;
	if (clip_yend < 0)
		clip_yend = 0;
	else if (clip_yend >= screen_height)
		clip_yend = screen_height-1;	
}

void vc_SetRenderDest()
{
	screen_width = ResolveOperand();
	screen_height = ResolveOperand();
	clip_x = 0;
	clip_y = 0;
	clip_xend = screen_width - 1;
	clip_yend = screen_height - 1;
	screen8 = (byte *) ResolveOperand();
}

void vc_RestoreRenderSettings()
{
	screen_width = v2_xres;
	screen_height = v2_yres;
	clip_x = 0;
	clip_y = 0;
	clip_xend = screen_width - 1;
	clip_yend = screen_height - 1;
	screen8 = realscreen;
}

void vc_PartyMove()
{
	if (!myself) err("vc_PartyMove: there is no player set!");
	myself = 0;
	vcpush(cameratracking);
	vcpush(tracker);

	if (cameratracking == 1)
	{
		cameratracking = 2;
		tracker = player;
	}
	string movescript = ResolveString();
	entity[player]->set_movescript(movescript.c_str());
	
	timer = 0;
	while (entity[player]->movecode)
	{
		while (timer)
		{
			timer--;
			ProcessEntities();
		}
		Render();
		ShowPage8();
	}
	
	tracker = (byte) vcpop();
	cameratracking = (byte) vcpop();
	myself = entity[player];
	timer = 1;
}

void vc_WrapBlit()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int sw = ResolveOperand();
	int sh = ResolveOperand();
	byte *src = (byte *) ResolveOperand();
	if (Lucent) 
		WrapBlit8_Lucent(x, y, sw, sh, src);
	else
		WrapBlit8(x, y, sw, sh, src);
}

void vc_TWrapBlit()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int sw = ResolveOperand();
	int sh = ResolveOperand();
	byte *src = (byte *) ResolveOperand();
	if (Lucent) 
		TWrapBlit8_Lucent(x, y, sw, sh, src);
	else
		TWrapBlit8(x, y, sw, sh, src);
}

void AdjustMousePos(int *x, int *y)
{
	POINT p;
	RECT r;
	GetClientRect(hMainWnd,&r);
	p.x=(float)*x/(float)r.right;
	p.y=(float)*y/(float)r.bottom;
	ClientToScreen(hMainWnd,&p);
	*x=p.x;
	*y=p.y;
}

void vc_SetMousePos()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
//	AdjustMousePos(&x, &y);
//	SetCursorPos(x, y);
}

void vc_HookKey()
{
	int key = ResolveOperand();
	if (key < 0) key = 0;
	if (key > 127) key = 127;	

	int script = 0;
	switch (GrabC())
	{
		case 1:
			script = ResolveOperand();
			break;
		case 2:
			script = USERFUNC_MARKER + GrabD();
			break;
	}
	bindarray[key] = script;
}

void vc_PlayMusic()
{
	string fname = ResolveString();
	PlayMusic(fname.c_str(), false);
}

int morph_step(int S, int D, int mix, int light)
{
	return (mix*(S - D) + (100*D))/100*light/64;
}

void vc_PaletteMorph()
{	
	int	n, rgb[3];	

	rgb[0] = ResolveOperand() * 255 / 63;
	rgb[1] = ResolveOperand() * 255 / 63;
	rgb[2] = ResolveOperand() * 255 / 63;
    int percent = ResolveOperand();
	int intensity = ResolveOperand();

    percent = 100-percent;

	for (n = 0; n < 3; n += 1)
	{
		if (rgb[n] < 0)
			rgb[n] = 0;
		else if (rgb[n] > 255)
			rgb[n] = 255;
	}

	for (n = 0; n < 3*256; n += 1)
		pal8[n] = (byte) morph_step(base_pal8[n], rgb[n % 3], percent, intensity);

	for (n=0; n<256; n++)
		pal[n] = MakeColor(pal8[(n*3)], pal8[(n*3)+1], pal8[(n*3)+2]);
}

void vc_OpenFile()
{	
	string filename = ResolveString();
	EnforceNoDirectories(filename);

	VFILE *vf = vopen(filename.c_str());
	vcreturn = (quad) vf;
	log(" --> VC opened file %s, ptr 0x%08X", filename.c_str(), (quad) vf);
	malloced_crap[num_malloced_crap++] = (int) vf;
}

void vc_CloseFile()
{		
	VFILE *vf = (VFILE *) ResolveOperand();
	if (!IsInThere((int) vf))
	{
		if (vc_paranoid) err("vc closed file that was not open: %d", (int) vf);
		return;
	}
	vclose(vf);
	RemoveMallocedThing((int) vf);
	log(" --> VC closed file ptr 0x%08X", (quad) vf);
}

void vc_QuickRead()
{
	int offset;
	char temp[256]={0};

	string filename = ResolveString();
	EnforceNoDirectories(filename);

	// which string are we reading into?
	char code = GrabC();
	switch (code)
	{
		case op_STRING:
			offset = GrabW();
			break;
		case op_SARRAY:
			offset = GrabW();
			offset += ResolveOperand();
			break;
		default:
			err("vc_QuickRead: bad string operator!!!");
	}

	// which line are we reading from the file?
	int seekline = ResolveOperand();
	if (seekline < 1) seekline = 1;

	VFILE* f = vopen(filename.c_str());
	if (!f)
		err("vc_QuickRead: could not open %s", filename.c_str());
	
	// seek to the line of interest
	for (int n=0; n<seekline; n++)
		vgets(temp, 255, f);

	// suppress trailing CR/LF
	char *p = temp;
	while (*p)
	{
		if ('\n' == *p || '\r' == *p)
			*p = '\0';
		p++;
	}

	// assign to vc string
	if (offset>=0 && offset<stralloc)
		vc_strings[offset]=temp;

	vclose(f);
}

void vc_AddFollower()
{
	int which = ResolveOperand();
	if (which >= entities && vc_paranoid)
		err("AddFollower: no such entity (%d)", which);
	if (which >= entities) return;

	if (!myself)
	{
		player = which;
		myself = entity[player];
		return;
	}

	Entity *cruise = myself;
	while (cruise->getfollower())
		cruise = cruise->getfollower();
	cruise->stalk(entity[which]);
}



void vc_CacheSound()
{
	string sname = ResolveString();
	if (vcsamples == 99) err("vc_CacheSound: out of sample handles.");
	samples[vcsamples] = CacheSample(VCsounds, sname.c_str());
	vcreturn = vcsamples++;
}

void FreeAllSounds()
{
	DeleteSampleCache(VCsounds);
	VCsounds = NewSampleCache();
	vcsamples = 0;
}

void vc_PlaySound()
{
	int slot = ResolveOperand();
	int volume = ResolveOperand();
	int pan = ResolveOperand();
//FIXME: use volume and pan
	if (slot>=vcsamples) err("vc_PlaySound: sample slot %d not allocated.", slot);
	PlayMenuSample(samples[slot]);
}

void vc_RotScale()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int width = ResolveOperand();
	int length = ResolveOperand();
	int angle = ResolveOperand();
	int scale = ResolveOperand();
	int ptr = ResolveOperand();
	if (Lucent)
		RotScale8_Lucent(x, y, width, length, angle*3.14159/180.0, scale/1000.0, (byte *)ptr);
	else
		RotScale8(x, y, width, length, angle*3.14159/180.0, scale/1000.0, (byte *)ptr);
}

void vc_val()
{
	string s = ResolveString();
	vcreturn = atoi(s.c_str());
}

void vc_TScaleSprite()
{
	int x		= ResolveOperand();
	int y		= ResolveOperand();
	int swidth	= ResolveOperand();
	int sheight = ResolveOperand();
	int dwidth	= ResolveOperand();
	int dheight = ResolveOperand();
	int ptr		= ResolveOperand();	
	if (Lucent)
		TScaleSprite8_Lucent(x, y, swidth, sheight, dwidth, dheight, (byte *) ptr);
	else
		TScaleSprite8(x, y, swidth, sheight, dwidth, dheight, (byte *) ptr);
}

void vc_GrabRegion()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int xe = ResolveOperand();
	int ye = ResolveOperand();
	byte *ptr = (byte *) ResolveOperand();

	// ensure arguments stay valid
	if (x<0)
		x=0;
	else if (x>=myscreen->width)
		x=myscreen->width-1;
	if (y<0)
		y=0;
	else if (y>=myscreen->height)
		y=myscreen->height-1;

	if (xe<0)
		xe=0;
	else if (xe>=myscreen->width)
		xe=myscreen->width-1;
	if (ye<0)
		ye=0;
	else if (ye>=myscreen->height)
		ye=myscreen->height-1;

	if (xe<x) SWAP(x,xe); 
	if (ye<y) SWAP(y,ye);

	xe = xe - x + 1;
	ye = ye - y + 1;

	byte* source = screen8 + (y * v2_xres) + x;
	while (ye)
	{
		memcpy(ptr, source, xe);
		ptr += xe;
		source += v2_xres;
		ye -= 1;
	}
}

void vc_Log()
{
	string s = ResolveString();
	log(s.c_str());
}

void vc_fseekline()
{
    int line = ResolveOperand()-1;
	VFILE *vf = (VFILE *) ResolveOperand();
	vseek(vf, 0, SEEK_SET);

	// 0 & 1 will yield first line
	char temp[256+1];
	do
		vgets(temp, 256, vf);
	while (--line > 0);
}

void vc_fseekpos()
{
	int pos = ResolveOperand();
	VFILE *vf = (VFILE *) ResolveOperand();
	vseek(vf, pos, 0);
}

void vc_fread()
{
	char *buffer = (char *) ResolveOperand();
	int len = ResolveOperand();
	VFILE *vf = (VFILE *) ResolveOperand();
	vread(buffer, len, vf);
}

void vc_fgetbyte()
{
	byte b;
	VFILE *vf = (VFILE *) ResolveOperand();
	vread(&b, 1, vf);
	vcreturn = b;
}

void vc_fgetword()
{	
	word w;
	VFILE *vf = (VFILE *) ResolveOperand();
	vread(&w, 2, vf);
	vcreturn = w;
}

void vc_fgetquad()
{
	quad q;
	VFILE *vf = (VFILE *) ResolveOperand();
	vread(&q, 4, vf);
	vcreturn = q;
}

void vc_fgetline()
{
	char temp[256+1];
	
	int code = GrabC();
	int offset = GrabW();
	if (op_SARRAY == code)
		offset+=ResolveOperand();

	VFILE *vf = (VFILE *) ResolveOperand();

	vgets(temp, 256, vf);
	temp[256]='\0';

	char *p = temp;
	while (*p)
	{
		if ('\n' == *p || '\r' == *p)
			*p = '\0';
		p++;
	}

	if (offset>=0 && offset<stralloc)
		vc_strings[offset]=temp;
}

void vc_fgettoken()
{
	char temp[256];

	int code = GrabC();
	int offset = GrabW();
    bool islocal = false;

	if (code == op_SARRAY)
		offset += ResolveOperand();
    else if (code == op_SLOCAL)
        islocal = true;

	VFILE *vf = (VFILE *) ResolveOperand();
	vscanf(vf, "%s", temp);

	if (offset>=0 && offset<stralloc)
	{
	    if (!islocal)
		    vc_strings[offset] = temp;
        else
            str_stack[str_stack_base+offset] = temp; // this doesn't work for some reason. :P
	}
}

void vc_fwritestring()
{
	string temp = ResolveString();
	FILE *f = (FILE *) ResolveOperand();
	fprintf(f, "%s\n", temp.c_str());
}

void vc_fwrite()
{
	char *buffer = (char *) ResolveOperand();
	int length = ResolveOperand();
	FILE *f = (FILE *) ResolveOperand();
	fwrite(buffer, 1, length, f);
}

void vc_frename()
{
	string a = ResolveString();
	string b = ResolveString();
	rename(a.c_str(), b.c_str());
	log("VC renamed %s to %s.", a.c_str(), b.c_str());
}

void vc_fdelete()
{
	string filename = ResolveString();
	EnforceNoDirectories(filename);
	remove(filename.c_str());
	log("VC deleted %s.", filename.c_str());
}

void vc_fwopen()
{
	FILE *f;
	string fname = ResolveString();
	EnforceNoDirectories(fname);
	f = fopen(fname.c_str(), "wb");
	if (!f)
		err("vc_fwopen: Unable to open %s for writing", fname.c_str());
	log("vc opened %s for writing, handle %d", fname.c_str(), (int) f);
	vcreturn = (int) f;
}

void vc_fwclose()
{	
	FILE *f = (FILE *) ResolveOperand();
	fclose(f);
	log("vc closed file (w) at handle %d", (int) f);
}

void vc_memcpy()
{
	char *dest = (char *) ResolveOperand();
	char *source = (char *) ResolveOperand();
	int length = ResolveOperand();
	memcpy(dest, source, length);
}

void vc_memset()
{
	char *dest = (char *) ResolveOperand();
	int value = ResolveOperand();
	int length = ResolveOperand();
	memset(dest, value, length);
}

void vc_Silhouette()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int width = ResolveOperand();
	int height = ResolveOperand();
	int color = ResolveOperand();
	int ptr = ResolveOperand();	
	if (Lucent)
		Silhouette8_Lucent(x, y, width, height, color, (byte *) ptr);
	else
		Silhouette8(x, y, width, height, color, (byte *) ptr);
}	

void vc_WriteVars(FILE *f)
{	
	fwrite(globalint, 1, 4*maxint, f);
	for (int n=0; n<stralloc; n++)
	{
		int z = vc_strings[n].length();
		fwrite(&z, 1, 4, f);
		fwrite(vc_strings[n].c_str(), 1, z, f);
		fputc(0, f);
	}	
}

void vc_ReadVars(FILE *f)
{
	char *temp = 0;
	fread(globalint, 1, 4*maxint, f);

	for (int n=0; n<stralloc; n++)
	{
		int z;
		fread(&z, 1, 4, f);
        temp = new char[z+1];
		if (!temp)
			err("vc_ReadVars: memory exhausted on %u bytes.", z);
		fread(temp, 1, z, f);
		temp[z]='\0';
		vc_strings[n].assign(temp);
        delete[] temp;
		temp = 0;
	}
}

void vc_Asc()
{
	vcreturn = ResolveString()[0];
}

void vc_Filesize()
{
	string filename = ResolveString();
	VFILE *vf = vopen(filename.c_str());
	vcreturn = filesize(vf);
	vclose(vf);
}

void vc_FTell()
{
	VFILE *vf = (VFILE *) ResolveOperand();
	vcreturn = vtell(vf);
}

void vc_ChangeCHR()
{
	int who = ResolveOperand();
	string chrname = ResolveString();

	if (who >= entities && vc_paranoid)
		err("vc_ChangeCHR(): no such entity index (%d)", who);
	if (who >= entities) return;

	if (entity[who]->chr)
		delete entity[who]->chr;
	CHR *t = new CHR(chrname.c_str());
	entity[who]->setchr(t);
}


void vc_fwritebyte()
{
	byte b = ResolveOperand();
	FILE *f = (FILE *) ResolveOperand();
	fwrite(&b, 1, 1, f);
}

void vc_fwriteword()
{
	word b = ResolveOperand();
	FILE *f = (FILE *) ResolveOperand();
	fwrite(&b, 1, 2, f);
}

void vc_fwritequad()
{
	quad b = ResolveOperand();
	FILE *f = (FILE *) ResolveOperand();
	fwrite(&b, 1, 4, f);
}

// ===================== End VC Standard Function Library =====================

void HandleStdLib()
{
	int x=0;
	byte c=0;

	c = GrabC();
	switch (c)
	{
		case 1: vc_Exit(); break;
		case 2: vc_Message(); break;
		case 3: vc_Malloc(); break;
		case 4: vc_Free(); break;
		case 5: vc_pow(); break;
		case 6: vc_LoadImage(); break;
		case 7: vc_CopySprite(); break;	
		case 8: vc_TCopySprite(); break;
		case 9: Render(); break;
		case 10: ShowPage8(); break;
		case 11: vc_EntitySpawn(); break;
		case 12: vc_SetPlayer(); break;
		case 13: vc_Map(); break;
		case 14: vc_LoadFont(); break;
		case 15: err("FLI disabled."); break;
		case 16: vc_GotoXY(); break;
		case 17: vc_PrintString(); break;
		case 18: vc_LoadRaw(); break;
		case 19: vc_SetTile(); break;
		case 20: ResolveOperand(); break; // AllowConsole()
		case 21: vc_ScaleSprite(); break;
		case 22: ProcessEntities();	break;
		case 23: UpdateControls(); break;
		case 24: vc_Unpress(); break;
		case 25: vc_EntityMove(); break;
		case 26: vc_HLine(); break;
		case 27: vc_VLine(); break;
		case 28: vc_Line(); break;
		case 29: vc_Circle(); break;
		case 30: vc_CircleFill(); break;
		case 31: vc_Rect();	break;
		case 32: vc_RectFill();	break;
		case 33: vc_strlen(); break;
		case 34: vc_strcmp(); break;
		case 35: break;	// CD_Stop()
		case 36: ResolveOperand(); break; // CD_Play()
		case 37: vc_FontWidth(); break;
		case 38: vc_FontHeight(); break;
		case 39: vc_SetPixel();	break;
		case 40: vc_GetPixel();	break;
		case 41: vc_EntityOnScreen(); break;
		case 42: 
			vcreturn = 0;
			x = ResolveOperand();
			if (x) vcreturn = rand() % x;
			break;
		case 43: vc_GetTile(); break;
		case 44: vc_HookRetrace(); break;
		case 45: vc_HookTimer(); break;
		case 46: vc_SetResolution(); break;
		case 47: vc_SetRString(); break;
		case 48: vc_SetClipRect(); break;
		case 49: vc_SetRenderDest(); break;
		case 50: vc_RestoreRenderSettings(); break;
		case 51: vc_PartyMove(); break;
		case 52:
			{
				int n = ResolveOperand();
                while (n < 0) n += 360;
                while (n > 360) n -= 360;
				vcreturn = sintbl[n];
			}
			break;
		case 53: 
			{
				int n = ResolveOperand();
                while (n < 0) n += 360;
                while (n > 360) n -= 360;
				vcreturn = costbl[n];
			}
			break;
		case 54:
			{
				int n = ResolveOperand();
				while (n < 0) n += 360;
                while (n > 360) n -= 360;
				vcreturn = tantbl[n];
			}
			break;
	    case 55: UpdateControls(); break; // CheckCorruption?
		case 56: ResolveOperand(); break; // ClipOn 
		case 57: 
			Lucent = ResolveOperand();
	        if (Lucent) SetLucent(50);
			    else SetLucent(0);
			break;
		case 58: vc_WrapBlit(); break;
		case 59: vc_TWrapBlit(); break;
		case 60: vc_SetMousePos(); break;
		case 61: vc_HookKey(); break;
		case 62: vc_PlayMusic(); break;
		case 63: StopMusic(); break;
		case 64: vc_PaletteMorph(); break;
		case 65: vc_OpenFile(); break;
		case 66: vc_CloseFile(); break;
		case 67: vc_QuickRead(); break;
//		case 68: vc_AddFollower(); break;
//	    case 69: vc_KillFollower(); break;
//	    case 70: vc_KillAllFollowers(); break;
//	    case 71: ResetFollowers();
		case 72: err("vc_FlatPoly disabled!");
		case 73: err("vc_TMapPoly disabled!");
		case 74: vc_CacheSound(); break;
		case 75: FreeAllSounds(); break;
		case 76: vc_PlaySound(); break;
		case 77: vc_RotScale(); break;
/*		case 78: vc_MapLine(); break;
		case 79: vc_TMapLine(); break;*/
		case 80: vc_val(); break;
		case 81: vc_TScaleSprite();	break;
		case 82: vc_GrabRegion(); break;
		case 83: vc_Log(); break;
		case 84: vc_fseekline(); break;
		case 85: vc_fseekpos();	break;
		case 86: vc_fread(); break;
		case 87: vc_fgetbyte(); break;
		case 88: vc_fgetword(); break;
		case 89: vc_fgetquad(); break;
		case 90: vc_fgetline(); break;
		case 91: vc_fgettoken(); break;
		case 92: vc_fwritestring(); break;
		case 93: vc_fwrite(); break;
		case 94: vc_frename(); break;
		case 95: vc_fdelete(); break;
		case 96: vc_fwopen(); break;
		case 97: vc_fwclose(); break;
		case 98: vc_memcpy(); break;
		case 99: vc_memset(); break;
		case 100: vc_Silhouette(); break;
		case 101: vcreturn = 0;	break;	
		case 102: err("Mosaic() disabled!"); break;
		case 103: vc_WriteVars((FILE *)ResolveOperand()); break;
		case 104: vc_ReadVars((FILE *)ResolveOperand()); break;
		case 105: ExecuteEvent(ResolveOperand()); break;
		case 106: vc_Asc();	break;
		case 107: ExecuteUserFunc(ResolveOperand()); break;
		case 108: vcreturn=GrabD(); break; // NumForScript
		case 109: vc_Filesize(); break;
		case 110: vc_FTell(); break;
		case 111: vc_ChangeCHR(); break;
		case 112: err("RGB() disabled!"); break;
		case 113: err("GetR() disabled!"); break;
		case 114: err("GetG() disabled!"); break;
		case 115: err("GetB() disabled!"); break;
		case 116: err("Mask() disabled!"); break;
		case 117: err("ChangeAll() disabled!"); break;
        case 118: vcreturn = sqrt(ResolveOperand()); break;
        case 119: vc_fwritebyte(); break;
        case 120: vc_fwriteword(); break;
        case 121: vc_fwritequad(); break;

		default:
			err("VC Execution error: Invalid STDLIB index. (%d)", (int)c);
	}
}