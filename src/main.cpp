/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 
**      10        20        30        40        50        60        70        80
**
** program:
**    APP_NAME - forked from APP_NAME_OLD (see cfgdef.h)
**
** original author:
**    Mirco "MacSlow" Müller <macslow@bangang.de>, <macslow@gmail.com>
**
** fork author:
**    Darrell Leggett ("pillbug") <SilverBridleCreek@gmail.com>
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
#define   DEBUGNSP      "main"

#include <glib.h>       // for ?
#include <glib/gi18n.h> // for 'international' text support
#include <stdio.h>      // for printf
#include <stdlib.h>     // for atof
#include <unistd.h>     // for chdir and unlink
#include "platform.h"   // platform specific

#include "draw.h"       // for ?
#include "copts.h"      // for ?
#include "debug.h"      // for debugging prints
#include "cfgdef.h"     // for ?
#include "config.h"     // for Config struct, CornerType enum, ...
#include "global.h"     // for ?
#include "cmdLine.h"    // for ?
#include "clockGUI.h"   // for ?
#include "utility.h"    // for main loop & threading funcs
#include "sound.h"      // for snd::play_alarm

#undef   _TESTABORTED   // define this to try and abort the app for testing purposes
#undef   _SETMEMLIMIT   // failed attempt at reducing memory usage after high-memory use cases

// -----------------------------------------------------------------------------
static void     print_theme_list();
static void     set_signal_handlers();
static void     wait_on_startup();

static gboolean alarm_time_timer(PWidget* pWidget);
static gboolean waitz_time_timer(PWidget* pWidget);
static gboolean waitZ_time_timer(PWidget* pWidget);

static guint    gAlarmTimerId = 0;
static guint    gWaitzTimerId = 0;
static guint    gWaitZTimerId = 0;

Runtime gRun;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
int main(int argc, char** argv)
{
#if _USEKDE
	QApplication app(argc, argv);
#endif

#ifdef DEBUGLOG
	DEBUGLOGPF("entry - glib at %d.%d.%d\n",
		glib_major_version, glib_minor_version, glib_micro_version);
#endif

#if 0
	char   cwd[PATH_MAX]; cwd[0] = '\0';
	getcwd(cwd, vectsz(cwd));

	char   exe[PATH_MAX]; exe[0] = '\0';
	readlink("/proc/self/exe", exe, PATH_MAX);

#ifdef DEBUGLOG
	DEBUGLOGPF("cwd:            *%s*\n", cwd);
	DEBUGLOGPF("argv[0]:        *%s*\n", argv[0]);
	DEBUGLOGPF("/proc/self/exe: *%s*\n", exe);
	DEBUGLOGPF("app  name:      *%s*\n", g_get_application_name());
	DEBUGLOGPF("prg  name:      *%s*\n", g_get_prgname());
	DEBUGLOGPF("user cnfg dir:  *%s*\n", g_get_user_config_dir());
	DEBUGLOGPF("user curr dir:  *%s*\n", g_get_current_dir());
	DEBUGLOGPF("user data dir:  *%s*\n", g_get_user_data_dir());
	DEBUGLOGPF("user home dir:  *%s*\n", g_get_home_dir());
	DEBUGLOGPF("user home env:  *%s*\n", g_getenv(_("HOME")));
	DEBUGLOGPF("user temp dir:  *%s*\n", g_get_tmp_dir());
#endif
#endif

#if 0
	if( true )
	{
		char       exe[PATH_MAX];
		memset   (&exe, 0, sizeof(exe));
		readlink ("/proc/self/exe", exe, vectsz(exe));
		char*      dir = g_path_get_dirname(exe);
#ifdef DEBUGLOG
		DEBUGLOGPF("/proc/self/exe: *%s*\n", exe);
		DEBUGLOGPF("exe dir:        *%s*\n", dir);
#endif
		g_free(dir);
	}
#endif

#if 0
	if( strcmp(cwd, "/home/me/bin") )
	{
#ifdef DEBUGLOG
		DEBUGLOGSF("cwd is NOT what was spec'd in the .desktop file Path parm");
		DEBUGLOGSF("changing cwd to be what it should already have been");
#endif
//		if( chdir("/home/me/bin") != 0 )
		if( chdir(g_get_home_dir()) != 0 )
		{
		}
	}
#endif

	chdir(g_get_home_dir()); // a 'known' place from which to begin

	memset(&gRun, 0, sizeof(gRun));

	gRun.appName     = "Just A Basic Svg Clock";
	gRun.appVersion  = "0.4.0";
	gRun.appStart    =  true;
	gRun.animWindW   =  gCfg.clockW;
	gRun.animWindH   =  gCfg.clockH;
	gRun.animScale   =  1;
	gRun.drawScaleX  =  gRun.drawScaleY = 1;
	gRun.hasParms    =  argc > 1;
	gRun.lockDims    =  1;
	gRun.lockOffs    =  1;
	gRun.renderIt    =  true;
	gRun.updateSurfs =  true;

#ifdef DEBUGLOG
	DEBUGLOGSF("bef cfg/draw init calls");
#endif

	cfg ::init(); // TODO: make sure these two inits don't use gtk funcs
	draw::init();

#ifdef DEBUGLOG
	DEBUGLOGSF("aft cfg/draw init calls");
#endif

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

		char buffer[PATH_MAX];
		buffer[0] = '\0';
		readlink("/proc/self/exe", buffer, PATH_MAX);
		printf  ("readlink: %s\n", buffer);
		printf  ("argv[0]:  %s\n", argv[0]);
	}
