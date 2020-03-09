#ifndef KEYBOARD_H
#define KEYBOARD_H

void key_Init();
void key_Update();
void key_ReceiveMessage(int vk, int state);
bool key_KeyPressed();
void key_ClearKeys();
int key_GetKey();

extern char keys[256];
extern char lastpressed;

#endif
