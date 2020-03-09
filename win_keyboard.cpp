/****************************************************************
	xerxes engine
	win_keyboard.cpp
 ****************************************************************/

#include "xerxes.h"

/***************************** data *****************************/

char keys[256];
char lastpressed;

char key_queue[256];
byte key_queuehead, key_queuetail;

bool key_initd = false;

/***************************** code *****************************/

void key_Init()
{
	key_ClearKeys();
	key_initd = true;
}


void key_ReceiveMessage(int vk, int state)
{
	byte k = (vk & 0xff);
	keys[k] = state;

	if (state)
	{
		lastpressed = k;
		key_queue[key_queuehead++] = k;
	}	
}


bool key_KeyPressed()
{
	return !(key_queuehead == key_queuetail);
}


void key_ClearKeys()
{
	memset(keys, 0, 256);
	memset(key_queue, 0, 256);
	lastpressed = 0;
	key_queuehead = key_queuetail = 0;
}


int key_GetKey()
{
	if (key_queuehead == key_queuetail)
		return 0;
	return key_queue[key_queuetail++];
}