#endif

#ifdef _USEMTHREADS
	g_init_threads_gui(); // must go before gtk_init_check call
#endif

#if _USEGTK
	if( !gtk_init_check(&argc, &argv) )
	{
#ifdef DEBUGLOG
		DEBUGLOGSF("There was an error while trying to initialize gtk.");
#endif
		return 1;
	}
#endif

	bool printHelp    = false;
	bool printThemes  = false;
	bool printVersion = false;
	bool xscrnSaver   = false;
	bool squareUp     = false;

	if( gRun.hasParms )
	{
		cmdLine(argc, argv, printHelp, printThemes, printVersion, xscrnSaver, squareUp);

		gRun.scrsaver = xscrnSaver;
		gRun.squareUp = squareUp;
	}
	else
	{
		cfg::load();
	}

	// NOTE: at this point the -D cli switch usage takes over for debug printing

	DEBUGLOGP("config loaded theme path is *%s*\n", gCfg.themePath);
	DEBUGLOGP("config loaded theme file is *%s*\n", gCfg.themeFile);

	if( gRun.portably )
		DEBUGLOGP("portable app home: %s\n", gRun.appHome);
	else
		DEBUGLOGP("normal app home: %s\n",   gRun.appHome);

	if( printHelp || printThemes || printVersion )
	{
		if( printThemes )
			print_theme_list();

		if( printVersion )
		{
			printf("%s %s\n", gRun.appName, gRun.appVersion);
			printf(" Copyright (C) 2015-2016 Darrell Leggett (\"pillbug\")\n");
			printf(" This is free software; see the source for copying conditions. There is NO\n");
			printf(" warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		}

		return 0;
	}

//	if( gCfg.clockWS )
//		gCfg.sticky = false;

	snd::init();

	if( xscrnSaver )
	{
		DEBUGLOGS("setting runtime/cfg values to 'screensaver mode'");

		gRun.scrsaver    = true;
//		gCfg.aniStartup  = false;
//		gCfg.clockC      = 0;
//		gCfg.clockX      = 0;
//		gCfg.clockY      = 0;
//		gCfg.clockWS     = 0;
		gCfg.keepOnBot   = false;
		gCfg.keepOnTop   = true;
		gCfg.showInPager = false;
		gCfg.showInTasks = false;
		gCfg.sticky      = true;
		gCfg.clickThru   = false;
		gRun.maximize    = true;
	}

#if _USEGTK
	GdkScreen*  pScreen  = gdk_screen_get_default();
	int         sw       = gdk_screen_get_width (pScreen);
	int         sh       = gdk_screen_get_height(pScreen);
	const char* wm       = gdk_x11_screen_get_window_manager_name(pScreen);
#else
	int         sw       = 1400;
	int         sh       = 1050;
	const char* wm       = "xfwm";
#endif

//	gRun.marcoCity = wm != NULL && strstr(wm, "Marco") != NULL && strstr(wm, "Metacity") != NULL;

	DEBUGLOGP("wm is %s, sw is %d, sh is %d\n", wm, sw, sh);

	if( gRun.maximize )
	{
		DEBUGLOGS("setting clock window values to maximized");

		bool useMin   = squareUp;
//		bool useMin   = squareUp &&  gRun.scrsaver;
//		bool useMin   = squareUp && !gRun.scrsaver;
		int  sm       = sw < sh ? sw : sh;

		gRun.prevWinX = gCfg.clockX;
		gRun.prevWinY = gCfg.clockY;
		gRun.prevWinW = gCfg.clockW;
		gRun.prevWinH = gCfg.clockH;

		gCfg.clockX   = useMin ? (sw - sm)/2 : 0;
		gCfg.clockY   = useMin ? (sh - sm)/2 : 0;
		gCfg.clockW   = useMin ?  sm : sw;
		gCfg.clockH   = useMin ?  sm : sh;
	}

	if( gCfg.noDisplay )
		gCfg.clickThru = true;

	if( !gRun.maximize )
		cfg::cnvp(gCfg, true); // make clockX/Y screen top-left relative

	if( gCfg.renderRate < MIN_REFRESH_RATE )
		gCfg.renderRate = MIN_REFRESH_RATE;
	if( gCfg.renderRate > MAX_REFRESH_RATE )
		gCfg.renderRate = MAX_REFRESH_RATE;

	wait_on_startup();

//	DEBUGLOGZ("bef calling cgui::init", 500);

	if( !cgui::init() )
	{
		DEBUGLOGS("The clock display could not get created");
		return 2;
	}

//	DEBUGLOGZ("aft calling cgui::init", 500);

#if _USEKDE
	app.setMainWidget(gRun.pMainWindow);
#endif

	alarm_set();

	set_signal_handlers(); // capture and handle SIGTERM (and others) as a normal app close request

#if _USEGTK
	DEBUGLOGS("bef gtk window calls");
#endif

#if _USEGTK
#if    !GTK_CHECK_VERSION(2,9,0)
	gtk_window_set_has_resize_grip(GTK_WINDOW(gRun.pMainWindow), FALSE);
#endif

#if     GTK_CHECK_VERSION(3,0,0)
	GtkStateFlags state =   GTK_STATE_FLAG_NORMAL;
	GdkRGBA       color = { 0, 0, 0, 0 };
	gtk_widget_override_background_color(gRun.pMainWindow, state,            &color);
#else
	GdkColor      color = { 0, 0, 0, 0 };
	gtk_widget_modify_bg                (gRun.pMainWindow, GTK_STATE_NORMAL, &color);
#endif
#endif

#if 0
#if _USEGTK
	int sw = gdk_screen_get_width (gdk_screen_get_default());
	int sh = gdk_screen_get_height(gdk_screen_get_default());
#else
	int sw = 1400;
	int sh = 1050;
#endif
	if( gRun.maximize )
	{
		DEBUGLOGS("setting clock window values to maximized");

		bool useMin   = squareUp;
//		bool useMin   = squareUp &&  gRun.scrsaver;
//		bool useMin   = squareUp && !gRun.scrsaver;
		int  sm       = sw < sh ? sw : sh;

		gRun.prevWinX = gCfg.clockX;
		gRun.prevWinY = gCfg.clockY;
		gRun.prevWinW = gCfg.clockW;
		gRun.prevWinH = gCfg.clockH;

		gCfg.clockX   = useMin ? (sw - sm)/2 : 0;
		gCfg.clockY   = useMin ? (sh - sm)/2 : 0;
		gCfg.clockW   = useMin ?  sm : sw;
		gCfg.clockH   = useMin ?  sm : sh;
	}
#endif

#if _USEGTK
#ifdef DEBUGLOG
	{
	gint                                                   wndX,  wndY;
	gint                                                   wndW,  wndH;
	gtk_window_get_position(GTK_WINDOW(gRun.pMainWindow), &wndX, &wndY);
	gtk_window_get_size    (GTK_WINDOW(gRun.pMainWindow), &wndW, &wndH);
	DEBUGLOGP("bef resize/move: wndX=%d, wndY=%d, wndW=%d, wndH=%d\n", wndX, wndY, wndW, wndH);
	}
#endif
#endif

#if _USEGTK
	gtk_window_resize(GTK_WINDOW(gRun.pMainWindow), gCfg.clockW, gCfg.clockH);
	gtk_window_move  (GTK_WINDOW(gRun.pMainWindow), gCfg.clockX, gCfg.clockY);
#endif

#if _USEGTK
#ifdef DEBUGLOG
	{
	gint                                                   wndX,  wndY;
	gint                                                   wndW,  wndH;
	gtk_window_get_position(GTK_WINDOW(gRun.pMainWindow), &wndX, &wndY);
	gtk_window_get_size    (GTK_WINDOW(gRun.pMainWindow), &wndW, &wndH);
	DEBUGLOGP("aft resize/move: wndX=%d, wndY=%d, wndW=%d, wndH=%d\n", wndX, wndY, wndW, wndH);
	}
#endif
#endif

	copts::keep_on_bot  (gRun.pMainWindow, copts::keep_on_bot());
	copts::keep_on_top  (gRun.pMainWindow, copts::keep_on_top());
	copts::pagebar_shown(gRun.pMainWindow, copts::pagebar_shown());
	copts::sticky       (gRun.pMainWindow, copts::sticky());
	copts::taskbar_shown(gRun.pMainWindow, copts::taskbar_shown());

	update_colormap(gRun.pMainWindow);

	cgui::initDragDrop();

	update_ts(true); // do this once here to preset the global time keeping values

	// NOTE: the following scaling is done to get the clock displayed quickly,
	//       in case the requested clock size is 'large', meaning it would take
	//       a longer amount of time to create and prepare the clock rendering
	//       surfaces, especially for clocks with 'complex' svgs

	int  maxCD      =  sw/6;
//	int  maxCD      =  200;
//	int  maxCD      =  213;
//	int  maxCD      =  sw > sh ? sw : sh;
	bool scale      =  gCfg.clockW*gCfg.clockH > maxCD*maxCD;
	gRun.animWindW  =  scale ? maxCD : gCfg.clockW;
	gRun.animWindH  =  scale ? maxCD : gCfg.clockH;
	int  fps        =  200;
//	int  fps        =  gCfg.showSeconds || gCfg.aniStartup ? 200 : 2;
//	int  fps        =  gCfg.showSeconds || gCfg.aniStartup ?  10 : 2;
//	int  fps        =  gCfg.showSeconds || gCfg.aniStartup ?  50 : 2;
//	int  fps        =  gCfg.showSeconds || gCfg.aniStartup ?  10 : 2;
	gRun.animScale  =  gCfg.aniStartup ? 1.0/(double)fps : 1.0;
	gRun.drawScaleX = (double)gCfg.clockW/(double)gRun.animWindW;
	gRun.drawScaleY = (double)gCfg.clockH/(double)gRun.animWindH;

//	if( gRun.scrsaver  && gRun.squareUp )
//		gRun.drawScaleX = gRun.drawScaleY;

	DEBUGLOGP("animation  is  %s\n",                                gCfg.aniStartup ? "on" : "off");
	DEBUGLOGP("clock W/Hs are %dx%d, animW/Hs are %dx%d\n",         gCfg.clockW, gCfg.clockH, gRun.animWindW, gRun.animWindH);
	DEBUGLOGP("drawScales are %f/%f, animScale is %f, fps is %d\n", gRun.drawScaleX, gRun.drawScaleY, gRun.animScale, fps);

#if _USEGTK
	if( !gtk_widget_get_visible(gRun.pMainWindow) )
	{
//		DEBUGLOGZ("bef realizing clock window", 3000);
		DEBUGLOGS("main window is now being realized");
		gtk_widget_realize(gRun.pMainWindow);
//		DEBUGLOGZ("aft realizing clock window", 3000);
	}
#endif

#if _USEGTK
//	if( !gRun.marcoCity )
//		gdk_window_set_opacity(gtk_widget_get_window(gRun.pMainWindow), 0.0);
#endif

#if _USEGTK
#if    !GTK_CHECK_VERSION(3,0,0)
#if 0
	GdkPixmap* pPixmap = draw::grabdm();
	DEBUGLOGP("pPixmap   is %p\n", pPixmap);
	DEBUGLOGP("wnd depth is %d\n", gdk_drawable_get_depth(gtk_widget_get_window(gRun.pMainWindow)));
	DEBUGLOGP("map depth is %d\n", gdk_drawable_get_depth(pPixmap));
#endif
//	gdk_window_set_back_pixmap(gtk_widget_get_window(gRun.pMainWindow), draw::grabdm(), FALSE);
	gdk_window_set_back_pixmap(gtk_widget_get_window(gRun.pMainWindow), NULL, FALSE);
#endif
#endif

	draw::render_set(gRun.pMainWindow, true, true, gRun.animWindW, gRun.animWindH);

	if( gRun.scrsaver )
	{
		DEBUGLOGS("setting fullscreen/saver window to fullscreen & hiding its cursor");
#if _USEGTK
//		gtk_window_fullscreen (GTK_WINDOW(gRun.pMainWindow));
		change_cursor(gRun.pMainWindow, GDK_BLANK_CURSOR);
#endif
	}
	else
	{
#if _USEGTK
		gtk_window_set_gravity(GTK_WINDOW(gRun.pMainWindow), GDK_GRAVITY_CENTER);
		change_cursor(gRun.pMainWindow, GDK_FLEUR);
#endif
	}

#if _USEGTK
	DEBUGLOGS("aft gtk window calls");
#endif

	// why draw::beg here? this is too late, yes?
	// cgui::init has already been called, which loads the theme rsvgs & sets the svg dims & ...?
	// it's being called here so the hand animation can be set correctly after
	// cfg loading

	// TODO: need to rework how theme loading, draw initing, ..., is handled
	//       to put all initing in its proper order (can inits be independent?)

	draw::beg(true);

#if 0
	if( true )
	{
		prt_main_surf_type(); // in draw at end (not in h yet)
//		return 0;
	}
#endif

	DEBUGLOGS("bef initing window shape masks and drawing surfs");

#if _USEGTK
	if( gtk_widget_is_composited(gRun.pMainWindow) )
#else
	if( true )
#endif
	{
		update_shapes(gRun.pMainWindow, gRun.animWindW, gRun.animWindH, true, true, false);
		draw::update_surfs_swap();
	}
	else
	{
		update_shapes(gRun.pMainWindow, gRun.animWindW, gRun.animWindH, true,  false, false);
		update_shapes(gRun.pMainWindow, gCfg.clockW,    gCfg.clockH,    false, true,  false);
		draw::update_surfs_swap();
	}

	DEBUGLOGS("aft initing window shape masks and drawing surfs");

	update_ts_info(gRun.pMainWindow, true, true, true);

//	cgui::initWorkspace(); // NOTE: moved to cgui::on_map

	set_window_icon(gRun.pMainWindow);

	gRun.renderUp = !gCfg.aniStartup;

#if _USEGTK
	gtk_widget_set_can_focus(gRun.pMainWindow, TRUE);
//	gtk_window_set_focus_on_map(GTK_WINDOW(gRun.pMainWindow), TRUE);
//	gtk_window_set_accept_focus(GTK_WINDOW(gRun.pMainWindow), FALSE);
	gtk_window_set_accept_focus(GTK_WINDOW(gRun.pMainWindow), gCfg.takeFocus ? TRUE : FALSE);
#endif

//	gRun.infoTimerId = g_timeout_add_full(infoPrio, 1000/2,   (GSourceFunc)info_time_timer, (gpointer)gRun.pMainWindow, NULL);

//	DEBUGLOGZ("bef showing clock window", 500);
	DEBUGLOGS("bef showing the clock window");
#if _USEGTK
	gtk_widget_show(gRun.pMainWindow);
#else
	gRun.pMainWindow->show();
#endif
	DEBUGLOGS("aft showing the clock window");
//	DEBUGLOGZ("aft showing clock window", 500);
#if 0
	gRun.infoTimerId = g_timeout_add_full(infoPrio, 1000/2,   (GSourceFunc)info_time_timer, (gpointer)gRun.pMainWindow, NULL);
	gRun.drawTimerId = g_timeout_add_full(drawPrio, 1000/fps, (GSourceFunc)draw_anim_timer, (gpointer)gRun.pMainWindow, NULL);
	DEBUGLOGP("render timer set to %d fps (%d ms)\n", fps, 1000/fps);
#endif
	DEBUGLOGS("bef g_main_beg call");

//	bool oloop  = true;  // use own looping
	bool oloop  = false; // don't use own looping
#if 0
//	int  osleep = G_USEC_PER_SEC/ gCfg.renderRate;
//	int  osleep = G_USEC_PER_SEC/(200);
	int  osleep = G_USEC_PER_SEC/(MAX_REFRESH_RATE*2);
//	int  osleep = G_USEC_PER_SEC/(MAX_REFRESH_RATE);
	int  isleep = 0;

//	int  rsleep = G_USEC_PER_SEC/ gCfg.renderRate;
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
#endif

#if _USEGTK
	g_main_beg(!oloop);
#else
	app.exec();
#endif

#if 0
	bool   render;
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
//				rsleep  = G_USEC_PER_SEC/gCfg.renderRate;
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
				draw_time_timer(gRun.pMainWindow);

//			rsleep -= osleep;
//			tb4     = g_get_monotonic_time();
//			tb4     = tbn;

			g_usleep(osleep);
		}
	}
