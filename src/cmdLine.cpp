/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP      "cli"

#include <stdlib.h>     // for atof, ...
#include <unistd.h>     // for readlink
#include <glib/gi18n.h> // for 'international' text support
#include "cmdLine.h"    //

#include "cfgdef.h"     //
#include "config.h"     // for Config struct, CornerType enum, ...
#include "global.h"     //
#include "utility.h"    //
#include "settings.h"   //
#include "debug.h"      // for debugging prints

#ifdef  __GNUC__
#define   GNUEXT __extension__
#else
#define   GNUEXT
#endif

// NOTE:  since command line parsing occurs before globally (gCfg) recognizing
//        the debug switch (-D), a new set of macros are used here to this
//        sources debugging info spewed out.

#ifdef    DEBUGLOG
#define   DEBUGLOGBD DEBUGLOGBF
#define   DEBUGLOGPD DEBUGLOGPF
#define   DEBUGLOGSD DEBUGLOGSF
#define   DEBUGLOGED DEBUGLOGEF
#else
#define   DEBUGLOGBD DEBUGLOGB
#define   DEBUGLOGPD DEBUGLOGP
#define   DEBUGLOGSD DEBUGLOGS
#define   DEBUGLOGED DEBUGLOGE
#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
enum varType
{
	VT_IBOOL  = 'b', // int BOOL
	VT_BBOOL  = 'B', // bool
	VT_FLOAT1 = 'f', // double
	VT_FLOAT2 = 'F', // double
	VT_INT1   = 'i', // int
	VT_INT2   = 'I', // int
	VT_STR1   = 's', // char*
	VT_STR2   = 'S', // char*
	VT_DONE   = 'x'  // done indicator
};

struct cmdSwitch
{
	gpointer     val; // entered value
	gpointer     def; // default value
	char         typ; // variable type (enum varType)
	bool         neg; // negate
	bool         req; // required
	bool         usd; // used in the command line
	GOptionEntry opt;
};

static bool cmdLine_internal(bool ucfg, int argc, char** argv, bool& prt_help, bool& prt_themes, bool& prt_version, bool& xscrn_saver, bool& square_up);

static gboolean cl_opt_parg_cb(const gchar*    optName,  const gchar*  optVal, gpointer userData, GError** error);
static gboolean cl_opt_post_cb(GOptionContext* pContext, GOptionGroup* pGroup, gpointer userData, GError** error);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cmdLine(int argc, char** argv, bool& prt_help, bool& prt_themes, bool& prt_version, bool& xscrn_saver, bool& square_up)
{
	DEBUGLOGBD;

	int    argn = argc;
	char** args = NULL;

	if( argn && (args = new char*[argn]) )
	{
		for( int a  = 0; a < argn; a++ )
			args[a] = argv[a];
	}

	if( cmdLine_internal(false, argn, args, prt_help, prt_themes, prt_version, xscrn_saver, square_up) == false )
		cmdLine_internal(true,  argc, argv, prt_help, prt_themes, prt_version, xscrn_saver, square_up);

	if( args )
		delete [] args;

	DEBUGLOGED;
}

