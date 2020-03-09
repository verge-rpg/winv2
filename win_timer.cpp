/****************************************************************
	xerxes engine
	win_timer.cpp
 ****************************************************************/

#include "xerxes.h"

/***************************** data *****************************/

quad systemtime = 0, timer = 0, vctimer = 0;;

/***************************** code *****************************/

void CALLBACK DefaultTimer(UINT uID,UINT uMsg,DWORD dwUser,DWORD dw1,DWORD dw2)
{	
	UpdateSound();
	systemtime++;
	timer++;
	vctimer++;
}

void timer_Init(int hz)
{
	static xTimer *systimer;
	systimer = new xTimer(hz, DefaultTimer);
}

/****************************************************************/

xTimer::xTimer(int hz, LPTIMECALLBACK TimeProc)
{
	if (currtimer)
		timeKillEvent(currtimer);
    currtimer = timeSetEvent(1000/hz,0,(LPTIMECALLBACK)TimeProc,0,TIME_PERIODIC); 
}

xTimer::~xTimer()
{
	timeKillEvent(currtimer);
    currtimer=0;
}
