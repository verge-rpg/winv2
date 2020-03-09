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
/*
char scan_dos2win[0x7F] =
{
//000 001 002 003 004 005 006 007 008 009 00A 00B 00C 00D 00E 00F
	0, 27, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,109,187,  8,  9,  // 0x00
		0 
};*/

/*char lowercasetable[0xFF] = 
{
//000 001 002 003 004 005 006 007 008 009 00A 00B 00C 00D 00E 00F
	0,	0,  0,  0,  0,  0,  0,  0,  8,  9,  0,  0,  0, 13,  0,  0,  // 0x00 
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 27,  0,  0,  0,  0,  // 0x10
  ' ',  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0x20
  '0','1','2','3','4','5','6','7','8','9','-','=',  0,  0,  0,  0,  // 0x30
    0,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',  // 0x40
  'p','q','r','s','t','u','v','w','x','y','z',  0,  0,  0,  0,  0,  // 0x50
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,'*','+',  0,'-','.','/',  // 0x60
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0x70
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0x80
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0x90
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0xA0
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,';','=',',','-','.','/',  // 0xB0
  '`',  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0xC0
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,'[', 92,']', 39,  0   // 0xD0
};


#define SCAN_ESC        0x01
#define SCAN_1          0x02
#define SCAN_2          0x03
#define SCAN_3          0x04
#define SCAN_4          0x05
#define SCAN_5          0x06
#define SCAN_6          0x07
#define SCAN_7          0x08
#define SCAN_8          0x09
#define SCAN_9          0x0a
#define SCAN_0          0x0b
#define SCAN_MINUS      0x0c
#define SCAN_EQUALS     0x0d
#define SCAN_BACKSP     0x0e
#define SCAN_TAB        0x0f
#define SCAN_Q          0x10
#define SCAN_W          0x11
#define SCAN_E          0x12
#define SCAN_R          0x13
#define SCAN_T          0x14
#define SCAN_Y          0x15
#define SCAN_U          0x16
#define SCAN_I          0x17
#define SCAN_O          0x18
#define SCAN_P          0x19
#define SCAN_LANGLE     0x1a
#define SCAN_RANGLE     0x1b
#define SCAN_ENTER      0x1c
#define SCAN_CTRL       0x1d
#define SCAN_A          0x1e
#define SCAN_S          0x1f
#define SCAN_D          0x20
#define SCAN_F          0x21
#define SCAN_G          0x22
#define SCAN_H          0x23
#define SCAN_J          0x24
#define SCAN_K          0x25
#define SCAN_L          0x26
#define SCAN_SCOLON     0x27
#define SCAN_QUOTA      0x28
#define SCAN_RQUOTA     0x29
#define SCAN_LSHIFT     0x2a
#define SCAN_BSLASH     0x2b
#define SCAN_Z          0x2c
#define SCAN_X          0x2d
#define SCAN_C          0x2e
#define SCAN_V          0x2f
#define SCAN_B          0x30
#define SCAN_N          0x31
#define SCAN_M          0x32
#define SCAN_COMA       0x33
#define SCAN_DOT        0x34
#define SCAN_SLASH      0x35
#define SCAN_RSHIFT     0x36
#define SCAN_GREY_STAR  0x37
#define SCAN_ALT        0x38
#define SCAN_SPACE      0x39
#define SCAN_CAPS       0x3a
#define SCAN_F1         0x3b
#define SCAN_F2         0x3c
#define SCAN_F3         0x3d
#define SCAN_F4         0x3e
#define SCAN_F5         0x3f
#define SCAN_F6         0x40
#define SCAN_F7         0x41
#define SCAN_F8         0x42
#define SCAN_F9         0x43
#define SCAN_F10        0x44
#define SCAN_NUMLOCK    0x45
#define SCAN_SCRLOCK    0x46
#define SCAN_HOME       0x47
#define SCAN_UP         0x48
#define SCAN_PGUP       0x49
#define SCAN_GREY_MINUS 0x4a
#define SCAN_LEFT       0x4b
#define SCAN_PAD_5      0x4c
#define SCAN_RIGHT      0x4d
#define SCAN_GREY_PLUS  0x4e
#define SCAN_END        0x4f
#define SCAN_DOWN       0x50
#define SCAN_PGDN       0x51
#define SCAN_INSERT     0x52
#define SCAN_DEL        0x53
#define SCAN_F11        0x57
#define SCAN_F12        0x58
*/

/***************************** code *****************************/

void UpdateControls()
{
	HandleMessages();
	joy_Update();
	mouse_Update();		

	if (keys[VK_UP] || sticks[0].up) up = true; else up = false;
	if (keys[VK_LEFT] || sticks[0].left) left = true; else left = false;
	if (keys[VK_DOWN] || sticks[0].down) down = true; else down = false;
	if (keys[VK_RIGHT] || sticks[0].right) right = true; else right = false;

	if (keys[VK_RETURN] || sticks[0].button[0]) b1 = true; else b1 = false;
	if (keys[VK_SPACE] || sticks[0].button[1]) b2 = true; else b2 = false;
	if (keys[VK_ESCAPE] || sticks[0].button[2]) b3 = true; else b3 = false;
	if (keys[VK_CONTROL] || sticks[0].button[3]) b4 = true; else b4 = false;

	
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
