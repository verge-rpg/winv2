#ifndef IMAGE_H
#define IMAGE_H

extern int width, depth;

image *xLoadImage(char *fname);
image *xLoadImage0(char *name);
byte *xLoadImage8bpp(char *fname);

#endif
