#define MIKMOD_STATIC 1
#include "xerxes.h"
#include "mikmod.h"
#include "mplayer.h"
#include "mdsfx.h"


class CacheEntry
{   
public:
    class CacheEntry *next, *prev;   // oh my a linked list!!
    char             *name;          // resource name/identifier

	~CacheEntry()
	{
		if(prev) prev->next = next;
		if(next) next->prev = prev;

		if(name) free(name);
	}


	bool CheckFor(const CacheEntry *targ)
	// See if the cache entry 'targ' exists in the given list!
	// Returns: 0 if not found.  1 if found.
	{
		CacheEntry *cruise;

		cruise = this;
		while(cruise)
		{   if(!strcmp(cruise->name, targ->name)) return 1;
			cruise = cruise->next;
		}
		return 0;
	}


	bool CheckFor(const char *res)
	// See if the resource name 'res exists in the given cache list.
	// Returns: 0 if not found.  1 if found.
	{
		CacheEntry *cruise;

		cruise = this;
		while(cruise)
		{   if(!strcmp(cruise->name, res)) return 1;
			cruise = cruise->next;
		}
		return 0;
	}


	void FreeList()
	{
		CacheEntry  *cruise;

		cruise = this;
		while(cruise)
		{   CacheEntry *old = cruise->next;
			delete cruise;
			cruise = old;
		}
	}
};

class SongCacheEntry:public CacheEntry
{
public:
    UNIMOD      *mf;

    SongCacheEntry(SongCacheEntry *list, const char *n)
    {   name       = strdup(n);
        next       = list;
        prev       = NULL;
        if(list) list->prev = this;
    };

    SongCacheEntry *Next()
    {   return (SongCacheEntry *)next;
    };

    UNIMOD *Fetch(const char *res)
    {
        // Search the list for the given resource
        return NULL;
    }

    UNIMOD *Fetch(const SongCacheEntry *res)
    {
        // Search the list for the given resource
        
        SongCacheEntry *cruise = this;
        
        while(cruise)
        {   SongCacheEntry *tmp = cruise->Next();
            if(!strcmp(res->name, cruise->name)) return cruise->mf;
            cruise = tmp;
        }
        return NULL;
    }
};


static void SampleCache_NewEntry(SampleCache *sbank, const char *si, MD_SAMPLE *serm)
{   SampleCacheList *neiu;
    
    neiu  = (SampleCacheList *)calloc(sizeof(SampleCacheList),1);
    neiu->name  = strdup(si);
    neiu->next  = sbank->list;

    if(sbank->list) sbank->list->prev = neiu;
    neiu->handle = serm;
}


static MD_SAMPLE *FetchSample(const SampleCache *leud, const CHAR *resid)
{
    // Search the list for the given resource
        
    SampleCacheList *cruise = leud->list;
        
    while(cruise)
    {   SampleCacheList *tmp = cruise->next;
        if(!strcmp(resid, cruise->name)) return cruise->handle;
        cruise = tmp;
    }
    return NULL;
}

/*****************************/
//    ---> Publics <---

char playingsng[80] = {0};
int  numsamples     = 0;
bool UseSound       = false;
bool music          = false;

/*****************************/
//    ---> Privates <---

static MDRIVER          *md;
static MPLAYER          *mp;
static UNIMOD           *mf;
static MD_VOICESET      *vs_global, *vs_music, *vs_sndfx, *vs_menu;
static SongCacheEntry   *m_curcache, *m_newcache;

struct
{   MPLAYER     *mp[8];
    char        sng[8][80];
    int         pos;
} fifo;

static bool Session;


static void ShutdownSound(void)
{
    //threadActive = 0;
    Mikmod_Exit(md);
}


static void errorhandler(int merrnum, const CHAR *merrstr)
{
    // the addition of a dialog box on certain error types might be a
    // useful 'user-friendly' idea.  Something to think about later...
    
    log("Mikmod Error %d > %s", merrnum, merrstr);
    
    UseSound = false;
    return;
}

