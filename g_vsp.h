struct vspanim_r
{
  word start;                        // strand start
  word finish;                       // strand end
  word delay;                        // tile-switch delay
  word mode;                         // tile-animation mode
};

class VSP
{
public:
	int  numtiles;
	quad mytimer;

	VSP(char *fname);
	~VSP();
	void blit(int x, int y, int tile, image *dest);
	void tblit(int x, int y, int tile, image *dest);
	void CheckTileAnimation();

	byte *vspspace;
	int  bppmultiplier;
	vspanim_r vspanim[100];
	int vadelay[100];
	int *tileidx;
	int *flipped;

	void LoadAnimation(VFILE *f);
	void Animate(byte);
	void AnimateTile(byte, int);
	void DecodeByteCompression(byte *dest, int len, byte *buf);
	void LoadVSP(char *fname);	
};
