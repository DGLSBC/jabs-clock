/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP    "cfg"

#include <stdio.h>    // for ?
#include <malloc.h>   // for memory functions
#include <locale.h>   // for ?
#include <stdlib.h>   // for abs
#include <langinfo.h> // for ?

#include "config.h"   // for Config struct, CornerType enum, ...
#include "cfgdef.h"   // for APP_NAME
#include "global.h"   // for gCfg extern (and vectsz)
#include "handAnim.h" // for hand_anim_get/set
#include "utility.h"  // for get_home_subpath and strcrep
#include "tzone.h"    // for timezone related
#include "debug.h"    // for debugging prints
#include "draw.h"     // for ANIM_FLICK

// NOTE:  since cfg usage may occur before globally (gCfg) recognizing the
//        debug switch (-D), the normally off debug logging is forced on here
//        if the DEBUGLOG switch is on.

#ifdef    DEBUGLOG
#undef    DEBUGLOGB
#undef    DEBUGLOGP
#undef    DEBUGLOGS
#undef    DEBUGLOGE
#define   DEBUGLOGB DEBUGLOGBF
#define   DEBUGLOGP DEBUGLOGPF
#define   DEBUGLOGS DEBUGLOGSF
#define   DEBUGLOGE DEBUGLOGEF
#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
//static const char* verstr = "# Version 0.4.0. This file is app-managed. User changes are not supported or encouraged.";
//static const int   versln =  12; // # of chars at beginning of version string length to use to determine version #

Config gCfg;

// -----------------------------------------------------------------------------
namespace cfg
{
	enum varID
	{
		CFGVAR_X,
		CFGVAR_Y,
		CFGVAR_W,
		CFGVAR_H,
		CFGVAR_SS,
		CFGVAR_SD,
		CFGVAR_T,
		CFGVAR_KOT,
		CFGVAR_AIP,
		CFGVAR_AIT,
		CFGVAR_S,
		CFGVAR_24,
		CFGVAR_RR,
		// 0.4.0 added
		CFGVAR_WS,
		CFGVAR_C,
		CFGVAR_R,
		CFGVAR_FD1,
		CFGVAR_KOB,
		CFGVAR_TP,
		CFGVAR_O,
		CFGVAR_FS,
		CFGVAR_FOY,
		CFGVAR_FTR,
		CFGVAR_FTG,
		CFGVAR_FTB,
		CFGVAR_FTA,
		CFGVAR_FSR,
		CFGVAR_FSG,
		CFGVAR_FSB,
		CFGVAR_FSA,
		CFGVAR_FSW,
		CFGVAR_FP,
		CFGVAR_FN,
		CFGVAR_FD12,
		CFGVAR_FD24,
		CFGVAR_FT12,
		CFGVAR_FT24,
		CFGVAR_TT12,
		CFGVAR_TT24,
		CFGVAR_AS,
		CFGVAR_QD,
		CFGVAR_RSC,
		CFGVAR_SA,
		CFGVAR_SH,
		CFGVAR_SM,
		CFGVAR_STT,
		CFGVAR_SHT,
		CFGVAR_PS,
		CFGVAR_PA,
		CFGVAR_PC,
		CFGVAR_ALS,
		CFGVAR_ALDS,
		CFGVAR_ALMS,
		CFGVAR_ALLS,
		CFGVAR_OH1,
		CFGVAR_OH2,
		CFGVAR_OH3,
		CFGVAR_OH4,
		CFGVAR_US1,
		CFGVAR_US2,
		CFGVAR_US3,
		CFGVAR_US4,
		CFGVAR_UPI,
		CFGVAR_A24,
		CFGVAR_BGR,
		CFGVAR_BGG,
		CFGVAR_BGB,
		CFGVAR_LAT,
		CFGVAR_LON,
		CFGVAR_GHO,
		CFGVAR_GMO,
		CFGVAR_SHO,
		CFGVAR_SMO,
		CFGVAR_TZN,
		CFGVAR_TZSN,
		CFGVAR_TAS1,
		CFGVAR_TAS2,
		CFGVAR_TAS3,
		CFGVAR_TBS1,
		CFGVAR_TBS2,
		CFGVAR_TBS3,
		CFGVAR_TAH1,
		CFGVAR_TAH2,
		CFGVAR_TAH3,
		CFGVAR_TBH1,
		CFGVAR_TBH2,
		CFGVAR_TBH3
	};

	struct CfgVar
	{
		varID       idx;
		const char* fmt;
		void*       var;
		int         len;
	};

	static bool load(Config& cfgData, const char* cfgPath, bool test=false);
	static bool save(Config& cfgData, const char* cfgPath, bool lock=false); // needs to temp mod Config fmt fields

	static int  fscanf(FILE* pFile, const char* fmt, void* val, int len);

	static const char* get_user_prefs_path(bool old, bool test=false);

	static int  getf(Config& cfgData, CfgVar** ppCfgVars);