#endif

	DEBUGLOGS("aft g_main_beg call");

	if( gAlarmTimerId )
		g_source_remove(gAlarmTimerId);
	gAlarmTimerId = 0;

	if( gRun.drawTimerId )
		g_source_remove(gRun.drawTimerId);
	gRun.drawTimerId = 0;

	if( gRun.infoTimerId )
		g_source_remove(gRun.infoTimerId);
	gRun.infoTimerId = 0;

	if( gWaitzTimerId )
		g_source_remove(gWaitzTimerId);
	gWaitzTimerId = 0;

	if( gWaitZTimerId )
		g_source_remove(gWaitZTimerId);
	gWaitZTimerId = 0;

	cgui::dnitWorkspace();

	draw::end();
	cgui::dnit();
	snd::dnit();
	cfg::save();

	if( get_theme_icon_filename(true, true) )
	{
//		g_unlink(get_theme_icon_filename(false));
		unlink(get_theme_icon_filename(false));
	}

#ifdef _USEMTHREADS
	g_sync_threads_gui_end();
#endif

	DEBUGLOGE;
	return 0;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include "utility.h"

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

//	for( te = theme_list_beg(tl); !theme_list_end(tl); te = theme_list_nxt(tl) )
//		printf("%-*.*s%s/%s\n", maxn, maxn, te.pName->str, te.pPath->str, te.pFile->str);

	int         nc;
	const int   mc      =   2;
	const char* tfrmt   =  "\t%-*.*s";
	const char* hfrmt   =  "\n%s %s Themes:\n";
	const char* tpath[] = { get_user_theme_path(),  get_user_old_theme_path(), get_system_theme_path() };
	const char* tphs1[] = { "User",                 "User",                    "System Installed" };
	const char* tphs2[] = { APP_NAME,               APP_NAME_OLD,              APP_NAME_OLD };

	for( size_t p = 0; p < vectsz(tpath); p++ )
	{
		printf(hfrmt, tphs1[p], tphs2[p]);

		nc = mc;
		for( te = theme_list_beg(tl); !theme_list_end(tl); te = theme_list_nxt(tl) )
		{
			if( strcmp(te.pPath->str, tpath[p]) == 0 )
			{
				printf(tfrmt, maxn, maxn, te.pName->str);

				if( --nc <= 0 )
				{
					nc    = mc;
					printf("\n");
				}
			}
		}

		if( p != vectsz(tpath)-1 )
			delete [] tpath[p];

		if( nc && nc != mc )
			printf("\n");
	}

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
	DEBUGLOGB;
	DEBUGLOGP("signal is %d\n", signal);