// -----------------------------------------------------------------------------
bool cmdLine_internal(bool ucfg, int argc, char** argv, bool& prt_help, bool& prt_themes, bool& prt_version, bool& xscrn_saver, bool& square_up)
{
	DEBUGLOGBD;
	DEBUGLOGPD("incoming ucfg is %s\n", ucfg ? "on" : "off");

	prt_help = prt_themes = prt_version = xscrn_saver = square_up = false;

	if( !gRun.hasParms )
		return true;

	DEBUGLOGSD("begin processing command line switches");

	#ifndef G_OPTION_FLAG_NONE
	#define G_OPTION_FLAG_NONE 0
	#endif

	Config  cfg, dfg;
	Runtime run, drn;

	const char*  empty      = "";
	gchar*       pcConfig   = NULL;
	gchar*       pcTheme    = NULL;
	gboolean     usercfg    = FALSE;
	gboolean     usercfgsv  = FALSE;
	gboolean     prtHelp    = prt_help      == true;
	gboolean     prtThemes  = prt_themes    == true;
	gboolean     prtVersion = prt_version   == true;
	gboolean     xscrnSaver = xscrn_saver   == true;
	gboolean     squareUp   = square_up     == true;
	gint         offsets    = 51423, offorig = offsets;
	gint         extents    = 0,     extorig = extents;

	#define   preCallback                   (gpointer)(GOptionArgFunc)cl_opt_parg_cb
	#define   BOOLOPT(VAR,CMD,CHR,HLP)      &VAR,     &VAR,      VT_IBOOL,  false, false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_NONE,     &VAR,        _(HLP), NULL }
	#define   NTEGOPT(VAR,CMD,CHR,HLP,NTEG) &VAR,     &VAR,      VT_INT1,   false, false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_INT,      &VAR,        _(HLP), NTEG }
	#define   STRGOPT(VAR,CMD,CHR,HLP,STRG) &VAR,      NULL,     VT_STR1,   false, false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_CALLBACK, preCallback, _(HLP), STRG }
	#define   INTBOPT(CFG,CMD,CHR,HLP)      &cfg.CFG, &gCfg.CFG, VT_IBOOL,  false, false, false, { CMD, CHR, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, preCallback, _(HLP), NULL }
	#define   INTFOPT(CFG,CMD,CHR,HLP)      &cfg.CFG, &gCfg.CFG, VT_IBOOL,  true,  false, false, { CMD, CHR, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, preCallback, _(HLP), NULL }
	#define   INTGOPT(CFG,CMD,CHR,HLP,INTG) &cfg.CFG, &gCfg.CFG, VT_INT1,   false, false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_CALLBACK, preCallback, _(HLP), INTG }
	#define   DBLGOPT(CFG,CMD,CHR,HLP,DBLG) &cfg.CFG, &gCfg.CFG, VT_FLOAT1, false, false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_CALLBACK, preCallback, _(HLP), DBLG }
	#define   INTROPT(RUN,CMD,CHR,HLP)      &run.RUN, &gRun.RUN, VT_BBOOL,  false, false, false, { CMD, CHR, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, preCallback, _(HLP), NULL }
	#define   RUNIOPT(RUN,CMD,CHR,HLP,RUNI) &run.RUN, &gRun.RUN, VT_INT1,   false, false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_CALLBACK, preCallback, _(HLP), RUNI }

	#define   BOOLNEG(VAR,CMD,CHR,HLP)      &VAR,     &VAR,      VT_IBOOL,  true,  false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_NONE,     &VAR,        _(HLP), NULL }
	#define   NTEGNEG(VAR,CMD,CHR,HLP,NTEG) &VAR,     &VAR,      VT_INT1,   true,  false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_INT,      &VAR,        _(HLP), NTEG }
	#define   STRGNEG(VAR,CMD,CHR,HLP,STRG) &VAR,      NULL,     VT_STR1,   true,  false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_CALLBACK, preCallback, _(HLP), STRG }
	#define   INTBNEG(CFG,CMD,CHR,HLP)      &cfg.CFG, &gCfg.CFG, VT_IBOOL,  true,  false, false, { CMD, CHR, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, preCallback, _(HLP), NULL }
	#define   INTFNEG(CFG,CMD,CHR,HLP)      &cfg.CFG, &gCfg.CFG, VT_IBOOL,  false, false, false, { CMD, CHR, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, preCallback, _(HLP), NULL }
	#define   INTGNEG(CFG,CMD,CHR,HLP,INTG) &cfg.CFG, &gCfg.CFG, VT_INT1,   true,  false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_CALLBACK, preCallback, _(HLP), INTG }
	#define   DBLGNEG(CFG,CMD,CHR,HLP,DBLG) &cfg.CFG, &gCfg.CFG, VT_FLOAT1, true,  false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_CALLBACK, preCallback, _(HLP), DBLG }
	#define   INTRNEG(RUN,CMD,CHR,HLP)      &run.RUN, &gRun.RUN, VT_BBOOL,  true,  false, false, { CMD, CHR, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, preCallback, _(HLP), NULL }
	#define   RUNINEG(RUN,CMD,CHR,HLP,RUNI) &run.RUN, &gRun.RUN, VT_INT1,   true,  false, false, { CMD, CHR, G_OPTION_FLAG_NONE,   G_OPTION_ARG_CALLBACK, preCallback, _(HLP), RUNI }

	#define   DONEOPT()                      NULL,     NULL,     VT_DONE,   false, false, false, { NULL }

	enum CliOptSwitch
	{
		// specials
		CLI_WORKSPACE  = '#',               // #
		CLI_HELP       = HOTKEY_HELP,       // ?
		CLI_TWELVE     = HOTKEY_TWELVE,     // 2
		CLI_TWENTYFOUR = HOTKEY_TWENTYFOUR, // 4
		// lowercase
		CLI_ANISTART   = 'a',               // a
		CLI_TASKBAR    = HOTKEY_TASKBAR,    // b
		CLI_CORNER     = 'c',               // c (hotkey center)
		CLI_DATE       = HOTKEY_DATE,       // d
		CLI_EXTENTS    = 'e',               // e
		CLI_FACEDATE   = HOTKEY_FACEDATE,   // f
		                                    // g - open
		CLI_HEIGHT     = 'h',               // h
		CLI_STICKY     = HOTKEY_STICKY,     // i
		                                    // j - open
		CLI_CLICKTHRU  = HOTKEY_CLICKTHRU,  // k
		CLI_LIST       = 'l',               // l
		CLI_MINIMIZE   = HOTKEY_MINIMIZE,   // m
		                                    // n - open
		CLI_OFFSETS    = 'o',               // o (hotkey opacity down)
		CLI_PAGER      = HOTKEY_PAGER,      // p
		                                    // q - open (hotkey quit 1)
		CLI_REFRESH    = HOTKEY_REFRESHDN,  // r
		CLI_SECONDS    = HOTKEY_SECONDS,    // s
		CLI_THEME      = 't',               // t
		CLI_USERCFGNS  = 'u',               // u
		CLI_VERSION    = 'v',               // v
		CLI_WIDTH      = 'w',               // w
		CLI_XPOSITION  = 'x',               // x
		CLI_YPOSITION  = 'y',               // y
		CLI_WAIT1      = 'z',               // z
		// uppercase
		CLI_ABOVEALL   = HOTKEY_ABOVEALL,   // A
		CLI_BELOWALL   = HOTKEY_BELOWALL,   // B
		CLI_CONFIG     = 'C',               // C - not used yet
		CLI_DEBUGSPEW  = 'D',               // D
		CLI_EVALDRAWS  = HOTKEY_EVALDRAWS,  // E
		CLI_FULLSCREEN = 'F',               // F (hotkey focus)
		                                    // G - open
		CLI_HANDSONLY  = HOTKEY_HANDSONLY,  // H
		                                    // I - open
		                                    // J - open
		CLI_NODISPLAY  = HOTKEY_NODISPLAY,  // K
		                                    // L - open
		CLI_MAXIMIZE   = HOTKEY_MAXIMIZE,   // M
		                                    // N - open
		CLI_OPACITY    = HOTKEY_OPACITYUP,  // O
		CLI_PORTABLE   = 'P',               // P
		                                    // Q - open (hotkey quit 2)
		                                    // R - open (hotkey refresh up)
		CLI_SQUAREUP   = HOTKEY_SQUAREUP,   // S
		CLI_TEXTONLY   = HOTKEY_TEXTONLY,   // T
		CLI_USERCFGSV  = 'U',               // U
		                                    // V - open
		                                    // W - open
		                                    // X - open
		                                    // Y - open
		CLI_WAIT2      = 'Z'                // Z
	};

	GNUEXT
	static
	cmdSwitch switchesHelp[] =
	{
		BOOLOPT(prtHelp,      "help",       CLI_HELP,       "print a usage description then exit [off]"),
		BOOLOPT(prtThemes,    "list",       CLI_LIST,       "list installed and usable themes then exit [off]"),
		BOOLOPT(prtVersion,   "version",    CLI_VERSION,    "print the app version then exit [off]\n"),

		BOOLOPT(usercfg,      "usercfg",    CLI_USERCFGNS,  "user configuration values replace defaults [off]"),
		BOOLOPT(usercfgsv,    "usercfgsv",  CLI_USERCFGSV,  "same as -u but changes are saved to the cfg [off]"),
		STRGOPT(pcConfig,     "config",     CLI_CONFIG,     "CFGPATH configuration values replace defaults [off]", "CFGPATH"),
		INTROPT(portably,     "portable",   CLI_PORTABLE,   "settings are assumed to be 'next to' the app [off]\n"),

		INTGOPT(clockX,       "xposition",  CLI_XPOSITION,  "offset clock CORNER horiz. X pixels from CORNER [0]", "X"),
		INTGOPT(clockY,       "yposition",  CLI_YPOSITION,  "offset clock CORNER vert.  Y pixels from CORNER [0]", "Y"),
		INTGOPT(clockW,       "width",      CLI_WIDTH,      "show with a clock width of WIDTH [128]", "WIDTH"),
		INTGOPT(clockH,       "height",     CLI_HEIGHT,     "show with a clock height of HEIGHT [128]", "HEIGHT"),
		NTEGOPT(extents,      "extents",    CLI_EXTENTS,    "show with WIDTH/HEIGHT of EXTENT (0 to not use) [0]", "EXTENT"),
		NTEGOPT(offsets,      "offsets",    CLI_OFFSETS,    "show with X/Y offsets of OFFSET [default to unused]", "OFFSET"),
		INTGOPT(clockC,       "corner",     CLI_CORNER,     "show in this CORNER (0-3 topleft clockwise) [0]\n" "                            "
		                                                    "  (a value of 4 may be used for screen centering)\n", "CORNER"),

		STRGOPT(pcTheme,      "theme",      CLI_THEME,      "use theme NAME to draw the clock [<default>]\n" "                            "
		                                                    "  (use double quotes if the NAME contains spaces)\n" "                            "
		                                                    "  (NAME is the theme directory or listed name)\n", "NAME"),

		INTBOPT(aniStartup,   "anistart",   CLI_ANISTART,   "animate the starting of the clock [off]"),
		INTGOPT(renderRate,   "refresh",    CLI_REFRESH,    "redraw RATE times per second [16]", "RATE"),
		INTFOPT(show24Hrs,    "twelve",     CLI_TWELVE,     "divide the face into 12 hours (am/pm mode) [on]"),
		INTBOPT(show24Hrs,    "twentyfour", CLI_TWENTYFOUR, "divide the face into 24 hours [off]"),
		INTBOPT(showSeconds,  "seconds",    CLI_SECONDS,    "draw the seconds hand [off]"),
		INTBOPT(showDate,     "date",       CLI_DATE,       "draw the date (and other associated) text [off]"),
		INTBOPT(faceDate,     "facedate",   CLI_FACEDATE,   "date text drawn below the hands [off]"),
		INTBOPT(keepOnTop,    "aboveall",   CLI_ABOVEALL,   "request to position above all other windows [off]"),
		INTBOPT(keepOnBot,    "belowall",   CLI_BELOWALL,   "request to position below all other windows [off]"),
		INTBOPT(showInTasks,  "taskbar",    CLI_TASKBAR,    "request to show in the taskbar [off]"),
		INTBOPT(showInPager,  "pager",      CLI_PAGER,      "request to show in the workspace pager [off]"),
		INTBOPT(sticky,       "sticky",     CLI_STICKY,     "request to show on all workspaces [off]"),
		INTGOPT(clockWS,      "workspace",  CLI_WORKSPACE,  "show only on the MASK'd workspaces (0 for all) [0]\n" "                            "
		                                                    "  (1=workspace 1, 2=2, 3=1 & 2, 4=3, 5=1 & 3, ...)", "MASK"),
		INTROPT(minimize,     "minimize",   CLI_MINIMIZE,   "show a minimized (iconified) clock on startup [off]"),
		INTROPT(maximize,     "maximize",   CLI_MAXIMIZE,   "show with a WIDTH/HEIGHT of the screen [off]"),
		BOOLOPT(xscrnSaver,   "fullscreen", CLI_FULLSCREEN, "'fake' fullscreen/screensaver mode [off]"),
		BOOLOPT(squareUp,     "squareup",   CLI_SQUAREUP,   "maximize/fullscreen clocks are forced square [off]"),
		INTBOPT(clickThru,    "clickthru",  CLI_CLICKTHRU,  "ignore mouse clicks (treat as glass) [off]\n" "                            "
		                                                    "  (requires showing in the taskbar as well (-b))"),
		INTBOPT(noDisplay,    "nodisplay",  CLI_NODISPLAY,  "same as -k only no clock is shown [off]"),
		INTBOPT(handsOnly,    "handsonly",  CLI_HANDSONLY,  "only the clock face hands are shown [off]"),
		DBLGOPT(opacity,      "opacity",    CLI_OPACITY,    "set the clock's OPACITY (0.0 to 1.0) [1.0]", "OPACITY"),
		INTBOPT(textOnly,     "textonly",   CLI_TEXTONLY,   "only the clock face text is shown [off]"),
		INTROPT(appDebug,     "debugspew",  CLI_DEBUGSPEW,  "output built-in debug print spew [off]"),
		INTROPT(evalDraws,    "evaldraws",  CLI_EVALDRAWS,  "evaluate (visualize drawing techniques) [off]"),
		RUNIOPT(waitSecs1,    "wait1",      CLI_WAIT1,      "wait SECONDS for the compositor to start [0]", "SECONDS"),
#if _USEGTK
		RUNIOPT(waitSecs2,    "wait2",      CLI_WAIT2,      "resend dock info after SECONDS at startup [0]"
		                                                    "\n\nGTK+ Options:", "SECONDS"),
#else
		RUNIOPT(waitSecs2,    "wait2",      CLI_WAIT2,      "resend dock info after SECONDS at startup [0]", "SECONDS"),
#endif
		DONEOPT()
	};

	GNUEXT
//	static
	cmdSwitch switches[] =
	{
		BOOLOPT(prtHelp,      "help",         CLI_HELP,       empty),
		BOOLOPT(prtThemes,    "list",         CLI_LIST,       empty),
		BOOLOPT(prtVersion,   "version",      CLI_VERSION,    empty),
		BOOLOPT(usercfg,      "usercfg",      CLI_USERCFGNS,  empty),
		BOOLOPT(usercfgsv,    "usercfgsv",    CLI_USERCFGSV,  empty),
		STRGOPT(pcConfig,     "config",       CLI_CONFIG,     empty, empty),
		INTROPT(portably,     "portable",     CLI_PORTABLE,   empty),
		INTBOPT(aniStartup,   "anistart",     CLI_ANISTART,   empty),
		INTBNEG(aniStartup,   "no-anistart",  0,              empty),
		INTGOPT(clockX,       "xposition",    CLI_XPOSITION,  empty, empty),
		INTGOPT(clockY,       "yposition",    CLI_YPOSITION,  empty, empty),
		INTGOPT(clockW,       "width",        CLI_WIDTH,      empty, empty),
		INTGOPT(clockH,       "height",       CLI_HEIGHT,     empty, empty),
		NTEGOPT(extents,      "extents",      CLI_EXTENTS,    empty, empty),
		NTEGOPT(offsets,      "offsets",      CLI_OFFSETS,    empty, empty),
		INTGOPT(clockC,       "corner",       CLI_CORNER,     empty, empty),
		STRGOPT(pcTheme,      "theme",        CLI_THEME,      empty, empty),
		INTGOPT(renderRate,   "refresh",      CLI_REFRESH,    empty, empty),
		INTFOPT(show24Hrs,    "twelve",       CLI_TWELVE,     empty),
		INTBOPT(show24Hrs,    "twentyfour",   CLI_TWENTYFOUR, empty),
		INTBOPT(showSeconds,  "seconds",      CLI_SECONDS,    empty),
		INTBNEG(showSeconds,  "no-seconds",   0,              empty),
		INTBOPT(showDate,     "date",         CLI_DATE,       empty),
		INTBNEG(showDate,     "no-date",      0,              empty),
		INTBOPT(faceDate,     "facedate",     CLI_FACEDATE,   empty),
		INTBNEG(faceDate,     "no-facedate",  0,              empty),
		INTBOPT(keepOnTop,    "aboveall",     CLI_ABOVEALL,   empty),
		INTBNEG(keepOnTop,    "no-aboveall",  0,              empty),
		INTBOPT(keepOnBot,    "belowall",     CLI_BELOWALL,   empty),
		INTBNEG(keepOnBot,    "no-belowall",  0,              empty),
		INTBOPT(showInTasks,  "taskbar",      CLI_TASKBAR,    empty),
		INTBNEG(showInTasks,  "no-taskbar",   0,              empty),
		INTBOPT(showInPager,  "pager",        CLI_PAGER,      empty),
		INTBNEG(showInPager,  "no-pager",     0,              empty),
		INTBOPT(sticky,       "sticky",       CLI_STICKY,     empty),
		INTBNEG(sticky,       "no-sticky",    0,              empty),
		INTGOPT(clockWS,      "workspace",    CLI_WORKSPACE,  empty, empty),
		INTROPT(minimize,     "minimize",     CLI_MINIMIZE,   empty),
		INTROPT(maximize,     "maximize",     CLI_MAXIMIZE,   empty),
		BOOLOPT(xscrnSaver,   "fullscreen",   CLI_FULLSCREEN, empty),
		BOOLOPT(squareUp,     "squareUp",     CLI_SQUAREUP,   empty),
		INTBOPT(clickThru,    "clickthru",    CLI_CLICKTHRU,  empty),
		INTBNEG(clickThru,    "no-clickthru", 0,              empty),
		INTBOPT(noDisplay,    "nodisplay",    CLI_NODISPLAY,  empty),
		INTBNEG(noDisplay,    "no-nodisplay", 0,              empty),
		INTBOPT(handsOnly,    "handsonly",    CLI_HANDSONLY,  empty),
		INTBNEG(handsOnly,    "no-handsonly", 0,              empty),
		DBLGOPT(opacity,      "opacity",      CLI_OPACITY,    empty, empty),
		INTBOPT(textOnly,     "textonly",     CLI_TEXTONLY,   empty),
		INTBNEG(textOnly,     "no-textonly",  0,              empty),
		INTROPT(appDebug,     "debugspew",    CLI_DEBUGSPEW,  empty),
		INTROPT(evalDraws,    "evaldraws",    CLI_EVALDRAWS,  empty),
		RUNIOPT(waitSecs1,    "wait1",        CLI_WAIT1,      empty, empty),
		RUNIOPT(waitSecs2,    "wait2",        CLI_WAIT2,      empty, empty),
		DONEOPT()
	};

	GOptionEntry cmdArgs[vectsz(switches)];

	for( size_t a  = 0; a < vectsz(cmdArgs); a++ )
		cmdArgs[a] = switches[a].opt;

	DEBUGLOGSD("bef cmd line switches cntxt processing to override def settings");

	cfg = gCfg;     // copy defaults into the cl-switch-used cfg to be optionally overridden
	run = gRun;     // ditto for the rt settings

	dfg = gCfg;     // copy defaults into the save storage for later possible use
	drn = gRun;     // ditto for the rt settings

	cfg::load();    // load ~/.APP_NAMErc settings values into gCfg
	cfg::prep(cfg); // set cl-switch-used cfg non-cl-switch values to what came from the user cfg, if any

	DEBUGLOGPD("initial config loaded theme path is *%s*\n", gCfg.themePath);
	DEBUGLOGPD("initial config loaded theme file is *%s*\n", gCfg.themeFile);

	// at this juncture:
	//   dfg/ drn contain defaults
	//  gCfg/gRun contain defaults + any overridden by the user's config
	//   cfg/ run contain defaults + any overridden by the user's config (non-cl-switch cfg vals only)

	const gchar* appdesc = _("- an analog clock drawn with vector graphics");

	// setup and process command-line options

	if( argc > 1 )
	{
		DEBUGLOGPD("there are %d cli args left; 1st is %s, 2nd is %s\n", argc, argv[0], argv[1]);

		GOptionContext* pContext = g_option_context_new(_(""));

		if( pContext )
		{
			GOptionEntry cmdHelp[] = { "help", '?', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &prtHelp, _(""), NULL, NULL };

			g_option_context_set_help_enabled(pContext, FALSE);
			g_option_context_add_main_entries(pContext, cmdHelp, APP_NAME);
#if _USEGTK
			g_option_context_add_group(pContext, gtk_get_option_group(TRUE));
#endif
			g_option_context_parse(pContext, &argc, &argv, NULL);
			g_option_context_free(pContext);
			pContext = NULL;

			DEBUGLOGPD("help is %s\n", prtHelp ? "requested" : "NOT requested");
			DEBUGLOGPD("there are %d cli args left; 1st is %s, 2nd is %s\n", argc, argv[0], argv[1]);

			if( prtHelp )
			{
				char    argv1[4];
				strvcpy(argv1, "-?");

				argc    = 2;
				argv[1] = argv1;

				if( pContext = g_option_context_new(appdesc) )
				{
					for( size_t a  = 0; a < vectsz(switchesHelp); a++ )
						cmdArgs[a] = switchesHelp[a].opt;

					g_option_context_set_help_enabled(pContext, TRUE);
					g_option_context_add_main_entries(pContext, cmdArgs, APP_NAME);
#if _USEGTK
					g_option_context_add_group       (pContext, gtk_get_option_group(TRUE));
#endif
					g_option_context_set_summary     (pContext, APP_NAME " is a much enhanced fork of " APP_NAME_OLD " with many new features.");

					g_option_context_set_description (pContext,
						"Running the app without any switches causes the app to use the current user's\n"
						"  configuration settings. Otherwise, no configuration settings are used, unless\n"
						"  the -u switch is used. Then the app uses the configuration settings to over-\n"
						"  ride the normal defaults.\n"
						"\n"
						"Where it makes sense, on/off switches have a --no- variation, normally used\n"
						"  to override any undesired user configuration settings, e.g., --no-seconds.\n"
						"\n"
						"See the man page (or get gui Help via the context menu) for a more complete\n"
						"  explanation on usage.\n");

					g_option_context_parse(pContext, &argc, &argv, NULL);
					g_option_context_free(pContext);

					pContext = NULL;
				}

				DEBUGLOGED;
				return true;
			}
		}
	}

	DEBUGLOGPD("there are %d cli args left; 1st is %s, 2nd is %s\n", argc, argv[0], argv[1]);

	GOptionContext* pContext = g_option_context_new(appdesc);

	if( pContext )
	{
		GOptionGroup* pGroup = g_option_group_new(APP_NAME, APP_NAME "-desc", APP_NAME "-help-desc", switches, NULL);

		DEBUGLOGPD("%s\n", pGroup ? "created group" : "failed to create group");

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

#if _USEGTK
		g_option_context_add_group      (pContext, gtk_get_option_group(TRUE));
#endif
		g_option_context_set_summary    (pContext, APP_NAME " is an enhanced fork of " APP_NAME_OLD " with many new features.");
		g_option_context_set_description(pContext, "See the man page (or get gui Help via the context menu) for a more complete\n  explanation on usage.\n");

		DEBUGLOGSD("bef command line parsing");

		g_option_context_parse(pContext, &argc, &argv, NULL);
#if 0
		if( g_option_context_parse(pContext, &argc, &argv, NULL) )
			DEBUGLOGSD("parsing successful");
		else
			DEBUGLOGSD("parsing unsuccessful");

		DEBUGLOGSD("aft command line parsing");
#endif
		g_option_context_free(pContext);
		pContext = NULL;
	}

	DEBUGLOGSD("aft cmd line switches cntxt processing to override def settings");

	if( usercfgsv )
	{
		usercfg = TRUE;
		gRun.cfgSaves = run.cfgSaves = true;
	}

	// workaround for a problem where -u must be used to output a list of 'portably' available themes

	if( run.portably && prtThemes )
		usercfg = TRUE;
#if 1
	if( pcConfig )
	{
		DEBUGLOGPD("entered run config of %s\n", pcConfig);

		strvcpy(run.appHome, pcConfig); // TODO: does this work?

		g_free(pcConfig);
//		pcConfig = NULL; // NOTE: don't reset here since using below

		usercfg  = TRUE; // to keep the dfg defaults from being put back into the cfg
		gRun     = run;  // NOTE: need to get the runtime appHome into global now
/*		gCfg     = cfg;
		gRun     = run;

//		cfg::load();     // load /APP_HOME/.APP_NAMErc settings values into gCfg
		cfg::load();     // load pcConfig settings values into gCfg
		cfg::prep(cfg);  // put gCfg non-cl switch vars into cfg

		cl_opt_post_cb(NULL, NULL, switches, NULL);*/
	}
#endif

	if( usercfg ) // if any cl-based switch values weren't given on the cl, set them to their cfg values (or default)
//	if( usercfg && !ucfg ) // if any cl-based switch values weren't given on the cl, set them to their cfg values (or default)
	{
		DEBUGLOGSD("user cfg usage specified");

		if( run.portably )
		{
			DEBUGLOGSD("portable user cfg usage specified");

			readlink  ("/proc/self/exe",   run.appHome, vectsz(run.appHome));
			DEBUGLOGPD("readlink: %s\n",   run.appHome);
			char* dir = g_path_get_dirname(run.appHome);
			DEBUGLOGPD("app dir:  %s\n",   dir);
			strvcpy   (run.appHome,        dir ? dir : "./"); // TODO: works-for-me - more testing needed?
			DEBUGLOGPD("app home: %s\n",   run.appHome);
			g_free    (dir);
			dir  = NULL;

			gRun = run; // NOTE: need to get the runtime appHome into global now

/*			// at this point, dfg contains the app defaulted values
			//               gCfg contains the user-cfg file values
			//                cfg contains the command line  values, defaulted from gCfg

			// cfg::prep sets cfg fields to corresponding gCfg field vals for those
			// fields that do not have a command line switch

			// calling cl_opt_post_cb again sets cfg fields to corresponding gCfg
			// field vals for those fields that do have a command line switch but
			// have not been entered on the command line
#if 1
			cfg::init();
//			gCfg = dfg;     // put defaults back into the cfg::prep's source cfg
//			cfg  = dfg;
			cfg  = dfg = gCfg;
//			gRun = drn;
//			cfg::prep(cfg);
#else
			gCfg = cfg;
			gRun = run;
#endif
			cfg::load();    // load /APP_HOME/.APP_NAMErc settings values into gCfg
			cfg::prep(cfg); // put gCfg non-cl switch vars into cfg

			cl_opt_post_cb(NULL, NULL, switches, NULL);*/
		}
#if 1
		if( pcTheme == NULL )
		{
			DEBUGLOGPD("replacing non-entered run theme with cfg theme of %s\n", gCfg.themeFile);
			pcTheme =  gCfg.themeFile;
			strvcpy(cfg.themeFile, gCfg.themeFile);
		}

/*		// TODO: why is this here and not the other font-related cfg vars?
//
//		if( !strcmp (cfg.fontFace,  dfg.fontFace) )
//			 strvcpy(cfg.fontFace, gCfg.fontFace);*/
#endif
		if( !ucfg )
		{
			DEBUGLOGSD("returning early: ucfg off; user cfg usage specified");
			DEBUGLOGED;
			gCfg = dfg; // back to defaults for all
			gRun = drn;
			if( run.portably || pcConfig )
				strvcpy(gRun.appHome, run.appHome);
			return false;
		}
	}
	else
	{
		DEBUGLOGSD("user cfg usage NOT specified");

		// at this point, dfg contains the app defaulted values
		//               gCfg contains the user-cfg file values
		//                cfg contains the command line  values, defaulted from gCfg

		// cfg::prep sets cfg fields to corresponding gCfg field vals for those
		// fields that do not have a command line switch

		// calling cl_opt_post_cb again sets cfg fields to corresponding gCfg
		// field vals for those fields that do have a command line switch but
		// have not been entered on the command line

		gCfg = dfg; // put defaults back into the cfg::prep's source cfg
//		gRun = drn;
		cfg::prep(cfg);

		cl_opt_post_cb(NULL, NULL, switches, NULL);
	}

	gCfg = cfg;
	gRun = run;

	if( pcTheme && pcTheme != gCfg.themeFile )
	{
		DEBUGLOGPD("replacing cfg theme with entered run theme of %s\n", pcTheme);

		strvcpy(gCfg.themeFile, pcTheme);
		g_free (pcTheme);

		gCfg.themePath[0] = '\0';
		pcTheme = NULL;

		DEBUGLOGPD("replaced cfg theme is now %s\n", gCfg.themeFile);
	}

	prt_help    = prtHelp    == TRUE;
	prt_themes  = prtThemes  == TRUE;
	prt_version = prtVersion == TRUE;
	xscrn_saver = xscrnSaver == TRUE;
	square_up   = squareUp   == TRUE;

	// TODO: need a cl_opt_used(switch-var) for offsets & extents 'entered' testing
	//       instead of what's currently used

	if( offsets != offorig )
	{
		gCfg.clockX = offsets;
		gCfg.clockY = offsets;
	}

	if( extents != extorig )
	{
		gCfg.clockW = extents;
		gCfg.clockH = extents;
	}

	DEBUGLOGPD("final config loaded theme path is *%s*\n", gCfg.themePath);
	DEBUGLOGPD("final config loaded theme file is *%s*\n", gCfg.themeFile);

	DEBUGLOGSD("end processing command line switches");
	DEBUGLOGED;

	return usercfg ? false : true;
}

