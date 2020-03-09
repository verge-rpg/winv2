#define MIKMOD_STATIC 1
#include "mikmod.h"
#include "mdsfx.h"

extern char playingsng[];
extern bool UseSound;

typedef struct SampleCacheList
{   MD_SAMPLE  *handle;
    CHAR    *name;
    struct SampleCacheList *prev, *next;
} SampleCacheList;

typedef struct SampleCache
{   SampleCacheList *list;
} SampleCache;


extern SampleCache *VCsounds;
extern MD_SAMPLE *samples[100];
extern int vcsamples;

void SuspendMusic();
void ResumeMusic();


void InitSound();
void PlayMusic(const char *sng, bool cache);
void StopMusic();
void ResetMusic();

SampleCache *NewSampleCache(void);
void DeleteSampleCache(SampleCache *sbank);

MD_SAMPLE *CacheSample(SampleCache *sbank, const char *si);
void FreeAllSamples(void);
int PlaySample(MD_SAMPLE *s);
int PlayMenuSample(MD_SAMPLE *s);

int  s_getglobalvolume();
int  s_getmusicvolume();
int  s_getsndfcvolume();
void s_setglobalvolume(int);
void s_setmusicvolume(int);
void s_setsndfxvolume(int);

void CacheMusic(const char *sng);
void s_CacheStartSession(void);
void s_CacheStopSession(void);

void UpdateSound(void);
