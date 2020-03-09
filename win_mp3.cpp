/****************************************************************
	xerxes engine
	win_mp3.cpp
	mp3 playback using amp11lib
 ****************************************************************/

#include "xerxes.h"
#include "amp11lib.h"

/***************************** data *****************************/

bool amp11lib_open = false;
ALhandle hPlayer = 0, hFile = 0, hDecoder = 0;

/***************************** code *****************************/
	
void mp3_Close()
{
	if (amp11lib_open)
	{
		alEndLibrary();
		amp11lib_open = false;
	}
}


void mp3_Init()
{
	if (amp11lib_open) return;
	alInitLibrary();
	hPlayer = alOpenPlayer(-1, 44100, ALtrue, 0.05f);
	alEnableRedirection(0.025f);
	atexit(mp3_Close);
}


void mp3_Stop()
{
	if (hDecoder)
	{
		alSetRedirection(hDecoder, 0);
		alClose(hDecoder); hDecoder = 0;
		alClose(hFile); hFile = 0;
	}
}


void mp3_Play(char *fn)
{
	if (hFile) mp3_Stop();
	hFile = alOpenInputFile(fn);
	hDecoder = alOpenDecoder(hFile);
	alSetRedirection(hDecoder, hPlayer);
}


void mp3_Pause()
{
	if (hDecoder)
		alSetRedirection(hDecoder, 0);
}


void mp3_Resume()
{
	if (hPlayer && hDecoder)
		alSetRedirection(hDecoder, hPlayer);
}