// -----------------------------------------------------------------------------
gboolean cl_opt_parg_cb(const gchar* optName, const gchar* optVal, gpointer userData, GError** error)
{
	DEBUGLOGBD;
	DEBUGLOGPD("  optName=%s, optVal=%s\n", optName, optVal);

	cmdSwitch* pSwitches = (cmdSwitch*)userData;

	for( size_t s = 0; pSwitches[s].val != NULL; s++ )
	{
		if( (optName[1] != '-' &&         optName[1] == pSwitches[s].opt.short_name) ||
		    (optName[1] == '-' && stricmp(optName+2,    pSwitches[s].opt. long_name) == 0) )
		{
			DEBUGLOGPD("option %s entered (%s:%s)\n", pSwitches[s].opt.long_name, optName, optVal);

			pSwitches[s].usd = true;
			bool upc         = pSwitches[s].typ >= 'A' && pSwitches[s].typ <= 'Z';

			switch( pSwitches[s].typ )
			{
//			case G_OPTION_ARG_DOUBLE:
			case VT_FLOAT1:
			case VT_FLOAT2:
				*((double*)pSwitches[s].val) = atof(optVal);
				DEBUGLOGPD("float value is %f\n", (float)(*((double*)pSwitches[s].val)));
				break;

//			case G_OPTION_ARG_INT:
			case VT_INT1:
			case VT_INT2:
				*((int*)pSwitches[s].val) = atoi(optVal);
				DEBUGLOGPD("int value is %d\n", *((int*)pSwitches[s].val));
				break;

//			case G_OPTION_ARG_NONE: // treat as a boolean or an int-bool
			case VT_BBOOL:
			case VT_IBOOL:
				if( upc )
				{
					*((bool*)pSwitches[s].val) = pSwitches[s].neg ? false : true;
					DEBUGLOGPD("bool value is %s\n", *((bool*)pSwitches[s].val) ? "true" : "false");
				}
				else
				{
					*((int*)pSwitches[s].val) = pSwitches[s].neg ? FALSE : TRUE;
					DEBUGLOGPD("int-bool value is %s\n", *((int*)pSwitches[s].val) ? "TRUE" : "FALSE");
				}

				// need to set associated 'opposite' switch as being used so it's
				//   associated var isn't changed back to its default in post

				for( size_t o = 0; pSwitches[o].val != NULL; o++ )
				{
					if( pSwitches[o].val == pSwitches[s].val && o != s )
					{
						DEBUGLOGPD("setting associated switch %s to used\n", pSwitches[o].opt.long_name);
						pSwitches[o].usd =  true;
						break;
					}
				}

				break;

//			case G_OPTION_ARG_STRING:
			case VT_STR1:
			case VT_STR2:
				{
				size_t vallen = strlen(optVal) +  1;
				DEBUGLOGPD("string lsize is %d\n", (int)vallen);
				  *((gchar**)pSwitches[s].val)  = (gchar*)g_malloc(vallen);
				*(*((gchar**)pSwitches[s].val)) = '\0';
				strvcpy(*((gchar**)pSwitches[s].val), optVal, vallen); // TODO: this is okay?
				DEBUGLOGPD("string value is %s\n", *((gchar**)pSwitches[s].val));
				}
				break;
			}

			break;
		}
	}

	DEBUGLOGED;
	return TRUE;
}

