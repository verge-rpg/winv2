/****************************************************************
	xerxes engine
	vid_ddbase.cpp
 ****************************************************************/

#include "xerxes.h"
#include "ddraw.h"

/***************************** data *****************************/

LPDIRECTDRAW dx_dd;
LPDIRECTDRAWSURFACE dx_ps,dx_bs,dx_os,dx_win_ps,dx_win_bs;
DDSURFACEDESC dx_psd,dx_bsd,dx_osd,dx_win_psd,dx_win_bsd;
bool dd_initd = false;

/***************************** code *****************************/

#define DX_RELEASE(x) { if (x) { x->Release(); x=0; } }


void dd_init()
{
	HRESULT hr = DirectDrawCreate(NULL, &dx_dd, NULL);
	if (hr != DD_OK)
		err("Could not create DirectDraw object.");

	screen = new image();

	dx_ps = 0;
	dx_bs = 0;
	dx_os = 0;
	dx_win_bs = 0;
	dx_win_ps = 0;

	/* Make suface descriptions */

	memset(&dx_psd, 0, sizeof(DDSURFACEDESC));
	dx_psd.dwSize = sizeof(DDSURFACEDESC);
	dx_psd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	dx_psd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
	dx_psd.dwBackBufferCount = 1;

	memset(&dx_bsd, 0, sizeof(DDSURFACEDESC));
	dx_bsd.dwSize = sizeof(DDSURFACEDESC);
	dx_bsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;// | DDSCAPS_FLIP | DDSCAPS_COMPLEX;

	memset(&dx_osd,0,sizeof DDSURFACEDESC);
	dx_osd.dwSize=sizeof DDSURFACEDESC;
	dx_osd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	dx_osd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

	memset(&dx_win_psd, 0, sizeof(DDSURFACEDESC));
	dx_win_psd.dwSize = sizeof(DDSURFACEDESC);
	dx_win_psd.dwFlags = DDSD_CAPS;
    dx_win_psd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	
	memset(&dx_win_bsd, 0, sizeof(DDSURFACEDESC));
	dx_win_bsd.dwSize = sizeof(DDSURFACEDESC);
    dx_win_bsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;    
    dx_win_bsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

	dd_initd = true;
}


void dd_Close()
{
	DX_RELEASE(dx_bs);
	DX_RELEASE(dx_os);
	DX_RELEASE(dx_ps);
	DX_RELEASE(dx_win_bs);
	DX_RELEASE(dx_win_ps);
	DX_RELEASE(dx_dd);
}


