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
#include <langinfo.h> // for ?

#include "cfgdef.h"   // for APP_NAME
#include "config.h"   // for Settings struct, CornerType enum, ...
#include "global.h"   // for gCfg extern (and vectsz)
#include "debug.h"    // for debugging prints

#include "draw.h"     // for ANIM_FLICK
#include "utility.h"  // for get_home_subpath and strcrep

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static const char* verstr = "# Version 0.4.0. This file is app-managed. User changes are not supported or encouraged.";
static const int   versln =  12; // # of chars at beginning of version string length to use to determine version #

// -----------------------------------------------------------------------------
namespace cfg
{
	struct CfgVar
	{
		int         idx;
		const char* fmt;
		void*       var;
		int         len;
	};

	static bool load(Settings& cfgData, const char* cfgPath, bool tst=false);
	static bool save(Settings& cfgData, const char* cfgPath); // needs to temp mod Settings fmt fields

	static int  fscanf(FILE* pFile, const char* fmt, void* val, int len);

	static const char* get_user_prefs_path(bool old, bool tst=false);

	static int  getf(Settings& cfgData, CfgVar** ppCfgVars);
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
	gCfg.refreshRate = 16;
	// 0.4.0 added
//	gCfg.aniStartup  = 0;
	gCfg.faceDate    = 1;
	gCfg.showHours   = 1;
	gCfg.showMinutes = 1;
	gCfg.shandType   = ANIM_FLICK;
	gCfg.opacity     = 1.00f;
//	gCfg.dateTextRed = 0.75f;
	gCfg.dateTextRed = 0.90f;
	gCfg.dateTextGrn = gCfg.dateTextRed;
	gCfg.dateTextBlu = gCfg.dateTextGrn;
	gCfg.dateTextAlf = 1.00f;
	gCfg.dateShdoRed = 0.10f;
	gCfg.dateShdoGrn = gCfg.dateShdoRed;
	gCfg.dateShdoBlu = gCfg.dateShdoGrn;
	gCfg.dateShdoAlf = 1.00f;
	gCfg.dateShdoWid = 0.10f;
	gCfg.fontOffY    = 1.00f;
	gCfg.fontSize    = 10;

	if( true ) // TODO: chg this to ?
	{
/*		// this only applies to the current locale
		const gchar* const* pLangs = g_get_language_names();
		const gchar* const* pLang  = pLangs;
		while( pLang && *pLang )
		{
			DEBUGLOGP("language %s available\n", *pLang);
			pLang++;
		}*/

//		locale -a gets a list of available locales - glib/gdk/gtk equivalent?

		char   tstr[64];
/*		char*  oldloc  =  setlocale(LC_TIME, NULL);*/
		char*  newloc  =  setlocale(LC_TIME, "");
//		char*  newloc  =  setlocale(LC_TIME, "fr_FR.UTF8");
//		char*  newloc  =  setlocale(LC_TIME, "es_ES.UTF8");
/*		if(   !newloc )
			   newloc  =  setlocale(LC_TIME, "");*/
		time_t ct      =  time(NULL);
		tm     lt      = *localtime(&ct);
		lt.tm_hour     =  14; // force to an hour that will be converted to either "02" or "14"
		mktime(&lt);

/*		GDateTime* pGDT=  g_date_time_new_local(lt.tm_year, lt.tm_mon, lt.tm_mday, 14, lt.tm_min, lt.tm_sec);
		gchar*     pStr=  g_date_time_format(pGDT, "%X");
//		char*      pLng=  nl_langinfo(_NL_ADDRESS_LANG_NAME);
//		char*      pLng=  nl_langinfo(_NL_ADDRESS_POSTAL_FMT);
		char*      pLng=  nl_langinfo(_NL_ADDRESS_COUNTRY_NAME);*/

		strftime(tstr, vectsz(tstr), "%X", &lt); // 'X' is supposedly locale specific

/*		gCfg.show12Hrs =  tstr[0] == '0';
		gCfg.show24Hrs = !gCfg.show12Hrs;*/
		gCfg.show24Hrs =  tstr[0] != '0';

		DEBUGLOGP("old locale       is %s\n", oldloc);
		DEBUGLOGP("new locale       is %s\n", newloc);
		DEBUGLOGP("clib time string is %s\n", tstr);
		DEBUGLOGP("glib time string is %s\n", pStr);
		DEBUGLOGP("language  string is %s\n", pLng);
		DEBUGLOGP("12-hour locale   is %s\n", gCfg.show12Hrs ? "in use" : "not in use");
		DEBUGLOGP("24-hour locale   is %s\n", gCfg.show24Hrs ? "in use" : "not in use");

//		gCfg.show12Hrs =  true;
//		gCfg.show24Hrs = !gCfg.show12Hrs;

//		setlocale(LC_TIME, oldloc);
/*		g_date_time_unref(pGDT);
		g_free(pStr);*/
	}

