/****************************************************************
	xerxes engine
	a_common.cpp
 ****************************************************************/

#include "xerxes.h"

/***************************** data *****************************/

FILE *logf;
bool log_on = false;

/***************************** code *****************************/


void log_Init(bool on)
{
	remove(LOGFILE);
	log_on = on;
}


void log(char *text, ...)
{
	va_list argptr;
	char msg[256];

	va_start(argptr, text);
	vsprintf(msg, text, argptr);
	va_end(argptr);

	if (!log_on) return;
	logf = fopen(LOGFILE,"aw");
	fprintf(logf,"%s\n",msg);
	fflush(logf);
	fclose(logf);
}


char *va(char* format, ...)
{
	va_list argptr;
    static char string[1024];

    va_start(argptr, format);
    vsprintf(string, format, argptr);
    va_end(argptr);

	return string;
}


int strcasecmp(char *s1, char *s2)
{
	while (true)
	{
		if (tolower(*s1) != tolower(*s2))
			return -1;
		if (!*s1)
			return 0;
		s1++;
		s2++;
	}
	return -1;
}


bool isletter(char c)
{
	if (tolower(c) < 'a' || tolower(c) > 'z') return false;
	return true;
}


void strclean(char *s)
{
	while (*s)
	{
		if (*s == 10 || *s == 13)
		{ 
			*s = 0; 
			return;
		}
		s++;
	}
}


int sgn(int a)
{
	if (a<0)
		return -1;
	else if (a>0)
		return +1;	
	return 0;
}


int rnd(int lo, int hi)
{
	int range = hi - lo + 1;
	int i = rand() % range;
	return i + lo;
}