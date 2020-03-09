class FNT
{
public:
	image *rawdata, *container;
	int width, height;
	int subsets, selected;
	int totalframes;
	
	FNT(char *fname);
	~FNT();
	void Render(int x, int y, int frame, image *dest);
	void Print(int x, int y, char *str, ...);
};

extern FNT *fnts[8];

void Font_GotoXY(int x, int y);
int LoadFont(char *fname);
void PrintText(int font, char *str);
