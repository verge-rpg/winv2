#include <math.h>
#include "sincos.h"

int Lucent;

void EnforceNoDirectories(string s)
{
	int	n = 0;        

    if (s[0]=='/' || s[0]=='\\')
		err("OpenFile does not allow accessing dir: %s", s.c_str());

    if (s[1]==':')
		err("OpenFile does not allow accessing dir: %s", s.c_str());

    n=0;
    while (n<s.length()-1)
    {
		if (s[n]=='.' && s[n+1]=='.')
			err("OpenFile does not allow accessing dir: %s", s.c_str());
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
	log(message.c_str());
}

void vc_Malloc()
{
	int size = ResolveOperand();
	if (!size) err("vc_Malloc: zero-sized block requested");
	vcreturn = (int) malloc(size);
	log("vc allocated %d bytes, ptr %d", size, vcreturn);	
}

void vc_Free()
{
	int ptr = ResolveOperand();
	log("vc freed ptr at %d", ptr);	
	//free((void*) ptr);	
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
	// FIXME: add some kind of image tracking?
}

void vc_CopySprite()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int width = ResolveOperand();
	int length = ResolveOperand();
	int ptr = ResolveOperand();
	Blit8(x, y, (char *) ptr, width, length, pal, myscreen);
}

void vc_TCopySprite()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int width = ResolveOperand();
	int length = ResolveOperand();
	int ptr = ResolveOperand();
	TBlit8(x, y, (char *) ptr, width, length, pal, myscreen);
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

void vc_Unpress()
{
	int n = ResolveOperand();
	switch (n)
	{
		case 0: UnB1(); UnB2(); UnB3(); UnB4();	break;
		case 1: UnB1(); break;
		case 2: UnB2(); break;
		case 3: UnB3(); break;
		case 4: UnB4(); break;
		case 5: UnUp(); break;
		case 6: UnDown(); break;
		case 7: UnLeft(); break;
		case 8: UnRight(); break;
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
	ScaleBlit8(x, y, dwidth, dheight, (byte *) ptr, swidth, sheight, pal, myscreen);
}


void vc_HLine()
{
	int x1 = ResolveOperand();
	int y1 = ResolveOperand();
	int x2 = ResolveOperand();
	int c = ResolveOperand();
	HLine(x1, y1, x2, pal[c], myscreen);
}

void vc_VLine()
{
	int x1 = ResolveOperand();
	int y1 = ResolveOperand();
	int y2 = ResolveOperand();
	int c = ResolveOperand();
	VLine(x1, y1, y2, pal[c], myscreen);
}

void vc_Line()
{

	int x1 = ResolveOperand();
	int y1 = ResolveOperand();
	int x2 = ResolveOperand();
	int y2 = ResolveOperand();
	int c  = ResolveOperand();
	Line(x1, y1, x2, y2, pal[c], myscreen);
}

void vc_Circle()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int radius = ResolveOperand();
	int color = ResolveOperand();
	Circle(x, y, radius, radius, pal[color], myscreen);
}

void vc_CircleFill()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int radius = ResolveOperand();
	int color = ResolveOperand();
	Sphere(x, y, radius, radius, pal[color], myscreen);
}

void vc_Rect()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int xe = ResolveOperand();
	int ye = ResolveOperand();
	int color = ResolveOperand();
	Box(x, y, xe, ye, pal[color], myscreen);
}

void vc_RectFill()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int xe = ResolveOperand();
	int ye = ResolveOperand();
	int color = ResolveOperand();
	Rect(x, y, xe, ye, pal[color], myscreen);
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
	PutPixel(x, y, pal[c], myscreen);
}