static void killmodule(void)
// Stops playing and unloads the current module.
{
    music = false;
    
    if (mf)
    {    // see if our resource is cached -- if not, then unload the song.
        if(mp)
        {   Player_Stop(mp);
		    Player_Free(mp);
            mp = NULL;
        }
        
        if(!m_curcache->CheckFor(playingsng))
        {   Unimod_Free(mf);
		    mf = NULL;
        }
	}
}

static bool FetchSongCache(SongCacheEntry *list, const char *res)
// See if the resource name 'res exists in the given cache list.  If so, it
// sets the mf global to that resource, so that it may be played.
// Returns: 0 if not found.  1 if found.
{
    SongCacheEntry *cruise;

    cruise = list;
    while(cruise)
    {   if(!strcmp(cruise->name, res))
        {   mf = cruise->mf;
            return 1;
        }
        cruise = cruise->Next();
    }
    return 0;
}


// ================================
//     Initialization and Stuff
// ================================

SampleCache *VCsounds;
MD_SAMPLE *samples[100];
int vcsamples = 0;

void nullLog(const char *s, ...)
{
}

void InitSound()
{
	fifo.pos   = 0;
    fifo.mp[0] = NULL;

    mp         = NULL;
    mf         = NULL;
    
    UseSound   = true;
    Session    = false;

    m_curcache = NULL;

    _mmlog_init(nullLog, NULL); 
    Mikmod_RegisterErrorHandler(errorhandler);

    Mikmod_RegisterLoader(load_it);
	Mikmod_RegisterLoader(load_xm);
	Mikmod_RegisterLoader(load_s3m);
	Mikmod_RegisterLoader(load_mod);
	Mikmod_RegisterAllDrivers();
	
    // AirChange:
    //  the new buffer size is set to 60 milliseconds, which is adequate for testing
    //  purposes.  We will want to toy with a 40 ms buffer later to see how stable it
    //  is on various hardware.

    // Notes:
    //  - We force dynamic sample support.  This is mostly a gesture of future expandability.

    md = Mikmod_Init(44010, 80, NULL, MD_STEREO, 0, DMODE_16BITS | DMODE_INTERP | DMODE_SAMPLE_DYNAMIC | DMODE_NOCLICK);

    // Blackstar voiceset organization:
    // Notes:
    // - The global voiceset has no voices since everything will be assignd to one
    //   of its child voicesets.
    // - The music voiceset has no voices since those will be allocated by the player.
    // - the sound effects are differentiated from the menu system sounds so that we
    //   can preempt all in-game sound effects when the user enters a menu.

    vs_global = Voiceset_Create(md,NULL,0,0);
    vs_music  = Voiceset_Create(md,vs_global,0,0);
    vs_sndfx  = Voiceset_Create(md,vs_global,8,0);
    vs_menu   = Voiceset_Create(md,vs_global,2,0);
    
    // We should load and set configured volume levels for each class
    // of sound here... (or somewhere)
    
	// VEC: Added menu sound bank / default caches.

	VCsounds = NewSampleCache();
    atexit(ShutdownSound);
}

void UpdateSound(void)
{
    if (!UseSound) return;
    Mikmod_Update(md);
}

/* ==========================================================================
  *** Music Manipulation Interface

  *** Proposed functionality method for the music system:

  PlayMusic:
  A call to PlayMusic will check the in-memory song cache to see if the re-
  quested song is already loaded.  If not, it will attempt to load the song
  using the vfile packfile interface.  If that fails, it will attempt to
  load the song from the OS filesystem.  If the second param ('cache') is set
  to true, then the song will be loaded into the precache area and will not be
  unloaded when stopped.  This allows us to optionally distribute some of the
  loading process.

  CacheMusic:
  Caches the given song.  Loads it using the same search order as PlayMusic.
  Cached songs are only unloaded when the engine called FreeAllMusic();
  However, it is usually better to let the sound system play it smart. --> See
  'StartCacheSession'

  s_CacheStartSession:
  This applies to both music and sound effects.  When the precaching session
  is started, all calls to CacheMusic are tracked.  Afterward, the user calls
  s_CacheEndSession, which will compare the requested precache setup to the
  current one - unloading anything not in the new precache and loading anything
  that isn't already.  This allows us to utilize pre-loaded data with maximum
  efficiency.

  FreeAllMusic()
  Unloads all music in the cache, and whatever might be playing at the moment.
  Generally speaking, this shouldn't be used in favor of the much smarter music
  manamgement system offered via Cache Sessions.  But, you never know. :)


  ===========================================
*/

