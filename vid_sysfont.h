#ifndef SYSFONT_H
#define SYSFONT_H

void GotoXY(int, int);
void PrintRight(int x1, int y1, char *str);
void PrintCenter(int x1, int y1, char *str, ...);
void print_char(char);
void PrintString(char *, ...);
int  pixels(char *);
void TextColor(int);
extern int fontx, fonty;

#endif
