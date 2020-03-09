/******************************************************************
 * winv2: g_startup.cpp                                           *
 * copyright (c) 2001 vecna                                       *
 ******************************************************************/


#include "xerxes.h"


/****************************** data ******************************/


int v2_xres, v2_yres;
bool hicolor;
bool eagle;
bool windowmode;
bool sound;
image *myscreen;
char mapname[80];


/****************************** code ******************************/


void InitDefaults()
{
	v2_xres = 320;
	v2_yres = 200;
	hicolor = false;
	eagle = true;
	windowmode = true;
	sound = true;
}


void LoadConfig()
{
	cfg_Init("user.cfg");
cfg_SetDefaultKeyValue("startmap", "zbegin.map");

	if (cfg_KeyPresent("xres"))
		v2_xres = atoi(cfg_GetKeyValue("xres"));
	if (cfg_KeyPresent("yres"))
		v2_yres = atoi(cfg_GetKeyValue("yres"));
	if (cfg_KeyPresent("hicolor"))
		hicolor = true;
	if (cfg_KeyPresent("eagle"))
		eagle = true;
	if (cfg_KeyPresent("windowmode"))
		windowmode = true;
	if (cfg_KeyPresent("nosound"))
		sound = false;
	if (cfg_KeyPresent("startmap"))
		strcpy(mapname, cfg_GetKeyValue("startmap"));
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


void xmain(int argc, char *argv[])
{
	InitDefaults();
	LoadConfig();
	InitVideo();
//	if (sound) InitSound();
			
	LoadSystemVC();
	RunVCAutoexec();
	LoadFont("system.fnt");

	while (true)
		Engine_Start(mapname);
}
