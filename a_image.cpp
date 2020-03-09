/****************************************************************
	xerxes engine
	a_image.cpp
 ****************************************************************/

#include "xerxes.h"

/***************************** data *****************************/

int width = 0, depth = 0;
byte palconv[768] = { 0 };

/***************************** code *****************************/


int img_load_CheckPCX(void *header)
{
	if( *((byte *)header)==10)
		header=(byte *)header+1;
	else return 0;
	
	if( *((byte *)header)==5)
		return 1;
	else return 0;
}

int img_load_CheckBMP(void *header)
{
	if(*((word *)header)==19778)
		return 1;
	else
		return 0;
}

int img_load_CheckGIF(void *header)
{
	if(strncmp((char *)header,"GIF",3))
		return 0;
	else
		return 1;
}


/********************* GIF imaging routines *********************/

#define TRUE  1
#define FALSE 0

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

typedef struct
{
  u8 bits;
  u8 background;
  u8 * palette;
  u8 * image;
  s16 wide, deep;
} gif_image_info;

typedef struct
{
  char sig[7];
  s16 screenwide, screendeep;
  u8 hflags;
  u8 background;
  u8 aspect;
} gif_header;

typedef struct
{
  s16 top, left;
  s16 wide, deep;
  u8 iflags;
} gif_image_descriptor;

int NO_CODE = -1,// TRUE = 1,
    ERROR_EOF = 0, ERROR_BAD_CODE = 1,
    ERROR_BAD_HEADER = 2, ERROR_BAD_STARTCODE = 3,
    ERROR_BAD_FIRST_CODE = 4, ERROR_BAD_FILE = 5,
    ERROR_NO_IMAGE = 6;

char* gif_error_messages[] =
{
  "Unexpected end of file\n",
  "Bad code\n",
  "Bad gif header\n",
  "Bad symbol size\n",
  "Bad first code\n",
  "Error opening file\n",
  "This file doesn't contain an image\n"
};

u8* gif_read_palette(FILE* fp, s32 bytes)
{
  s32 i = 0;
  u8* block = 0L;
  s32 components = (bytes / 3) * 3;

  block = new u8[components];
	  
  for (i = 0; i < components; ++i)
    block[i] = fgetc(fp);

  return block;
}

// read a block of bytes into memory
s32 block_mem_read(FILE* fp, u8* buffer, s32 bytes)
{
  s32 status = 0;

  status = fread(buffer, 1, bytes, fp);
  if (status != bytes) return EOF;

  return TRUE;
}

// read a unsigned 16 bit value from file, low byte first; note that this
// is reverse endian-ness (ie. fwrite(&s,1,2,fp); writes high byte first).

s16 read_word_lbf(FILE* fp)
{
  s32 a, b;

  a = fgetc(fp);
  b = fgetc(fp);

  return (s16) ((b << 8) | a);
}

// read the GIF file header structure into a sequence
gif_header* get_gif_header(FILE* fp)
{
  gif_header* h = 0L;

  h = new gif_header;

  fread(h->sig, 1, 6, fp);
  h->sig[6] = 0;

  if (strncmp(h->sig, "GIF", 3) != 0)
    return NULL;

  h->screenwide = read_word_lbf(fp); width=h->screenwide;
  h->screendeep = read_word_lbf(fp); depth=h->screendeep;
  h->hflags = fgetc(fp);
  h->background = fgetc(fp);
  h->aspect = fgetc(fp);

  return h;
}

// gif file can contain more than one image,
// each image is preceeded by a header structure
gif_image_descriptor* get_image_descriptor(FILE* fp)
{
  gif_image_descriptor* id = 0L;

  id = new gif_image_descriptor;

  id->left = read_word_lbf(fp);
  id->top = read_word_lbf(fp);
  id->wide = read_word_lbf(fp);
  id->deep = read_word_lbf(fp);
  id->iflags = fgetc(fp);

  return id;
}

static u16 word_mask_table[] =
{
  0x0000, 0x0001, 0x0003, 0x0007,
  0x000F, 0x001F, 0x003F, 0x007F,
  0x00FF, 0x01FF, 0x03FF, 0x07FF,
  0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF
};

static u8 inc_table[] = { 8,8,4,2,0 };
static u8 start_table[] = { 0,4,2,1,0 };

