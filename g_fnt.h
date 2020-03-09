class FNT
{
public:
	byte *imagedata;
	int width, height;
	int subsets, selected;
	int totalframes;
	
	FNT(char *fname);
	~FNT();
	void Render(int x, int y, int frame, image *dest);
	void Print(int x, int y, char *str, ...);
	void PrintRaw(int x, int y, char *str);
	int uncompress(byte* dest, int len, char *buf);
};

extern FNT *fnts[8];
extern int numfonts;
extern int my_x, my_y;

void Font_GotoXY(int x, int y);
int LoadFont(char *fname);
void PrintText(int font, char *str);