//#endif

	gRun.appStop = true;

	if( get_theme_icon_filename(true, true) )
	{
//		g_unlink(get_theme_icon_filename(false));
		unlink(get_theme_icon_filename(false));
#if 0
		const char* tif = get_theme_icon_filename(false);
		DEBUGLOGP("deleting icon file %s\n", tif);
//		g_unlink(tif);
		unlink(tif);
#endif
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
	DEBUGLOGE;
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
// -----------------------------------------------------------------------------
void wait_on_startup()
{
	// waste time if no compositor available since apparently no 'compositor initd' event available and
	// some (all?) desktop linuxs start user startup apps at the same time lower-level processes are
	// starting up

	// can also be used to wait for the desktop background display in case of a non-composited desktop

	DEBUGLOGB;

	int         waitZ   = gRun.waitSecs2;
#if _USEGTK
	GdkDisplay* display = gdk_display_get_default();
	GdkScreen*  screen  = gdk_display_get_default_screen(display);
	bool        canpose = gdk_display_supports_composite(display) == TRUE;

	gRun.composed       = canpose && gdk_screen_is_composited(screen) && gdk_screen_get_rgba_visual(screen);

	DEBUGLOGP("composite support is %s\n", canpose ? "on" : "off");
	DEBUGLOGP("display screen %s composited\n", gRun.composed ? "is" : "is NOT");

	if( !gRun.composed && canpose && gRun.waitSecs1 )
	{
		const int waitz          = gRun.waitSecs1;                  // secs to wait for compositing use
//		const int times_per_sec  = 4;                               // sleep for 1/4th of a sec each time
		const int times_per_sec  = 1;                               // sleep for 1/4th of a sec each time
		const int usec_per_sec   = 1000000;                         // # of usecs in a sec
		const int sleep_usecs    = usec_per_sec/times_per_sec;      // # of usecs to sleep each time
		int       times_to_sleep = waitz*usec_per_sec/sleep_usecs;  // # of times to do compositing check
		waitZ                    = gRun.waitSecs2 - gRun.waitSecs1; //

		DEBUGLOGP("secs_to_wait=%d, times_per_sec=%d, sleep_usecs=%d, times_to_sleep=%d\n",
			waitz, times_per_sec, sleep_usecs, times_to_sleep);

#ifdef _DEBUGLOG
		{
			char   tstr[128];
			time_t timeCur =  time(NULL);
			tm     timeCtm = *localtime(&timeCur);
			strfmtdt  (tstr,  vectsz(tstr), "%-H:%M:%S", &timeCtm);
			DEBUGLOGSF("bef sleep looping");
			DEBUGLOGSF(tstr);
		}
#endif
		while( times_to_sleep-- )
		{
			if( gdk_screen_is_composited(screen) )
			{
				DEBUGLOGS("screen is composited");

				if( gdk_screen_get_rgba_visual(screen) )
				{
					DEBUGLOGS("screen has an rgba visual - compositing now okay?");
					gRun.composed = true;
					break;
				}
			}

			DEBUGLOGS("sleeping in compositing test loop");
			usleep(sleep_usecs);

			if( gRun.appStop )
				break;
		}

#ifdef _DEBUGLOG
		{
			char       tstr[128];
			time_t     timeCur =  time(NULL);
			tm         timeCtm = *localtime(&timeCur);
			strfmtdt  (tstr,  vectsz(tstr), "%-H:%M:%S", &timeCtm);
			DEBUGLOGSF("aft sleep looping");
			DEBUGLOGSF(tstr);
		}
#endif
	}
#endif // _USEGTK

	if( waitZ > 0 && !gRun.appStop )
	{
		DEBUGLOGP("waiting %d seconds to redraw icon\n", waitZ);
		gWaitZTimerId = g_timeout_add_full(waitPrio, waitZ*1000, (GSourceFunc)waitZ_time_timer, (gpointer)gRun.pMainWindow, NULL);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
gboolean waitz_time_timer(PWidget* pWidget)
{
	DEBUGLOGB;

	if( !gRun.appStop )
	{
		g_source_remove(gWaitzTimerId);
		gWaitzTimerId = 0;
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean waitZ_time_timer(PWidget* pWidget)
{
	DEBUGLOGB;

	if( !gRun.appStop )
	{
		g_source_remove(gWaitZTimerId);
		gWaitZTimerId = 0;

		DEBUGLOGS("making theme icon");
		make_theme_icon(pWidget);
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void alarm_set(bool force, bool update)
{
	if( gAlarmTimerId && !force )
		return;

	if( !gCfg.alarms[0] )
		return;

	DEBUGLOGB;

	if( gAlarmTimerId )
		g_source_remove(gAlarmTimerId);
	gAlarmTimerId = 0;

	time_t at;
	tm     attm;
	bool   adowok;
	int    alarmSecs;
	char   gRun_aldow[8];
	gint   alhors, almins;
	char  *atime, *adow, *amsg, *alen;

	time_t ct   =  time(NULL);
	tm     cttm = *localtime(&ct);

	DEBUGLOGP("current time is at %d\n", (int)ct);
	DEBUGLOGP("current time is at %s",    asctime(&cttm));

	gRun_aldow[0] = '\0';
	gRun.almsg[0] = '\0';
	gRun.allen[0] = '\0';

	for( size_t l = 0; l < 2; l++ ) // possibly loop thru a second time to set "tomorrow's" first alarm
	{
		atime = gCfg.alarms;
		adow  = gCfg.alarmDays;
		amsg  = gCfg.alarmMsgs;
		alen  = gCfg.alarmLens;

		DEBUGLOGP("alarm str *%s*\n", atime);
		DEBUGLOGP("adow  str *%s*\n", adow);
		DEBUGLOGP("amsg  str *%s*\n", amsg);
		DEBUGLOGP("alen  str *%s*\n", alen);

		while( (alhors   = strtol(atime, &atime, 10)) > 0 )
		{
			atime++;                        // skip past : TODO: make skip more robust

			if( (almins  = strtol(atime, &atime, 10)) < 0 )
				break;

			attm         = cttm;
			attm.tm_hour = alhors;
			attm.tm_min  = almins;
			attm.tm_sec  = 0;
			at           = mktime(&attm);
			adowok       = adow && strlen(adow) > 6 ? adow[attm.tm_wday] != '.' : true;
			alarmSecs    = difftime(at, ct);

			DEBUGLOGP("alarm to go off at %2.2d:%2.2d, %d secs from now\n", (int)alhors, almins, alarmSecs);
			DEBUGLOGP("which is on/at %s", asctime(&attm));

			if( alarmSecs <= 0 || !adowok ) // alarm time passed or the day-of-the-week not to be used?
			{
				atime++;                    // skip past - TODO: make skip more robust

				adow  = adow ?  strchr(adow, ';') : NULL;
				adow += adow != NULL ? 1 : 0;
				amsg  = amsg ?  strchr(amsg, ';') : NULL;
				amsg += amsg != NULL ? 1 : 0;
				alen  = alen ?  strchr(alen, ';') : NULL;
				alen += alen != NULL ? 1 : 0;

				DEBUGLOGP("alarm str *%s*\n", atime);
				DEBUGLOGP("adow  str *%s*\n", adow);
				DEBUGLOGP("amsg  str *%s*\n", amsg);
				DEBUGLOGP("alen  str *%s*\n", alen);

				continue;                   // try the next alarm time
			}

			alarmSecs++;                    // give the app enough time to process the minute change

			char *edow = adow, *emsg = amsg, *elen = alen;

			DEBUGLOGS("bef find-end-of-strs code");
			if( adow )
				edow = strchr(adow, ';') ? strchr(adow, ';') : adow+strlen(adow);

			if( amsg )
				emsg = strchr(amsg, ';') ? strchr(amsg, ';') : amsg+strlen(amsg);

			if( alen )
				elen = strchr(alen, ';') ? strchr(alen, ';') : alen+strlen(alen);
			DEBUGLOGS("aft find-end-of-strs code");

			DEBUGLOGS("bef copy-strs-to-runtime code");
			if( edow-adow > 0 && edow-adow <= vectsz(gRun_aldow) )
			{
//				strvcpy(gRun.aldow, adow, edow-adow);
				strvcpy(gRun_aldow, adow, edow-adow);
				gRun_aldow[edow-adow] = '\0';
			}

			if( emsg-amsg > 0 && emsg-amsg <= vectsz(gRun.almsg) )
			{
				strvcpy(gRun.almsg, amsg, emsg-amsg);
				gRun.almsg[emsg-amsg] = '\0';
			}

			if( elen-alen > 0 && elen-alen <= vectsz(gRun.allen) )
			{
				strvcpy(gRun.allen, alen, elen-alen);
				gRun.allen[elen-alen] = '\0';
			}
			DEBUGLOGS("aft copy-strs-to-runtime code");

			gRun.alhors    =  alhors;
			gRun.almins    =  almins;
			gRun.angleAlm  = ((double)alhors + (double)almins/60.0)*30.0;
			gRun.angleAlm *=  gCfg.show24Hrs ?  0.5 : 1.0;
			gAlarmTimerId  =  g_timeout_add_seconds(alarmSecs, (GSourceFunc)alarm_time_timer, (gpointer)gRun.pMainWindow);

			DEBUGLOGP("alarm to go off at %s",                   asctime(&attm));
			DEBUGLOGP("alarm to go off at %2.2d:%2.2d\n",       (int)gRun.alhors, (int)gRun.almins);
			DEBUGLOGP("alarm to go off at %d\n",                (int)at);
			DEBUGLOGP("alarm to go off in %d secs from now\n",  (int)alarmSecs);
			DEBUGLOGP("alarm angle set at %f\n",                 gRun.angleAlm);
//			DEBUGLOGP("alarm activation days are %s\n",          gRun.aldow);
			DEBUGLOGP("alarm activation days are *%s*\n",        gRun_aldow);
			DEBUGLOGP("alarm message is *%s*\n",                 gRun.almsg);
			DEBUGLOGP("alarm message display len is %s secs\n", *gRun.allen ? gRun.allen : "15");
			break;
		}

		if( gAlarmTimerId )
			break;

		cttm.tm_mday++; // TODO: does this become invalid when at end-of-month?
		cttm.tm_hour = cttm.tm_min = cttm.tm_sec = 0;
//		mktime(&cttm);  // TODO: does this fix the potential end-of-month problem?
	}

	if( update )
	{
		draw::update_bkgnd();
#if _USEGTK
		gtk_widget_queue_draw(gRun.pMainWindow);
#endif
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
gboolean alarm_time_timer(PWidget* pWidget)
{
	DEBUGLOGB;

	if( !gRun.appStop )
	{
		if( gAlarmTimerId )
			g_source_remove(gAlarmTimerId);
		gAlarmTimerId = 0;
#if 0
		time_t ct   =  time(NULL);
		tm     cttm = *localtime(&ct);
#endif
		// TODO: this is not usable since it hangs the app when the user clicks on close - why?
#if 0
#if _USEGTK
		PWidget* pMsgBox =
			gtk_message_dialog_new(GTK_WINDOW(pWidget),
				GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
				"The clock alarm has been activated.\n\nIt is now %2.2d:%2.2d.",
				cttm.tm_hour, cttm.tm_min);

		if( pMsgBox )
		{
//			g_signal_connect_swapped(pMsgBox, "response", G_CALLBACK(gtk_widget_destroy), pMsgBox);
			gtk_window_set_transient_for(GTK_WINDOW(pMsgBox), GTK_WINDOW(pWidget));
			gtk_window_set_position(GTK_WINDOW(pMsgBox), GTK_WIN_POS_CENTER);
			gtk_window_set_title(GTK_WINDOW(pMsgBox), gRun.appName);
			gtk_dialog_run(GTK_DIALOG(pMsgBox));
			gtk_widget_hide(pMsgBox);
			gtk_widget_destroy(pMsgBox);
		}
#endif
#endif
		static const char* fmt1 = "%sIt is now %s.\n";
		static const char* fmt2 = "notify-send --urgency=normal --expire-time=%s000 --icon=\"%s\" \"%s\" \"%s\"\n";
		static const char* smry = "The clock alarm has been activated";

		char        body[2048], nmsg[2048];
		const char* msglen = *gRun.allen ? gRun.allen : "15";
		const char* premsg = *gRun.almsg ? ""         : "\n";
		const char* msgico =  get_theme_icon_filename(false);

		snprintf(body, vectsz(body), fmt1, premsg, gRun.ttlTimeTxt);

		if( *gRun.almsg )
		{
			snprintf(nmsg, vectsz(nmsg), "%s\n%s", body, gRun.almsg);
			strvcpy (body, nmsg);
		}

		snprintf(nmsg, vectsz(nmsg), fmt2, msglen, msgico, smry, body);
		system  (nmsg);

		DEBUGLOGP("notification msg request sent:\n%s\n", nmsg);
#if 0
		// TODO: try this out and see if you can get it to work (add libnotify to make)
		{
			notify_init();
			notify_set_app_name(gRun.appName); // necessary?
			NotifyNotification* pNN = notify_notification_new("req summary", "opt body", "opt icon");
			notify_notification_set_timeout(pNN, millisecs-timeout-int-val);
			notify_notification_show(pNN);
			// unref since a GObject?
			notify_uninit();
		}
#endif
		if( gCfg.doSounds && gCfg.doAlarms && !gRun.appStart && !gRun.appStop )
			snd::play_alarm(true);

		alarm_set(true, true);
	}

	DEBUGLOGE;
	return FALSE;
}