void vc_GetTile()
{
	int x = ResolveOperand();
	int y = ResolveOperand();
	int l = ResolveOperand();
	vcreturn = 0;

	if (x<0 || y<0)
		return;
	if (l<6 && (x >= current_map->layer[l].sizex || y >= current_map->layer[l].sizey))
 		return;
	if (l==6 || l==7)
	{
		if (x>=current_map->mapwidth() || y>=current_map->mapheight())
			return;
	}
	// FIXME:: fixme up more!!
	
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




void vc_SetRString()
{
	string rstring = ResolveString();
	strcpy(current_map->rstring, rstring.c_str());
}

void vc_SetClipRect()
{	
	int x = ResolveOperand();
	int y = ResolveOperand();
	int xend = ResolveOperand();
	int yend = ResolveOperand();

	if (x>xend) SWAP(x,xend);
	if (y>yend) SWAP(y,yend);

	if (x < 0) x = 0;
	else if (x >= myscreen->width) x = myscreen->width - 1;
	if (y < 0) y = 0;
	else if (y >= myscreen->height) y = myscreen->height - 1;

	if (xend<0) xend=0;
	else if (xend>=myscreen->width) xend=myscreen->width - 1;
	if (yend<0) yend=0;
	else if (yend>=myscreen->height)
		yend=myscreen->height - 1;

	myscreen->SetClip(x, y, xend, yend);
}

void vc_SetRenderDest()
{
}

void vc_RestoreRenderSettings()
{
	myscreen->SetClip(0, 0, v2_xres-1, v2_yres-1);
}

void vc_PartyMove()
{
	//myself=0;

//	vcpush(cameratracking);
//	vcpush(tracker);

/*	if (1==cameratracking)
	{
		cameratracking=2;
		tracker=playernum;
	}*/
err("");
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
		ShowPage();
	}

//	tracker=(byte)vcpop();
//	cameratracking=(byte)vcpop();

//	player=&entity[playernum];
	timer = 0;
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



void vc_PaletteMorph()
{	
	image *hole = new image(1,1);
	int r = ResolveOperand();
	int g = ResolveOperand();
	int b = ResolveOperand();
    int percent = ResolveOperand();
	int intensity = ResolveOperand();

    if (hicolor)
		return;

    percent = 100-percent;
	intensity = intensity * 100 / 64;
	for (int i=0; i<256; i++)
	{
		PutPixel(0, 0, base_pal[i], hole);
		SetLucent(percent);
		PutPixel(0, 0, MakeColor(r, g, b), hole);
		SetLucent(intensity);
		PutPixel(0, 0, 0, hole);
		pal[i] = ReadPixel(0, 0, hole);
		SetLucent(0);
	}
}

void vc_OpenFile()
{	
	string filename = ResolveString();
	EnforceNoDirectories(filename);

	VFILE *vf = vopen(filename.c_str());
	vcreturn = (quad) vf;

	log(" --> VC opened file %s, ptr 0x%08X", filename.c_str(), (quad) vf);
}

void vc_CloseFile()
{		
	VFILE *vf = (VFILE *) ResolveOperand();
	vclose(vf);
	log(" --> VC closed file ptr 0x%08X", (quad) vf);
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


void vc_val()
{
	string s = ResolveString();
	vcreturn = atoi(s.c_str());
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

// swap?
	if (xe<x) SWAP(x,xe); 
	if (ye<y) SWAP(y,ye);

	xe = xe - x + 1;
	ye = ye - y + 1;
		
	while (ye)
	{
		memset(ptr, 0, xe);
		ptr += xe;		
		ye -= 1;
	}
}

void vc_Log()
{
	string s = ResolveString();
	log(s.c_str());
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




void vc_Asc()
{
	vcreturn = ResolveString()[0];
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
		case 9: if (current_map) current_map->Render(xwin, ywin, myscreen); break;
		case 10: ShowPage(); break;
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
//		case 25: vc_EntityMove(); break;
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
//		case 40: vc_GetPixel();	break;
//		case 41: vc_EntityOnScreen(); break;
		case 42: 
			vcreturn = 0;
			x = ResolveOperand();
			if (x) vcreturn = rand() % x;
			break;
		case 43: vc_GetTile(); break;
		case 44: vc_HookRetrace(); break;
		/*case 45: vc_HookTimer(); break;
		case 46: vc_SetResolution(); break;*/
		case 47: vc_SetRString(); break;
		case 48: vc_SetClipRect(); break;
	//	case 49: vc_SetRenderDest(); break;
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
	    case 55: UpdateControls(); break;
		case 56: ResolveOperand(); break; // ClipOn 
		case 57: 
			Lucent = ResolveOperand();
			if (!hicolor)
			{
				 if (Lucent) SetLucent(50);
				 else SetLucent(0);
			}
			else SetLucent(Lucent);
			break;
/*		case 58: vc_WrapBlit(); break;
		case 59: vc_TWrapBlit(); break;*/
		case 60: vc_SetMousePos(); break;
		case 61: vc_HookKey(); break;
		case 62: vc_PlayMusic(); break;
		case 63: StopMusic(); break;
		case 64: vc_PaletteMorph(); break;
		case 65: vc_OpenFile(); break;
		case 66: vc_CloseFile(); break;
/*		case 67: vc_QuickRead(); break;
		case 68: vc_AddFollower(); break;
	//case 69: vc_KillFollower(); break;
	//case 70: vc_KillAllFollowers(); break;
	//case 71: ResetFollowers(); */
		case 72: err("vc_FlatPoly disabled!"); // vc_FlatPoly(); break;
		case 73: err("vc_TMapPoly disables!"); // vc_TMapPoly(); break;
		case 74: vc_CacheSound(); break;
			case 75: FreeAllSounds(); break;
		case 76: vc_PlaySound(); break;
/*		case 77: vc_RotScale(); break;
		case 78: vc_MapLine(); break;
		case 79: vc_TMapLine(); break;*/
		case 80: vc_val(); break;
//		case 81: vc_TScaleSprite();	break;
		case 82: vc_GrabRegion(); break;
		case 83: vc_Log(); break;
		/*case 84: vc_fseekline(); break;
		case 85: vc_fseekpos();	break;
		case 86: vc_fread(); break;*/
		case 87: vc_fgetbyte(); break;
		case 88: vc_fgetword(); break;
		case 89: vc_fgetquad(); break;
		case 90: vc_fgetline(); break;
		case 91: vc_fgettoken(); break;
		case 92: vc_fwritestring(); break;
		case 93: vc_fwrite(); break;
//		case 94: vc_frename(); break;
//		case 95: vc_fdelete(); break;
		case 96: vc_fwopen(); break;
		case 97: vc_fwclose(); break;
/*		case 98: vc_memcpy(); break;
		case 99: vc_memset(); break;
		case 100: vc_Silhouette(); break;
		case 101: vcreturn=(int) InitMosaicTable();	break;	
		case 102: vc_Mosaic(); break;
		case 103: vc_WriteVars(); break;
		case 104: vc_ReadVars(); break;*/
		case 105: ExecuteEvent(ResolveOperand()); break;
		case 106: vc_Asc();	break;
		case 107: ExecuteUserFunc(ResolveOperand()); break;
/*		case 108: vc_NumForScript(); break;
		case 109: vc_Filesize(); break;
		case 110: vc_FTell(); break;
		case 111: vc_ChangeCHR(); break;
		case 112: vc_RGB(); break;
		case 113: vc_GetR(); break;
		case 114: vc_GetG(); break;
		case 115: vc_GetB(); break;
		case 116: vc_Mask(); break;
		case 117: vc_ChangeAll(); break;*/
        case 118: vcreturn = sqrt(ResolveOperand()); break;
        case 119: vc_fwritebyte(); break;
        case 120: vc_fwriteword(); break;
        case 121: vc_fwritequad(); break;
//        case 122: CalcLucent(ResolveOperand()); break;

		default:
			err("VC Execution error: Invalid STDLIB index. (%d)", (int)c);
	}
}