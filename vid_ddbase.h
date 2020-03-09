#ifndef DDBASE_H
#define DDBASE_H

void ddwin_Flip();
void dd_Flip();
void dd_Fallback();
int dd_SetMode(int xres, int yres, int bpp, bool windowflag);
void dd_Close();
void dd_RegisterBlitters();

#endif