	static bool get_latlon(double& lat, double& lon);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cfg::init()
{
	DEBUGLOGB;

	memset(&gCfg, 0, sizeof(gCfg)); // set all settings to 'off'

	gCfg.clockX      = 0;
	gCfg.clockY      = gCfg.clockX;
	gCfg.clockW      = 128;
	gCfg.clockH      = gCfg.clockW;
	gCfg.renderRate  = 16;
	// 0.4.0 added
	gCfg.queuedDraw  = 1;
	gCfg.faceDate    = 1;
	gCfg.showAlarms  = 1;
	gCfg.showHours   = 1;
	gCfg.showMinutes = 1;
	gCfg.showTTips   = 0;
	gCfg.shandType   = draw::ANIM_FLICK;
	gCfg.takeFocus   = 1;
	gCfg.opacity     = 1.00f;
//	gCfg.dateTxtRed  = 0.75f;
	gCfg.dateTxtRed  = 0.90f;
	gCfg.dateTxtGrn  = gCfg.dateTxtRed;
	gCfg.dateTxtBlu  = gCfg.dateTxtGrn;
	gCfg.dateTxtAlf  = 1.00f;
	gCfg.dateShdRed  = 0.10f;
	gCfg.dateShdGrn  = gCfg.dateShdRed;
	gCfg.dateShdBlu  = gCfg.dateShdGrn;
	gCfg.dateShdAlf  = 1.00f;
	gCfg.dateShdWid  = 0.10f;
	gCfg.fontOffY    = 1.00f;
	gCfg.fontSize    = 10;

	gCfg.sfTxta1     = 1.00f;
	gCfg.hfTxta1     = 0.00f;
//	gCfg.haTxta1     = 0.00f;

	gCfg.sfTxta2     = 0.50f;
	gCfg.hfTxta2     = 0.00f;
//	gCfg.haTxta2     = 0.00f;

	gCfg.sfTxta3     = 0.50f;
	gCfg.hfTxta3     = 0.00f;
//	gCfg.haTxta3     = 0.00f;

	gCfg.sfTxtb1     = 1.00f;
	gCfg.hfTxtb1     = 0.00f;
//	gCfg.haTxtb1     = 0.00f;

	gCfg.sfTxtb2     = 0.50f;
	gCfg.hfTxtb2     = 0.00f;
//	gCfg.haTxtb2     = 0.00f;

	gCfg.sfTxtb3     = 0.50f;
	gCfg.hfTxtb3     = 0.00f;
//	gCfg.haTxtb3     = 0.00f;

	gCfg.auto24      = 1;

//	if( true ) // TODO: chg this to ?
	{
#if 0
		// this only applies to the current locale
		const gchar* const* pLangs = g_get_language_names();
		const gchar* const* pLang  = pLangs;
		while( pLang && *pLang )
		{
			DEBUGLOGP("language %s available\n", *pLang);
			pLang++;
		}
#endif
//		locale -a gets a list of available locales - glib/gdk/gtk equivalent?

		char   tstr[64];
#if 0
		char*  oldloc  =  setlocale(LC_TIME, NULL);
#endif
		char*  newloc  =  setlocale(LC_TIME, "");
//		char*  newloc  =  setlocale(LC_TIME, "fr_FR.UTF8");
//		char*  newloc  =  setlocale(LC_TIME, "es_ES.UTF8");
#if 0
		if(   !newloc )
			   newloc  =  setlocale(LC_TIME, "");
#endif
		time_t ct      =  time(NULL);
		tm     lt      = *localtime(&ct);
		lt.tm_hour     =  14; // force to an hour that will be converted to either "02" or "14"
		mktime(&lt);
#if 0
		GDateTime* pGDT=  g_date_time_new_local(lt.tm_year, lt.tm_mon, lt.tm_mday, 14, lt.tm_min, lt.tm_sec);
		gchar*     pStr=  g_date_time_format(pGDT, "%X");
//		char*      pLng=  nl_langinfo(_NL_ADDRESS_LANG_NAME);
//		char*      pLng=  nl_langinfo(_NL_ADDRESS_POSTAL_FMT);
		char*      pLng=  nl_langinfo(_NL_ADDRESS_COUNTRY_NAME);
#endif
		strfmtdt(tstr, vectsz(tstr), "%X", &lt); // 'X' is supposedly locale specific

		gCfg.show24Hrs = tstr[0] != '0'; // default 12/24 setting to locale 'choice'
#if 0
		DEBUGLOGP("old locale       is %s\n",  oldloc);
		DEBUGLOGP("new locale       is %s\n",  newloc);
		DEBUGLOGP("clib time string is %s\n",  tstr);
		DEBUGLOGP("glib time string is %s\n",  pStr);
		DEBUGLOGP("language  string is %s\n",  pLng);
		DEBUGLOGP("12-hour locale   is %s\n", !gCfg.show24Hrs ? "in use" : "not in use");
		DEBUGLOGP("24-hour locale   is %s\n",  gCfg.show24Hrs ? "in use" : "not in use");
#endif
//		gCfg.show24Hrs = false;

//		setlocale(LC_TIME, oldloc);
#if 0
		g_date_time_unref(pGDT);
		g_free(pStr);
#endif
		// assume tm_gmtoff is in seconds as indicated in the libc docs
		gCfg.tzHorGmtOff = double(lt.tm_gmtoff/3600);
		gCfg.tzMinGmtOff = double(abs(lt.tm_gmtoff%3600)/60);

		get_latlon(gCfg.tzLatitude, gCfg.tzLngitude);

		DEBUGLOGP("set runtime gmt offset to %d:%2.2d\n",        (int)gCfg.tzHorGmtOff,   (int)gCfg.tzMinGmtOff);
		DEBUGLOGP("set runtime tz lat/lon to %2.2f, %2.2f\n", (double)gCfg.tzLatitude, (double)gCfg.tzLngitude);

/*		if( gCfg.tzLatitude == 0.0 && gCfg.tzLngitude == 0.0 )
		{
		gCfg.tzLatitude = 0; // set to the equator // TODO: any way to guess which N/S hemisphere?
		gCfg.tzLngitude = ((double)gCfg.tzHorGmtOff + (double)gCfg.tzMinGmtOff*60.0)*15.0;
		DEBUGLOGP("set runtime tz lat/lon to %2.2f, %2.2f\n", (double)gCfg.tzLatitude,  (double)gCfg.tzLngitude);
		}*/

		strfmtdt(gCfg.tzShoName, vectsz(gCfg.tzShoName), "%Z", &lt);
		DEBUGLOGP("got default shown tzname of *%s*\n", gCfg.tzShoName);
#if 1
		// TODO: need to find a default gCfg.tzName?
		//       possible to take localtime's tm::tm_gmtoff and use it to find
		//       the zone.tab entry that contains it, and thus also find that
		//       timezone's 'name', lat/lon (which must just be some sort of
		//       boundary/averaging values), etc?
		//       NOGO: too many entries in zone.tab with same gmt offset
		//             and no entries in zone.tab for zone abbreviations

		tzn::beg();

//		if( tzn::got(lt.tm_gmtoff) )
//		if( tzn::got(gCfg.tzShoName) )
		if( gCfg.tzShoName[0] )
		{
//bool get(const char* tzcode,    int* loc_offset, int* utc_offset, double* latitude=NULL, double* longitude=NULL, char* tzname=NULL, size_t tzn_len=0, char* comment=NULL, size_t cmt_len=0);
//bool get(long        tm_gmtoff, int* loc_offset, int* utc_offset, double* latitude=NULL, double* longitude=NULL, char* tzname=NULL, size_t tzn_len=0, char* comment=NULL, size_t cmt_len=0);
//			tzn::get(lt.tm_gmtoff, NULL, NULL, gCfg.tzName, vectsz(gCfg.tzName));
			double                                     lat,  lon;
			char                                                  tzName[96];
			if( tzn::get (gCfg.tzShoName, NULL, NULL, &lat, &lon, tzName, vectsz(tzName)) )
			{
				DEBUGLOGP("got default id tzname of *%s*\n", tzName);
				DEBUGLOGP("lat=%2.2f, lon=%2.2f\n",         (double)lat, (double)lon);
				strvcpy  (gCfg.tzName, tzName);
				gCfg.tzLatitude = lat;
				gCfg.tzLngitude = lon;
			}
		}
#if 0
		DEBUGLOGS("bef tzn::get calls");

		const char*          abb1 = "America/Chicago";
		int                        off1 = 0, utc1 = 0;
		char                                       tzc1[128];
		bool oka1 = tzn::get(abb1, off1,     utc1, tzc1, vectsz(tzc1));
		if( oka1 )  DEBUGLOGP("found timezone1 %s - label is %s, utc offset is %d\n", abb1, tzc1, utc1);
		else        DEBUGLOGP("didn't find timezone1 %s\n", abb1);

		const char*          abb2 = "America/Los_Angeles";
		int                        off2 = 0, utc2 = 0;
		char                                       tzc2[128];
		bool oka2 = tzn::get(abb2, off2,     utc2, tzc2, vectsz(tzc2));
		if( oka2 ) DEBUGLOGP("found timezone2 %s - label is %s, utc offset is %d\n", abb2, tzc2, utc2);
		else       DEBUGLOGP("didn't find timezone2 %s\n", abb2);

		DEBUGLOGS("aft tzn::get calls");

		if( oka1 && oka2 )
		{
			double of1c = (double) off1         /3600.0f;
			double of2c = (double) off2         /3600.0f;
			double utcd = (double)(utc2 -  utc1)/3600.0f; // 3600 converts from seconds to hours
			bool  less =           utc2 <  utc1;
			bool  same =           utc2 == utc1;
			const char* szo1 = "offset from";
			const char* szo2 = less ? "behind" : (same ? "offset from" : "ahead of");
			DEBUGLOGP("%s is %f (%f) hours %s %s\n", tzc1, 0.0f, of1c, szo1, tzc1);
			DEBUGLOGP("%s is %f (%f) hours %s %s\n", tzc2, utcd, of2c, szo2, tzc1);
		}
#endif
		tzn::end();
#endif
	}

	// TODO: the formats following need to be changed to work for all (TODO: huh? locales maybe?)

	strvcpy(gCfg.themeFile, "<simple>");
	strvcpy(gCfg.fmtDate12, "%a %b %d");
	strvcpy(gCfg.fmtDate24, "%a %d %b");
	strvcpy(gCfg.fmtTime12, "%-I:%M %P");
	strvcpy(gCfg.fmtTime24, "%-H:%M");
	strvcpy(gCfg.fmtTTip12, "%A%n%B %d %Y%n%-I:%M:%S %P");
	strvcpy(gCfg.fmtTTip24, "%A%n%B %d %Y%n%T");

	gCfg.alarms   [0] = '\0';
	gCfg.alarmDays[0] = '\0';
	gCfg.alarmMsgs[0] = '\0';
	gCfg.alarmLens[0] = '\0';

	for( size_t h = 0; h < vectsz(gCfg.optHand); h++ )
		gCfg.optHand[h]  = TRUE;

	for( size_t h = 0; h < vectsz(gCfg.useSurf); h++ )
		gCfg.useSurf[h]  = TRUE;

	gCfg.useSurf[0] = !gCfg.useSurf[0]; // alarm - draw to bkgnd surf, not own
	gCfg.useSurf[1] = !gCfg.useSurf[1]; // hour  - draw to bkgnd surf, not own
//	gCfg.useSurf[2] = !gCfg.useSurf[2]; // minute

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cfg::prep(Config& cfgData) // copy from gCfg into cfgData, but only non-cl switch vars
{
	DEBUGLOGB;

	cfgData.clockR      = gCfg.clockR;
	cfgData.queuedDraw  = gCfg.queuedDraw;
	cfgData.refSkipCnt  = gCfg.refSkipCnt;
	cfgData.refSkipCur  = gCfg.refSkipCur;
	cfgData.showAlarms  = gCfg.showAlarms;
	cfgData.showHours   = gCfg.showHours;
	cfgData.showMinutes = gCfg.showMinutes;
	cfgData.showTTips   = gCfg.showTTips;
	cfgData.shandType   = gCfg.shandType;
	cfgData.doSounds    = gCfg.doSounds;
	cfgData.doAlarms    = gCfg.doAlarms;
	cfgData.doChimes    = gCfg.doChimes;
	cfgData.dateTxtRed  = gCfg.dateTxtRed;
	cfgData.dateTxtGrn  = gCfg.dateTxtGrn;
	cfgData.dateTxtBlu  = gCfg.dateTxtBlu;
	cfgData.dateTxtAlf  = gCfg.dateTxtAlf;
	cfgData.dateShdRed  = gCfg.dateShdRed;
	cfgData.dateShdGrn  = gCfg.dateShdGrn;
	cfgData.dateShdBlu  = gCfg.dateShdBlu;
	cfgData.dateShdAlf  = gCfg.dateShdAlf;
	cfgData.dateShdWid  = gCfg.dateShdWid;
	cfgData.fontOffY    = gCfg.fontOffY;
	cfgData.fontSize    = gCfg.fontSize;
	cfgData.sfTxta1     = gCfg.sfTxta1;
	cfgData.hfTxta1     = gCfg.hfTxta1;
	cfgData.sfTxta2     = gCfg.sfTxta2;
	cfgData.hfTxta2     = gCfg.hfTxta2;
	cfgData.sfTxta3     = gCfg.sfTxta3;
	cfgData.hfTxta3     = gCfg.hfTxta3;
	cfgData.sfTxtb1     = gCfg.sfTxtb1;
	cfgData.hfTxtb1     = gCfg.hfTxtb1;
	cfgData.sfTxtb2     = gCfg.sfTxtb2;
	cfgData.hfTxtb2     = gCfg.hfTxtb2;
	cfgData.sfTxtb3     = gCfg.sfTxtb3;
	cfgData.hfTxtb3     = gCfg.hfTxtb3;
	cfgData.pngIcon     = gCfg.pngIcon;
	cfgData.auto24      = gCfg.auto24;
	cfgData.bkgndRed    = gCfg.bkgndRed;
	cfgData.bkgndGrn    = gCfg.bkgndGrn;
	cfgData.bkgndBlu    = gCfg.bkgndBlu;
	cfgData.tzLatitude  = gCfg.tzLatitude;
	cfgData.tzLngitude  = gCfg.tzLngitude;
	cfgData.tzHorGmtOff = gCfg.tzHorGmtOff;
	cfgData.tzMinGmtOff = gCfg.tzMinGmtOff;
	cfgData.tzHorShoOff = gCfg.tzHorShoOff;
	cfgData.tzMinShoOff = gCfg.tzMinShoOff;

	strvcpy(cfgData.themePath, gCfg.themePath);
	strvcpy(cfgData.fontFace,  gCfg.fontFace);
	strvcpy(cfgData.fontName,  gCfg.fontName);
	strvcpy(cfgData.fmtDate12, gCfg.fmtDate12);
	strvcpy(cfgData.fmtDate24, gCfg.fmtDate24);
	strvcpy(cfgData.fmtTime12, gCfg.fmtTime12);
	strvcpy(cfgData.fmtTime24, gCfg.fmtTime24);
	strvcpy(cfgData.fmtTTip12, gCfg.fmtTTip12);
	strvcpy(cfgData.fmtTTip24, gCfg.fmtTTip24);

	strvcpy(cfgData.alarms,    gCfg.alarms);
	strvcpy(cfgData.alarmDays, gCfg.alarmDays);
	strvcpy(cfgData.alarmMsgs, gCfg.alarmMsgs);
	strvcpy(cfgData.alarmLens, gCfg.alarmLens);

	strvcpy(cfgData.tzName,    gCfg.tzName);
	strvcpy(cfgData.tzShoName, gCfg.tzShoName);

	for( size_t h = 0; h < vectsz(gCfg.optHand); h++ )
		cfgData.optHand[h] = gCfg.optHand[h];

	for( size_t h = 0; h < vectsz(gCfg.useSurf); h++ )
		cfgData.useSurf[h] = gCfg.useSurf[h];

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool cfg::load()
{
	DEBUGLOGB;
	DEBUGLOGS("  (1)");

	const char* cfgPath;
	bool  okay, pfnd;

	cfgPath  =  get_user_prefs_path(false, true);          // look for v 0.4.0 new cfg first
	if( okay = (cfgPath != NULL) )
	{
		if( pfnd = g_isa_file(cfgPath) )                   // it's okay if it's missing since defs are used
		{
			DEBUGLOGS("  (1) found/using tst cfg path");
			okay = load(gCfg, cfgPath, true);
		}

		delete [] cfgPath;
		cfgPath = NULL;

		if( !pfnd )                                        // if v 0.4.0 new cfg not found
		{
			cfgPath  =  get_user_prefs_path(false);        // look for v 0.4.0 cfg first
			if( okay = (cfgPath != NULL) )
			{
				if( pfnd = g_isa_file(cfgPath) )           // it's okay if it's missing since defs are used
				{
					DEBUGLOGS("  (1) found/using new cfg path");
					okay = load(gCfg, cfgPath);
				}

				delete [] cfgPath;
				cfgPath = NULL;

				if( !pfnd )                                // if v 0.4.0 cfg not found
				{
					cfgPath  =  get_user_prefs_path(true); // look for older cfg
					if( okay = (cfgPath != NULL) )
					{
						if( pfnd = g_isa_file(cfgPath) )
						{
							DEBUGLOGS("  (1) found/using old cfg path");
							okay = load(gCfg, cfgPath);
						}

						delete [] cfgPath;
						cfgPath = NULL;
					}
				}
			}
		}
	}
#if 0
	if( !okay )
		DEBUGLOGS("There was an error while trying to read the preferences");
#endif
	DEBUGLOGS("  (1)");
	DEBUGLOGE;

	return okay;
}

// -----------------------------------------------------------------------------
bool cfg::load(Config& cfgData, const char* cfgPath, bool test)
{
	DEBUGLOGB;
	DEBUGLOGS("  (2)");

	bool okay = false;

	DEBUGLOGP("  (2) cfgPath is '%s'\n", cfgPath);

	if( test )
	{
		GKeyFile* pKF = g_key_file_new();

		if( pKF )
		{
			DEBUGLOGS("  (2) key file successfully created");

			if( okay = (g_key_file_load_from_file(pKF, cfgPath, G_KEY_FILE_KEEP_COMMENTS, NULL)) )
			{
				DEBUGLOGS("  (2) key file successfully loaded");
				DEBUGLOGS("  (2) bef getf call");

				const char* eql;
				char        key[1024],  val;
				const char  grp[]    = "General";
//				const char* grp      =  test ? "General" : "";
				int         f        =  0;
				CfgVar*     pCfgVars =  NULL;
				int         fmtn     =  getf(cfgData, &pCfgVars);

				DEBUGLOGP("  (2) aft getf call, fmtn is %d, pCfgVars is %p\n", fmtn, pCfgVars);

				if( pCfgVars && fmtn )
				{
					foreachv( i, fmtn )
					{
						char    fmt[1024], *eol;
						strvcpy(fmt, pCfgVars[f].fmt);
						if(  eol = strchr(fmt, '\n') )
							*eol = '\0';
						DEBUGLOGP("  (2) fmt %d is \"%s\"\n", i, fmt);

						eql = strchr(pCfgVars[f].fmt, '=');
						val = eql[2];

						strvcpy(key, pCfgVars[f].fmt, eql-pCfgVars[f].fmt);
						key[eql-pCfgVars[f].fmt] = '\0';

						if( g_key_file_has_key(pKF, grp, key, NULL) )
						{
#ifdef DEBUGLOG
							char*  vstr = g_key_file_get_value(pKF, grp, key, NULL);
							DEBUGLOGP("  (2) group %s, key %s, value (%c) %s\n", grp, key, val, vstr);
							g_free(vstr);
#endif
							switch( val )
							{
							case 'b': *((bool*)  pCfgVars[f].var) = (bool) (g_key_file_get_boolean(pKF, grp, key, NULL) == TRUE); break;
							case 'd': *((int*)   pCfgVars[f].var) = (int)   g_key_file_get_integer(pKF, grp, key, NULL);          break;
							case 'f': *((double*)pCfgVars[f].var) = (double)g_key_file_get_double (pKF, grp, key, NULL);          break;
							case 's':
								{
									char* vstr = g_key_file_get_string(pKF, grp, key, NULL);
									strvcpy((char*)pCfgVars[f].var, vstr, pCfgVars[f].len);
									g_free(vstr);
								}
								break;
							}
						}
						else
						{
							DEBUGLOGP("  (2) group %s, key %s, value (%c), missing\n", grp, key, val);
						}

						f++;
					}

					delete [] pCfgVars;
					pCfgVars = NULL;
				}
			}
			else
			{
				DEBUGLOGS("  (2) key file NOT successfully loaded");
			}

			g_key_file_free(pKF);
			pKF = NULL;
		}
		else
		{
			DEBUGLOGS("  (2) key file NOT successfully created");
		}
	}
	else // this is only good for cairo-clock 3.4 or less cfgs as it assumes a specific line ordering
	{
		FILE* pFile;

		if( okay = (cfgPath && (pFile = fopen(cfgPath, "r"))) )
		{
			int     res;
			int     f        = 0;
			CfgVar* pCfgVars = NULL;
			int     fmtn     = getf(cfgData, &pCfgVars);
			int     numf     = fmtn < 13 ? fmtn : 13;

			char*    line = NULL;
			size_t          lcnt = 0;
			getline(&line, &lcnt, pFile);

			if( line ) free(line);

			foreachv( i, numf )
			{
				DEBUGLOGP("  (2) fmt %d is \"%s\"\n", f, pCfgVars[f].fmt);
				okay &= (res = fscanf(pFile, pCfgVars[f].fmt, pCfgVars[f].var), pCfgVars[f].len) != EOF;
				DEBUGLOGP("  (2) fscanf %s\n", okay ? "good" : "bad");
#ifdef DEBUGLOG
				if( okay )
				{
					switch( pCfgVars[f].fmt[strlen(pCfgVars[f].fmt)-2] )
					{
					case 'd': DEBUGLOGP(", value is *%d*\n",  *((int*)pCfgVars[f].var)); break;
					case 's': DEBUGLOGP(", value is *%s*\n",   (char*)pCfgVars[f].var);  break;
					default : DEBUGLOGS(", format value type unknown");                  break;
					}
				}
#endif
				f++;
			}

			okay &= (res = fclose(pFile)) == 0;
		}
	}

	cfgData.refSkipCur = cfgData.refSkipCnt;

	DEBUGLOGP("config loaded theme path is *%s*\n", gCfg.themePath);
	DEBUGLOGP("config loaded theme file is *%s*\n", gCfg.themeFile);

	DEBUGLOGS("  (2)");
	DEBUGLOGE;

	return okay;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool cfg::save(bool lock, bool force)
{
	DEBUGLOGB;
	DEBUGLOGS("  (1)");

	bool        okay;
	const char* cfgPath  = get_user_prefs_path(false, true);

	if( okay = (cfgPath != NULL) )
	{
		DEBUGLOGP("  (1) tst cfgPath is \"%s\"\n", cfgPath);

		force |= gRun.cfgSaves;

//		if( gRun.hasParms )
		if( gRun.hasParms && !force )
		{
			DEBUGLOGS("  (1) cmdline parms given (and not forced to), so save not attempted");
		}
		else
		{
			DEBUGLOGS("  (1) cmdline parms not given (or forced to), so save attempted");

			if( !(okay = save(gCfg, cfgPath, lock)) )
			{
				DEBUGLOGS("  (1) There was an error while trying to save the preferences");
			}
		}

		delete [] cfgPath;
		cfgPath = NULL;
	}

	DEBUGLOGS("  (1)");
	DEBUGLOGE;

	return okay;
}

// -----------------------------------------------------------------------------
bool cfg::save(Config& cfgData, const char* cfgPath, bool lock)
{
	DEBUGLOGB;
	DEBUGLOGS("  (2)");

	bool      okay = false;
	GKeyFile* pKF  = g_key_file_new();

	if( pKF )
	{
		DEBUGLOGS("  (2) key file successfully created");

		g_key_file_load_from_file(pKF, cfgPath, G_KEY_FILE_KEEP_COMMENTS, NULL);

		Config
			 cfgTemp = cfgData;
		cnvp(cfgTemp,  false, lock);

		DEBUGLOGS("  (2) bef getf call");

		const char* eql;
		char        key[1024],  val;
		const char  grp[]    = "General";
		int         f        =  0;
		CfgVar*     pCfgVars =  NULL;
//		int         fmtn     =  getf(cfgData, &pCfgVars);
		int         fmtn     =  getf(cfgTemp, &pCfgVars);

		DEBUGLOGP("  (2) aft getf call, fmtn is %d, pCfgVars is %p\n", fmtn, (void*)pCfgVars);

		if( pCfgVars && fmtn )
		{
			foreachv( i, fmtn )
			{
				DEBUGLOGP("  (2) fmt %d is \"%s\"\n", i, pCfgVars[f].fmt);

				if( gRun.portably && pCfgVars[f].idx == CFGVAR_TP )
				{
					DEBUGLOGS("  (2) skipping theme path saving since running portably");
				}
				else
				{
					eql = strchr(pCfgVars[f].fmt, '=');
					val = eql[2];

					strvcpy(key, pCfgVars[f].fmt, eql-pCfgVars[f].fmt);
					key[eql-pCfgVars[f].fmt] = '\0';

					DEBUGLOGP("  (2) group %s, key %s, value (%c)\n", grp, key, val);

					switch( val )
					{
					case 'b': g_key_file_set_boolean(pKF, grp, key, *((bool*)  pCfgVars[f].var)); break;
					case 'd': g_key_file_set_integer(pKF, grp, key, *((int*)   pCfgVars[f].var)); break;
					case 'f': g_key_file_set_double (pKF, grp, key, *((double*)pCfgVars[f].var)); break;
					case 's': g_key_file_set_string (pKF, grp, key,   (char*)  pCfgVars[f].var);  break;
					}
				}

				f++;
			}

//			int     a = -1;
#if 0
			switch( gCfg.shandType )
			{
			case draw::ANIM_ORIG:   a = 0; break;
			case draw::ANIM_FLICK:  a = 1; break;
			case draw::ANIM_SWEEP:  a = 2; break;
			case draw::ANIM_CUSTOM: a = 3; break;
			}
#endif
//			if( a >= 0 )
//			if( gCfg.shandType == draw::ANIM_CUSTOM )
			{
				const double          *pXs, *pYs;
				int n = hand_anim_get(&pXs, &pYs);
				int a = 3;

				if( n && pXs && pYs )
				{
					char     grp[16];
					snprintf(grp, vectsz(grp), "HandAnim%d", a);
					const char*   nms[] =    { "Original", "Flick", "Sweep", "Custom" };

					g_key_file_set_string     (pKF, grp, "Name",             nms[a]);
					g_key_file_set_integer    (pKF, grp, "Count",            n);
					g_key_file_set_double_list(pKF, grp, "X", (gdouble*)pXs, n);
					g_key_file_set_double_list(pKF, grp, "Y", (gdouble*)pYs, n);
				}
			}

			delete [] pCfgVars;
			pCfgVars = NULL;
		}

		okay = g_key_file_save_to_file(pKF, cfgPath, NULL);

		g_key_file_free(pKF);
		pKF  = NULL;
	}

	DEBUGLOGS("  (2)");
	DEBUGLOGE;

	return okay;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cfg::cnvp(Config& cfgData, bool screen, bool lock)
{
	if( lock )
		g_sync_threads_gui_beg();

#if _USEGTK
	int dsw = gdk_screen_get_width (gdk_screen_get_default());
	int dsh = gdk_screen_get_height(gdk_screen_get_default());
#else
	int dsw = 1400;
	int dsh = 1050;
#endif // _USEGTK

	if( lock )
		g_sync_threads_gui_end();

	if( cfgData.clockW < MIN_CLOCKW )
		cfgData.clockW = MIN_CLOCKW;
	if( cfgData.clockW > dsw )
		cfgData.clockW = dsw;

	if( cfgData.clockH < MIN_CLOCKH )
		cfgData.clockH = MIN_CLOCKH;
	if( cfgData.clockH > dsh )
		cfgData.clockH = dsh;

	DEBUGLOGP("bef test, clockC=%d, clockX=%d, clockY=%d, clockW=%d, clockH=%d\n", cfgData.clockC, cfgData.clockX, cfgData.clockY, cfgData.clockW, cfgData.clockH);

	switch( cfgData.clockC )
	{
	case CORNER_TOP_LEFT:                                                              break; // nothing to do
	case CORNER_TOP_RIGHT: cfgData.clockX = dsw   - cfgData.clockX - cfgData.clockW;   break;
	case CORNER_BOT_RIGHT: cfgData.clockX = dsw   - cfgData.clockX - cfgData.clockW;
	                       cfgData.clockY = dsh   - cfgData.clockY - cfgData.clockH;   break;
	case CORNER_BOT_LEFT:  cfgData.clockY = dsh   - cfgData.clockY - cfgData.clockH;   break;
	case CORNER_CENTER:    cfgData.clockX = dsw/2 - cfgData.clockX - cfgData.clockW/2;
	                       cfgData.clockY = dsh/2 - cfgData.clockY - cfgData.clockH/2; break;
	}

	DEBUGLOGP("aft test, clockC=%d, clockX=%d, clockY=%d, clockW=%d, clockH=%d\n", cfgData.clockC, cfgData.clockX, cfgData.clockY, cfgData.clockW, cfgData.clockH);

	if( screen )
	{
		// if cfg spec'd & necessary, force the window onto the screen
#if 0
		if( true ) // TODO: change this to check the cfg value for its related switch
		{
			if( cfgData.clockX <  0 )
				cfgData.clockX =  0;
			if( cfgData.clockX > (dsw - cfgData.clockW) )
				cfgData.clockX = (dsw - cfgData.clockW);

			if( cfgData.clockY <  0 )
				cfgData.clockY =  0;
			if( cfgData.clockY > (dsh - cfgData.clockH) )
				cfgData.clockY = (dsh - cfgData.clockH);
		}
#endif
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
int cfg::fscanf(FILE* pFile, const char* fmt, void* val, int len)
{
	int  ret;
	char frmt[64], line[4096];

	strvcpy(frmt, fmt);

	g_strstrip(frmt);
	strvcat(frmt, " "); // so we don't get EOF on a sscanf call

	DEBUGLOGP("entry, frmt is '%s'\n", frmt);

	while( true )
	{
		line[0]    = '\0';
		if( !(ret  = fgets(line, vectsz(line), pFile) != NULL ? 1 : 0) )
			break;
		if( *line != '\n' && *line != '#' )
			break;
	}

	if( ret )
	{
		g_strstrip(line);
		strvcat(line, " "); // so we don't get EOF on a sscanf call

		DEBUGLOGP("line is '%s'\n", line);

		const char* eql = strchr(frmt, '=');

		if( eql && strcmp(eql+1, "%s ") == 0 )
		{
			DEBUGLOGP("found 'key=val where val is a string' line, frmt is '%s', line is '%s'\n", frmt, line);

			char* str = strchr(line, '=');

			if( str && strnicmp(frmt, line, (int)(str-line)) == 0 )
			{
				g_strchomp(++str);
				DEBUGLOGP("read line matches up with frmt, copying string into return val, string is '%s'\n", str);
				strvcpy((char*)val, str, len); // TODO: this is okay?
				ret = 1;
			}
			else
				ret = EOF;
		}
		else
		if( (ret = sscanf(line, frmt, val)) == EOF )
		{
			const char* eql = strchr(line, '=');

			if( eql )
			{
				if( strnicmp(frmt, line, (int)(eql-line)) == 0 )
					ret = 0; // key= lines w/o the val are okay - indicate so to caller
			}
		}
	}
	else
		ret = EOF;

	DEBUGLOGP("exit, frmt is '%s', ret is %d, line is\n'%s'\n", frmt, ret, line);
	return ret;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
const char* cfg::get_user_prefs_path(bool old, bool test)
{
//	static const char* rcpath_old = "/." APP_NAME     "rc";
	static const char* rcpath_old = "/." APP_NAME_OLD "rc";
	static const char* rcpath_new = "/." APP_NAME     "rc";
	static const char* rcpath_tst = "/." APP_NAME "/"  APP_NAME "rc";

	const  char* rcpath = old ? rcpath_old : (test ? rcpath_tst : rcpath_new);
	return get_home_subpath(rcpath, strlen(rcpath), true);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
int cfg::getf(Config& cfgData, CfgVar** ppCfgVars)
{
	static CfgVar cfgVars[] =
	{
		{ CFGVAR_X,    "x=%d\n",                   &gCfg.clockX,      sizeof(gCfg.clockX) },
		{ CFGVAR_Y,    "y=%d\n",                   &gCfg.clockY,      sizeof(gCfg.clockY) },
		{ CFGVAR_W,    "width=%d\n",               &gCfg.clockW,      sizeof(gCfg.clockW) },
		{ CFGVAR_H,    "height=%d\n",              &gCfg.clockH,      sizeof(gCfg.clockH) },
		{ CFGVAR_SS,   "show-seconds=%d\n",        &gCfg.showSeconds, sizeof(gCfg.showSeconds) },
		{ CFGVAR_SD,   "show-date=%d\n",           &gCfg.showDate,    sizeof(gCfg.showDate) },
		{ CFGVAR_T,    "theme=%s\n",                gCfg.themeFile,   vectsz(gCfg.themeFile) },
		{ CFGVAR_KOT,  "keep-on-top=%d\n",         &gCfg.keepOnTop,   sizeof(gCfg.keepOnTop) },
		{ CFGVAR_AIP,  "appear-in-pager=%d\n",     &gCfg.showInPager, sizeof(gCfg.showInPager) },
		{ CFGVAR_AIT,  "appear-in-taskbar=%d\n",   &gCfg.showInTasks, sizeof(gCfg.showInTasks) },
		{ CFGVAR_S,    "sticky=%d\n",              &gCfg.sticky,      sizeof(gCfg.sticky) },
		{ CFGVAR_24,   "twentyfour=%d\n",          &gCfg.show24Hrs,   sizeof(gCfg.show24Hrs) },
		{ CFGVAR_RR,   "refreshrate=%d\n",         &gCfg.renderRate,  sizeof(gCfg.renderRate) },
		// 0.4.0 added
		{ CFGVAR_WS,   "workspace=%d\n",           &gCfg.clockWS,     sizeof(gCfg.clockWS) },
		{ CFGVAR_C,    "corner=%d\n",              &gCfg.clockC,      sizeof(gCfg.clockC) },
		{ CFGVAR_R,    "rotation=%f\n",            &gCfg.clockR,      sizeof(gCfg.clockR) },
		{ CFGVAR_FD1,  "face-date=%d\n",           &gCfg.faceDate,    sizeof(gCfg.faceDate) },
		{ CFGVAR_KOB,  "keep-on-bottom=%d\n",      &gCfg.keepOnBot,   sizeof(gCfg.keepOnBot) },
		{ CFGVAR_TP,   "theme-path=%s\n",           gCfg.themePath,   vectsz(gCfg.themePath) },
		{ CFGVAR_O,    "opacity=%f\n",             &gCfg.opacity,     sizeof(gCfg.opacity) },
		{ CFGVAR_FS,   "font-size=%d\n",           &gCfg.fontSize,    sizeof(gCfg.fontSize) },
		{ CFGVAR_FOY,  "font-offy=%f\n",           &gCfg.fontOffY,    sizeof(gCfg.fontOffY) },
		{ CFGVAR_FTR,  "font-text-red=%f\n",       &gCfg.dateTxtRed,  sizeof(gCfg.dateTxtRed) },
		{ CFGVAR_FTG,  "font-text-green=%f\n",     &gCfg.dateTxtGrn,  sizeof(gCfg.dateTxtGrn) },
		{ CFGVAR_FTB,  "font-text-blue=%f\n",      &gCfg.dateTxtBlu,  sizeof(gCfg.dateTxtBlu) },
		{ CFGVAR_FTA,  "font-text-alpha=%f\n",     &gCfg.dateTxtAlf,  sizeof(gCfg.dateTxtAlf) },
		{ CFGVAR_FSR,  "font-shadow-red=%f\n",     &gCfg.dateShdRed,  sizeof(gCfg.dateShdRed) },
		{ CFGVAR_FSG,  "font-shadow-green=%f\n",   &gCfg.dateShdGrn,  sizeof(gCfg.dateShdGrn) },
		{ CFGVAR_FSB,  "font-shadow-blue=%f\n",    &gCfg.dateShdBlu,  sizeof(gCfg.dateShdBlu) },
		{ CFGVAR_FSA,  "font-shadow-alpha=%f\n",   &gCfg.dateShdAlf,  sizeof(gCfg.dateShdAlf) },
		{ CFGVAR_FSW,  "font-shadow-width=%f\n",   &gCfg.dateShdWid,  sizeof(gCfg.dateShdWid) },
		{ CFGVAR_FP,   "font-path=%s\n",            gCfg.fontFace,    vectsz(gCfg.fontFace) },
		{ CFGVAR_FN,   "font-name=%s\n",            gCfg.fontName,    vectsz(gCfg.fontName) },
		{ CFGVAR_FD12, "format-date12=%s\n",        gCfg.fmtDate12,   vectsz(gCfg.fmtDate12) },
		{ CFGVAR_FD24, "format-date24=%s\n",        gCfg.fmtDate24,   vectsz(gCfg.fmtDate24) },
		{ CFGVAR_FT12, "format-time12=%s\n",        gCfg.fmtTime12,   vectsz(gCfg.fmtTime12) },
		{ CFGVAR_FT24, "format-time24=%s\n",        gCfg.fmtTime24,   vectsz(gCfg.fmtTime24) },
		{ CFGVAR_TT12, "format-ttip12=%s\n",        gCfg.fmtTTip12,   vectsz(gCfg.fmtTTip12) },
		{ CFGVAR_TT24, "format-ttip24=%s\n",        gCfg.fmtTTip24,   vectsz(gCfg.fmtTTip24) },
		{ CFGVAR_AS,   "animate-startup=%d\n",     &gCfg.aniStartup,  sizeof(gCfg.aniStartup) },
		{ CFGVAR_QD,   "queued-draw=%d\n",         &gCfg.queuedDraw,  sizeof(gCfg.queuedDraw) },
		{ CFGVAR_RSC,  "refresh-skip-count=%d\n",  &gCfg.refSkipCnt,  sizeof(gCfg.refSkipCnt) },
		{ CFGVAR_SA,   "show-alarms=%d\n",         &gCfg.showAlarms,  sizeof(gCfg.showAlarms) },
		{ CFGVAR_SH,   "show-hours=%d\n",          &gCfg.showHours,   sizeof(gCfg.showHours) },
		{ CFGVAR_SM,   "show-minutes=%d\n",        &gCfg.showMinutes, sizeof(gCfg.showMinutes) },
		{ CFGVAR_STT,  "show-tooltip=%d\n",        &gCfg.showTTips,   sizeof(gCfg.showTTips) },
		{ CFGVAR_SHT,  "second-hand-type=%d\n",    &gCfg.shandType,   sizeof(gCfg.shandType) },
		{ CFGVAR_PA,   "play-alarms=%d\n",         &gCfg.doAlarms,    sizeof(gCfg.doAlarms) },
		{ CFGVAR_PC,   "play-chimes=%d\n",         &gCfg.doChimes,    sizeof(gCfg.doChimes) },
		{ CFGVAR_PS,   "play-sounds=%d\n",         &gCfg.doSounds,    sizeof(gCfg.doSounds) },
		{ CFGVAR_ALS,  "alarms=%s\n",               gCfg.alarms,      vectsz(gCfg.alarms) },
		{ CFGVAR_ALDS, "alarm-dows=%s\n",           gCfg.alarmDays,   vectsz(gCfg.alarmDays) },
		{ CFGVAR_ALMS, "alarm-msgs=%s\n",           gCfg.alarmMsgs,   vectsz(gCfg.alarmMsgs) },
		{ CFGVAR_ALLS, "alarm-msg-timeouts=%s\n",   gCfg.alarmLens,   vectsz(gCfg.alarmLens) },
		{ CFGVAR_OH1,  "opt-hand-alarm=%d\n",      &gCfg.optHand[0],  sizeof(gCfg.optHand[0]) },
		{ CFGVAR_OH2,  "opt-hand-hour=%d\n",       &gCfg.optHand[1],  sizeof(gCfg.optHand[1]) },
		{ CFGVAR_OH3,  "opt-hand-minute=%d\n",     &gCfg.optHand[2],  sizeof(gCfg.optHand[2]) },
		{ CFGVAR_OH4,  "opt-hand-second=%d\n",     &gCfg.optHand[3],  sizeof(gCfg.optHand[3]) },
		{ CFGVAR_US1,  "use-surf-alarm=%d\n",      &gCfg.useSurf[0],  sizeof(gCfg.useSurf[0]) },
		{ CFGVAR_US2,  "use-surf-hour=%d\n",       &gCfg.useSurf[1],  sizeof(gCfg.useSurf[1]) },
		{ CFGVAR_US3,  "use-surf-minute=%d\n",     &gCfg.useSurf[2],  sizeof(gCfg.useSurf[2]) },
		{ CFGVAR_US4,  "use-surf-second=%d\n",     &gCfg.useSurf[3],  sizeof(gCfg.useSurf[3]) },
		{ CFGVAR_UPI,  "use-png-icon=%d\n",        &gCfg.pngIcon,     sizeof(gCfg.pngIcon) },
		{ CFGVAR_A24,  "auto-switch24=%d\n",       &gCfg.auto24,      sizeof(gCfg.auto24) },
		{ CFGVAR_BGR,  "background-red=%f\n",      &gCfg.bkgndRed,    sizeof(gCfg.bkgndRed) },
		{ CFGVAR_BGG,  "background-green=%f\n",    &gCfg.bkgndGrn,    sizeof(gCfg.bkgndGrn) },
		{ CFGVAR_BGB,  "background-blue=%f\n",     &gCfg.bkgndBlu,    sizeof(gCfg.bkgndBlu) },
		{ CFGVAR_TZN,  "tz-name=%s\n",              gCfg.tzName,      vectsz(gCfg.tzName) },
		{ CFGVAR_TZSN, "tz-show-name=%s\n",         gCfg.tzShoName,   vectsz(gCfg.tzShoName) },
		{ CFGVAR_GHO,  "tz-gmt-hour-offset=%d\n",  &gCfg.tzHorGmtOff, sizeof(gCfg.tzHorGmtOff) },
		{ CFGVAR_GMO,  "tz-gmt-min-offset=%d\n",   &gCfg.tzMinGmtOff, sizeof(gCfg.tzMinGmtOff) },
		{ CFGVAR_SHO,  "tz-show-hour-offset=%d\n", &gCfg.tzHorShoOff, sizeof(gCfg.tzHorShoOff) },
		{ CFGVAR_SMO,  "tz-show-min-offset=%d\n",  &gCfg.tzMinShoOff, sizeof(gCfg.tzMinShoOff) },
		{ CFGVAR_LAT,  "tz-latitude=%f\n",         &gCfg.tzLatitude,  sizeof(gCfg.tzLatitude) },
		{ CFGVAR_LON,  "tz-longitude=%f\n",        &gCfg.tzLngitude,  sizeof(gCfg.tzLngitude) },

		{ CFGVAR_TAS1, "text-above-scale1=%f\n",   &gCfg.sfTxta1,     sizeof(gCfg.sfTxta1) },
		{ CFGVAR_TAS2, "text-above-scale2=%f\n",   &gCfg.sfTxta2,     sizeof(gCfg.sfTxta2) },
		{ CFGVAR_TAS3, "text-above-scale3=%f\n",   &gCfg.sfTxta3,     sizeof(gCfg.sfTxta3) },
		{ CFGVAR_TBS1, "text-below-scale1=%f\n",   &gCfg.sfTxtb1,     sizeof(gCfg.sfTxtb1) },
		{ CFGVAR_TBS2, "text-below-scale2=%f\n",   &gCfg.sfTxtb2,     sizeof(gCfg.sfTxtb2) },
		{ CFGVAR_TBS3, "text-below-scale3=%f\n",   &gCfg.sfTxtb3,     sizeof(gCfg.sfTxtb3) },

		{ CFGVAR_TAH1, "text-above-htfac1=%f\n",   &gCfg.hfTxta1,     sizeof(gCfg.hfTxta1) },
		{ CFGVAR_TAH2, "text-above-htfac2=%f\n",   &gCfg.hfTxta2,     sizeof(gCfg.hfTxta2) },
		{ CFGVAR_TAH3, "text-above-htfac3=%f\n",   &gCfg.hfTxta3,     sizeof(gCfg.hfTxta3) },
		{ CFGVAR_TBH1, "text-below-htfac1=%f\n",   &gCfg.hfTxtb1,     sizeof(gCfg.hfTxtb1) },
		{ CFGVAR_TBH2, "text-below-htfac2=%f\n",   &gCfg.hfTxtb2,     sizeof(gCfg.hfTxtb2) },
		{ CFGVAR_TBH3, "text-below-htfac3=%f\n",   &gCfg.hfTxtb3,     sizeof(gCfg.hfTxtb3) },
	};
	const size_t nvars = vectsz(cfgVars);

	void*   var;
	CfgVar* pCfgVars = new CfgVar[nvars];
	*ppCfgVars       = pCfgVars;

	foreachv( v, nvars )
	{
		pCfgVars[v].idx = cfgVars[v].idx;
		pCfgVars[v].fmt = cfgVars[v].fmt;
		pCfgVars[v].len = cfgVars[v].len;

		switch( cfgVars[v].idx )
		{
		case CFGVAR_X:    var = &cfgData.clockX;      break;
		case CFGVAR_Y:    var = &cfgData.clockY;      break;
		case CFGVAR_W:    var = &cfgData.clockW;      break;
		case CFGVAR_H:    var = &cfgData.clockH;      break;
		case CFGVAR_SS:   var = &cfgData.showSeconds; break;
		case CFGVAR_SD:   var = &cfgData.showDate;    break;
		case CFGVAR_T:    var =  cfgData.themeFile;   break;
		case CFGVAR_KOT:  var = &cfgData.keepOnTop;   break;
		case CFGVAR_AIP:  var = &cfgData.showInPager; break;
		case CFGVAR_AIT:  var = &cfgData.showInTasks; break;
		case CFGVAR_S:    var = &cfgData.sticky;      break;
		case CFGVAR_24:   var = &cfgData.show24Hrs;   break;
		case CFGVAR_RR:   var = &cfgData.renderRate;  break;
		case CFGVAR_WS:   var = &cfgData.clockWS;     break;
		case CFGVAR_C:    var = &cfgData.clockC;      break;
		case CFGVAR_R:    var = &cfgData.clockR;      break;
		case CFGVAR_FD1:  var = &cfgData.faceDate;    break;
		case CFGVAR_KOB:  var = &cfgData.keepOnBot;   break;
		case CFGVAR_TP:   var =  cfgData.themePath;   break;
		case CFGVAR_O:    var = &cfgData.opacity;     break;
		case CFGVAR_FS:   var = &cfgData.fontSize;    break;
		case CFGVAR_FOY:  var = &cfgData.fontOffY;    break;
		case CFGVAR_FTR:  var = &cfgData.dateTxtRed;  break;
		case CFGVAR_FTG:  var = &cfgData.dateTxtGrn;  break;
		case CFGVAR_FTB:  var = &cfgData.dateTxtBlu;  break;
		case CFGVAR_FTA:  var = &cfgData.dateTxtAlf;  break;
		case CFGVAR_FSR:  var = &cfgData.dateShdRed;  break;
		case CFGVAR_FSG:  var = &cfgData.dateShdGrn;  break;
		case CFGVAR_FSB:  var = &cfgData.dateShdBlu;  break;
		case CFGVAR_FSA:  var = &cfgData.dateShdAlf;  break;
		case CFGVAR_FSW:  var = &cfgData.dateShdWid;  break;
		case CFGVAR_FP:   var =  cfgData.fontFace;    break;
		case CFGVAR_FN:   var =  cfgData.fontName;    break;
		case CFGVAR_FD12: var =  cfgData.fmtDate12;   break;
		case CFGVAR_FD24: var =  cfgData.fmtDate24;   break;
		case CFGVAR_FT12: var =  cfgData.fmtTime12;   break;
		case CFGVAR_FT24: var =  cfgData.fmtTime24;   break;
		case CFGVAR_TT12: var =  cfgData.fmtTTip12;   break;
		case CFGVAR_TT24: var =  cfgData.fmtTTip24;   break;
		case CFGVAR_AS:   var = &cfgData.aniStartup;  break;
		case CFGVAR_QD:   var = &cfgData.queuedDraw;  break;
		case CFGVAR_RSC:  var = &cfgData.refSkipCnt;  break;
		case CFGVAR_SA:   var = &cfgData.showAlarms;  break;
		case CFGVAR_SH:   var = &cfgData.showHours;   break;
		case CFGVAR_SM:   var = &cfgData.showMinutes; break;
		case CFGVAR_STT:  var = &cfgData.showTTips;   break;
		case CFGVAR_SHT:  var = &cfgData.shandType;   break;
		case CFGVAR_PS:   var = &cfgData.doSounds;    break;
		case CFGVAR_PA:   var = &cfgData.doAlarms;    break;
		case CFGVAR_PC:   var = &cfgData.doChimes;    break;
		case CFGVAR_ALS:  var =  cfgData.alarms;      break;
		case CFGVAR_ALDS: var =  cfgData.alarmDays;   break;
		case CFGVAR_ALMS: var =  cfgData.alarmMsgs;   break;
		case CFGVAR_ALLS: var =  cfgData.alarmLens;   break;
		case CFGVAR_OH1:  var = &cfgData.optHand[0];  break;
		case CFGVAR_OH2:  var = &cfgData.optHand[1];  break;
		case CFGVAR_OH3:  var = &cfgData.optHand[2];  break;
		case CFGVAR_OH4:  var = &cfgData.optHand[3];  break;
		case CFGVAR_US1:  var = &cfgData.useSurf[0];  break;
		case CFGVAR_US2:  var = &cfgData.useSurf[1];  break;
		case CFGVAR_US3:  var = &cfgData.useSurf[2];  break;
		case CFGVAR_US4:  var = &cfgData.useSurf[3];  break;
		case CFGVAR_UPI:  var = &cfgData.pngIcon;     break;
		case CFGVAR_A24:  var = &cfgData.auto24;      break;
		case CFGVAR_BGR:  var = &cfgData.bkgndRed;    break;
		case CFGVAR_BGG:  var = &cfgData.bkgndGrn;    break;
		case CFGVAR_BGB:  var = &cfgData.bkgndBlu;    break;
		case CFGVAR_LAT:  var = &cfgData.tzLatitude;  break;
		case CFGVAR_LON:  var = &cfgData.tzLngitude;  break;
		case CFGVAR_GHO:  var = &cfgData.tzHorGmtOff; break;
		case CFGVAR_GMO:  var = &cfgData.tzMinGmtOff; break;
		case CFGVAR_SHO:  var = &cfgData.tzHorShoOff; break;
		case CFGVAR_SMO:  var = &cfgData.tzMinShoOff; break;
		case CFGVAR_TZN:  var =  cfgData.tzName;      break;
		case CFGVAR_TZSN: var =  cfgData.tzShoName;   break;
		case CFGVAR_TAS1: var = &cfgData.sfTxta1;     break;
		case CFGVAR_TAS2: var = &cfgData.sfTxta2;     break;
		case CFGVAR_TAS3: var = &cfgData.sfTxta3;     break;
		case CFGVAR_TBS1: var = &cfgData.sfTxtb1;     break;
		case CFGVAR_TBS2: var = &cfgData.sfTxtb2;     break;
		case CFGVAR_TBS3: var = &cfgData.sfTxtb3;     break;
		case CFGVAR_TAH1: var = &cfgData.hfTxta1;     break;
		case CFGVAR_TAH2: var = &cfgData.hfTxta2;     break;
		case CFGVAR_TAH3: var = &cfgData.hfTxta3;     break;
		case CFGVAR_TBH1: var = &cfgData.hfTxtb1;     break;
		case CFGVAR_TBH2: var = &cfgData.hfTxtb2;     break;
		case CFGVAR_TBH3: var = &cfgData.hfTxtb3;     break;
		}

		pCfgVars[v].var = var;
	}

	return vectsz(cfgVars);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <geoclue/geoclue-position.h> //
#include "loadlib.h"                  // for runtime loading

// -----------------------------------------------------------------------------
bool cfg::get_latlon(double& lat, double& lon)
{
	DEBUGLOGB;

	lib::symb g_symbs[] = { { "geoclue_position_new", NULL }, { "geoclue_position_get_position", NULL } };

	typedef GeocluePosition*      (*GPN) (const char*, const char*);
	typedef GeocluePositionFields (*GPGP)(GeocluePosition*, int*, double*, double*, double*, GeoclueAccuracy**, GError**);

	GPN  g_geoclue_position_new;
	GPGP g_geoclue_position_get_position;

	lib::LoadLib* g_pLLib   = lib::create();
	bool          g_pLLibok = g_pLLib && lib::load(g_pLLib, "geoclue", ".0", g_symbs, vectsz(g_symbs));

	if( g_pLLibok )
	{
		DEBUGLOGS("geoclue lib loaded ok - before typedef'd func address setting");

		if( !lib::func(g_pLLib, LIB_FUNC_ADDR(g_geoclue_position_new)) ||
		    !lib::func(g_pLLib, LIB_FUNC_ADDR(g_geoclue_position_get_position)) )
		{
			lib::destroy(g_pLLib);
			g_pLLibok = false;
			g_pLLib   = NULL;
		}

		DEBUGLOGS("geoclue lib loaded ok - after  typedef'd func address setting");
		DEBUGLOGP("all lib funcs %sretrieved\n", g_pLLibok ? "" : "NOT ");

#if DEBUGLOG
		for( size_t s = 0; s < vectsz(g_symbs); s++ )
		{
			if( !g_symbs[s].symb )
				DEBUGLOGP("failed to retrieve func %s's address\n", g_symbs[s].name);
		}
#endif
	}

	bool ret = false;

//	g_type_init(); // deprecated

	if( g_pLLibok )
	{
		// TODO: make more robust by using 'master provider' coding example

		DEBUGLOGS("bef geoclue position new call");

		const char* service  =  "org.freedesktop.Geoclue.Providers.Hostip";
		const char* path     = "/org/freedesktop/Geoclue/Providers/Hostip";
#if 1
		GeocluePosition* pos = g_geoclue_position_new(service, path);
#else
		GeocluePosition* pos =   geoclue_position_new(service, path);
#endif
		DEBUGLOGS("aft geoclue position new call");

		if( pos )
		{
			DEBUGLOGS("bef geoclue position get position call");

			double  la, lo;
			GError* error = NULL;
#if 1
			GeocluePositionFields fields = g_geoclue_position_get_position(pos, NULL, &la, &lo, NULL, NULL, &error);
#else
			GeocluePositionFields fields =   geoclue_position_get_position(pos, NULL, &la, &lo, NULL, NULL, &error);
#endif
			DEBUGLOGS("aft geoclue position get position call");

			if( error )
			{
				DEBUGLOGP("error in geoclue position get position:\n\t%s\n", error->message);
				g_error_free(error);
			}
			else
			{
				if( fields & GEOCLUE_POSITION_FIELDS_LATITUDE && fields & GEOCLUE_POSITION_FIELDS_LONGITUDE )
				{
					DEBUGLOGP("hostip.info says system's at %.3f, %.3f\n", (float)lat, (float)lon);
					lat = la;
					lon = lo;
					ret = true;
				}
/*				else
				{
					g_print("Hostip does not have a valid location available.\nVisit http://www.hostip.info/ to correct this\n");
				}*/
			}

			g_object_unref(pos);
			pos = NULL;
		}
	}

#if 0 // NOTE: unloading causes crash
	if( g_pLLib )
		lib::destroy(g_pLLib);
	g_pLLibok = false;
	g_pLLib   = NULL;
#endif

	DEBUGLOGE;
	return ret;
}

