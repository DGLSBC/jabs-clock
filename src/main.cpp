/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 
**      10        20        30        40        50        60        70        80
**
** program:
**    APP_NAME (see cfgdef.h) (forked from cairo-clock)
**
** original author:
**    Mirco "MacSlow" Müller <macslow@bangang.de>, <macslow@gmail.com>
**
** fork author:
**    Darrell "pillbug" Leggett <SilverBridleCreek@gmail.com>
**
** fork created:
**    2015.05.05 - patched a number of documented and undocumented bugs
**    2015.05.05 - reorganized/cleaned up the code & converted it to my preferred style
**    2015.05.05 - added a number of new features
**
** originally created:
**    2006.01.10 (or so)
**
** original last change:
**    2007.08.17
**
** original notes:
**    In my ongoing efforts to do something useful while learning the cairo-API
**    I produced this nifty program. Surprisingly it displays the current system
**    time in the old-fashioned way of an analog clock. I place this program
**    under the "GNU General Public License". If you don't know what that means
**    take a look a here...
**
**               http://www.gnu.org/licenses/licenses.html#GPL
**
** original todo:
**    clean up code and make it sane to read/understand
**
** original's supplied patches:
**    2006.03.26 - received a patch to add a 24h-mode from Darryll "Moppsy"
**                 Truchan <moppsy@comcast.net>
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "main"

#undef   _TESTABORTED    // define this to try and abort the app for testing purposes

#include "draw.h"
#include "clock.h"
#include "debug.h"       // for debugging prints
#include "cfgdef.h"
#include "global.h"
#include "cmdLine.h"
#include "clockGUI.h"
#include "utility.h"     // for main loop funcs
#include "x.h"           // for g_init_threads

#include <gtk/gtk.h>
#include <glib/gi18n.h>  // for 'international' text support
#include <glib/gstdio.h> // for g_unlink

#define   drawPrio (G_PRIORITY_DEFAULT+G_PRIORITY_HIGH)/2
#define   infoPrio (G_PRIORITY_DEFAULT+G_PRIORITY_HIGH_IDLE)/2
#define   waitPrio (G_PRIORITY_HIGH_IDLE)

#define  _ENABLE_DND  // enable support for drag-n-dropping theme archives onto the clock window
#undef   _SETMEMLIMIT // failed attempt at reducing memory usage after high-memory use episodes

// -----------------------------------------------------------------------------
static void     print_theme_list();
static void     set_signal_handlers();
static void     wait_on_startup();
static gboolean wait_time_handler(GtkWidget* pWidget);

static guint    gWaitTimerId = 0;