	// TODO: the formats following need to be changed to work for all (TODO: huh? locales maybe?)

	strvcpy(gCfg.themeFile, "<simple>");
	strvcpy(gCfg.fmt12Hrs,  "%A%n%B %d %Y%n%I:%M:%S %P");
	strvcpy(gCfg.fmt24Hrs,  "%A%n%B %d %Y%n%T");
	strvcpy(gCfg.fmtDate,   "%a %b %d");
	strvcpy(gCfg.fmtTime,   "%I:%M %P");

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cfg::prep(Settings& cfgData) // copy from gCfg into cfgData, but only non-cl switch vars
{
	DEBUGLOGB;

//	cfgData.clockWS     = gCfg.clockWS;
//	cfgData.clockC      = gCfg.clockC;
//	cfgData.keepOnBot   = gCfg.keepOnBot;
	cfgData.showHours   = gCfg.showHours;
	cfgData.showMinutes = gCfg.showMinutes;
	cfgData.shandType   = gCfg.shandType;
	cfgData.doSounds    = gCfg.doSounds;
	cfgData.doAlarms    = gCfg.doAlarms;
	cfgData.doChimes    = gCfg.doChimes;
//	cfgData.opacity     = gCfg.opacity;
	cfgData.dateTextRed = gCfg.dateTextRed;
	cfgData.dateTextGrn = gCfg.dateTextGrn;
	cfgData.dateTextBlu = gCfg.dateTextBlu;
	cfgData.dateTextAlf = gCfg.dateTextAlf;
	cfgData.dateShdoRed = gCfg.dateShdoRed;
	cfgData.dateShdoGrn = gCfg.dateShdoGrn;
	cfgData.dateShdoBlu = gCfg.dateShdoBlu;
	cfgData.dateShdoAlf = gCfg.dateShdoAlf;
	cfgData.dateShdoWid = gCfg.dateShdoWid;
	cfgData.fontOffY    = gCfg.fontOffY;
	cfgData.fontSize    = gCfg.fontSize;

//	strvcpy(cfgData.themePath, gCfg.themePath); // TODO: is this necessary or not?
	strvcpy(cfgData.fontFace,  gCfg.fontFace);
	strvcpy(cfgData.fontName,  gCfg.fontName);
	strvcpy(cfgData.fmt12Hrs,  gCfg.fmt12Hrs);
	strvcpy(cfgData.fmt24Hrs,  gCfg.fmt24Hrs);
	strvcpy(cfgData.fmtDate,   gCfg.fmtDate);
	strvcpy(cfgData.fmtTime,   gCfg.fmtTime);

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
			DEBUGLOGS("(1) found/using tst cfg path");
			okay = load(gCfg, cfgPath, true);
		}

		delete [] cfgPath; cfgPath = NULL;

		if( !pfnd )                                        // if v 0.4.0 new cfg not found
		{
			cfgPath  =  get_user_prefs_path(false);        // look for v 0.4.0 cfg first
			if( okay = (cfgPath != NULL) )
			{
				if( pfnd = g_isa_file(cfgPath) )           // it's okay if it's missing since defs are used
				{
					DEBUGLOGS("(1) found/using new cfg path");
					okay = load(gCfg, cfgPath);
				}

				delete [] cfgPath; cfgPath = NULL;

				if( !pfnd )                                // if v 0.4.0 cfg not found
				{
					cfgPath  =  get_user_prefs_path(true); // look for older cfg
					if( okay = (cfgPath != NULL) )
					{
						if( pfnd = g_isa_file(cfgPath) )
						{
							DEBUGLOGS("(1) found/using old cfg path");
							okay = load(gCfg, cfgPath);
						}

						delete [] cfgPath; cfgPath = NULL;
					}
				}
			}
		}
	}

