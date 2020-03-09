/****************************************************************
	xerxes engine
	g_controls.cpp
 ****************************************************************/

#include "xerxes.h"

/***************************** data *****************************/

byte up, down, left, right;
byte b1, b2, b3, b4;
byte kill_up, kill_down, kill_left, kill_right;
byte kill_b1, kill_b2, kill_b3, kill_b4;

/***************************** code *****************************/

void UpdateControls()
{
	HandleMessages();
	joy_Update();
	mouse_Update();		
	UpdateKeyboard();

	if (keys[SCAN_UP] || sticks[0].up) up = true; else up = false;
	if (keys[SCAN_LEFT] || sticks[0].left) left = true; else left = false;
	if (keys[SCAN_DOWN] || sticks[0].down) down = true; else down = false;
	if (keys[SCAN_RIGHT] || sticks[0].right) right = true; else right = false;

	if (keys[SCAN_ENTER] || sticks[0].button[0]) b1 = true; else b1 = false;
	if (keys[SCAN_ALT] || sticks[0].button[1]) b2 = true; else b2 = false;
	if (keys[SCAN_ESC] || sticks[0].button[2]) b3 = true; else b3 = false;
	if (keys[SCAN_SPACE] || sticks[0].button[3]) b4 = true; else b4 = false;
	
	if (!up && kill_up) kill_up = false;
	if (!down && kill_down) kill_down = false;
	if (!left && kill_left) kill_left = false;
	if (!right && kill_right) kill_right = false;

	if (!b1 && kill_b1) kill_b1 = false;
	if (!b2 && kill_b2) kill_b2 = false;
	if (!b3 && kill_b3) kill_b3 = false;
	if (!b4 && kill_b4) kill_b4 = false;
	
	if (up && kill_up) up = false;
	if (down && kill_down) down = false;
	if (left && kill_left) left = false;
	if (right && kill_right) right = false;

	if (b1 && kill_b1) b1 = false;
	if (b2 && kill_b2) b2 = false;
	if (b3 && kill_b3) b3 = false;
	if (b4 && kill_b4) b4 = false;
}
