void LoadSystemVC();
void ExecuteEvent(int i);
void RunVCAutoexec();
void LoadMapVC(VFILE *f);
void FreeMapVC();
void HookRetrace();
void HookKey(int script);
extern char kill;
extern image *vcbuf;
extern int vclayer;
extern int bindarray[256];