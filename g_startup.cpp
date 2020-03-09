/******************************************************************
 * winv2: g_startup.cpp                                           *
 * copyright (c) 2001 vecna                                       *
 ******************************************************************/

#include "xerxes.h"
#include "vergepal.h"

/******************
  TODO:
	* maybe fix switch() ??
*******************/


/****************************** data ******************************/

int v2_xres, v2_yres;
bool eagle;
bool windowmode;
bool sound;
bool cheats;
image *myscreen;
char mapname[80];

/****************************** code ******************************/

void InitDefaults()
{
	v2_xres = 320;
	v2_yres = 200;	
	eagle = false;
	windowmode = false;
	sound = true;	
	cheats = false;
}

void LoadConfig()
{
	cfg_Init("user.cfg");
	cfg_SetDefaultKeyValue("startmap", "test.map");

	if (cfg_KeyPresent("xres"))
		v2_xres = atoi(cfg_GetKeyValue("xres"));
	if (cfg_KeyPresent("yres"))
		v2_yres = atoi(cfg_GetKeyValue("yres"));
	if (cfg_KeyPresent("eagle"))
		eagle = atoi(cfg_GetKeyValue("eagle")) ? true : false;
	if (cfg_KeyPresent("windowmode"))
		windowmode = atoi(cfg_GetKeyValue("windowmode")) ? true : false;
	if (cfg_KeyPresent("nosound"))
		sound = atoi(cfg_GetKeyValue("nosound")) ? false : true;
	if (cfg_KeyPresent("startmap"))
		strcpy(mapname, cfg_GetKeyValue("startmap"));
	if (cfg_KeyPresent("paranoid"))
		vc_paranoid = atoi(cfg_GetKeyValue("paranoid"));
	if (cfg_KeyPresent("arraycheck"))
		vc_arraycheck = atoi(cfg_GetKeyValue("arraycheck"));
	if (cfg_KeyPresent("appname"))
		SetWindowText(hMainWnd, cfg_GetKeyValue("appname"));
	if (cfg_KeyPresent("vecnascockishuge"))
		cheats = true;

	if (cfg_KeyPresent("mount1"))
		MountVFile(cfg_GetKeyValue("mount1"));
	if (cfg_KeyPresent("mount2"))
		MountVFile(cfg_GetKeyValue("mount2"));
	if (cfg_KeyPresent("mount3"))
		MountVFile(cfg_GetKeyValue("mount3"));
}

void InitVideo()
{
	if (!eagle && !windowmode)
	{
		int result = vid_SetMode(v2_xres, v2_yres, 16, 0, MODE_SOFTWARE);
		if (!result)
			result = vid_SetMode(v2_xres, v2_yres, DesktopBPP, 1, MODE_SOFTWARE);
		if (!result)
			err("Could not set video mode!");
		myscreen = screen;
		return;
	}
	if (!eagle && windowmode)
	{
		int result = vid_SetMode(v2_xres, v2_yres, DesktopBPP, 1, MODE_SOFTWARE);
		if (!result)
			err("Could not set video mode!");
		myscreen = screen;
		return;
	}
	if (eagle && !windowmode)
	{
		int result = vid_SetMode(v2_xres * 2, v2_yres * 2, 16, 0, MODE_SOFTWARE);
		if (!result)
			result = vid_SetMode(v2_xres * 2, v2_yres * 2, DesktopBPP, 1, MODE_SOFTWARE);
		if (!result)
			err("Could not set video mode!");
		myscreen = new image(v2_xres, v2_yres);
		return;
	}
	if (eagle && windowmode)
	{
		int result = vid_SetMode(v2_xres * 2, v2_yres * 2, DesktopBPP, 1, MODE_SOFTWARE);
		if (!result)
			err("Could not set video mode!");
		myscreen = new image(v2_xres, v2_yres);
		return;
	}
}

void ShowPage()
{
	if (eagle)
		run2xSAI(myscreen, screen);
	Flip();
}

void InitVergePalette()
{
	for (int i=0; i<768; i++)
		base_pal8[i] = vergepal[i] * 255 / 63;
	memcpy(pal8, base_pal8, sizeof pal8);
	
	for (i=0; i<256; i++)
		pal[i] = MakeColor(pal8[i * 3], pal8[(i * 3) + 1], pal8[(i * 3) + 2]);
	memcpy(base_pal, pal, sizeof base_pal);
}

void InitTransTbl()
{	
	VFILE *f = vopen("trans.tbl");
	if (!f)
	{
		for (int y=0; y<256; y++)
			for (int x=0; x<256; x++)
				transtbl[y][x] = x;
		return;
	}
	vread(transtbl, 65535, f);
	vclose(f);	
}

void xmain(int argc, char *argv[])
{
	InitDefaults();
	LoadConfig();
	InitVideo();
	Init8bpp();
	InitVergePalette();
	InitTransTbl();
	if (sound) InitSound();
			
	LoadFont("system.fnt");
	LoadSystemVC();
	RunVCAutoexec();

	while (true)
		Engine_Start(mapname);
}