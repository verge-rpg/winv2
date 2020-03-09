void LoadSystemVC();
void ExecuteEvent(int i);
void RunVCAutoexec();
void LoadMapVC(VFILE *f);
void FreeMapVC();
void HookRetrace();
extern char kill;
extern image *vcbuf;
extern int vclayer;