// -----------------------------------------------------------------------------
gboolean cl_opt_post_cb(GOptionContext* pContext, GOptionGroup* pGroup, gpointer userData, GError** error)
{
	DEBUGLOGBD;

	cmdSwitch* pSwitches = (cmdSwitch*)userData;

	for( size_t s = 0; pSwitches[s].val != NULL; s++ )
	{
#if 0
#ifdef DEBUGLOG
		if( pSwitches[s].req && !pSwitches[s].usd )
			DEBUGLOGPD("required option %s not entered\n", pSwitches[s].opt.long_name);
		else
		if( pSwitches[s].usd )
			DEBUGLOGPD("option %s entered\n", pSwitches[s].opt.long_name);
#endif
#endif

		if( pSwitches[s].neg ) // ignore negating switches, e.g. no-...
			continue;

#ifdef DEBUGLOG
#else
		if( pSwitches[s].usd == false )
#endif
		{
			DEBUGLOGPD("option %s %sentered\n", pSwitches[s].opt.long_name, pSwitches[s].usd ? "" : "NOT ");

			bool upc = pSwitches[s].typ >= 'A' && pSwitches[s].typ <= 'Z';

			switch( pSwitches[s].typ )
			{
//			case G_OPTION_ARG_DOUBLE:
			case VT_FLOAT1:
			case VT_FLOAT2:
				if( pSwitches[s].usd == false )
				*((double*)pSwitches[s].val) = *((double*)pSwitches[s].def);
				DEBUGLOGPD("float value is now %f\n", (float)(*((double*)pSwitches[s].val)));
				break;

//			case G_OPTION_ARG_INT:
			case VT_INT1:
			case VT_INT2:
				if( pSwitches[s].usd == false )
				*((int*)pSwitches[s].val) = *((int*)pSwitches[s].def);
				DEBUGLOGPD("int value is now %d\n", *((int*)pSwitches[s].val));
				break;

//			case G_OPTION_ARG_NONE: // treat as a boolean or an int-bool
			case VT_BBOOL:
			case VT_IBOOL:
//				if( !pSwitches[s].neg ) // ignore negating switches, e.g. no-...
//				{
				if( upc )
				{
					if( pSwitches[s].usd == false )
					*((bool*)pSwitches[s].val) = *((bool*)pSwitches[s].def);
					DEBUGLOGPD("bool value is now %s\n", *((bool*)pSwitches[s].val) ? "true" : "false");
				}
				else
				{
					if( pSwitches[s].usd == false )
					*((int*)pSwitches[s].val) = *((int*)pSwitches[s].def);
					DEBUGLOGPD("int-bool value is now %s\n", *((int*)pSwitches[s].val) ? "TRUE" : "FALSE");
				}
//				}
				break;

//			case G_OPTION_ARG_STRING:
			case VT_STR1:
			case VT_STR2:
				if( pSwitches[s].usd == false )
				DEBUGLOGPD("string value is now %s\n", (gchar*)pSwitches[s].val ? *((gchar**)pSwitches[s].val) : "NULL");
				break;
			}
		}
#if 0
		else
		{
			DEBUGLOGPD("option %s entered\n", pSwitches[s].opt.long_name);
		}
#endif
	}

	DEBUGLOGED;
	return TRUE;
}