/*	if( !okay )
		DEBUGLOGS("There was an error while trying to read the preferences");*/

	DEBUGLOGE;
	DEBUGLOGS("  (1)");

	return okay;
}

// -----------------------------------------------------------------------------
bool cfg::load(Settings& cfgData, const char* cfgPath, bool tst)
{
	DEBUGLOGB;
	DEBUGLOGS("  (2)");

	bool okay = false;

	DEBUGLOGP("(2) cfgPath is '%s'\n", cfgPath);

	if( tst )
	{
		GKeyFile* pKF = g_key_file_new();

		if( pKF )
		{
			DEBUGLOGS("(2) key file successfully created");

			if( okay = (g_key_file_load_from_file(pKF, cfgPath, G_KEY_FILE_KEEP_COMMENTS, NULL)) )
			{
				DEBUGLOGS("(2) key file successfully loaded");
				DEBUGLOGS("(2) bef getf call");

				const char* eql;
				char        key[1024],  val;
				const char  grp[]    = "General";
				int         f        =  0;
				CfgVar*     pCfgVars =  NULL;
				int         fmtn     =  getf(cfgData, &pCfgVars);

				DEBUGLOGP("(2) aft getf call, fmtn is %d, pCfgVars is %p\n", fmtn, pCfgVars);

				if( pCfgVars && fmtn )
				{
					foreach( i, fmtn )
					{
						DEBUGLOGP("(2) fmt %d is \"%s\"\n", i, pCfgVars[f].fmt);

						eql = strchr(pCfgVars[f].fmt, '=');
						val = eql[2];

						strvcpy(key, pCfgVars[f].fmt, eql-pCfgVars[f].fmt);
						key[eql-pCfgVars[f].fmt] = '\0';

						if( g_key_file_has_key(pKF, grp, key, NULL) )
						{
							DEBUGLOGP("(2) group %s, key %s, value (%c) %s\n", grp, key, val, g_key_file_get_value(pKF, grp, key, NULL));

							switch( val )
							{
							case 'b': *((bool *)pCfgVars[f].var) = (bool) g_key_file_get_boolean(pKF, grp, key, NULL);  break;
							case 'd': *((int  *)pCfgVars[f].var) = (int)  g_key_file_get_integer(pKF, grp, key, NULL);  break;
							case 'f': *((float*)pCfgVars[f].var) = (float)g_key_file_get_double (pKF, grp, key, NULL);  break;
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
							DEBUGLOGP("(2) group %s, key %s, value (%c), missing\n", grp, key, val);
						}

						f++;
					}

					delete [] pCfgVars;
				}
			}
			else
			{
				DEBUGLOGS("(2) key file NOT successfully loaded");
			}

			g_key_file_free(pKF);
			pKF = NULL;
		}
		else
		{
			DEBUGLOGS("(2) key file NOT successfully created");
		}
	}
	else
	{
		FILE* pFile;

		if( okay = (cfgPath && (pFile = fopen(cfgPath, "r"))) )
		{
			int     res;
			int     f        =   0;
			bool    ver3     =   true;
			CfgVar* pCfgVars =   NULL;
			int     fmtn     =   getf(cfgData, &pCfgVars);
			char*   strs[]   = { cfgData.fmtDate, cfgData.fmtTime, cfgData.fmt12Hrs, cfgData.fmt24Hrs };
			int     strn     =   vectsz(strs);
			int     numf     =   fmtn;

			char*    line = NULL;
			size_t          lcnt = 0;
			getline(&line, &lcnt, pFile);

			if( line )
			{
				ver3 = lcnt < 14 || strncmp(line, verstr, versln) != 0;
				DEBUGLOGP("(2) version is %s, line is\n%s\n", ver3 ? "3-" : "4+", line);
				free(line);
			}

			if( ver3 )
				numf = 13; // valid cfgs prior to version 4 only contain the first 13 cfg struct fields

			foreach( s, strn )   strcrep(strs[s], ' ', '+');
			foreach( i, numf ) { okay &= (res = fscanf(pFile, pCfgVars[f].fmt, pCfgVars[f].var), pCfgVars[f].len) != EOF; f++; }
			foreach( s, strn )   strcrep(strs[s], '+', ' ');

			okay &= (res = fclose(pFile)) == 0;
		}
	}

	DEBUGLOGE;
	DEBUGLOGS("  (2)");

	return okay;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool cfg::save()
{
	DEBUGLOGB;
	DEBUGLOGS("  (1)");

	bool        okay;
	const char* cfgPath  = get_user_prefs_path(false, true);

	if( okay = (cfgPath != NULL) )
	{
		DEBUGLOGP("(1) tst cfgPath is \"%s\"\n", cfgPath);

		if( gRun.hasParms )
		{
			DEBUGLOGS("(1) cmdline parms given, so save not attempted");
		}
		else
		{
			DEBUGLOGS("(1) cmdline parms not given, so save attempted");

			if( !(okay = save(gCfg, cfgPath)) )
			{
				DEBUGLOGS("There was an error while trying to save the preferences");
			}
		}

		delete [] cfgPath; cfgPath = NULL;
	}

	DEBUGLOGE;
	DEBUGLOGS("  (1)");

	return okay;
}

// -----------------------------------------------------------------------------
bool cfg::save(Settings& cfgData, const char* cfgPath)
{
	DEBUGLOGB;
	DEBUGLOGS("  (2)");

	bool      okay = false;
	GKeyFile* pKF  = g_key_file_new();

	if( pKF )
	{
		DEBUGLOGS("(2) key file successfully created");

		g_key_file_load_from_file(pKF, cfgPath, G_KEY_FILE_KEEP_COMMENTS, NULL);

		Settings cfgTemp = cfgData;
		cnvp(cfgTemp, false);

		DEBUGLOGS("(2) bef getf call");

		const char* eql;
		char        key[1024],  val;
		const char  grp[]    = "General";
		int         f        =  0;
		CfgVar*     pCfgVars =  NULL;
//		int         fmtn     =  getf(cfgData, &pCfgVars);
		int         fmtn     =  getf(cfgTemp, &pCfgVars);

		DEBUGLOGP("(2) aft getf call, fmtn is %d, pCfgVars is %p\n", fmtn, (void*)pCfgVars);

		if( pCfgVars && fmtn )
		{
			foreach( i, fmtn )
			{
				DEBUGLOGP("(2) fmt %d is \"%s\"\n", i, pCfgVars[f].fmt);

				eql = strchr(pCfgVars[f].fmt, '=');
				val = eql[2];

				strvcpy(key, pCfgVars[f].fmt, eql-pCfgVars[f].fmt);
				key[eql-pCfgVars[f].fmt] = '\0';

				DEBUGLOGP("(2) group %s, key %s, value (%c)\n", grp, key, val);

				switch( val )
				{
				case 'b': g_key_file_set_boolean(pKF, grp, key, *((bool *)pCfgVars[f].var)); break;
				case 'd': g_key_file_set_integer(pKF, grp, key, *((int  *)pCfgVars[f].var)); break;
				case 'f': g_key_file_set_double (pKF, grp, key, *((float*)pCfgVars[f].var)); break;
				case 's': g_key_file_set_string (pKF, grp, key,   (char *)pCfgVars[f].var);  break;
				}

				f++;
			}

/*			// TODO: convert animated data usage to cfg data to reduce exe size
			//       and allow for customized animation curves

			{
				static double timeX[] =
				{
					0.000000000, 0.033333333, 0.066666667, 0.100000000, 0.133333333,
					0.166666667, 0.200000000, 0.233333333, 0.266666667, 0.300000000,
					0.333333333, 0.366666667, 0.400000000, 0.433333333, 0.466666667,
					0.500000000, 0.533333333, 0.566666667, 0.600000000, 0.633333333,
					0.666666667, 0.700000000, 0.733333333, 0.766666667, 0.800000000,
					0.833333333, 0.866666667, 0.900000000, 0.933333333, 0.966666667, 1.0
				};
				static double bounceY[] =
				{
					0.000,       0.000,       0.000,       0.000,       0.000,
					0.010,       0.010,       0.020,       0.030,       0.050,
					0.080,       0.110,       0.150,       0.200,       0.260,
					0.320,       0.400,       0.480,       0.570,       0.660,
					0.760,       0.850,       0.940,       1.020,       1.090,
					1.150,       1.180,       1.190,       1.160,       1.100,       1.0
				};
				static double flickY[] =
				{
					0.000,       0.000,       0.000,       0.000,       0.000,
					0.000,       0.000,       0.000,       0.000,       0.000,
					0.000,       0.000,       0.000,       0.000,       0.000,
					0.000,       0.000,       0.000,       0.000,       0.000,
					0.000,       0.005,       0.042,       0.096,       0.200,
					0.450,       1.000,       1.200,       1.180,       1.100,       1.0
				};
				static double smoothY[] =
				{
					0.000000000, 0.033333333, 0.066666667, 0.100000000, 0.133333333,
					0.166666667, 0.200000000, 0.233333333, 0.266666667, 0.300000000,
					0.333333333, 0.366666667, 0.400000000, 0.433333333, 0.466666667,
					0.500000000, 0.533333333, 0.566666667, 0.600000000, 0.633333333,
					0.666666667, 0.700000000, 0.733333333, 0.766666667, 0.800000000,
					0.833333333, 0.866666667, 0.900000000, 0.933333333, 0.966666667, 1.0
				};
				static double timeXn[] = { 0.0, 0.5, 1.0 }; // min of 3 pts for gsl::interp funcs use
				static double rotaYn[] = { 0.0, 0.5, 1.0 };

				const char* group;
				group = "HandAnim0";
				g_key_file_set_string     (pKF, group, "Name",  "Cairo-Clock");
				g_key_file_set_integer    (pKF, group, "Count", vectsz(timeX));
				g_key_file_set_double_list(pKF, group, "X",     timeX,   vectsz(timeX));
				g_key_file_set_double_list(pKF, group, "Y",     bounceY, vectsz(bounceY));

				group = "HandAnim1";
				g_key_file_set_string     (pKF, group, "Name",  "Flick");
				g_key_file_set_integer    (pKF, group, "Count", vectsz(timeX));
				g_key_file_set_double_list(pKF, group, "X",     timeX,   vectsz(timeX));
				g_key_file_set_double_list(pKF, group, "Y",     flickY,  vectsz(flickY));

				group = "HandAnim2";
				g_key_file_set_string     (pKF, group, "Name",  "Sweep");
				g_key_file_set_integer    (pKF, group, "Count", vectsz(timeX));
				g_key_file_set_double_list(pKF, group, "X",     timeX,   vectsz(timeX));
				g_key_file_set_double_list(pKF, group, "Y",     smoothY, vectsz(smoothY));

				group = "HandAnim3";
				g_key_file_set_string     (pKF, group, "Name",  "Simple");
				g_key_file_set_integer    (pKF, group, "Count", vectsz(timeXn));
				g_key_file_set_double_list(pKF, group, "X",     timeXn,  vectsz(timeXn));
				g_key_file_set_double_list(pKF, group, "Y",     rotaYn,  vectsz(rotaYn));
			}*/

			delete [] pCfgVars;
		}

		okay = g_key_file_save_to_file(pKF, cfgPath, NULL);

		g_key_file_free(pKF);
		pKF  = NULL;
	}

	DEBUGLOGE;
	DEBUGLOGS("  (2)");

	return okay;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cfg::cnvp(Settings& cfgData, bool screen)
{
	int dsw = gdk_screen_get_width (gdk_screen_get_default());
	int dsh = gdk_screen_get_height(gdk_screen_get_default());

	if( cfgData.clockW < MIN_CWIDTH )
		cfgData.clockW = MIN_CWIDTH;
//	if( cfgData.clockW > MAX_CWIDTH )
//		cfgData.clockW = MAX_CWIDTH;
	if( cfgData.clockW > dsw )
		cfgData.clockW = dsw;

//	if( is_power_of_two(cfgData.clockW) )  // no longer necessary
//		cfgData.clockW += 1;

	if( cfgData.clockH < MIN_CHEIGHT )
		cfgData.clockH = MIN_CHEIGHT;
//	if( cfgData.clockH > MAX_CHEIGHT )
//		cfgData.clockH = MAX_CHEIGHT;
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

/*		if( true ) // TODO: change this to check the cfg value for its related switch
		{
			if( cfgData.clockX <  0 )
				cfgData.clockX =  0;
			if( cfgData.clockX > (dsw - cfgData.clockW) )
				cfgData.clockX = (dsw - cfgData.clockW);

			if( cfgData.clockY <  0 )
				cfgData.clockY =  0;
			if( cfgData.clockY > (dsh - cfgData.clockH) )
				cfgData.clockY = (dsh - cfgData.clockH);
		}*/
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
const char* cfg::get_user_prefs_path(bool old, bool tst)
{
//	static const char* rcpath_old = "/." APP_NAME "rc";
	static const char* rcpath_old = "/.cairo-clockrc";
	static const char* rcpath_new = "/." APP_NAME "rc";
	static const char* rcpath_tst = "/." APP_NAME "/" APP_NAME "rc";

	const  char* rcpath = old ? rcpath_old : (tst ? rcpath_tst : rcpath_new);
	return get_home_subpath(rcpath, strlen(rcpath));
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
int cfg::getf(Settings& cfgData, CfgVar** ppCfgVars)
{
	enum cfgvar
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
		CFGVAR_WS,
		CFGVAR_C,
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
		CFGVAR_FP,
		CFGVAR_FN,
		CFGVAR_FD2,
		CFGVAR_FT,
		CFGVAR_F12,
		CFGVAR_F24,
		CFGVAR_AS,
		CFGVAR_SH,
		CFGVAR_SM,
		CFGVAR_SHT,
		CFGVAR_PS,
		CFGVAR_PA,
		CFGVAR_PC,
		CFGVAR_FSW
	};

	static CfgVar cfgVars[] =
	{
		{ CFGVAR_X,   "x=%d\n",                 &gCfg.clockX,      sizeof(gCfg.clockX) },
		{ CFGVAR_Y,   "y=%d\n",                 &gCfg.clockY,      sizeof(gCfg.clockY) },
		{ CFGVAR_W,   "width=%d\n",             &gCfg.clockW,      sizeof(gCfg.clockW) },
		{ CFGVAR_H,   "height=%d\n",            &gCfg.clockH,      sizeof(gCfg.clockH) },
		{ CFGVAR_SS,  "show-seconds=%d\n",      &gCfg.showSeconds, sizeof(gCfg.showSeconds) },
		{ CFGVAR_SD,  "show-date=%d\n",         &gCfg.showDate,    sizeof(gCfg.showDate) },
		{ CFGVAR_T,   "theme=%s\n",              gCfg.themeFile,   vectsz(gCfg.themeFile) },
		{ CFGVAR_KOT, "keep-on-top=%d\n",       &gCfg.keepOnTop,   sizeof(gCfg.keepOnTop) },
		{ CFGVAR_AIP, "appear-in-pager=%d\n",   &gCfg.showInPager, sizeof(gCfg.showInPager) },
		{ CFGVAR_AIT, "appear-in-taskbar=%d\n", &gCfg.showInTasks, sizeof(gCfg.showInTasks) },
		{ CFGVAR_S,   "sticky=%d\n",            &gCfg.sticky,      sizeof(gCfg.sticky) },
		{ CFGVAR_24,  "twentyfour=%d\n",        &gCfg.show24Hrs,   sizeof(gCfg.show24Hrs) },
		{ CFGVAR_RR,  "refreshrate=%d\n",       &gCfg.refreshRate, sizeof(gCfg.refreshRate) },
		// 0.4.0 added
		{ CFGVAR_WS,  "workspace=%d\n",         &gCfg.clockWS,     sizeof(gCfg.clockWS) },
		{ CFGVAR_C,   "corner=%d\n",            &gCfg.clockC,      sizeof(gCfg.clockC) },
		{ CFGVAR_FD1, "face-date=%d\n",         &gCfg.faceDate,    sizeof(gCfg.faceDate) },
		{ CFGVAR_KOB, "keep-on-bottom=%d\n",    &gCfg.keepOnBot,   sizeof(gCfg.keepOnBot) },
		{ CFGVAR_TP,  "theme-path=%s\n",         gCfg.themePath,   vectsz(gCfg.themePath) },
		{ CFGVAR_O,   "opacity=%f\n",           &gCfg.opacity,     sizeof(gCfg.opacity) },
		{ CFGVAR_FS,  "font-size=%d\n",         &gCfg.fontSize,    sizeof(gCfg.fontSize) },
		{ CFGVAR_FOY, "font-offy=%f\n",         &gCfg.fontOffY,    sizeof(gCfg.fontOffY) },
		{ CFGVAR_FTR, "font-text-red=%f\n",     &gCfg.dateTextRed, sizeof(gCfg.dateTextRed) },
		{ CFGVAR_FTG, "font-text-green=%f\n",   &gCfg.dateTextGrn, sizeof(gCfg.dateTextGrn) },
		{ CFGVAR_FTB, "font-text-blue=%f\n",    &gCfg.dateTextBlu, sizeof(gCfg.dateTextBlu) },
		{ CFGVAR_FTA, "font-text-alpha=%f\n",   &gCfg.dateTextAlf, sizeof(gCfg.dateTextAlf) },
		{ CFGVAR_FSR, "font-shadow-red=%f\n",   &gCfg.dateShdoRed, sizeof(gCfg.dateShdoRed) },
		{ CFGVAR_FSG, "font-shadow-green=%f\n", &gCfg.dateShdoGrn, sizeof(gCfg.dateShdoGrn) },
		{ CFGVAR_FSB, "font-shadow-blue=%f\n",  &gCfg.dateShdoBlu, sizeof(gCfg.dateShdoBlu) },
		{ CFGVAR_FSA, "font-shadow-alpha=%f\n", &gCfg.dateShdoAlf, sizeof(gCfg.dateShdoAlf) },
		{ CFGVAR_FP,  "font-path=%s\n",          gCfg.fontFace,    vectsz(gCfg.fontFace) },
		{ CFGVAR_FN,  "font-name=%s\n",          gCfg.fontName,    vectsz(gCfg.fontName) },
		{ CFGVAR_FD2, "format-date=%s\n",        gCfg.fmtDate,     vectsz(gCfg.fmtDate) },
		{ CFGVAR_FT,  "format-time=%s\n",        gCfg.fmtTime,     vectsz(gCfg.fmtTime) },
		{ CFGVAR_F12, "format-12Hrs=%s\n",       gCfg.fmt12Hrs,    vectsz(gCfg.fmt12Hrs) },
		{ CFGVAR_F24, "format-24Hrs=%s\n",       gCfg.fmt24Hrs,    vectsz(gCfg.fmt24Hrs) },
		// TODO: newly added - test
		{ CFGVAR_AS,  "animate-startup=%d\n",   &gCfg.aniStartup,  sizeof(gCfg.aniStartup) },
		{ CFGVAR_SH,  "show-hours=%d\n",        &gCfg.showHours,   sizeof(gCfg.showHours) },
		{ CFGVAR_SM,  "show-minutes=%d\n",      &gCfg.showMinutes, sizeof(gCfg.showMinutes) },
		{ CFGVAR_SHT, "second-hand-type=%d\n",  &gCfg.shandType,   sizeof(gCfg.shandType) },
		{ CFGVAR_PS,  "play-sounds=%d\n",       &gCfg.doSounds,    sizeof(gCfg.doSounds) },
		{ CFGVAR_PA,  "play-alarms=%d\n",       &gCfg.doAlarms,    sizeof(gCfg.doAlarms) },
		{ CFGVAR_PC,  "play-chimes=%d\n",       &gCfg.doChimes,    sizeof(gCfg.doChimes) },
		{ CFGVAR_FSW, "font-shadow-width=%f\n", &gCfg.dateShdoWid, sizeof(gCfg.dateShdoWid) },
	};
	const size_t nvars = vectsz(cfgVars);

	void*   var;
	CfgVar* pCfgVars = new CfgVar[nvars];
	*ppCfgVars       = pCfgVars;

	foreach( v, nvars )
	{
		pCfgVars[v].idx = cfgVars[v].idx;
		pCfgVars[v].fmt = cfgVars[v].fmt;
		pCfgVars[v].len = cfgVars[v].len;

		switch( cfgVars[v].idx )
		{
		case CFGVAR_X:   var = &cfgData.clockX;      break;
		case CFGVAR_Y:   var = &cfgData.clockY;      break;
		case CFGVAR_W:   var = &cfgData.clockW;      break;
		case CFGVAR_H:   var = &cfgData.clockH;      break;
		case CFGVAR_SS:  var = &cfgData.showSeconds; break;
		case CFGVAR_SD:  var = &cfgData.showDate;    break;
		case CFGVAR_T:   var =  cfgData.themeFile;   break;
		case CFGVAR_KOT: var = &cfgData.keepOnTop;   break;
		case CFGVAR_AIP: var = &cfgData.showInPager; break;
		case CFGVAR_AIT: var = &cfgData.showInTasks; break;
		case CFGVAR_S:   var = &cfgData.sticky;      break;
		case CFGVAR_24:  var = &cfgData.show24Hrs;   break;
		case CFGVAR_RR:  var = &cfgData.refreshRate; break;
		case CFGVAR_WS:  var = &cfgData.clockWS;     break;
		case CFGVAR_C:   var = &cfgData.clockC;      break;
		case CFGVAR_FD1: var = &cfgData.faceDate;    break;
		case CFGVAR_KOB: var = &cfgData.keepOnBot;   break;
		case CFGVAR_TP:  var =  cfgData.themePath;   break;
		case CFGVAR_O:   var = &cfgData.opacity;     break;
		case CFGVAR_FS:  var = &cfgData.fontSize;    break;
		case CFGVAR_FOY: var = &cfgData.fontOffY;    break;
		case CFGVAR_FTR: var = &cfgData.dateTextRed; break;
		case CFGVAR_FTG: var = &cfgData.dateTextGrn; break;
		case CFGVAR_FTB: var = &cfgData.dateTextBlu; break;
		case CFGVAR_FTA: var = &cfgData.dateTextAlf; break;
		case CFGVAR_FSR: var = &cfgData.dateShdoRed; break;
		case CFGVAR_FSG: var = &cfgData.dateShdoGrn; break;
		case CFGVAR_FSB: var = &cfgData.dateShdoBlu; break;
		case CFGVAR_FSA: var = &cfgData.dateShdoAlf; break;
		case CFGVAR_FP:  var =  cfgData.fontFace;    break;
		case CFGVAR_FN:  var =  cfgData.fontName;    break;
		case CFGVAR_FD2: var =  cfgData.fmtDate;     break;
		case CFGVAR_FT:  var =  cfgData.fmtTime;     break;
		case CFGVAR_F12: var =  cfgData.fmt12Hrs;    break;
		case CFGVAR_F24: var =  cfgData.fmt24Hrs;    break;
		case CFGVAR_AS:  var = &cfgData.aniStartup;  break;
		case CFGVAR_SH:  var = &cfgData.showHours;   break;
		case CFGVAR_SM:  var = &cfgData.showMinutes; break;
		case CFGVAR_SHT: var = &cfgData.shandType;   break;
		case CFGVAR_PS:  var = &cfgData.doSounds;    break;
		case CFGVAR_PA:  var = &cfgData.doAlarms;    break;
		case CFGVAR_PC:  var = &cfgData.doChimes;    break;
		case CFGVAR_FSW: var = &cfgData.dateShdoWid; break;
		}

		pCfgVars[v].var = var;
	}

	return vectsz(cfgVars);
}

