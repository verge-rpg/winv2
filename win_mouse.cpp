/****************************************************************
	xerxes engine
	win_mouse.cpp
 ****************************************************************/

#include "xerxes.h"

/***************************** data *****************************/

int mouse_x, mouse_y, mouse_l, mouse_r, mouse_m;
float mwheel;
bool WinCursor = true;

/***************************** code *****************************/

void mouse_Init()
{
	mouse_l = 0;
	mouse_r = 0;
	mouse_m = 0;
	mwheel = 0.0;
}


void mouse_Update()
{
	RECT dr;
	POINT pt;

	if (vid_window)
	{
		GetClientRect(hMainWnd, &dr);
		ClientToScreen(hMainWnd, (LPPOINT) &dr);
		ClientToScreen(hMainWnd, (LPPOINT) &dr + 1);
		if (!GetCursorPos(&pt)) return;
		mouse_x = (int)((double)(pt.x-dr.left)/((double)(dr.right-dr.left)/(double)vid_xres));
		mouse_y = (int)((double)(pt.y-dr.top)/((double)(dr.bottom-dr.top)/(double)vid_yres));
	}
	else
	{
		if (!GetCursorPos(&pt)) return;
		mouse_x = pt.x;
		mouse_y = pt.y;
		if (WinCursor && AppIsForeground)
		{
			ShowCursor(0);
			WinCursor = false;
		}
		if (!WinCursor && !AppIsForeground)
		{
			ShowCursor(1);
			WinCursor = true;
		}
	}
}