// enables me to use indices as per EUPHORiA (ie. converts to C's 0 base)
#define eui(i) ((i)-1)

// unpack an LZW compressed image
// returns a sequence containing screen display lines of the image
u8* unpack_image(FILE* fp, s32 start_code_size, u32 width, u32 depth, u32 flags)
{
  u8* buffer;
  u8* line_buffer;

  u16 first_code_stack[4096];
  u16 last_code_stack[4096];
  u16 code_stack[4096];

  s32 bits_left;
  s32 clear_code;
  s32 code_size;
  s32 code_size2;
  s32 next_code;
  s32 this_code;
  s32 old_token;
  s32 current_code;
  s32 old_code;
  s32 block_size=0;
  s32 line;
  s32 a_byte;
  s32 pass;
  s32 u;

  u8 b[256]; // read buffer; for block reads
  u8* p; // current byte in read buffer
  u8* q; // last byte in read buffer + 1

  line_buffer = new u8[width];
  buffer      = new u8[width * depth];

  a_byte = 0;
  line = 0;
  pass = 0;
  bits_left = 8;

  if (start_code_size < 2 || start_code_size > 8)
    err("ERROR_BAD_STARTCODE"); // bad symbol size

  p = b;
  q = b;

  clear_code = 1 << start_code_size; //pow(2, start_code_size);
  next_code = clear_code + 2;
  code_size = start_code_size + 1;
  code_size2 = 1 << code_size; //pow(2, code_size);
  old_code = NO_CODE;
  old_token = NO_CODE;

  while (1)
  {
    if (bits_left == 8)
    {
      ++p;
      if (p >= q)
      {
        block_size = fgetc(fp);
        if (block_mem_read(fp, b, block_size) == EOF)
          err("ERROR_EOF");
        p = b;
        q = b + block_size;
      }
      bits_left = 0;
    }

    this_code = *p;
    current_code = code_size + bits_left;

    if (current_code <= 8)
    {
      *p = *p >> code_size;
      bits_left = current_code;
    }
    else
    {
      ++p;
      if (p >= q)
      {
        block_size = fgetc(fp);
        if (block_mem_read(fp, b, block_size) == EOF)
          err("ERROR_EOF");
        p = b;
        q = b + block_size;
      }

      this_code |= (*p << (8 - bits_left));

      if (current_code <= 16)
      {
        bits_left = current_code - 8;
        *p = *p >> bits_left;
      }
      else
      {
        if (++p >= q)
        {
          block_size = fgetc(fp);
          if (block_mem_read(fp, b, block_size) == EOF)
            err("ERROR_EOF");
          p = b;
          q = b + block_size;
        }

        this_code |= (*p << (16 - bits_left));

        bits_left = current_code - 16;
        *p = *p >> bits_left;
      }
    }

    this_code &= word_mask_table[code_size];
    current_code = this_code;

    if (this_code == (clear_code+1) || block_size == 0)
      break;
    if (this_code > next_code)
      err("ERROR_BAD_CODE");

    if (this_code == clear_code)
    {
      next_code = clear_code + 2;
      code_size = start_code_size + 1;
      code_size2 = 1 << code_size; //pow(2, code_size);
      old_code = NO_CODE;
      old_token = NO_CODE;
    }
    else
    {
      u = 1;
      if (this_code == next_code)
      {
        if (old_code == NO_CODE)
          err("ERROR_BAD_FIRST_CODE");

        first_code_stack[eui(u)] = (u16) old_token;
        u++;
        this_code = old_code;
      }

      while (this_code >= clear_code)
      {
        first_code_stack[eui(u)] = last_code_stack[eui(this_code)];
        u++;
        this_code = code_stack[eui(this_code)];
      }

      old_token = this_code;
      while (1)
      {
        line_buffer[a_byte] = (u8) this_code;
        a_byte++;
        if (a_byte >= (signed) width)
        {
          // full image line so add it into screen image
          memcpy(buffer + (line * width), line_buffer, width);

          a_byte = 0;
          if (flags & 0x40)
          {
            line += inc_table[pass];
            if (line >= (signed) depth)
            {
              pass++;
              line = start_table[pass];
            }
          }
          else
          {
            line++;
          }
        }

        // no more bytes on stack
        if (u == 1) break;

        u--;
        this_code = first_code_stack[eui(u)];
      }

      if (next_code < 4096 && old_code != NO_CODE)
      {
        code_stack[eui(next_code)] = (u16) old_code;
        last_code_stack[eui(next_code)] = (u16) old_token;
        next_code++;
        if (next_code >= code_size2 && code_size < 12)
        {
          code_size++;
          code_size2 = 1 << code_size; //pow(2, code_size);
        }
      }

      old_code = current_code;
    }
  }

  // completed reading the image so return it
  delete [] line_buffer;
  return buffer;
}

