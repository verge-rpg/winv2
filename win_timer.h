#ifndef TIMER_H
#define TIMER_H

extern quad systemtime, timer, vctimer;

class xTimer
{
	MMRESULT currtimer;
public:
	xTimer(int hz, LPTIMECALLBACK TimeProc);
	~xTimer();
};

void timer_Init(int hz);

#endif