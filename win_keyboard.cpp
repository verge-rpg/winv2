/******************************************************************
 * winv2: win_keyboard.cpp                                        *
 * copyright (c) 2001 vecna                                       *
 ******************************************************************/

#define DIRECTINPUT_VERSION 0x0300
#include "xerxes.h"
#include "dinput.h"

/****************************** data ******************************/

LPDIRECTINPUT dinput;
DIPROPDWORD dipdw;
LPDIRECTINPUTDEVICE  di_keyb;
LPDIRECTINPUTDEVICE di_joy;
char keys[256];
byte lastpressed;

/****************************** code ******************************/

int ParseKeyEvent()
{
	HRESULT hr;
	int y=0;
	DIDEVICEOBJECTDATA rgod[64];
	DWORD cod=64, iod;
	bool key_pressed = false;
	
	HandleMessages();	
	hr = di_keyb -> GetDeviceData(sizeof(DIDEVICEOBJECTDATA),rgod, &cod, 0);	//retrive data
	if(hr!=DI_OK&&hr!=DI_BUFFEROVERFLOW)
	{
		hr = di_keyb -> Acquire();
		if(!SUCCEEDED(hr))
			if(hr==DIERR_OTHERAPPHASPRIO) return 1;
	}
	else if(cod>0&&cod<=64)
	{
		for(iod=0;iod<cod;iod++)
		{
			if(rgod[iod].dwData==0x80)
			{
				keys[rgod[iod].dwOfs]=(char) 0x80;
				lastpressed = (char) rgod[iod].dwOfs;
				key_pressed = true;
				if (lastpressed != repeatedkey)
				{
					key_timer = systemtime;
					key_repeater = 0;
					repeatedkey = lastpressed;
				}
			}
			if(rgod[iod].dwData==0)
			{				
				keys[rgod[iod].dwOfs]=0;
				if (rgod[iod].dwOfs == (unsigned) repeatedkey)
				{
					key_timer = 0;
					key_repeater = 0;
					repeatedkey = 0;
				}
				switch(rgod[iod].dwOfs)
				{
				case 0xCB: keys[SCAN_LEFT]=0;
					key_timer=0; key_repeater=0; repeatedkey=0;
					break;
				case 0xCD: keys[SCAN_RIGHT]=0;
					key_timer=0; key_repeater=0; repeatedkey=0;
					break;
				case 0xD0: keys[SCAN_DOWN]=0;
					key_timer=0; key_repeater=0; repeatedkey=0;
					break;
				case 0xC8: keys[SCAN_UP]=0; 
					key_timer=0; key_repeater=0; repeatedkey=0;
					break;
				case 184: keys[SCAN_ALT]=0;
					key_timer=0; key_repeater=0; repeatedkey=0;
					break;
				case 156: keys[SCAN_ENTER]=0;
					key_timer=0; key_repeater=0; repeatedkey=0;
					break;
				}
			}
		}
	}
	if(keys[SCAN_ALT] && keys[SCAN_X]) err("");
	if(keys[SCAN_ALT] && keys[SCAN_TAB])
	{
		keys[SCAN_TAB]=0;
		keys[SCAN_ALT]=0;
		repeatedkey=0;
		keys[184]=0;
		if(lastpressed==SCAN_TAB) lastpressed=0;
	}
	if(keys[0xCB]) { keys[SCAN_LEFT]=(char)128; keys[0xCB]=0; lastpressed=SCAN_LEFT; key_timer=systemtime; repeatedkey=SCAN_LEFT; key_repeater=0;}
	if(keys[0xCD]) { keys[SCAN_RIGHT]=(char)128; keys[0xCD]=0; lastpressed=SCAN_RIGHT; key_timer=systemtime; repeatedkey=SCAN_RIGHT; key_repeater=0;}
	if(keys[0xD0]) { keys[SCAN_DOWN]=(char)128; keys[0xD0]=0; lastpressed=SCAN_DOWN; key_timer=systemtime;repeatedkey=SCAN_DOWN; key_repeater=0;}
	if(keys[0xC8]) { keys[SCAN_UP]=(char)128; keys[0xC8]=0; lastpressed=SCAN_UP; key_timer=systemtime; repeatedkey=SCAN_UP; key_repeater=0;}
	if(keys[184]) { keys[SCAN_ALT]=(char)128; keys[184]=0; lastpressed=SCAN_ALT; key_timer=systemtime; repeatedkey=SCAN_ALT; key_repeater=0;}
	if(keys[156]) { keys[SCAN_ENTER]=(char)128; keys[156]=0; lastpressed=SCAN_ENTER; key_timer=systemtime; repeatedkey=SCAN_ENTER; key_repeater=0;}

	if (lastpressed == SCAN_LEFT && keys[SCAN_RIGHT]) keys[SCAN_RIGHT]=0;
	if (lastpressed == SCAN_RIGHT && keys[SCAN_LEFT]) keys[SCAN_LEFT]=0;
	if (lastpressed == SCAN_UP && keys[SCAN_DOWN]) keys[SCAN_DOWN]=0;
	if (lastpressed == SCAN_DOWN && keys[SCAN_UP]) keys[SCAN_UP]=0;

	if (key_pressed && bindarray[lastpressed])
		HookKey(bindarray[lastpressed]);
	if (key_pressed) 
		return 1;
	return 0;
}

void UpdateKeyboard()
{
	int result;
	do 
		result = ParseKeyEvent();
	while (result);
}

void ShutdownKeyboard()
{
   if (di_keyb)
   {
      di_keyb -> Unacquire();
      di_keyb -> Release();
      di_keyb = NULL;
   }
   if (dinput)
   {
      dinput -> Release();
      dinput = NULL;
   }
}

void InitKeyboard()
{
	HRESULT hr;

	hr = DirectInputCreate(hMainInst, DIRECTINPUT_VERSION, &dinput, NULL);
	if (FAILED(hr))
		err("InitKeyboard: DirectInputCreate");

	hr = dinput -> CreateDevice(GUID_SysKeyboard, &di_keyb, NULL);   
	if (FAILED(hr))
        err("InitKeyboard: dinput CreateDevice");

	hr = di_keyb -> SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(hr))
        err("InitKeyboard: dinput SetDataFormat");

	hr = di_keyb -> SetCooperativeLevel(hMainWnd,DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (FAILED(hr))
        err("InitKeyboard: sinput SetCooperativeLevel");

	dipdw.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj = 0;
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData = 64;

	hr = di_keyb -> SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);

	if (FAILED(hr))
        err("InitKeyboard: Set buffer size");
	atexit(ShutdownKeyboard);
	return;
}