// skip the extension blocks as we are only after the image
void skip_extension(FILE* fp)
{
  s32 n;
  char temp[256];

  n = fgetc(fp); // throwaway extension function code
  n = fgetc(fp); // get length of block

  while (n > 0 && n != EOF)
  {
    // throwaway block
    fread(temp, 1, n, fp);

    n = fgetc(fp); // get length of next block
  }
}

// unpack the GIF file
// returns ImageInfo sequence containing image and image data
gif_image_info* unpack_gif(char* filename)
{
  VFILE *f;
  FILE* fp;
  s32 c, b;
  gif_header* h = 0L;
  gif_image_info* ii = 0L;
  gif_image_descriptor* id = 0L;
  u8* local_palette = 0L;

  ii = new gif_image_info;

  f = vopen(filename);
  if (!f) err("unpack_gif(), could not open GIF file %s.",filename);
  fp = f->fp;
  if (!fp) err("Bad filename");

  // file starts with the Logical Screen Descriptor structure
  h = get_gif_header(fp);

  // Size of Global Color Table
  ii->bits = (h->hflags & 7) + 1;
  ii->background = h->background;

  // get Global colour palette if there is one
  if (h->hflags & 0x80) // is flags bit 8 set?
  {
    c = 3 << ii->bits; // size of global colour map
    ii->palette = gif_read_palette(fp, c);
    memcpy(palconv, ii->palette, 768);
    delete [] ii -> palette;
  }
  delete h;

  c = fgetc(fp);

  while (c == 0x2c || c == 0x21 || c == 0)
  {
    // image separator so unpack the image
    if (c == 0x2c)
    {
      id = get_image_descriptor(fp); // get the Image Descriptor
      // if there is a local Color Table then overwrite the global table
      if (id->iflags & 0x80)
      {
        ii->bits = (id->iflags & 7) + 1;
        b = 3 << ii->bits;
        if (local_palette)
          delete [] local_palette;
        local_palette = gif_read_palette(fp, b);
        memcpy(palconv, local_palette, 768);
        delete [] local_palette;
      }

      c = fgetc(fp); // get the LZW Minimum Code Size
      ii->image = unpack_image(fp, c, id->wide, id->deep, id->iflags);
      vclose(f);

      // error reading image
      if (!ii->image)
        err("error reading image data");

      ii->wide = id->wide;
      ii->deep = id->deep;
      delete id;

      // return imagedata
      return ii;
    }
    // extension introducer
    else if (c == 0x21)
    {
      skip_extension(fp); // throw the extension away
    }

    c = fgetc(fp);
  } 
  // no image?
  return NULL;
}

byte *LoadGIFBuf(char *fname)
{
  gif_image_info *ii=0;
  byte *t;

  ii = unpack_gif(fname);
  width = ii->wide;
  depth = ii->deep;
  t = (byte *) ii->image;
  delete ii;
  return t;
}

// ========================= PCX Imaging routines ============================

int TwentyFourBit;

// ================================= Code ====================================

void pcx_decode_scan(VFILE* vf, byte* start, short skip, int width)
{
	int c, n, run;

	n = 0;
	while (n < width)
	{
		run = 1;
		c = vgetc(vf) & 0xff;
		if ((c & 0xc0) == 0xc0)
		{
			run = c & 0x3f;
			c = vgetc(vf);
		}
		n += run;
		do
		{
			*start = (byte) c;
			start += skip;
			run -= 1;
		}
		while (run);
	}
}


