/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "snd"

#include "sound.h"   //
#include "debug.h"   // for debugging prints
#include "cfgdef.h"  // for APP_NAME, ...
#include "global.h"  // for Run struct, ....
#include "config.h"  // for Config struct, ...
#include "loadlib.h" // for runtime loading
#include "utility.h" // for g_isa_dir, g_isa_file, ...

#if      _USEGTK
#if      !GTK_CHECK_VERSION(3,0,0)
#include <canberra-gtk.h> // TODO: make canberra use runtime dependent, i.e., use its shared library
#endif
#endif

#if      _USEGTK
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#if       GTK_CHECK_VERSION(3,0,0)

void snd::init() {}
void snd::dnit() {}
void snd::play_alarm(bool newPlay)                 {}
void snd::play_chime(bool newMinute, bool newHour) {}

#else //  GTK_CHECK_VERSION(3,0,0)

namespace snd
{

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static bool play_file(const char* sndFile, bool& playIt);
static void ca_finish_cb(ca_context* c, uint32_t id, int error_code, void* userData);

static const char* g_rcpath = "/." APP_NAME "/sounds";

static const char* gCfg_spath     = "westminster_chimes"; // TODO: put all of these in cfg
static const bool  gCfg_onceAlarm =  true;                // play alarm once if true or for 1 minute
static const bool  gCfg_onceChime =  true;                // one chime per hour if true or chime H times per hour, where H is the hour #
static const char* gCfg_alrmChime = "alarm.wav";
static const char* gCfg_qtr0Chime = "hrChime.wav";
static const char* gCfg_qtr1Chime = "qtrChime.wav";
static const char* gCfg_qtr2Chime = "halfChime.wav";
static const char* gCfg_qtr3Chime = "qtr3Chime.wav";

static lib::LoadLib* g_pLLib1   = NULL;
static lib::LoadLib* g_pLLib2   = NULL;
static bool          g_pLLibok  = false;
static lib::symb     g_symbs1[] =
{
	{ "ca_context_play_full", NULL },
	{ "ca_proplist_create",   NULL },
	{ "ca_proplist_destroy",  NULL },
	{ "ca_proplist_sets",     NULL }
};
static lib::symb     g_symbs2[] =
{
	{ "ca_gtk_context_get",   NULL }
};

typedef int         (*CCPF)(ca_context* c, uint32_t id, ca_proplist* p, ca_finish_callback_t cb, void* userdata);
typedef int         (*CPC) (ca_proplist** p);
typedef int         (*CPD) (ca_proplist*  p);
typedef int         (*CPS) (ca_proplist*  p, const char* key, const char* value);
typedef ca_context* (*CGCG)();

static CCPF g_ca_context_play_full;
static CPC  g_ca_proplist_create;
static CPD  g_ca_proplist_destroy;
static CPS  g_ca_proplist_sets;
static CGCG g_ca_gtk_context_get;

} // namespace snd

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void snd::init()
{
	DEBUGLOGB;

	g_pLLib1  = lib::create();
	g_pLLib2  = lib::create();
	g_pLLibok = g_pLLib1 && lib::load(g_pLLib1, "canberra",     ".0", g_symbs1, vectsz(g_symbs1)) &&
	            g_pLLib2 && lib::load(g_pLLib2, "canberra-gtk", ".0", g_symbs2, vectsz(g_symbs2));

	if( g_pLLibok )
	{
		DEBUGLOGS("canberra lib loaded ok - before typedef'd func address setting");

		bool
		okay  = true;
		okay &= lib::func(g_pLLib1, LIB_FUNC_ADDR(g_ca_context_play_full));
//		okay &= lib::func(g_pLLib1, LIB_FUNC_ADDR(g_ca_gtk_context_get)); // for testing - should fail here
		okay &= lib::func(g_pLLib1, LIB_FUNC_ADDR(g_ca_proplist_create));
		okay &= lib::func(g_pLLib1, LIB_FUNC_ADDR(g_ca_proplist_destroy));
		okay &= lib::func(g_pLLib1, LIB_FUNC_ADDR(g_ca_proplist_sets));
//		okay &= lib::func(g_pLLib1, LIB_FUNC_ADDR(g_ca_gtk_context_get)); // for testing - should fail here
		okay &= lib::func(g_pLLib2, LIB_FUNC_ADDR(g_ca_gtk_context_get));

		DEBUGLOGS("canberra lib loaded ok - after  typedef'd func address setting");
		DEBUGLOGP("all lib funcs %sretrieved\n", okay ? "" : "NOT ");

		if( !okay )
			dnit();
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void snd::dnit()
{
	DEBUGLOGB;

	if( g_pLLib2 )
		lib::destroy(g_pLLib2);
	if( g_pLLib1 )
		lib::destroy(g_pLLib1);

	g_pLLibok = false;
	g_pLLib2  = NULL;
	g_pLLib1  = NULL;

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void snd::play_alarm(bool newPlay)
{
	static bool playIt  = true;
	static int  playCnt = 0;

	if( !playIt || (!newPlay && playCnt <= 0) )
		return;

	DEBUGLOGB;

	char     fpath[PATH_MAX];
	snprintf(fpath, vectsz(fpath), "%s/%s/", g_rcpath, gCfg_spath);
	const char* spath = get_home_subpath(fpath, strlen(fpath), true);

	if( spath )
	{
		char     sndFile[PATH_MAX];
		snprintf(sndFile, vectsz(sndFile), "%s%s", spath, gCfg_alrmChime);

		if( play_file(sndFile, playIt) )
		{
			if( newPlay )
				playCnt = gCfg_onceAlarm ? 1 : 60; // once or for 1 minute (assuming a 1 second play time)
			playCnt--;
		}

		delete [] spath;
		spath = NULL;
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void snd::play_chime(bool newMinute, bool newHour)
{
	static char sndFile[PATH_MAX];
	static bool playIt  = true;
	static int  playCnt = 0;

	if( !playIt || (!newMinute && playCnt <= 0) )
		return;

	DEBUGLOGB;

	bool playSnd =  playCnt > 0;

	if( !playSnd && newMinute )
	{
		static const char* chimes[] = { gCfg_qtr0Chime, gCfg_qtr1Chime, gCfg_qtr2Chime, gCfg_qtr3Chime };
		static int         ctimes[] = { 0,              15,             30,             45 };

		for( size_t c = 0; c < vectsz(chimes); c++ )
		{
			if( gRun.timeCtm.tm_min == ctimes[c] )
			{
				char     fpath[PATH_MAX];
				snprintf(fpath, vectsz(fpath), "%s/%s/", g_rcpath, gCfg_spath);
				const char* spath = get_home_subpath(fpath, strlen(fpath), true);

				if( playSnd = spath != NULL )
				{
					snprintf(sndFile, vectsz(sndFile), "%s%s", spath, chimes[c]);
					delete [] spath;
					spath = NULL;
				}

				break;
			}
		}
	}

	if( playSnd && play_file(sndFile, playIt) )
	{
		if( newMinute )
			playCnt = gCfg_onceChime || newHour ? 1 : gRun.timeCtm.tm_hour;
		playCnt--;
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool snd::play_file(const char* sndFile, bool& playIt)
{
	DEBUGLOGB;
	bool ret = false;

	if( g_pLLibok )
	{
		if( ret = g_isa_file(sndFile) )
		{
			DEBUGLOGP("playing sound file *%s*\n", sndFile);

			ca_proplist* pl = NULL;
#if 1
			g_ca_proplist_create(&pl);
			g_ca_proplist_sets(pl, CA_PROP_MEDIA_FILENAME, sndFile);
			g_ca_context_play_full(g_ca_gtk_context_get(), 0, pl, ca_finish_cb, &(playIt=false));
			g_ca_proplist_destroy(pl);
#else
			ca_proplist_create(&pl);
			ca_proplist_sets(pl, CA_PROP_MEDIA_FILENAME, sndFile);
			ca_context_play_full(ca_gtk_context_get(), 0, pl, ca_finish_cb, &(playIt=false));
			ca_proplist_destroy(pl);
#endif
			pl = NULL;
		}
	}

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
void snd::ca_finish_cb(ca_context* c, uint32_t id, int error_code, void* userData)
{
	if( userData )
	{
		bool* b = (bool*)userData;
		*b      =  true;
	}
}

#endif //  GTK_CHECK_VERSION(3,0,0)

#else  // _USEGTK

void snd::init() {}
void snd::dnit() {}
void snd::play_alarm(bool newPlay)                 {}
void snd::play_chime(bool newMinute, bool newHour) {}

#endif // _USEGTK

