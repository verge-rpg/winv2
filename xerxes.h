#ifndef XERXES_H
#define XERXES_H

/* xerxes system types */

typedef unsigned int   quad;
typedef unsigned short word;
typedef unsigned char  byte;

/* macros */

#define APPNAME "WinV2"
#define LOGFILE "winv2.log"
#define ORDER_INT(a,b) { if (a>b) { a=a-b; b=a+b; a=b-a; } }
#define SWAP(a,b) { a=a-b; b=a+b; a=b-a; }

/* system includes */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0500
#include <crtdbg.h>
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/* xerxes includes */

#include "vid_ddbase.h"
#include "vid_manager.h"
#include "vid_sysfont.h"
#include "a_common.h"
#include "a_config.h"
#include "a_image.h"
#include "a_vfile.h"
#include "win_joystick.h"
#include "win_network.h"
#include "win_keyboard.h"
#include "win_mouse.h"
#include "win_system.h"
#include "win_sound.h"
#include "win_timer.h"
#include "g_8bpp.h"
#include "g_controls.h"
#include "g_chr.h"
#include "g_fnt.h"
#include "g_vsp.h"
#include "g_map.h"
#include "g_vc.h"
#include "g_engine.h"
#include "g_startup.h"

/* prototypes */

void xmain(int argc, char *argv[]);
void err(char *s, ...);
void HandleMessages();

#endif