static byte *LoadPCXBuf(char *fname)
{
#pragma pack(1)
	typedef struct
	{
		char manufacturer;	// always 10
		char version;
		char encoding;
		char bpp;			// bits per pixel
		word xmin, ymin;
		word xmax, ymax;
		word hres, vres;
		char palette[48];
		char reserved;
		char color_planes;
		word bpl;			// bytes per line
		word palette_type;
		char filler[58];
	
	} header;
#pragma pack()
	
	header	h={0};
	VFILE	*vfp=0;
	byte	*buf=0,	*dest=0;
	int		i=0;

	vfp=vopen(fname);
	if (!vfp)
		err("LoadPCXBuf(), could not open PCX file %s.", fname);
	
	vread(&h, 128, vfp);
	
	// do some checking
	if (10 != h.manufacturer)
		err("%s is not a PCX file.", fname);
	if (8 != h.bpp)
		err("%s is not a valid 8-bit PCX. (reported %i bits)", fname, h.bpp);
	TwentyFourBit = (3 == h.color_planes);

	width=h.xmax-h.xmin+1;
	depth=h.ymax-h.ymin+1;
	
	buf = new byte[width * depth * (TwentyFourBit ? 3 : 1)];

	// decode image into buf
	if (!TwentyFourBit)
		for (dest=buf,i=depth; i--; dest+=width)
			pcx_decode_scan(vfp, dest, 1, h.bpl);
	else
		for (dest=buf,i=depth; i--; dest+=(width*3))
		{
			pcx_decode_scan(vfp, dest+0, 3, h.bpl);
			pcx_decode_scan(vfp, dest+1, 3, h.bpl);
			pcx_decode_scan(vfp, dest+2, 3, h.bpl);			
		}
	
	// palette available?
	if (12 == vgetc(vfp))
		vread(palconv, 768, vfp);
	
	vclose(vfp);

	return buf;
}

// *********************
// * Imaging Interface *
// *********************

int imagetype;

void vstrlwr(char *c)
{
   while (*c)
   {
      if ((char) *c >= 'A' && (char) *c <= 'Z')
      {
         *c = *c - 'A';
         *c = *c + 'a';
      }
      c++;
   }
}


void DetermineFileType(char *fname)
{
   vstrlwr(fname);
   if (!strcmp(fname+(strlen(fname)-3),"pcx")) imagetype=0;
   if (!strcmp(fname+(strlen(fname)-3),"gif")) imagetype=1;
}

image *xLoadImage(char *name)
{
	byte *blah;
	image *b;
	
	width=depth=0;
	imagetype = 255;
	DetermineFileType(name);
	if (imagetype == 255) err("%s is not a recognized image type.",name);
	switch (imagetype)
	{
		case 0: blah = LoadPCXBuf(name); 
				if (!TwentyFourBit)
				{
					b = ImageFrom8bpp(blah, width, depth, palconv); 
					break;
				}
				else
				{
					b = ImageFrom24bpp(blah, width, depth);
					break;
				}
		case 1: blah = LoadGIFBuf(name); b = ImageFrom8bpp(blah, width, depth, palconv); break;
		default: err("xLoadImage: Internal error.");
	}
	delete [] blah;
	return b;
}

image *xLoadImage0(char *name)
{
	byte *blah;
	image *b;
	
	width=depth=0;
	imagetype = 255;
	DetermineFileType(name);
	if (imagetype == 255) err("%s is not a recognized image type.",name);
	switch (imagetype)
	{
		case 0: blah = LoadPCXBuf(name);
				if (!TwentyFourBit)
				{
					palconv[0]=255; palconv[1]=0; palconv[2]=255; 
					b = ImageFrom8bpp(blah, width, depth, palconv); 
					break;
				}
				else
				{
					b = ImageFrom24bpp(blah, width, depth);
					break;
				}
		case 1: blah = LoadGIFBuf(name); palconv[0]=255; palconv[1]=0; palconv[2]=255; b = ImageFrom8bpp(blah, width, depth, palconv); break;
		default: err("xLoadImage0: Internal error.");
	}
	delete [] blah;
	return b;
}

byte *xLoadImage8bpp(char *name)
{
	byte *blah;
		
	width=depth=0;
	imagetype = 255;
	DetermineFileType(name);
	if (imagetype == 255) err("%s is not a recognized image type.",name);
	switch (imagetype)
	{
		case 0: blah = LoadPCXBuf(name);
				if (TwentyFourBit)
					err("Cannot load 24bpp PCX in 8bit mode.");
				break;
		case 1: blah = LoadGIFBuf(name); break;
		default: err("xLoadImage8bpp: Internal error.");
	}
	return blah;
}
