class CHR
{
public:
	image *rawdata, *container;

	word fxsize, fysize;                // frame x/y dimensions
	word hx, hy;                        // x/y obstruction hotspot
	word hw, hh;
	word totalframes;                   // total # of frames.
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