void s_CacheStartSession(void)
{
    m_newcache = NULL;
    Session = true;
}

void s_CacheStopSession(void)
{
    if (!UseSound) return;
    SongCacheEntry *cruise;

    // Compare the two linked lists -- currently cached and 'to be cached.'
    // and take appropriate action!

    // Step one, look for anything to unload.
    
    cruise = m_curcache;
    while(cruise)
    {   SongCacheEntry *tmp = cruise->Next();
        if((!m_newcache) || (!m_newcache->CheckFor(cruise)))
        {   // this resource isn't in the newcache list so lets unload it.
            Unimod_Free(cruise->mf);
        }
        cruise = tmp;
    }

    // Step Two, look for anything to load.
    
    cruise = m_newcache;
    while(cruise)
    {   SongCacheEntry *tmp = cruise->Next();
        if((!m_curcache) || (cruise->mf = m_curcache->Fetch(cruise)) == NULL)
        {   // this resource isn't in the curcache list so lets load it.
            cruise->mf = Unimod_Load(md,cruise->name);
        }
        cruise = tmp;
    }

    // free up the curcache and move newcache over it.

    m_curcache->FreeList();
    m_curcache = m_newcache;

    Session = false;
}

static bool loadmodule(const char *sng)
{
    // We don't know how long it could take to load the song,
    // so lets clear out the mixing buffer.
    Mikmod_WipeBuffers(md);
        
    // Load the song and add it to the list of loaded resources.
    mf = Unimod_Load(md,sng);
    if (!mf)
    {   err("Could not load module %s", sng);
        return 1;
    }
    return 0;
}

void CacheMusic(const char *sng)
{
    if (!UseSound) return;

    if(Session)
    {   // Put this song onto the 'NewCache' songlist to be loaded later.
        m_newcache = new SongCacheEntry(m_newcache, sng);
    } else
    {   loadmodule(sng);
        m_curcache     = new SongCacheEntry(m_curcache, sng);
        m_curcache->mf = mf;
    }
}

void PlayMusic(const char *sng, bool cache)
{
	if (!UseSound) return;
	if (!strlen(sng)) return;
	if (!strcmp(sng, playingsng)) return;
	
    killmodule();

    strcpy(playingsng, sng);

    if(!FetchSongCache(m_curcache,sng))
    {   // song is not cached, so load it up ...
        if(cache) CacheMusic(sng); else loadmodule(sng);
    }
    
    if(mf)
    {   mp = Player_InitSong(mf, vs_music, PF_LOOP, 24);
        Player_Start(mp);
        music = true;
    }
}

void StopMusic()
{
	if (!UseSound) return;

    killmodule();
    playingsng[0]=0;
}

void ResetMusic()
{
	if (!UseSound) return;
	Player_Cleaner(mp);
}

// --->  Music Suspension  <---

void SuspendMusic()
// Nested high-level music suspension stuff.  This sucker suspends the currently
// active song and puts it on the suspended-songs FIFO.  A call to ResumeMusic
// wipes it.
{
    if(mf && mp)
    {   Player_Pause(mp,0);    // pause with complete silence
        fifo.mp[fifo.pos] = mp;
        strcpy(fifo.sng[fifo.pos],playingsng);

        fifo.pos++;
        mp = NULL;
        mf = NULL;
        playingsng[0] = 0;
    }
}

