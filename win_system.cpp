/****************************************************************
	xerxes engine
	win_system.cpp
 ****************************************************************/

/**************** TODO *************
  + Add a getch() style function to win_keyboard that handles case, etc. dpad??
  + add volume controls to win_mp3, find a way to handle looping???
  + Redo VFILE* stuff and A_IMAGE.
  + Memory system???
  + Mouse routines! :o
  + fix strlwr in a_image
 ***********************************/

#include "xerxes.h"

/***************************** data *****************************/

HWND hMainWnd;
HINSTANCE hMainInst;

/****************************************************************/

bool AppIsForeground;
int DesktopBPP;

/***************************** code *****************************/

LRESULT APIENTRY WndProc(HWND hWnd, UINT message,WPARAM wParam, LPARAM lParam);
void HandleMessages();


int APIENTRY WinMain(HINSTANCE hCurrentInst, HINSTANCE zwhocares, LPSTR szCommandline, int nCmdShow)
{
	WNDCLASS WndClass;
	hMainInst = hCurrentInst;
	memset(&WndClass, 0, sizeof(WNDCLASS));
	WndClass.hCursor = LoadCursor(0, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
	WndClass.lpszClassName = "xerxestype";
	WndClass.hInstance = hMainInst;
	WndClass.lpfnWndProc = WndProc;
	RegisterClass(&WndClass);
	
	hMainWnd = CreateWindowEx(0, "xerxestype", "xerxes", WS_VISIBLE | WS_OVERLAPPED | WS_THICKFRAME | WS_CAPTION, GetSystemMetrics(SM_CXSCREEN)/2-160, GetSystemMetrics(SM_CYSCREEN)/2-140, 320, 240, NULL, NULL, hMainInst, NULL);
	ShowWindow(hMainWnd, SW_NORMAL);
	UpdateWindow(hMainWnd);
	HandleMessages();
	
	AppIsForeground = true;
	DesktopBPP = GetDeviceCaps(GetDC(NULL), BITSPIXEL);
	SetWindowText(hMainWnd, APPNAME);
	
	srand(timeGetTime());
	log_Init(true);
	mouse_Init();
	InitKeyboard();
	joy_Init();
	timer_Init(100);
	InitSound();
	ShowCursor(0);

	xmain(__argc,__argv);
	err("");
	return 0;
}


void HandleMessages(void)
{
	MSG msg;
	while (PeekMessage(&msg, hMainWnd, (int) NULL, (int) NULL, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


LRESULT APIENTRY WndProc(HWND hWnd, UINT message,WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_DESTROY:
		case WM_CLOSE:
			err("");
			break;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE)
				AppIsForeground = false;
			else 
				AppIsForeground = true;
			break;
		case WM_ACTIVATEAPP:
			if (!(BOOL)wParam)
				AppIsForeground = false;
			else 
				AppIsForeground = true;
			return 0;
		case WM_SYSCOMMAND:
			if (wParam == SC_CLOSE)
				err("");			
			break;
		case WM_SYSKEYDOWN:
			if (wParam == 'X')
				err("");
			if(wParam==VK_RETURN&&!(lParam&0x40000000))
			{
				if(!vid_SetMode(vid_xres,vid_yres,vid_bpp,!vid_window,MODE_SOFTWARE))
					err("");
				return 0;
			}
		break;

			break;
		case WM_KEYDOWN: return 0;
		case WM_KEYUP: return 0;
		case WM_LBUTTONDOWN: mouse_l = true; break; 
		case WM_LBUTTONUP:   mouse_l = false; break;
		case WM_RBUTTONDOWN: mouse_r = true; break;
		case WM_RBUTTONUP:   mouse_r = false; break;
		case WM_MBUTTONDOWN: mouse_m = true; break;
		case WM_MBUTTONUP:   mouse_m = false; break;
		case WM_MOUSEWHEEL:
			mwheel += (short) HIWORD(wParam);
			break;
	}	
	return DefWindowProc(hWnd, message, wParam, lParam);
}


void err(char *str, ...)
{		
	va_list argptr;
	char msg[256];

	va_start(argptr, str);
	vsprintf(msg, str, argptr);
	va_end(argptr);
	
	if (strlen(msg))
	{
		MessageBox(GetDesktopWindow(), msg, APPNAME, MB_OK | MB_TASKMODAL);
		log("Exiting: %s", msg);		
	}	
	UseSound = false;
	PostQuitMessage(0);
	exit(strlen(msg)==0?0:-1);
}