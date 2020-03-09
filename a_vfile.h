#define vscanf _vscanf

struct VFILE
{
  FILE *fp;                           // real file pointer.
  quad i;                             // which file index in vfile is it?
  char s;                             // 0=real file 1=vfile;
  char v;                             // if vfile, which vfile index
};

struct filestruct
{
  unsigned char fname[84];            // pathname thingo
  int size;                           // size of the file
  int packofs;                        // where the file can be found in PACK
  int curofs;                         // current file offset.
  char extractable;                   // irrelevant to runtime, but...
  char override;                      // should we override?
};

struct mountstruct
{
  char mountname[80];                 // name of VRG packfile.
  FILE *vhandle;                      // Real file-handle of packfile.
  struct filestruct *files;           // File record array.
  int numfiles;                       // number of files in pack.
  int curofs;                         // Current filepointer.
};

extern mountstruct pack[3];
extern char filesmounted;

int Exist(char *fname);
VFILE *vopen(char *fname);
void MountVFile(char *fname);
void vread(void *dest, int len, VFILE *f);
void vclose(VFILE *f);
int filesize(VFILE *f);
void vseek(VFILE *f, int offset, int origin);
void vscanf(VFILE *f, char *format, char *dest);
void vscanf(VFILE *f, char *format, int *dest);
char vgetc(VFILE *f);
word vgetw(VFILE *f);
void vgets(char *str, int len, VFILE *f);
int vtell(VFILE* f);