void LoadSystemVC();
void ExecuteEvent(int i);
void RunVCAutoexec();
void LoadMapVC(VFILE *f);
void FreeMapVC();
void HookRetrace();
void HookKey(int script);
void HookTimer();

void vc_WriteVars(FILE *f);
void vc_ReadVars(FILE *f);

extern char kill;
extern int bindarray[256], vc_paranoid, vc_arraycheck;