Runtime  gRun;
Settings gCfg;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
int main(int argc, char** argv)
{
	DEBUGLOGP("entry - glib at %d.%d.%d\n",
		glib_major_version, glib_minor_version, glib_micro_version);

/*	char   cwd[PATH_MAX]; cwd[0] = '\0';
	getcwd(cwd, vectsz(cwd));

	char   exe[PATH_MAX]; exe[0] = '\0';
	readlink("/proc/self/exe", exe, PATH_MAX);

	DEBUGLOGP("cwd:            *%s*\n", cwd);
	DEBUGLOGP("argv[0]:        *%s*\n", argv[0]);
	DEBUGLOGP("/proc/self/exe: *%s*\n", exe);
	DEBUGLOGP("app  name:      *%s*\n", g_get_application_name());
	DEBUGLOGP("prg  name:      *%s*\n", g_get_prgname());
	DEBUGLOGP("user cnfg dir:  *%s*\n", g_get_user_config_dir());
	DEBUGLOGP("user curr dir:  *%s*\n", g_get_current_dir());
	DEBUGLOGP("user data dir:  *%s*\n", g_get_user_data_dir());
	DEBUGLOGP("user home dir:  *%s*\n", g_get_home_dir());
	DEBUGLOGP("user home env:  *%s*\n", g_getenv(_("HOME")));
	DEBUGLOGP("user temp dir:  *%s*\n", g_get_tmp_dir());*/

/*	if( true )
	{
		char       exe[PATH_MAX];
		memset   (&exe, 0, sizeof(exe));
		readlink ("/proc/self/exe", exe, vectsz(exe));
		char*      dir = g_path_get_dirname(exe);
		DEBUGLOGP("/proc/self/exe: *%s*\n", exe);
		DEBUGLOGP("exe dir:        *%s*\n", dir);
		g_free(dir);
	}*/

/*	if( strcmp(cwd, "/home/me/bin") )
	{
		DEBUGLOGS("cwd is NOT what was spec'd in the .desktop file Path parm");
		DEBUGLOGS("changing cwd to be what it should already have been");

//		if( chdir("/home/me/bin") != 0 )
		if( chdir(g_get_home_dir()) != 0 )
		{
		}
	}*/

	chdir(g_get_home_dir()); // a 'known' place from which to begin

	memset(&gRun, 0, sizeof(gRun));

	gRun.appName     = "Pillbug's Cairo-Clock";
	gRun.appVersion  = "0.4.0";
	gRun.appStart    = true;
	gRun.renderIt    = true;
	gRun.hasParms    = argc > 1;
	gRun.lockDims    = 1;
	gRun.updateSurfs = true;
	gRun.drawScaleX  = gRun.drawScaleY = 1;

//	DEBUGLOGF("bef cfg/draw init calls\n");
	cfg ::init();
	draw::init();
//	DEBUGLOGF("aft cfg/draw init calls\n");

#ifdef _SETMEMLIMIT
	setmemlimit();
#endif

	bindtextdomain         (GETTEXT_PACKAGE, APP_NAMELOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain             (GETTEXT_PACKAGE);

#ifdef _TESTSTUFF
	{
		puts("List of system data dirs:");
		const char* const* dirs = g_get_system_data_dirs();

		for( size_t d = 0; dirs[d] != NULL; d++ )
			puts(dirs[d]);

		char buffer[1024];
		buffer[0] = '\0';
		readlink("/proc/self/exe", buffer, 1024);
		printf  ("readlink: %s\n", buffer);
		printf  ("argv[0]:  %s\n", argv[0]);
	}
#endif

#ifdef _USEMTHREADS
	g_init_threads();    // must go before gtk_init_check call
	gdk_threads_init();  // ditto
	gdk_threads_enter(); // ditto
#endif

	if( !gtk_init_check(&argc, &argv) )
	{
		DEBUGLOGS("There was an error while trying to initialize gtk");
		return 1;
	}

	bool printHelp    = false;
	bool printThemes  = false;
	bool printVersion = false;
	bool xscrnSaver   = false;

	if( gRun.hasParms )
	{
		cmdLine(argc, argv, printHelp, printThemes, printVersion, xscrnSaver);
	}
	else
	{
		cfg::load();
	}

	if( gCfg.clockWS )
		gCfg.sticky = false;

	if( xscrnSaver )
	{
		DEBUGLOGS("setting runtime/cfg values to 'screensaver mode'");

		gRun.scrsaver    = true;
		gCfg.aniStartup  = false;
		gCfg.clockC      = 0;
		gCfg.clockX      = 0;
		gCfg.clockY      = 0;
		gCfg.clockWS     = 0;
		gCfg.keepOnBot   = false;
		gCfg.keepOnTop   = true;
		gCfg.showInPager = false;
		gCfg.showInTasks = false;
		gCfg.sticky      = false;
		gRun.clickthru   = false;
		gRun.maximize    = true;
	}

	if( gCfg.aniStartup )
	{
		gRun.drawScaleX = gRun.drawScaleY = 0.05;
	}

	if( gRun.nodisplay )
		gRun.clickthru  = true;

	cfg::cnvp(gCfg, true); // make clockX/Y screen top-left relative

//	if( is_power_of_two(gCfg.clockH) ) // no longer necessary
//		gCfg.clockH += 1;

	if( gCfg.refreshRate < MIN_REFRESH_RATE )
		gCfg.refreshRate = MIN_REFRESH_RATE;
	if( gCfg.refreshRate > MAX_REFRESH_RATE )
		gCfg.refreshRate = MAX_REFRESH_RATE;

	if( printHelp || printThemes || printVersion )
	{
		if( printThemes )
			print_theme_list();

		if( printVersion )
		{
			printf("%s %s\n", gRun.appName, gRun.appVersion);
			printf(" Copyright (C) 2015 Darrell \"pillbug\" Leggett\n");
			printf(" This is free software; see the source for copying conditions. There is NO\n");
			printf(" warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		}

		return 0;
	}

	// waste time if no compositor available since apparently no 'compositor initd' event available
	// and some (all?) desktop linuxs start user startup apps at the same time low-level processes
	// are starting up

	wait_on_startup();

	// TODO: put following fail test into gui::init?

	if( !gui::init() )
	{
/*		// TODO: change gui::init to return false if main window could not be created (only)
		//       and then re-enable the following return since that would be considered as abortable
		DEBUGLOGP("The glade xml based gui could not initialized: \"%s\"\n", get_glade_filename());
		DEBUGLOGP("The glade xml based gui could not initialized: \"%s\"\n", gui::getGladeFilename());
		return 3;*/
	}

	DEBUGLOGS("bef gtk window calls");

	set_signal_handlers(); // capture and handle SIGTERM as a normal app close request

	gtk_window_set_focus_on_map   (GTK_WINDOW(gRun.pMainWindow), FALSE);

#if !GTK_CHECK_VERSION(2,9,0)
	gtk_window_set_has_resize_grip(GTK_WINDOW(gRun.pMainWindow), FALSE);
#endif

	GdkColor color = { 0, 0, 0, 0 };
	gtk_widget_modify_bg        (gRun.pMainWindow, GTK_STATE_NORMAL, &color);
	gtk_widget_set_app_paintable(gRun.pMainWindow, TRUE);

	int sw = gdk_screen_get_width (gdk_screen_get_default());
	int sh = gdk_screen_get_height(gdk_screen_get_default());

	if( gRun.maximize )
	{
		DEBUGLOGS("setting clock window values to maximized");

		gRun.prevWinX = gCfg.clockX;
		gRun.prevWinY = gCfg.clockY;
		gRun.prevWinW = gCfg.clockW;
		gRun.prevWinH = gCfg.clockH;
//		gCfg.clockW   = MAX_CWIDTH;
//		gCfg.clockH   = MAX_CHEIGHT;
		gCfg.clockX   = 0;
		gCfg.clockY   = 0;
		gCfg.clockW   = sw;
		gCfg.clockH   = sh;
	}

	gtk_window_set_decorated(GTK_WINDOW(gRun.pMainWindow), FALSE);
	gtk_window_set_resizable(GTK_WINDOW(gRun.pMainWindow), TRUE);
	gtk_window_set_title    (GTK_WINDOW(gRun.pMainWindow), gRun.appName);
	gtk_window_resize       (GTK_WINDOW(gRun.pMainWindow), gCfg.clockW, gCfg.clockH);
	gtk_window_move         (GTK_WINDOW(gRun.pMainWindow), gCfg.clockX, gCfg.clockY);

	cclock::keep_on_bot  (gRun.pMainWindow, cclock::keep_on_bot());
	cclock::keep_on_top  (gRun.pMainWindow, cclock::keep_on_top());
	cclock::pagebar_shown(gRun.pMainWindow, cclock::pagebar_shown());
	cclock::sticky       (gRun.pMainWindow, cclock::sticky());
	cclock::taskbar_shown(gRun.pMainWindow, cclock::taskbar_shown());

	GdkGeometry hints;
	hints.min_width	 = MIN_CWIDTH;
	hints.min_height = MIN_CHEIGHT;
//	hints.max_width	 = MAX_CWIDTH;
//	hints.max_height = MAX_CHEIGHT;
	hints.max_width	 = sw;
	hints.max_height = sh;
//	hints.min_aspect = 1.0;
//	hints.max_aspect = 1.0;

	gtk_window_set_geometry_hints(GTK_WINDOW(gRun.pMainWindow), gRun.pMainWindow, &hints, GdkWindowHints(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
//	gtk_window_set_geometry_hints(GTK_WINDOW(gRun.pMainWindow), gRun.pMainWindow, &hints, GdkWindowHints(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_ASPECT));

//	on_alpha_screen_changed(gRun.pMainWindow, NULL, NULL);
	update_colormap(gRun.pMainWindow);

//#ifdef _ENABLE_DND
/*
//	static gchar          target_type[] =     "text/uri-list";
//	static GtkTargetEntry target_list[] = { { target_type, GTK_TARGET_OTHER_APP, TGT_SEL_STR } };

	DEBUGLOGS("bef setting drag destination");
//	gtk_drag_dest_set(gRun.pMainWindow, GTK_DEST_DEFAULT_ALL, target_list, G_N_ELEMENTS(target_list), GDK_ACTION_COPY);
	gtk_drag_dest_set(gRun.pMainWindow, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);
	g_signal_connect (G_OBJECT(gRun.pMainWindow), "drag_data_received", G_CALLBACK(on_drag_data_received), NULL);
	gtk_drag_dest_add_uri_targets(gRun.pMainWindow);
	DEBUGLOGS("aft  setting drag destination");
*/
	gui::initDragDrop();
//#endif

//	gtk_widget_add_events(gRun.pMainWindow, GDK_BUTTON_PRESS_MASK | GDK_SCROLL_MASK | GDK_BUTTON1_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

	// TODO: really need to do away with this dup code - another func?

	// do this once here to preset the global time keeping values
	time_t ct    =  time(NULL);
	tm     lt    = *localtime(&ct);
	gRun.hours   =  lt.tm_hour;
	gRun.hours   =  gCfg.show24Hrs ? gRun.hours : (gRun.hours % 12);
	gRun.minutes =  lt.tm_min;
	gRun.seconds =  lt.tm_sec;

	if( GTK_WIDGET_VISIBLE(gRun.pMainWindow) )
	{
		gdk_window_set_opacity(gRun.pMainWindow->window, 0.0);
	}
	else
	{
		gtk_widget_realize(gRun.pMainWindow);
		gdk_window_set_back_pixmap(gRun.pMainWindow->window, NULL, FALSE);
		gdk_window_set_opacity(gRun.pMainWindow->window, 0.0);
		gtk_widget_show_all(gRun.pMainWindow);
	}

	if( gRun.scrsaver )
	{
		DEBUGLOGS("setting fullscreen/saver window to fullscreen & hiding its cursor");
		gtk_window_fullscreen(GTK_WINDOW(gRun.pMainWindow));
		gdk_window_set_cursor(gRun.pMainWindow->window, gdk_cursor_new_for_display(gdk_display_get_default(), GDK_BLANK_CURSOR));
	}

	DEBUGLOGS("aft gtk window calls");

	// why draw::beg here? this is too late, yes?
	// gui::init has already been called, which loads the theme rsvgs & sets the svg dims & ...?
	// it's being called here so the hand animation can be set correctly after
	// cfg loading

	// TODO: need to rework how theme loading, draw initing, ..., is handled
	//       to put all initing in its proper order (can inits be independent?)

	draw::beg(true);

/*	if( true )
	{
		prt_main_surf_type(); // in draw at end (not in h yet)
//		return 0;
	}*/

//	update_input_shape(gRun.pMainWindow, gCfg.clockW, gCfg.clockH, false);
	update_input_shape(gRun.pMainWindow, gCfg.clockW, gCfg.clockH, true, true, false);

	gtk_window_set_focus_on_map(GTK_WINDOW(gRun.pMainWindow), TRUE);

	// TODO: make my own since there's no way to control when it pops up?
//	gtk_widget_set_has_tooltip(gRun.pMainWindow, TRUE);

	update_ts_info();

	gui::initWorkspace(); // TODO: should this be called earlier?

	set_window_icon(gRun.pMainWindow);

	gRun.renderUp = !gCfg.aniStartup;

	int fps          = gCfg.showSeconds || gCfg.aniStartup ? 200 : 2;
	gRun.drawTimerId = g_timeout_add_full(drawPrio, 1000/fps, (GSourceFunc)draw_time_handler, (gpointer)gRun.pMainWindow, NULL);
	gRun.infoTimerId = g_timeout_add_full(infoPrio, 1000/2,   (GSourceFunc)info_time_handler, (gpointer)gRun.pMainWindow, NULL);

	DEBUGLOGP("render timer set to %d fps\n", fps);

	DEBUGLOGS("before gtk_main call");

//	bool oloop  = true;  // use own looping
	bool oloop  = false; // don't use own looping
/*
//	int  osleep = G_USEC_PER_SEC/ gCfg.refreshRate;
//	int  osleep = G_USEC_PER_SEC/(200);
	int  osleep = G_USEC_PER_SEC/(MAX_REFRESH_RATE*2);
//	int  osleep = G_USEC_PER_SEC/(MAX_REFRESH_RATE);
	int  isleep = 0;

//	int  rsleep = G_USEC_PER_SEC/ gCfg.refreshRate;
//	int  rsleep = gRun.appStart ? G_USEC_PER_SEC/(MAX_REFRESH_RATE*2) : (int)((double)G_USEC_PER_SEC*(draw::refreshTime[isleep+1] - draw::refreshTime[isleep]));
	int  rsleep = (int)((double)G_USEC_PER_SEC*(draw::refreshTime[isleep+1] - draw::refreshTime[isleep]));

	DEBUGLOGP("isleep=%d, rsleep=%d\n", isleep, rsleep);

	for( int i = 0; i < draw::refreshCount; i++ )
	{
		double tdiff = i == 0 ? 0 : draw::refreshTime[i] - draw::refreshTime[i-1];

		DEBUGLOGP("%2.2d: %6.6f, %6.6f, %6.6f, %6.6f\n",
			i, draw::refreshTime[i], draw::refreshFrac[i], draw::refreshCumm[i], tdiff);
	}

	DEBUGLOGS("");
*/
	g_main_beg(!oloop);

/*	bool   render;
	gint64 tbd, tbn, tb4 = g_get_monotonic_time();
//	gint64 tbd, tbn, tb4 = g_get_real_time();

	while( g_main_looped() )
	{
		if( g_main_pump(false) ) // process gui messages until none or done
		{
			render  = false;
			tbn     = g_get_monotonic_time();
//			tbn     = g_get_real_time();
			tbd     = tbn - tb4;
			rsleep -= tbd;

//			if( rsleep <= osleep )
			if( rsleep <= 0 )
			{
//				rsleep  = G_USEC_PER_SEC/gCfg.refreshRate;
//				rsleep  = (int)((double)G_USEC_PER_SEC*(draw::refreshTime[isleep+1] - draw::refreshTime[isleep]));
//				rsleep  = gRun.appStart ? G_USEC_PER_SEC/(MAX_REFRESH_RATE*2) : (int)((double)G_USEC_PER_SEC*(draw::refreshTime[isleep+1] - draw::refreshTime[isleep]));
				rsleep  = (int)((double)G_USEC_PER_SEC*(draw::refreshTime[isleep+1] - draw::refreshTime[isleep]));
//				rsleep += osleep;

				render  = gRun.renderIt;

				if( !gRun.appStart )
				{
					if( ++isleep >= draw::refreshCount-1 )
						isleep = 0;
				}

				DEBUGLOGP("isleep=%d, rsleep=%d\n", isleep, rsleep);
			}

			if( render )
				draw_time_handler(gRun.pMainWindow);

//			rsleep -= osleep;
//			tb4     = g_get_monotonic_time();
//			tb4     = tbn;

			g_usleep(osleep);
		}
	}*/

	DEBUGLOGS("after  gtk_main call");

	if( gRun.drawTimerId )
		g_source_remove(gRun.drawTimerId);
	gRun.drawTimerId = 0;

	if( gRun.infoTimerId )
		g_source_remove(gRun.infoTimerId);
	gRun.infoTimerId = 0;

	if( gWaitTimerId )
		g_source_remove(gWaitTimerId);
	gWaitTimerId = 0;

	gui::dnitWorkspace();

	draw::end();
	gui::dnit();
	cfg::save();

	if( get_theme_icon_filename(true, true) )
		g_unlink(get_theme_icon_filename(false));

#ifdef _USEMTHREADS
	gdk_threads_leave();
#endif

	DEBUGLOGE;
	return 0;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void print_theme_list()
{
	ThemeEntry     te;
	ThemeList*     tl;
	theme_list_get(tl);

	int maxn = 0;
	for( te  = theme_list_beg(tl); !theme_list_end(tl); te = theme_list_nxt(tl) )
		if( maxn < te.pName->len )
			maxn = te.pName->len;
	maxn    += 2;

	for( te  = theme_list_beg(tl); !theme_list_end(tl); te = theme_list_nxt(tl) )
		printf("%-*.*s%s/%s\n", maxn, maxn, te.pName->str, te.pPath->str, te.pFile->str);

	theme_list_del(tl);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <signal.h>

static struct sigaction sa_action[16];
static int              sa_signal[16];
static size_t           sa_count;

// -----------------------------------------------------------------------------
static void sighandler(int signal)
{
//#ifdef _TESTABORTED
//	DEBUGLOGB;
//#endif

	gRun.appStop = true;

	if( get_theme_icon_filename(true, true) )
	{
		g_unlink(get_theme_icon_filename(false));
/*		const char* tif = get_theme_icon_filename(false);
		DEBUGLOGP("deleting icon file %s\n", tif);
		g_unlink(tif);*/
	}

	if( signal == SIGHUP || signal == SIGINT || signal == SIGQUIT )
		cfg::save();

	struct sigaction* sh = NULL;

	for( size_t s = 0; s <  sa_count; s++ )
	{
		if( sa_signal[s] == signal )
		{
			sh = &sa_action[s];
			break;
		}
	}

	if( sh )
		sigaction(signal, sh, NULL); // restore the old handler

//#ifdef _TESTABORTED
//	DEBUGLOGE;
//#endif

	if( sh )
		raise(signal);
}

// -----------------------------------------------------------------------------
void set_signal_handlers()
{
	DEBUGLOGB;

	static int       sigs[] = { SIGABRT, SIGALRM, SIGHUP, SIGINT, SIGQUIT, SIGSEGV, SIGTERM, /*SIGXCPU*/ };
	struct sigaction act    = { sighandler, 0, 0, 0, 0 };

	sa_count = 0;

	for( size_t s = 0; s < vectsz(sigs); s++ )
		sigaction(sa_signal[s] = sigs[s], &act, &sa_action[s]);

	sa_count = vectsz(sigs);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void wait_on_startup()
{
	DEBUGLOGB;

	bool wait2 = true;

//	if( gRun.waitSecs1 && gdk_display_supports_composite(gtk_widget_get_display(gRun.pMainWindow)) )
	if( gRun.waitSecs1 && gdk_display_supports_composite(gdk_display_get_default()) )
	{
		const int wait_secs      = gRun.waitSecs1;                     // secs to wait for compositing use
		const int times_per_sec  = 4;                                  // sleep for 1/4th of a sec each time
		const int usec_per_sec   = 1000000;                            // # of usecs in a sec
		const int sleep_usecs    = usec_per_sec/times_per_sec;         // # of usecs to sleep each time
		int       times_to_sleep = wait_secs*usec_per_sec/sleep_usecs; // # of times to do compositing check

		wait2 = gRun.waitSecs1 < gRun.waitSecs2;

		DEBUGLOGP("secs_to_wait=%d, times_per_sec=%d, sleep_usecs=%d, times_to_sleep=%d\n",
			wait_secs, times_per_sec, sleep_usecs, times_to_sleep);

		GdkScreen* screen = gdk_display_get_default_screen(gdk_display_get_default());

		while( times_to_sleep-- )
		{
//			if( gdk_screen_is_composited(gtk_widget_get_screen(gRun.pMainWindow)) )
			if( gdk_screen_is_composited(screen) )
			{
				if( gdk_screen_get_rgba_visual(screen) )
				{
					DEBUGLOGS("can now use composition");
					break;
				}
			}

			DEBUGLOGS("sleeping in composition test loop");
			usleep(sleep_usecs);

			if( gRun.appStop )
				break;
		}

/*		// Figure out if we have rgba capabilities.
		GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(sakura.main_window));
		GdkVisual *visual = gdk_screen_get_rgba_visual(screen);

		if (visual != NULL && gdk_screen_is_composited(screen))
		{
			gtk_widget_set_visual(GTK_WIDGET(sakura.main_window), visual);
			sakura.has_rgba = true;
		}
		else
		{
			// Probably not needed, as is likely the default initializer
			sakura.has_rgba = false;
		}*/
	}

	if( gRun.waitSecs2 && wait2 && !gRun.appStop )
	{
		DEBUGLOGP("waiting %d seconds to redraw icon\n", gRun.waitSecs2);
		gWaitTimerId = g_timeout_add_full(waitPrio, gRun.waitSecs2*1000, (GSourceFunc)wait_time_handler, (gpointer)gRun.pMainWindow, NULL);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
gboolean wait_time_handler(GtkWidget* pWidget)
{
	if( gRun.appStop )
		return FALSE;

	g_source_remove(gWaitTimerId);
	gWaitTimerId = 0;

	DEBUGLOGS("redrawing icon");
	make_theme_icon(pWidget);

	return FALSE;
}