void ResumeMusic()
// Pick the first voiceset off the FIFO and resume it.  If the fifo is empty or
// the pointer is null, it'll be ok.  We're not stupid! ;)
// If a song is currently playing, we stop and unload it.
// **NEW** Fades the song in!
{
    killmodule();

    if(fifo.pos)
    {   fifo.pos--;
        if(fifo.mp[fifo.pos])
        {   //MusicFadeIn(mp=fifo.mp[fifo.pos]);
            Player_Start(mp = fifo.mp[fifo.pos]);
            Player_QuickFade(mp, 0, 128, 2);
            mf = (UNIMOD*)mp->mf;
			strcpy(playingsng, fifo.sng[fifo.pos]);
            music = true;
            fifo.mp[fifo.pos] = NULL;
        }
    }
}

void FreeAllMusic()
{
    SongCacheEntry *cruise;
  	
    if (!UseSound) return;

    cruise = m_curcache;
    while(cruise)
    {   SongCacheEntry  *tmp = cruise->Next();
        Unimod_Free(cruise->mf);
        delete cruise;
        cruise = tmp;
    }

}


// =================================
// *** Sound Effects Interface
//
// Because sound effects tend to be used in a far more frequent manner than
// music, loading the sounds returns a 'sample handle' which is used from
// that point on to reference the loaded sampledata.  This avoids overuse of
// a slower string-ID system (based on filename).
//
// Currently, a sample must always be cached in order to be played.  However,
// the possibility of making a sample-oneshot function (plays a sample given
// a filename), isn't impossible.  If it is ever needed, just ask and I will
// will code it up.
//
// Caching:
// =================================

SampleCache *NewSampleCache(void)
{
    SampleCache *neiu;
    
    neiu = (SampleCache *)calloc(sizeof(SampleCache),1);
    return neiu;
}

void DeleteSampleCache(SampleCache *sbank)
{
    SampleCacheList *cruise;

    if(!UseSound) return;
    if(!sbank) return;

    cruise = sbank->list;
    while(cruise)
    {   SampleCacheList *tmp = cruise->next;
        mdsfx_free(cruise->handle);
        free(cruise);
        cruise = tmp;
    }

    sbank->list = NULL;
}


MD_SAMPLE *CacheSample(SampleCache *sbank, const char *si)
{
	MD_SAMPLE           *serm;

    if(!UseSound) return NULL;
    if(!sbank) return NULL;

    // Check if our sample is already loaded first.  If so, just duplicate it!

    if(serm=FetchSample(sbank, si))
        mdsfx_duplicate(serm);
    else
    {   serm  = mdsfx_loadwav(md, si);
        SampleCache_NewEntry(sbank, si, serm);
    }
    return serm;
}

int PlaySample(MD_SAMPLE *s)
{
    if (!UseSound) return 0;
    mdsfx_playeffect(s, vs_sndfx, 0, 0);
    return 0;
}


int PlayMenuSample(MD_SAMPLE *s)
{
	if (!UseSound) return 0;
    mdsfx_playeffect(s, vs_menu, SF_START_BEGIN, 0);
    return 0;
}


// ===========================================================================
// Volume set/retrive!!
//
// I assume you want all volumes range from 0 to 100, since you were returning
// 100 as the default music volume.  I made a macro anyway, so we can change
// thevolume scale with ease!

#define VOLSCALE  100

#define bs_setvol(x,a) Voiceset_SetVolume(x, (a * 128) / VOLSCALE)
#define bs_getvol(x)   ((x->volume * VOLSCALE) / 128)

int s_getglobalvolume()
{
    return bs_getvol(vs_global);
}

void s_setglobalvolume(int v)
{
    bs_setvol(vs_global, v);
}

int s_getmusicvolume()
{
    return bs_getvol(vs_music);
}

void s_setmusicvolume(int v)
{
    bs_setvol(vs_music, v);
}


int s_getsndfxvolume()
{
    return bs_getvol(vs_sndfx);
}

void s_setsndfxvolume(int v)
{
    bs_setvol(vs_sndfx, v);
}
