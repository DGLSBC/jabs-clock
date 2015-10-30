/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP      "cli"

#include <stdlib.h>
#include <glib/gi18n.h> // for 'international' text support

#include "global.h"
#include "cfgdef.h"
#include "cmdLine.h"
#include "utility.h"
#include "settings.h"
#include "debug.h"      // for debugging prints

#ifdef __GNUC__
#define  GNUEXT __extension__
#else
#define  GNUEXT
#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
struct cmdSwitch
{
//	GOptionArg   typ; // variable type
//	char         typ; // variable type
	gpointer     val; // entered value
	gpointer     def; // default value
	char         typ; // variable type
	bool         neg; // negate
	bool         req; // required
	bool         usd; // used in the command line
	GOptionEntry opt;
};

static gboolean cl_opt_parg_cb(const gchar*    optName,  const gchar*  optVal, gpointer userData, GError** error);
static gboolean cl_opt_post_cb(GOptionContext* pContext, GOptionGroup* pGroup, gpointer userData, GError** error);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cmdLine(int argc, char** argv, bool& prt_help, bool& prt_themes, bool& prt_version, bool& xscrn_saver)
{
	DEBUGLOGB;

	prt_help = prt_themes = prt_version = xscrn_saver = false;

	if( !gRun.hasParms )
		return;

	DEBUGLOGS("begin processing command line switches");

	#ifndef G_OPTION_FLAG_NONE
	#define G_OPTION_FLAG_NONE 0
	#endif

	Runtime   run, drn;
	Settings  cfg, dfg;

	gchar*    pcTheme    = NULL;
	gboolean  prtHelp    = prt_help    == true;
	gboolean  prtThemes  = prt_themes  == true;
	gboolean  prtVersion = prt_version == true;
	gboolean  xscrnSaver = xscrn_saver == true;
	gboolean  usercfg    = FALSE;
	gint      extents    = 0;

	#define   preCallback                   (gpointer)(GOptionArgFunc)cl_opt_parg_cb
	#define   BOOLOPT(VAR,CMD,CHR,HLP)      &VAR,     &VAR,      'b', false, false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_NONE,     &VAR,        _(HLP), NULL }
	#define   NTEGOPT(VAR,CMD,CHR,HLP,NTEG) &VAR,     &VAR,      'i', false, false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_INT,      &VAR,        _(HLP), NTEG }
	#define   STRGOPT(VAR,CMD,CHR,HLP,STRG) &VAR,      NULL,     's', false, false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_CALLBACK, preCallback, _(HLP), STRG }
	#define   INTBOPT(CFG,CMD,CHR,HLP)      &cfg.CFG, &gCfg.CFG, 'b', false, false, false, { CMD, CHR, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, preCallback, _(HLP), NULL }
	#define   INTFOPT(CFG,CMD,CHR,HLP)      &cfg.CFG, &gCfg.CFG, 'b', true,  false, false, { CMD, CHR, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, preCallback, _(HLP), NULL }
	#define   INTGOPT(CFG,CMD,CHR,HLP,INTG) &cfg.CFG, &gCfg.CFG, 'i', false, false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_CALLBACK, preCallback, _(HLP), INTG }
	#define   DBLGOPT(CFG,CMD,CHR,HLP,DBLG) &cfg.CFG, &gCfg.CFG, 'f', false, false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_CALLBACK, preCallback, _(HLP), DBLG }
	#define   INTROPT(RUN,CMD,CHR,HLP)      &run.RUN, &gRun.RUN, 'B', false, false, false, { CMD, CHR, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, preCallback, _(HLP), NULL }
	#define   RUNIOPT(RUN,CMD,CHR,HLP,RUNI) &run.RUN, &gRun.RUN, 'i', false, false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_CALLBACK, preCallback, _(HLP), RUNI }
	#define   DONEOPT()                      NULL,     NULL,     'x', false, false, false, { NULL }

	GNUEXT
//	static
	cmdSwitch switches[] =
	{
		BOOLOPT(prtHelp,      "help",       '?', "print a usage description and exit (off def.)"),
		BOOLOPT(prtThemes,    "list",       'l', "list installed themes and exit (off def.)"),
		BOOLOPT(prtVersion,   "version",    'v', "print version of program and exit (off def.)\n"),

		BOOLOPT(usercfg,      "usercfg",    'u', "user configuration vals replace defaults (off def.)"),
		INTBOPT(aniStartup,   "anistart",   'a', "animate startup display (off def.)\n"),

		INTGOPT(clockX,       "xposition",  'x', "show at an X offset from CORNER (0 def.)", "X"),
		INTGOPT(clockY,       "yposition",  'y', "show at an Y offset from CORNER (0 def.)", "Y"),
		INTGOPT(clockW,       "width",      'w', "show with a WIDTH width (128 def.)", "WIDTH"),
		INTGOPT(clockH,       "height",     'h', "show with a HEIGHT height (128 def.)", "HEIGHT"),
		NTEGOPT(extents,      "extents",    'e', "show with an EXTENT width/height [0 is off] (0 def.)", "EXTENT"),
		INTGOPT(clockC,       "corner",     'c', "show in this CORNER [0-3 topleft clockwise] (0 def.)\n" "                            "
		                                         "  (a value of 4 may be used for clock centering)", "CORNER"),
		STRGOPT(pcTheme,      "theme",      't', "use theme NAME to draw the clock (<simple> def.)", "NAME"),
		INTGOPT(refreshRate,  "refresh",    'r', "redraw RATE times per second (16 def.)", "RATE"),
		INTFOPT(show24Hrs,    "twelve",     '2', "face divided into 12 hours (am/pm mode) (on def.)"),
		INTBOPT(show24Hrs,    "twentyfour", '4', "face divided into 24 hours (off def.)"),
		INTBOPT(showSeconds,  "seconds",    's', "draw the seconds hand (off def.)"),
		INTBOPT(showDate,     "date",       'd', "draw the date text (off def.)"),
		INTBOPT(faceDate,     "facedate",   'f', "draw the date text below the hands (off def.)"),
		INTBOPT(keepOnBot,    "belowall",   'B', "stay below all windows (off def.)"),
		INTBOPT(keepOnTop,    "ontop",      'o', "stay above all windows (off def.)"),
		INTBOPT(showInPager,  "pager",      'p', "show in the workspace pager (off def.)"),
		INTBOPT(showInTasks,  "taskbar",    'b', "show in the taskbar (off def.)"),
		INTBOPT(sticky,       "sticky",     'i', "show on all workspaces (off def.)"),
//#ifdef   _USEWKSPACE
		INTGOPT(clockWS,      "workspace",  '#', "show only on the #th workspace [0 for all] (0 def.)", "#"),
//#endif
		DBLGOPT(opacity,      "opacity",    'O', "set the clock's OPACITY [0.0 to 1.0] (1.0 def.)", "OPACITY"),
		INTROPT(maximize,     "maximize",   'm', "show as maximized, e.g., fullscreen (off def.)"),
		INTROPT(clickthru,    "clickthru",  'k', "ignore mouse clicks (treat as glass) (off def.)\n" "                            "
		                                         "  (requires showing in the taskbar as well (-b))"),
		INTROPT(appDebug,     "debugspew",  'D', "output built-in debug print spew (off def.)"),
		INTROPT(evalDraws,    "evaldraws",  'E', "evaluate (visualize drawing techniques) (off def.)"),
		BOOLOPT(xscrnSaver,   "fullscreen", 'F', "'fake' fullscreen/screensaver mode (off def.)"),
		INTROPT(nodisplay,    "nodisplay",  'K', "same as -k only no clock is shown (off def.)"),
		INTROPT(portably,     "portable",   'P', "settings are assumed 'next to' the app (off def.)"),
		INTROPT(textonly,     "textonly",   'T', "only the clock face text is shown (off def.)"),
		RUNIOPT(waitSecs1,    "wait1",      'z', "wait SECONDS for the compositor to start (0 def.)", "SECONDS"),
		RUNIOPT(waitSecs2,    "wait2",      'Z', "resend dock info after SECONDS at startup (0 def.)"
		                                         "\n\nGTK+ Options:", "SECONDS"),
		DONEOPT()
	};
	GOptionEntry cmdArgs[vectsz(switches)];

	for( size_t a  = 0; a < vectsz(cmdArgs); a++ )
		cmdArgs[a] = switches[a].opt;

	DEBUGLOGS("bef cmd line switches cntxt processing to override def settings");

	cfg = gCfg;     // copy defaults into the cl-switch-used cfg to be optionally overridden
	run = gRun;     // ditto for the rt settings

	dfg = gCfg;     // copy defaults into the save storage for later possible use
	drn = gRun;     // ditto for the rt settings

	cfg::load();    // load ~/.APP_NAMErc settings values into gCfg
	cfg::prep(cfg); // set cl-switch-used cfg non-cl-switch values to what came from the user cfg, if any

	// at this juncture:
	//   dfg/ drn contain defaults
	//  gCfg/gRun contain defaults + any overridden by the user's config
	//   cfg/ run contain defaults + any overridden by the user's config (non-cl-switch cfg vals only though)

	// setup and process command-line options

	GOptionContext* pContext = g_option_context_new(_("- an analog clock drawn with vector graphics"));

	if( pContext )
	{
		GOptionGroup* pGroup = g_option_group_new(APP_NAME, APP_NAME "-desc", APP_NAME "-help-desc", switches,  NULL);

		DEBUGLOGP("%s\n", pGroup ? "created group" : "failed to create group");

		if( pGroup )
		{
			g_option_group_set_parse_hooks (pGroup,   NULL, cl_opt_post_cb);
			g_option_group_add_entries     (pGroup,   cmdArgs);
			g_option_context_set_main_group(pContext, pGroup);
		}
		else
		{
			g_option_context_add_main_entries(pContext, cmdArgs, APP_NAME);
		}

		g_option_context_add_group      (pContext, gtk_get_option_group(TRUE));
		g_option_context_set_summary    (pContext, APP_NAME " is an enhanced fork of " APP_NAME_OLD " with many new features.");
		g_option_context_set_description(pContext, "See the man page (or get gui Help via the context menu) for a more complete\n  explanation on usage.\n");

		DEBUGLOGS("bef command line parsing");

		g_option_context_parse(pContext, &argc, &argv, NULL);
/*
		if( g_option_context_parse(pContext, &argc, &argv, NULL) )
			DEBUGLOGS("parsing successful");
		else
			DEBUGLOGS("parsing unsuccessful");

		DEBUGLOGS("aft command line parsing");
*/
		g_option_context_free(pContext);
		pContext = NULL;
	}

	DEBUGLOGS("aft cmd line switches cntxt processing to override def settings");

	if( usercfg ) // if any cl-based switch values weren't given on the cl, set them to their cfg values (or default)
	{
		DEBUGLOGS("user cfg usage specified");

		if( pcTheme == NULL )
		{
			DEBUGLOGP("replacing non-entered run theme with cfg theme of %s\n", gCfg.themeFile);
			pcTheme =  gCfg.themeFile;
			strvcpy (cfg.themeFile, gCfg.themeFile);
		}

		// TODO: why is this here and not the other font-related cfg vars?

		if( !strcmp (cfg.fontFace,   dfg.fontFace) )
			 strvcpy(cfg.fontFace,  gCfg.fontFace);
	}
	else
	{
		gCfg = dfg;     // put defaults back into the prep cfg
//		gRun = drn;
		cfg::prep(cfg); // set cl-switch-used cfg non-cl-switch values back to their defaults
	}

	gCfg = cfg;
	gRun = run;

	if( pcTheme && pcTheme != gCfg.themeFile )
	{
		DEBUGLOGP("replacing cfg theme with entered run theme of %s\n", pcTheme);

		strvcpy(gCfg.themeFile, pcTheme);
		g_free (pcTheme);
		pcTheme = NULL;

		DEBUGLOGP("replaced cfg theme is now %s\n", gCfg.themeFile);
	}

	prt_help    = prtHelp    == TRUE;
	prt_themes  = prtThemes  == TRUE;
	prt_version = prtVersion == TRUE;
	xscrn_saver = xscrnSaver == TRUE;

	if( extents )
	{
		gCfg.clockW = extents;
		gCfg.clockH = extents;
	}

	DEBUGLOGS("end processing command line switches");
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
gboolean cl_opt_parg_cb(const gchar* optName, const gchar* optVal, gpointer userData, GError** error)
{
	DEBUGLOGB;
	DEBUGLOGP("  optName=%s, optVal=%s\n", optName, optVal);

	cmdSwitch* pSwitches = (cmdSwitch*)userData;

	for( size_t s = 0; pSwitches[s].val != NULL; s++ )
	{
		if( (optName[1] != '-' &&         optName[1] == pSwitches[s].opt.short_name) ||
		    (optName[1] == '-' && stricmp(optName+2,    pSwitches[s].opt. long_name) == 0) )
		{
			DEBUGLOGP("option %s entered (%s:%s)\n", pSwitches[s].opt.long_name, optName, optVal);

			pSwitches[s].usd = true;
			bool upc         = pSwitches[s].typ >= 'A' && pSwitches[s].typ <= 'Z';

			switch( pSwitches[s].typ )
			{
//			case G_OPTION_ARG_DOUBLE: // treat as a float since that's what's used in my cfg struct
			case 'f':
			case 'F':
				*((gfloat*)pSwitches[s].val) = atof(optVal);
				DEBUGLOGP("float value is %f\n", *((gfloat*)pSwitches[s].val));
				break;

//			case G_OPTION_ARG_INT:
			case 'i':
			case 'I':
				*((gint*)pSwitches[s].val) = atoi(optVal);
				DEBUGLOGP("int value is %d\n", *((gint*)pSwitches[s].val));
				break;

//			case G_OPTION_ARG_NONE: // treat as a boolean
			case 'b':
			case 'B':
				if( upc )
					*((bool*)    pSwitches[s].val) = pSwitches[s].neg ? false : true;
				else
					*((gboolean*)pSwitches[s].val) = pSwitches[s].neg ? FALSE : TRUE;
				DEBUGLOGP("bool value is %s\n", *((gboolean*)pSwitches[s].val) ? "TRUE" : "FALSE");

				// if a negating switch, need to show associated normal switch
				// as being used so it's not changed back to its default in post

				if( pSwitches[s].neg )
				{
					for( size_t o = 0; pSwitches[o].val != NULL; o++ )
					{
						if( pSwitches[o].val == pSwitches[s].val && !pSwitches[o].neg )
						{
							DEBUGLOGP("setting associated switch %s to used\n", pSwitches[o].opt.long_name);
							pSwitches[o].usd =  true;
							break;
						}
					}
				}

				break;

//			case G_OPTION_ARG_STRING:
			case 's':
			case 'S':
				{
				size_t vallen = strlen(optVal) +  1;
				DEBUGLOGP("string lsize is %d\n", (int)vallen);
				  *((gchar**)pSwitches[s].val)  = (gchar*)g_malloc(vallen);
				*(*((gchar**)pSwitches[s].val)) = '\0';
				strvcpy(*((gchar**)pSwitches[s].val), optVal, vallen); // TODO: this is okay?
				DEBUGLOGP("string value is %s\n", *((gchar**)pSwitches[s].val));
				}
				break;
			}

			break;
		}
	}

	DEBUGLOGE;
	return TRUE;
}

// -----------------------------------------------------------------------------
gboolean cl_opt_post_cb(GOptionContext* pContext, GOptionGroup* pGroup, gpointer userData, GError** error)
{
	DEBUGLOGB;

	cmdSwitch* pSwitches = (cmdSwitch*)userData;

	for( size_t s = 0; pSwitches[s].val != NULL; s++ )
	{
/*		if( pSwitches[s].req && !pSwitches[s].usd )
			DEBUGLOGP("required option %s not entered\n", pSwitches[s].opt.long_name);
		else
		if( pSwitches[s].usd )
			DEBUGLOGP("option %s entered\n", pSwitches[s].opt.long_name);
		else*/
/*		if( pSwitches[s].usd == false )
			DEBUGLOGP("option %s not entered\n", pSwitches[s].opt.long_name);*/

		if( pSwitches[s].usd == false )
		{
			bool upc         =  pSwitches[s].typ >= 'A' && pSwitches[s].typ <= 'Z';

			switch( pSwitches[s].typ )
			{
//			case G_OPTION_ARG_DOUBLE: // treat as a float since that's what's used in my cfg struct
			case 'f':
			case 'F':
				*((gfloat*)pSwitches[s].val) =   *((gfloat*)pSwitches[s].def);
				DEBUGLOGP("float value is %f\n", *((gfloat*)pSwitches[s].val));
				break;

//			case G_OPTION_ARG_INT:
			case 'i':
			case 'I':
				*((gint*)pSwitches[s].val) =   *((gint*)pSwitches[s].def);
				DEBUGLOGP("int value is %d\n", *((gint*)pSwitches[s].val));
				break;

//			case G_OPTION_ARG_NONE: // treat as a boolean
			case 'b':
			case 'B':
				if( upc )
					*((bool*)    pSwitches[s].val) = *((bool*)    pSwitches[s].def);
				else
					*((gboolean*)pSwitches[s].val) = *((gboolean*)pSwitches[s].def);
				DEBUGLOGP("bool value is %s\n", *((gboolean*)pSwitches[s].val) ? "TRUE" : "FALSE");
				break;

//			case G_OPTION_ARG_STRING:
			case 's':
			case 'S':
//				*((gchar**)pSwitches[s].val) = *((gchar**)pSwitches[s].def);
				pSwitches[s].val = pSwitches[s].def;
				DEBUGLOGP("string value is %s\n", pSwitches[s].val ? *((gchar**)pSwitches[s].val) : "NULL");
				break;
			}
		}
	}

	DEBUGLOGE;
	return TRUE;
}