int dd_SetMode(int xres, int yres, int bpp, bool windowflag)
{
	quad ws;
	RECT r, r2;
	HRESULT hr;
	LPDIRECTDRAWCLIPPER clipper;	

	if (!dd_initd)
		dd_init();
	
	if (!windowflag)
	{
		hr = dx_dd->SetCooperativeLevel(hMainWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
		if (hr != DD_OK)
		{
			return 0;
		}
		dx_dd->SetDisplayMode(xres, yres, bpp);
		DX_RELEASE(dx_bs);
		DX_RELEASE(dx_ps);
		DX_RELEASE(dx_os);
		DX_RELEASE(dx_win_bs);
		DX_RELEASE(dx_win_ps);
		
		hr = dx_dd->CreateSurface(&dx_psd, &dx_ps, NULL);
		if (hr != DD_OK)
		{
			return 0;
		}
		dx_osd.dwWidth=xres;
		dx_osd.dwHeight=yres;
		hr=dx_dd->CreateSurface(&dx_osd,&dx_os,NULL);
		if(hr!=DD_OK)
		{
			DX_RELEASE(dx_ps);
			return 0;
		}
		hr = dx_ps->GetAttachedSurface(&dx_bsd.ddsCaps, &dx_bs);
		if (hr != DD_OK)
		{
			DX_RELEASE(dx_ps);
			DX_RELEASE(dx_os);
			return 0;
		}

		ws = GetWindowLong(hMainWnd, GWL_STYLE);
		ws &= ~WS_OVERLAPPEDWINDOW;
		ws |= WS_POPUP;
		SetWindowLong(hMainWnd, GWL_STYLE, ws);

		DDPIXELFORMAT ddpf;
		ddpf.dwSize = sizeof(ddpf);
		ddpf.dwFlags = DDPF_RGB;
		hr = dx_ps->GetPixelFormat(&ddpf);
		if (hr != DD_OK) err("Could not get pixel format!");
		if (ddpf.dwRBitMask == 0x7C00 && bpp == 16)
			vid_bpp = 15, vid_bytesperpixel = 2;
		else
			vid_bpp = bpp, vid_bytesperpixel = bpp / 8;
		
		vid_xres = xres;
		vid_yres = yres;
		vid_window = windowflag;

		screen->alphamap = 0;
		screen->bpp = bpp;
		screen->width = xres;
		screen->height = yres;
		screen->cx1 = 0;
		screen->cx2 = xres-1;
		screen->cy1 = 0;
		screen->cy2 = yres-1;

		dd_RegisterBlitters();

		Flip();
		return 1;
	}
	else
	{
		if (bpp != DesktopBPP) return 0; 
		if (!vid_window)
			dx_dd->RestoreDisplayMode();
		dx_dd->SetCooperativeLevel(hMainWnd, DDSCL_NORMAL);
		DX_RELEASE(dx_bs);
		DX_RELEASE(dx_ps);
		DX_RELEASE(dx_win_bs);
		DX_RELEASE(dx_win_ps);
		
		dx_win_bsd.dwWidth = xres;
		dx_win_bsd.dwHeight = yres;
		hr = dx_dd->CreateSurface(&dx_win_psd, &dx_win_ps, NULL);
		if (hr != DD_OK)
		{
			return 0;
		}
		hr = dx_dd->CreateSurface(&dx_win_bsd, &dx_win_bs, NULL);
		if (hr != DD_OK)
		{
			DX_RELEASE(dx_win_ps);
			return 0;
		}
		hr = dx_win_bs->Lock(0, &dx_win_bsd, 0, 0);
		if (hr != DD_OK)
		{
			DX_RELEASE(dx_win_ps);
			DX_RELEASE(dx_win_bs);
			return 0;
		}
		dx_win_bs->Unlock(0);
		hr = dx_dd->CreateClipper(0, &clipper, 0);
		if (hr != DD_OK)
		{
			DX_RELEASE(dx_win_ps);
			DX_RELEASE(dx_win_bs);
			return 0;
		}
		hr = clipper->SetHWnd(0, hMainWnd);
		if (hr != DD_OK)
		{
			DX_RELEASE(dx_win_ps);
			DX_RELEASE(dx_win_bs);
			DX_RELEASE(clipper);
			return 0;
		}
		hr = dx_win_ps->SetClipper(clipper);
		if (hr != DD_OK)
		{
			DX_RELEASE(dx_win_ps);
			DX_RELEASE(dx_win_bs);
			DX_RELEASE(clipper);
			return 0;
		}
		clipper->Release();

		ws = GetWindowLong(hMainWnd, GWL_STYLE);
		ws &= ~WS_POPUP;
		ws |= WS_OVERLAPPEDWINDOW;
		SetWindowLong(hMainWnd, GWL_STYLE, ws);
		GetWindowRect(hMainWnd, &r);
		SetRect(&r2, 0, 0, xres, yres);
		AdjustWindowRectEx(&r2, ws, GetMenu(hMainWnd) != NULL, GetWindowLong(hMainWnd, GWL_EXSTYLE));
		SetWindowPos(hMainWnd, 0, r.left, r.top, r2.right - r2.left, r2.bottom - r2.top, SWP_NOZORDER | SWP_NOACTIVATE);

		DDPIXELFORMAT ddpf;
		ddpf.dwSize = sizeof(ddpf);
		ddpf.dwFlags = DDPF_RGB;
		hr = dx_win_ps->GetPixelFormat(&ddpf);
		if (hr != DD_OK) err("Could not get pixel format!");
		if (ddpf.dwRBitMask == 0x7C00 && bpp == 16)
			vid_bpp = 15, vid_bytesperpixel = 2;
		else
			vid_bpp = bpp, vid_bytesperpixel = bpp / 8;
		
		vid_xres = xres;
		vid_yres = yres;
		vid_window = windowflag;
		
		screen->data = dx_win_bsd.lpSurface;
		screen->alphamap = 0;
		screen->bpp = bpp;
		screen->width = xres;
		screen->height = yres;
		screen->pitch = dx_win_bsd.lPitch / vid_bytesperpixel;
		screen->cx1 = 0;
		screen->cx2 = xres-1;
		screen->cy1 = 0;
		screen->cy2 = yres-1;
		
		dd_RegisterBlitters();
		return 1;
	}
	return 0;
}


void ddwin_Flip()
{
	RECT r;
	GetClientRect(hMainWnd, &r);
	ClientToScreen(hMainWnd, (POINT *)&r);
	ClientToScreen(hMainWnd, (POINT *)&r + 1);
	dx_win_ps->Blt(&r, dx_win_bs, 0, 0, 0);
}


void dd_Flip()
{
	int i=0;
	HRESULT hr;

	hr = dx_bs->BltFast(0,0,dx_os,NULL,DDBLTFAST_WAIT | DDBLTFAST_NOCOLORKEY);
	if(hr==DDERR_SURFACELOST)
	{
		dx_bs->Restore();
		dx_os->Restore();
		dx_bs->BltFast(0,0,dx_os,NULL,DDBLTFAST_WAIT | DDBLTFAST_NOCOLORKEY);
	}

	hr=dx_ps->Flip(0,DDFLIP_WAIT);// | DDFLIP_NOVSYNC);
	//dx_ps->Flip(0,0);

	if(hr==DDERR_SURFACELOST)
	{
		dx_ps->Restore();
		hr=dx_ps->Flip(0,DDFLIP_WAIT);// | DDFLIP_NOVSYNC);
	}
	
	
	hr=dx_os->Lock(0,&dx_osd,DDLOCK_SURFACEMEMORYPTR | DDLOCK_WRITEONLY | DDLOCK_WAIT,0);
	if(hr==DDERR_SURFACELOST)
	{
		dx_os->Restore();
		hr=dx_os->Lock(0,&dx_osd,DDLOCK_SURFACEMEMORYPTR | DDLOCK_WRITEONLY | DDLOCK_WAIT,0);
	}
	dx_os->Unlock(0);

	screen->data=dx_osd.lpSurface;
	screen->pitch=dx_osd.lPitch/vid_bytesperpixel;
}

