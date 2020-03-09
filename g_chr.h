class CHR
{
public:
	byte *imagedata;

	int fxsize, fysize;                // frame x/y dimensions
	int hx, hy;                        // x/y obstruction hotspot
	int hw, hh;
	int totalframes;                   // total # of frames.
    int idle[4];                       // way handier

	char lanim[100];
	char ranim[100];
	char uanim[100];
	char danim[100];

	CHR(char *fname);
	~CHR();
	void Render(int x, int y, int frame, image *dest);
	int uncompress(byte* dest, int len, char *buf);
};