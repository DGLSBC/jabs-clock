/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP    "glbl"

#include <math.h>     // for roundf
#include <stdio.h>    // for sprintf
#include <limits.h>   // for PATH_MAX
#include <unistd.h>   // for getpid

#include "global.h"   //
#include "platform.h" // platform specifics

#include "draw.h"     //
#include "copts.h"    // for copts gets/sets
#include "debug.h"    // for debugging prints
#include "glade.h"    //
#include "cfgdef.h"   //
#include "config.h"   // for Config struct, CornerType enum, ...
#include "utility.h"  // for ?
#include "dayNight.h" // for sunrise/set calculations
#include "settings.h" // for PREFSTR_ and ...
#include "sound.h"    // for snd::update

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static gboolean    draw_time_timerQ(PWidget* pWindow); // on_expose via queued draw rendering
static gboolean    draw_time_timerD(PWidget* pWindow); // direct context rendering
static const char* get_icon_filename();

// -----------------------------------------------------------------------------
static time_t g_timeCur   = 0; // 'current' local time
static time_t g_timeBef   = 0; // previous 'current' local time
static gint   g_timeDif   = 0; // 'current' diff between the two
static bool   g_icoBlding = false;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void change_ani_rate(PWidget* pWindow, int renderRate, bool force, bool setgui)
{
	DEBUGLOGB;

	if( gRun.appStart )
	{
		DEBUGLOGR(1);
		return;
	}

	renderRate  = renderRate < MIN_REFRESH_RATE ? MIN_REFRESH_RATE : renderRate;
	renderRate  = renderRate > MAX_REFRESH_RATE ? MAX_REFRESH_RATE : renderRate;

	bool nuRate = gCfg.renderRate != renderRate;

	if( nuRate || force )
	{
		// timer runs 10 times faster than the frame rate so we can pick better
		// points in the animation curve, i.e., fewer points are picked in the
		// flat part(s) of the curve and more points are picked in the curvy
		// part(s) of the curve, and it is at these points that the frames
		// are drawn

		gRun.renderFrame = 0;
		gCfg.renderRate  = renderRate;

//		renderRate      *= 10;
#if 0
		renderRate       = 1000/renderRate;
		renderRate       = renderRate == 0 ? 1 : renderRate;
#endif
//		if( nuRate || !gRun.drawTimerId )
		{
#if 0
			if( gRun.drawTimerId )
			{
				GSource* pS = g_main_context_find_source_by_id(NULL, gRun.drawTimerId);
				gint64   rt = g_source_get_ready_time(pS);
				DEBUGLOGP("ready time is %u\n", (unsigned int)rt);
			}
#endif
//			DEBUGLOGP("changing renderRate to %d\n", gCfg.renderRate);

			// TODO: figure out why textOnly doesn't immediately update fuzzy
			//       date on clock face but it does in the tooltip text, then
			//       chg the showSlow time back to once every 15 secs

//			bool showSlow    = !gCfg.showSeconds;
			bool showSlow    = !gCfg.showSeconds ||  gCfg.textOnly ||  gCfg.noDisplay;
			bool showNormal  =  gCfg.showSeconds && !gCfg.textOnly && !gCfg.noDisplay && gRun.renderIt;
			int  normaltps   =  gCfg.renderRate*(gCfg.refSkipCnt+1);

//			int  timeoutms   =  showNormal ? 1000/gCfg.renderRate : (showSlow ? 15000/1 : 1000/2);
			int  timeoutms   =  showNormal ? roundf(1000.0f/float(normaltps)) : (showSlow ? 10000/1 : 1000/2);
//			int  timeoutms   =  showNormal ? roundf(1000.0f/float(normaltps)) : (showSlow ?  1000/1 : 1000/2);

//			int  refreshRate = (int)((double)gCfg.renderRate*(1.0-gRun.secDrift));
//			int  timeoutms   =  showNormal ? 1000/refreshRate : (showSlow ? 15000/1 : 1000/2);

			if( gRun.drawTimerId )
				g_source_remove(gRun.drawTimerId);

			DEBUGLOGP("draws/s=%d, skips/s=%d, timeouts/s=%d, timeout duration=%d ms\n",  gCfg.renderRate, gCfg.refSkipCnt, normaltps, timeoutms);
			GSourceFunc draw_timer = (GSourceFunc)(gCfg.queuedDraw ? draw_time_timerQ  :  draw_time_timerD);
			gRun.drawTimerId       = g_timeout_add_full(drawPrio, timeoutms, draw_timer, (gpointer)pWindow, NULL);
			DEBUGLOGP("draw timer using %s draw rendering\n", draw_timer == (GSourceFunc)draw_time_timerQ ? "widget queued" : "direct call");
			DEBUGLOGS("after g_timeout_add_full call");
		}

		if( setgui )
		{
#if _USEGTK
			DEBUGLOGS("before prefs::set_ani_rate call");
			prefs::set_ani_rate(gCfg.renderRate);
			DEBUGLOGS("after  prefs::set_ani_rate call");
#endif
		}
	}

	DEBUGLOGE;
}

#if _USEGTK
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void change_cursor(PWidget* pWindow, GdkCursorType type)
{
//	gdk_window_set_cursor(gtk_widget_get_window(pWindow), gdk_cursor_new_for_display(gdk_display_get_default(), type));

	if( pWindow )
	{
		GdkCursor* pCursor = gdk_cursor_new_for_display(gtk_widget_get_display(pWindow), type);

		if( pCursor )
		{
			gdk_window_set_cursor(gtk_widget_get_window(pWindow), pCursor);
#if GTK_CHECK_VERSION(3,0,0)
			g_object_unref  (pCursor);
#else
			gdk_cursor_unref(pCursor);
#endif
		}
	}
}
#endif // _USEGTK

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#ifdef _USEMTHREADS
struct ChangeTheme
{
	PWidget* pWindow;
	bool     updateSurfs;
//	char     path[PATH_MAX];
//	char     file [64];
//	char     modes[32];
};

// -----------------------------------------------------------------------------
static void change_theme_end(gpointer data, const ThemeEntry& te, bool valid)
{
	DEBUGLOGB;

	ChangeTheme* pCT = (ChangeTheme*)data;

	DEBUGLOGP("  tepath is %s\n", te.pPath  && te.pPath->str  ? te.pPath->str  : "");
//	DEBUGLOGP("  ctpath is %s\n", pCT->path);
	DEBUGLOGP("  tefile is %s\n", te.pFile  && te.pFile->str  ? te.pFile->str  : "");
//	DEBUGLOGP("  ctfile is %s\n", pCT->file);
	DEBUGLOGP("  temode is %s\n", te.pModes && te.pModes->str ? te.pModes->str : "");
//	DEBUGLOGP("  ctmode is %s\n", pCT->modes);
	DEBUGLOGP("  theme  is %s\n", valid ? "valid" : "NOT valid");

	g_sync_threads_gui_beg();
	PWidget*   pWidget = pCT->pWindow;
#if _USEGTK
	bool       wndOkay = pWidget && gtk_widget_get_has_window(pWidget);
	GdkWindow* pWindow = wndOkay ?  gtk_widget_get_window    (pWidget) : NULL;
#else
	bool       wndOkay = false;
	void*      pWindow = NULL;
#endif
	g_sync_threads_gui_end();

	bool doIcon = valid && !gRun.appStart;

	if( valid )
	{
		strvcpy(gCfg.themePath, te.pPath->str);
		strvcpy(gCfg.themeFile, te.pFile->str);
	}

	if( wndOkay )
	{
		if( pCT->updateSurfs ) // TODO: when would this ever NOT be set when changing themes?
		{
			bool yield    =  true;
//			bool themes24 =  doIcon    &&  true && (strstr(te.pFile->str, "-24") != NULL || strcmp(te.pModes->str, "24") == 0);
//			bool switch24 = (themes24  && !gCfg.show24Hrs) || (!themes24 && gCfg.show24Hrs);
			bool mode24   =  te.pModes &&  strcmp(te.pModes->str, "24") == 0;
			bool themes24 =  doIcon    &&  true && (strstr(te.pFile->str, "-24") != NULL || mode24);
			bool switch24 = (themes24  && !gCfg.show24Hrs) || (!themes24 && gCfg.show24Hrs);

			if( switch24 )
				gCfg.show24Hrs = !gCfg.show24Hrs;

			g_expri_thread(+10);
			g_yield_thread(yield);
			gRun.updateSurfs = true;
			DEBUGLOGS("bef surfaces updating");
			update_shapes(pWidget, gCfg.clockW, gCfg.clockH, true, false, false); // surfs only
			DEBUGLOGS("aft surfaces updating");
			g_yield_thread(yield);
			g_expri_thread(-10);

			draw::lock(true);
			DEBUGLOGS("bef surfaces swapping");

			draw::update_surfs_swap();
			draw::render_set(pWidget, true, false);

			DEBUGLOGS("aft surfaces swapping");
			draw::lock(false);
#if _USEGTK
			g_sync_threads_gui_beg();
			gdk_window_process_updates(pWindow, TRUE);
			g_sync_threads_gui_end();
#endif
			g_expri_thread(0);
			g_yield_thread(yield);
			DEBUGLOGS("bef shape masks updating");
			update_shapes(pWidget, gCfg.clockW, gCfg.clockH, false, true, true);  // masks only
			DEBUGLOGS("aft shape masks updating");
			g_yield_thread(yield);

			if( switch24 )
			{
				g_sync_threads_gui_beg();
				alarm_set(true);
				update_ts_info(pWidget, true, true, true);
#if _USEGTK
				prefs::ToggleBtnSet(prefs::PREFSTR_24HOUR, gCfg.show24Hrs);
#endif
				doIcon = false; // timestamp info updating takes care of this
				g_sync_threads_gui_end();
			}
		}
#if _USEGTK
		DEBUGLOGS("requesting redraw");
		g_sync_threads_gui_beg();
		gtk_widget_queue_draw(pWidget);
		gdk_window_process_updates(pWindow, TRUE);
		g_sync_threads_gui_end();
#endif
	}

	if( doIcon )
	{
		DEBUGLOGS("making theme icon");
		g_sync_threads_gui_beg();
		make_theme_icon(pWidget);
		g_sync_threads_gui_end();
	}

	if( valid )
		cfg::save(true);

	if( wndOkay )
	{
#if _USEGTK
		if( !gRun.scrsaver )
		{
			g_sync_threads_gui_beg();
			change_cursor(pWidget, GDK_FLEUR);
			g_sync_threads_gui_end();
		}
#endif
	}

	gRun.updating = false;

	DEBUGLOGE;
}
#endif // _USEMTHREADS

// -----------------------------------------------------------------------------
void change_theme(PWidget* pWindow, const ThemeEntry& te, bool doSurfs)
{
	DEBUGLOGB;

	// TODO: move the badTE testing to a func within themes (theme_ntry_bad?)
	// TODO: move the retrieval of the internal theme to a func within themes (theme_ntry_int?)

	ThemeEntry tv     =  te;
	bool       direct =  false;
	bool       badTE  = !te.pPath || !te.pPath->str || !(*te.pPath->str) || !g_isa_dir (te.pPath->str) ||
	                    !te.pFile || !te.pFile->str || !(*te.pFile->str) || !g_isa_sdir(te.pPath->str, te.pFile->str);
	if( badTE )
	{
		DEBUGLOGS("theme full path not found - using internal");

		ThemeList*     tl;
		theme_list_get(tl);
		int            tc = theme_list_cnt(tl);
		theme_ntry_cpy(tv,  theme_list_nth(tl, tc-1)); // should always have the internal theme listed
		theme_list_del(tl);

		direct = true;
	}

	DEBUGLOGP("path is %s\n", tv.pPath  && tv.pPath->str  ? tv.pPath->str  : "");
	DEBUGLOGP("file is %s\n", tv.pFile  && tv.pFile->str  ? tv.pFile->str  : "");
	DEBUGLOGP("name is %s\n", tv.pName  && tv.pName->str  ? tv.pName->str  : "");
	DEBUGLOGP("mode is %s\n", tv.pModes && tv.pModes->str ? tv.pModes->str : "");

	if( doSurfs )
		gRun.updateSurfs = true;

	gRun.updating = true;

#if _USEGTK
	bool wndOkay  = pWindow && gtk_widget_get_has_window(pWindow);
#else
	bool wndOkay  = false;
#endif

	if( wndOkay )
	{
#if _USEGTK
		if( !gRun.scrsaver )
			change_cursor(pWindow, GDK_WATCH);
#endif
	}

#ifdef _USEMTHREADS
	if( pWindow )
	{
		static ChangeTheme ct;

		ct.pWindow     = pWindow;
		ct.updateSurfs = gRun.updateSurfs;

		bool valid = draw::update_theme(tv, change_theme_end, &ct);

		DEBUGLOGP("async theme updating start is %s\n", valid ? "'valid'" : "not 'valid'");

		if( direct )
			theme_ntry_del(tv);

		DEBUGLOGR(2);
		return;
	}
#endif // _USEMTHREADS

	bool valid = draw::update_theme(tv, NULL, NULL);

	// TODO: need to combine the following into one with the change_theme_end func

	DEBUGLOGP("sync theme change is %s\n", valid ? "'valid'" : "not 'valid' (no drawable elements)");

	if( wndOkay )
	{
#if _USEGTK
		DEBUGLOGS("requesting redraw");
		gtk_widget_queue_draw(pWindow);

		if( gRun.updateSurfs ) // TODO: when would this ever NOT be set when theme changing?
		{
			DEBUGLOGS("updating surfaces");
			gdk_window_process_updates(gtk_widget_get_window(pWindow), TRUE);
		}
#endif
	}

	if( valid )
	{
		strvcpy(gCfg.themePath, tv.pPath->str);
		strvcpy(gCfg.themeFile, tv.pFile->str);

		bool doIcon   =  true;
//		bool doIcon   = !gRun.appStart;
		bool mode24   =  tv.pModes &&  strcmp(tv.pModes->str, "24") == 0;
		bool themes24 =  doIcon    &&  true && (strstr(tv.pFile->str, "-24") != NULL || mode24);
		bool switch24 = (themes24  && !gCfg.show24Hrs) || (!themes24 && gCfg.show24Hrs);

		if( switch24 )
			gCfg.show24Hrs = !gCfg.show24Hrs;

		if( !gRun.appStart )
		{
			DEBUGLOGS("making theme icon");
			make_theme_icon(pWindow);
		}
	}

	if( wndOkay )
	{
#if _USEGTK
		if( !gRun.scrsaver )
			change_cursor(pWindow, GDK_FLEUR);
#endif
	}

	if( direct )
		theme_ntry_del(tv);

	gRun.updating = false;

	DEBUGLOGE;
}

#if _USEGTK
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void docks_send_update()
{
	if( !gCfg.showInTasks )
		return;

	DEBUGLOGB;

	static const char* dbus   =  "org.cairodock.CairoDock";
	static const char* objpth = "/org/cairodock/CairoDock";
	static const char* iface  =  dbus;

	GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL, dbus, objpth, iface, NULL, NULL);

	if( proxy )
	{
		char  tstr[128];
		const char* tfmt = gCfg.show24Hrs ? "%-H:%M" : "%-I:%M"; // TODO: another new cfg format string
		strfmtdt(tstr, vectsz(tstr), tfmt, &gRun.timeCtm);
//		const int   toff = tfmt[1] == 'I' && *tstr == '0' ? 1 : 0;

		if( strlen(tstr) > 5 )
			tstr[5] = '\0';

		// TODO: figure out if all dbus target servers can be determined at runtime
		//       and have similar support for dbus-spec'd on-icon labeling strings
		//       (cairo-dock (done), awn?, docky? (no), plank? (no), others?)

		GVariant*   parms;
		GVariant*   rcode;
//		char        clasn[128];
		const char* vafmt = "(ss)";
		const char* clasn = "class=" APP_NAME;

		// TODO: way to set specific icon for each running clock?
//		snprintf(clasn, vectsz(clasn), "class=%s%d", APP_NAME, (int)getpid());

//		if( parms = g_variant_new(vafmt, tstr+toff, clasn) )
		if( parms = g_variant_new(vafmt, tstr, clasn) )
			if( rcode = g_dbus_proxy_call_sync(proxy, "SetQuickInfo", parms, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL) )
				g_variant_unref(rcode);

		if( !g_icoBlding && get_theme_icon_filename() )
		{
			if( parms = g_variant_new(vafmt, get_theme_icon_filename(false), clasn) )
				if( rcode = g_dbus_proxy_call_sync(proxy,  "SetIcon", parms, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL) )
					g_variant_unref(rcode);
		}

		g_object_unref(proxy);
		proxy = NULL;
	}

	DEBUGLOGE;
}
#endif // _USEGTK

// -----------------------------------------------------------------------------
gboolean draw_anim_timer(PWidget* pWindow)
{
	if( gRun.appStop )
		return FALSE;

	GTimeVal            ct;
	g_get_current_time(&ct);

	static int    nr =  0;
	static double bt =  0;
	double        nt = (double)ct.tv_sec + (double)ct.tv_usec*1.0e-6;
	double        dt =  nr ? nt - bt : 0;
	bt               =  nt;
	nr++;

	DEBUGLOGB;
	DEBUGLOGP("rendering (%d) at %.4f (%d fps)\n", nr, (float)dt, nr == 1 ? 0 : (int)(1.0/dt));

	gboolean ret    = TRUE;
	bool     set    = true;
	gRun.animScale += dt*2.0;

	if( gRun.renderUp || (gRun.animScale >= 1.0) )
	{
		if( gRun.drawTimerId )
			g_source_remove(gRun.drawTimerId);
		gRun.drawTimerId = 0;

		DEBUGLOGS("resetting animscale, appstart, & renderup");

		gRun.animScale = 1;
		gRun.appStart  = false;
		gRun.renderUp  = false;
		ret            = FALSE;

		if( !gCfg.aniStartup )
		{
			draw::render_set(NULL, false, false, gRun.animWindW, gRun.animWindH);
			draw::render(pWindow);
		}

#if _USEGTK
		gdk_window_set_opacity(gtk_widget_get_window(pWindow), gCfg.opacity);
#endif
		change_ani_rate (pWindow, gCfg.renderRate, true, false);

		draw::render_set(pWindow, true, true, gRun.animWindW, gRun.animWindH);

		DEBUGLOGS("making theme icon");
		make_theme_icon(pWindow);

		set = gCfg.clockW != gRun.animWindW || gCfg.clockH != gRun.animWindH;

		if( set )
		{
			DEBUGLOGS("updating rendering surfaces to clock size");
			DEBUGLOGP("  clock W/H   is (%d, %d)\n", gCfg.clockW,     gCfg.clockH);
			DEBUGLOGP("  animW W/H   is (%d, %d)\n", gRun.animWindW,  gRun.animWindH);
			DEBUGLOGP("  drawScaling is (%f, %f)\n", gRun.drawScaleX, gRun.drawScaleY);

			update_wnd_dim(pWindow, gCfg.clockW, gCfg.clockH, gRun.animWindW, gRun.animWindH, true, false);
		}
		else
		{
			gRun.drawScaleX = gRun.drawScaleY = 1;
			draw::render_set(pWindow, true, false);
			set = false;
		}
	}

//	if( !gRun.marcoCity )
//		gdk_window_set_opacity(gtk_widget_get_window(pWindow), gCfg.opacity*gRun.animScale);

#if _USEGTK
	gdk_window_set_opacity(gtk_widget_get_window(pWindow), gCfg.opacity*gRun.animScale);
#endif

	if( set )
		draw::render_set(NULL, false, false, gRun.animWindW, gRun.animWindH);

//#ifdef DEBUGLOG
	{
		static int ct = 0;
//		if( ++ct < 100 ) DEBUGLOGP("rendering(%d)\n", ct);
		DEBUGLOGP("rendering(%d)\n", ++ct);
	}
//#endif

	gCfg.refSkipCur = 0; // to prevent this frame from being skipped (not drawn)

#if _USEGTK
	gtk_widget_queue_draw(pWindow);
//	gdk_window_invalidate_rect(gtk_widget_get_window(pWindow), NULL, FALSE);
//	gdk_window_process_updates(gtk_widget_get_window(pWindow), FALSE);
#endif

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
gboolean draw_time_timerD(PWidget* pWindow)
{
	if( gRun.appStop )
		return FALSE;

	if( gCfg.refSkipCur-- > 0 )
		return TRUE;

	gCfg.refSkipCur = gCfg.refSkipCnt;

	DEBUGLOGB;

#ifdef DEBUGLOG
	{
		static int ct = 0;
//		if( ++ct < 100 ) DEBUGLOGP("rendering(%d)\n", ct);
		DEBUGLOGP("rendering(%d)\n", ++ct);
	}
#endif

#if _USEGTK
#ifdef DEBUGLOG
	{
	gint                                              wndX,  wndY;
	gint                                              wndW,  wndH;
	gtk_window_get_position(GTK_WINDOW(pWindow),     &wndX, &wndY);
	gtk_window_get_size    (GTK_WINDOW(pWindow),     &wndW, &wndH);
	DEBUGLOGP("wndX=%d, wndY=%d, wndW=%d, wndH=%d\n", wndX,  wndY, wndW, wndH);
	}
#endif
#endif

#if 0 // NOTE: this is a test of how the new animation scaling & rotation works
	{
		static double dir  =-1.00;
		gRun.animScale    += 0.03*dir;
		gCfg.clockR       += 0.03*dir*5.0;

		if( gRun.animScale < 0.99 )
		{
			gRun.animScale = 0.99;
			gCfg.clockR    =-2.50;
			dir = -dir;
		}
		if( gRun.animScale > 1.00 )
		{
			gRun.animScale = 1.00;
			gCfg.clockR    = 2.50;
			dir = -dir;
		}

		draw::render_set();
	}
#endif

//	DEBUGLOGZ("bef calling draw::render", 5000);

	gRun.secDrift = (double)(draw::render() % 1000000)*1.0e-6;

//	DEBUGLOGZ("aft calling draw::render", 5000);

	DEBUGLOGE;
	return TRUE;
}

// -----------------------------------------------------------------------------
gboolean draw_time_timerQ(PWidget* pWindow)
{
	if( gRun.appStop )
		return FALSE;

	if( gCfg.refSkipCur-- > 0 )
		return TRUE;

	gCfg.refSkipCur = gCfg.refSkipCnt;

	DEBUGLOGB;

#ifdef DEBUGLOG
	{
		static int ct = 0;
//		if( ++ct < 100 ) DEBUGLOGP("rendering(%d)\n", ct);
		DEBUGLOGP("rendering(%d)\n", ++ct);
	}
#endif

#if _USEGTK
#ifdef DEBUGLOG
	{
	gint                                              wndX,  wndY;
	gint                                              wndW,  wndH;
	gtk_window_get_position(GTK_WINDOW(pWindow),     &wndX, &wndY);
	gtk_window_get_size    (GTK_WINDOW(pWindow),     &wndW, &wndH);
	DEBUGLOGP("wndX=%d, wndY=%d, wndW=%d, wndH=%d\n", wndX,  wndY, wndW, wndH);
	}
#endif
#endif

#if 0 // NOTE: this is a test of how the new animation scaling & rotation works
	{
		static double dir  =-1.00;
		gRun.animScale    += 0.03*dir;
		gCfg.clockR       += 0.03*dir*5.0;

		if( gRun.animScale < 0.99 )
		{
			gRun.animScale = 0.99;
			gCfg.clockR    =-2.50;
			dir = -dir;
		}
		if( gRun.animScale > 1.00 )
		{
			gRun.animScale = 1.00;
			gCfg.clockR    = 2.50;
			dir = -dir;
		}

		draw::render_set();
	}
#endif

#if _USEGTK
//	DEBUGLOGZ("bef calling draw::render", 5000);

	gtk_widget_queue_draw(pWindow);
//	gdk_window_invalidate_rect(gtk_widget_get_window(pWindow), NULL, FALSE);
//	gdk_window_process_updates(gtk_widget_get_window(pWindow), FALSE);

//	DEBUGLOGZ("aft calling draw::render", 5000);
#endif

	DEBUGLOGE;
	return TRUE;
}

#if 0
#ifdef _USEOLDDRAWTIMETIMER
// -----------------------------------------------------------------------------
gboolean draw_time_timer(PWidget* pWindow)
{
	if( gRun.appStop )
		return FALSE;

	DEBUGLOGB;

	{
		static int ct = 0;
		if( ++ct < 100 ) DEBUGLOGP("rendering(%d)\n", ct);
	}

	bool forceDraw = gRun.appStart || gRun.renderUp;

#if 0
	if( forceDraw )
	{
		if( gRun.renderUp || ((gRun.drawScaleX = gRun.drawScaleY += 0.01) >= 1.00) )
		{
			gRun.drawScaleX =  gRun.drawScaleY = 1;
			gRun.appStart   =  false;
			gRun.renderUp   =  false;

			change_ani_rate(pWindow, gCfg.renderRate, true, true);
			make_theme_icon(pWindow);
		}

		gdk_window_set_opacity(gtk_widget_get_window(pWindow), gCfg.opacity*gRun.drawScaleX);

		draw::render_set(pWindow);
		draw::render(pWindow);
	}
	else
#endif
//	{
		draw::render();
//	}

	DEBUGLOGE;
	return TRUE;

//	if( forceDraw || ++gRun.renderFrame >= 10 )
	{
		{
			static int ct = 0;
//			if( ++ct < 10 ) DEBUGLOGP("rendering(%d)\n", ct);
			DEBUGLOGP("rendering(%d)\n", ++ct);
		}
#if 0
//		gtk_widget_queue_draw(pWidget);
		gdk_window_invalidate_rect(gtk_widget_get_window(pWidget), NULL, FALSE);
		gdk_window_process_updates(gtk_widget_get_window(pWidget), TRUE);
#endif
		draw::render(forceDraw ? pWidget : NULL);
//		draw::render(pWidget);
//		draw::render();
#if 0
//		cairo_t*
		static cairo_t*
			pContext =  NULL;
		if( pContext == NULL )
			pContext =  gdk_cairo_create(gtk_widget_get_window(pWidget));

		if( pContext )
#endif
		{
//			gdk_window_invalidate_rect(gtk_widget_get_window(pWidget), NULL, FALSE);
//			draw::renderer()(pContext, gRun.drawScaleX, gRun.drawScaleY, gCfg.clockW, gCfg.clockH, gRun.appStart);
//			draw::renderer()(NULL, gRun.drawScaleX, gRun.drawScaleY, gCfg.clockW, gCfg.clockH, gRun.appStart);
//			cairo_destroy(pContext);
		}

//		GdkRegion* pRegion = gdk_window_get_update_area(gtk_widget_get_window(pWidget));
//		if(        pRegion ) gdk_region_destroy(pRegion);

		gRun.renderFrame = 0;
	}

//	draw::render(pWidget, forceDraw);

	DEBUGLOGE;
	return TRUE;

}
#endif // _USEOLDDRAWTIMETIMER
#endif

#if _USEGTK
// -----------------------------------------------------------------------------
void get_cursor_pos(PWidget* pWidget, int& x, int& y, GdkModifierType& m)
{
	GdkWindow*  pWindow  = gtk_widget_get_window (pWidget);
	GdkDisplay* pDisplay = gdk_window_get_display(pWindow);
#if GTK_CHECK_VERSION(3,0,0)
//	TODO: using the following produces a compilation error due to a buggy header
//	gdk_window_get_device_position(pWindow, pDisplay->core_pointer, &x, &y, &m);
	gdk_display_get_pointer(pDisplay, NULL, &x, &y, &m);
#else
	gdk_display_get_pointer(pDisplay, NULL, &x, &y, &m);
#endif
}
#endif // _USEGTK

// -----------------------------------------------------------------------------
const char* get_icon_filename()
{
	static const char* aico = DATA_DIR "/pixmaps/" APP_NAME_OLD ".png";

	const char* ret = NULL;

	if( g_isa_file(aico) )
	{
		ret = aico;
	}
#if _USEGTK
	else
	{
		static const char* inms[] = { APP_NAME, "clock", NULL };

		// TODO: how to not hardcode 48? how to only call this when necessary, e.g., on theme change?
		GtkIconInfo* pII = gtk_icon_theme_choose_icon(gtk_icon_theme_get_default(), inms, 48, (GtkIconLookupFlags)0);

		if( pII )
		{
			const char* iif = gtk_icon_info_get_filename(pII);

			if( iif )
			{
				static  char  retp[PATH_MAX];
				strvcpy(retp, iif);
				ret =   retp;
			}

#if GTK_CHECK_VERSION(3,0,0)
			g_object_unref(pII);
#else
			gtk_icon_info_free(pII);
#endif
		}
	}
#endif // _USEGTK

	DEBUGLOGP("ret=*%s*\n", ret);

	return ret;
}

//#endif // _USEGTK

// -----------------------------------------------------------------------------
const char* get_theme_icon_filename(bool check, bool update)
{
	static char  path[PATH_MAX];
	static char  anmp[PATH_MAX];
	static int   pid   = 0;
	static bool  first = true;
	const  char* retp  = path;

	if( first )
	{
		first            = false;
		pid              = getpid();
		const char* tdir = get_user_appnm_path();

//		sprintf(path, "%s/theme%d.png", tdir ? tdir : ".", pid);
		sprintf(path, "%s/theme%d.%s",  tdir ? tdir : ".", pid, gCfg.pngIcon ? "png" : "svg");
		strvcpy(anmp,  tdir ? tdir : ".");

		if( tdir )
			delete [] tdir;

		DEBUGLOGP("anmp=*%s*\n", anmp);
		DEBUGLOGP("path=*%s*\n", path);
	}

	bool okay = g_isa_dir(anmp) && g_isa_file(path);

	if( check )
	{
		if( !okay )
			retp = update ? NULL : get_icon_filename();
	}
	else
	if( !okay && !update )
		retp  =   get_icon_filename();

	return retp;
}

#if _USEGTK
// -----------------------------------------------------------------------------
gboolean info_time_timer(PWidget* pWindow)
{
	if( gRun.appStop )
		return FALSE;

	DEBUGLOGB;

#ifdef _TESTABORTED
	static int cnt = 2*5; // try and cause abort after 5 secs (since called every 1/2 sec)

	if( --cnt == 0 )
	{
		DEBUGLOGS("bef invalid change_theme call");
#if 0
		ThemeEntry            te = { NULL, NULL, NULL, NULL };
		change_theme(pWindow, te,    true);
		abort();
#endif
		DEBUGLOGS("aft invalid change_theme call");
	}
#endif

	update_ts_info(pWindow);

	DEBUGLOGE;
	return TRUE;
}

#endif // _USEGTK

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static gpointer make_theme_icon_func(gpointer data);
static bool     mtif_lock = false;

// -----------------------------------------------------------------------------
void make_theme_icon(PWidget* pWindow)
{
	if( gRun.appStop )
		return;

	DEBUGLOGB;

#if _USEGTK
	if( g_icoBlding == false && !gRun.scrsaver && copts::taskbar_shown() && !gRun.appStart )
	{
		g_icoBlding =  true;
		DEBUGLOGP("creating theme ico image file for theme %s\n", gCfg.themeFile);

#ifdef _USEMTHREADS
		mtif_lock        = true;
		GThread* pThread = g_thread_try_new(__func__, make_theme_icon_func, (gpointer)pWindow, NULL);

		if( pThread )
		{
			g_thread_unref(pThread);
		}
		else
		{
			mtif_lock    = false;
			make_theme_icon_func(pWindow);
		}
#else
		mtif_lock = false;
		make_theme_icon_func(pWindow);
#endif
	}
	else
	{
		DEBUGLOGP("NOT creating theme ico image file for theme %s\n", gCfg.themeFile);
	}
#endif // _USEGTK

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
gpointer make_theme_icon_func(gpointer data)
{
	DEBUGLOGB;

#if _USEGTK
	if( mtif_lock )
		g_expri_thread(+10);

	if( false ) // TODO: change to setting for not making theme-based icons
	{
		g_icoBlding = false; // put here so can send ico update requests to docks

		docks_send_update();

		DEBUGLOGR(1);
		return 0;
	}

	if( mtif_lock )
		g_sync_threads_gui_beg();

	const char* iconPath = get_theme_icon_filename(false, true);

	if( mtif_lock )
		g_sync_threads_gui_end();

	if( iconPath )
	{
		draw::make_icon(iconPath);

		g_icoBlding = false; // put here so can send ico update requests to docks

		if( mtif_lock )
			g_sync_threads_gui_beg();

		set_window_icon((PWidget*)data);

		if( mtif_lock )
			g_sync_threads_gui_end();

		docks_send_update();
	}
	else
	{
		DEBUGLOGP("can't create the theme ico image file:\n*%s*\n", iconPath);
	}
#endif // _USEGTK

	DEBUGLOGE;
	return 0;
}

// -----------------------------------------------------------------------------
void set_window_icon(PWidget* pWindow)
{
#if _USEGTK
	if( pWindow && !g_icoBlding && get_theme_icon_filename() )
		gtk_window_set_icon_from_file(GTK_WINDOW(pWindow), get_theme_icon_filename(false), NULL);
#endif
}

// -----------------------------------------------------------------------------
void update_colormap(PWidget* pWindow)
{
	if( gRun.appStop || !pWindow )
		return;

	DEBUGLOGB;

#if _USEGTK
	GdkScreen* pScreen = gtk_widget_get_screen(pWindow);

	if( pScreen )
	{
#if !GTK_CHECK_VERSION(3,0,0)
		GdkColormap* pColormap = gdk_screen_get_rgba_colormap(pScreen);

		if( pColormap == NULL )
			pColormap =  gdk_screen_get_rgb_colormap(pScreen);

		if( pColormap )
			gtk_widget_set_colormap(pWindow, pColormap);
#else
		GdkVisual* pVisual = gdk_screen_get_rgba_visual(pScreen);

		if( pVisual == NULL )
			pVisual =  gdk_screen_get_system_visual(pScreen);

		if( pVisual )
			gtk_widget_set_visual(pWindow, pVisual);
#endif
	}
#endif // _USEGTK

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void update_shapes(PWidget* pWindow, int width, int height, bool dosurfs, bool domask, bool lock)
{
	if( gRun.appStop )
		return;

	DEBUGLOGB;

#if _USEGTK
	if( dosurfs )
	{
		DEBUGLOGS("updating surfs");
		draw::update_surfs(pWindow, width, height, lock);
	}

	if( domask )
	{
		DEBUGLOGS("updating masks");

		// TODO: need to rework when a square clock outline is needed instead
		//       of always assigning a shaped clock outline; in other words,
		//       only assign a shaped clock outline when 1) the window is not
		//       composited, and 2), the window is being moved (since we don't
		//       need a shaped clock outline when the clock isn't moving, unless
		//       the 'background' changes, so also 3), the window is not 'below
		//       all other window's; any others? (need some way to figure out
		//       that the desktop background has changed)))

		bool shapeWidget = !gRun.composed;                       // clock outlined if not composited
		bool squareInput =  gCfg.clickThru &&  gCfg.showInTasks; // square clicking mask (mask is empty)
		bool shapedInput = !squareInput    && !gRun.scrsaver;    // shaped clicking mask

		if( gRun.appStart && (gRun.animWindW != gCfg.clockW || gRun.animWindH != gCfg.clockH) )
		{
			squareInput  =  true;
			shapedInput  =  false;
		}

		DEBUGLOGP("widget %s composited\n",      shapeWidget ? "is NOT" : "is");
		DEBUGLOGP("clicking input mask is %s\n", squareInput ? "square" : (shapedInput ? "shaped" : "not modified"));

		if( squareInput || shapedInput )
		{
			draw::make_mask(pWindow, width, height, shapedInput, true, lock);      // for input

			if( shapeWidget )
			{
				draw::make_mask(pWindow, width, height, shapedInput, false, lock); // for clock outline
			}
			else
			{
				if( lock )
					g_sync_threads_gui_beg();
#if GTK_CHECK_VERSION(3,0,0)
				gtk_widget_shape_combine_region(pWindow, NULL);
#else
				gtk_widget_shape_combine_mask  (pWindow, NULL, 0, 0);
#endif
				if( lock )
					g_sync_threads_gui_end();
			}
		}
	}
#endif // _USEGTK

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool update_ts(bool force, bool* newHour, bool* newMin)
{
	DEBUGLOGB;

#if 0 // for testing
	gCfg.tzHorShoOff = 10;
	gCfg.tzMinShoOff = 30;
#endif

	g_timeBef = g_timeCur;
	g_timeCur = time(NULL);
	g_timeDif = g_timeCur - g_timeBef;

	if( !force && !g_timeDif )
	{
		DEBUGLOGR(1);
		return false;
	}

	gRun.timeBtm =  gRun.timeCtm;
	gRun.timeCtm = *localtime(&g_timeCur);

	if( gCfg.tzHorShoOff || gCfg.tzMinShoOff )
	{
		DEBUGLOGP("tzadj: old:(%2.2d:%2.2d)\n",   gRun.timeCtm.tm_hour, gRun.timeCtm.tm_min);
		gRun.timeCtm.tm_hour += gCfg.tzHorShoOff; // adjust for requested tzone
		gRun.timeCtm.tm_min  += gCfg.tzMinShoOff; // ditto
		DEBUGLOGP("tzadj: mid:(%2.2d:%2.2d)\n",   gRun.timeCtm.tm_hour, gRun.timeCtm.tm_min);
		mktime(&gRun.timeCtm);                    // adjust components as needed
		DEBUGLOGP("tzadj: new:(%2.2d:%2.2d)\n",   gRun.timeCtm.tm_hour, gRun.timeCtm.tm_min);
	}

	bool nuHour = gRun.timeCtm.tm_hour != gRun.timeBtm.tm_hour;
	bool nuMin  = gRun.timeCtm.tm_min  != gRun.timeBtm.tm_min;

	if( newHour ) *newHour = nuHour;
	if( newMin  ) *newMin  = nuMin;

	if( force || nuHour || nuMin )
	{
		draw::lock(true);
		gRun.hours     =  gRun.timeCtm.tm_hour;
		gRun.hours     =  gCfg.show24Hrs ? gRun.hours : gRun.hours % 12; // 1300 == 1pm for 12hr face
		gRun.minutes   =  gRun.timeCtm.tm_min;
		gRun.seconds   =  gRun.timeCtm.tm_sec;
		gRun.angleHor  = (gRun.minutes   + 60.0*gRun.hours)*0.5;         // 360/12=30 deg/hour
		gRun.angleHor *=  gCfg.show24Hrs ?  0.5 : 1.0;                   // 15 deg/hour for 24hr face
		gRun.angleMin  =  gRun.minutes*6.0;                              // 360/60= 6 deg/minute
		gRun.angleSec  =  gRun.seconds*6.0;                              // 360/60= 6 deg/second
		draw::lock(false);
	}

	DEBUGLOGE;
	return true;
}

// -----------------------------------------------------------------------------
void update_ts_info(PWidget* pWindow, bool forceTTip, bool forceDate, bool forceTime)
{
	if( gRun.appStop )
		return;

	DEBUGLOGB;

	bool newHour, newMinute;

	if( !update_ts(forceTTip || forceDate || forceTime, &newHour, &newMinute) )
	{
		DEBUGLOGR(1);
		return;
	}

	DEBUGLOGP("new time: %s hr face, hour %d, tz offset %d (%2.2f deg)\n", gCfg.show24Hrs ? "24" : "12", gRun.hours, gCfg.tzHorGmtOff, gRun.angleHor);

	static
	bool firstCall =  true;

	bool appIdle   =  firstCall || !gRun.appStart;
	bool updtTTip  = (forceTTip ||  gCfg.showTTips) && gRun.isMousing;
	bool updtDate  =  forceDate ||  gRun.timeCtm.tm_mday != gRun.timeBtm.tm_mday || gRun.timeCtm.tm_mon  != gRun.timeBtm.tm_mon || gRun.timeCtm.tm_year != gRun.timeBtm.tm_year;
	bool updtTime  =  forceTime ||  newHour || newMinute;
	bool updtTitle =  false;
	bool updtIcon  =  false;
	bool drawBkgnd =  false;
	bool drawDate  =  false;
	bool drawDraw  =  false;
	     firstCall =  false;

	if( !gCfg.showSeconds && newMinute )
	{
		DEBUGLOGS("!showSecs && minChg: setting drawDraw on");
		drawDraw = true;
	}

	if( updtDate )
	{
		char     tstr [vectsz(gRun.ttlDateTxt)];
		strfmtdt(tstr, vectsz(tstr), gCfg.show24Hrs ? gCfg.fmtDate24 : gCfg.fmtDate12, &gRun.timeCtm);

		DEBUGLOGS("updtDate: setting drawDate on");
		DEBUGLOGP("\tprev date str is %s\n", gRun.ttlDateTxt);
		DEBUGLOGP("\tnext date str is %s\n", tstr);

		strvcpy (gRun.ttlDateTxt, tstr);
		updtTitle = true;
		drawDate  = true;
	}

	if( updtTime )
	{
		DEBUGLOGS("updating timestamp & date/time text info");

		if( update_ts_text(newHour || forceDate) )
		{
			DEBUGLOGS("updtTime: setting drawDate on");
			drawDate = true;
		}

		if( newHour || forceTTip || forceDate || forceTime )
		{
			DEBUGLOGS("updating sunrise/set text info");

			struct tm    rise,      sets;
			char         timr[16],  tims[16];
			const char*  gCfg_srs = gCfg.show24Hrs ? "%R" : "%-I:%M %P"; // TODO: put into cfg/gui/etc?

			if( gCfg.tzLatitude != 0.0 || gCfg.tzLngitude != 0.0 )
			{
				DayNight     dn(gCfg.tzLatitude, gCfg.tzLngitude);
				dn.get(rise, sets);

				strfmtdt(timr, vectsz(timr),  gCfg_srs, &rise);
				strfmtdt(tims, vectsz(tims),  gCfg_srs, &sets);
#if 0
				char* begr = timr[0] == '0' ? timr+1 : timr;
				char* begs = tims[0] == '0' ? tims+1 : tims;

				snprintf(gRun.riseSetTxt, vectsz(gRun.riseSetTxt), "rise/set: %s, %s", begr, begs);
#else
				snprintf(gRun.riseSetTxt, vectsz(gRun.riseSetTxt), "rise/set: %s, %s", timr, tims);
#endif
			}
			else
			{
				strvcpy(gRun.riseSetTxt, "rise/set: - unknown -");
			}

			DEBUGLOGP("rise/set: %s\n", gRun.riseSetTxt);
		}

		if( (gCfg.optHand[1] && !gCfg.useSurf[1]) || (gCfg.optHand[2] && !gCfg.useSurf[2]) )
		{
			DEBUGLOGS("updtTime: setting drawBkgnd on if drawDate is off");
			drawBkgnd =  true;
		}

		updtTitle = true;
		updtIcon  = true;
	}

	if( updtTitle )
	{
		char     tstr[128];
		char*    fmtT = gCfg.show24Hrs ? gCfg.fmtTime24 : gCfg.fmtTime12;
//		int      toff = fmtT[1] == 'I' && *gRun.ttlTimeTxt == '0' ? 1 : 0;
//		snprintf(tstr, vectsz(tstr), "%s %s", gRun.ttlDateTxt, gRun.ttlTimeTxt+toff);
		snprintf(tstr, vectsz(tstr), "%s %s", gRun.ttlDateTxt, gRun.ttlTimeTxt);
		DEBUGLOGS("updating title");
#if _USEGTK
		gtk_window_set_title(GTK_WINDOW(pWindow), tstr);
#else
		gRun.pMainWindow->setWindowTitle(QString(tstr));
#endif
	}

	if( updtTTip )
	{
#if _USEGTK
		DEBUGLOGS("updating tooltip text");
		gtk_widget_trigger_tooltip_query(pWindow);
#endif
	}

	if( appIdle )
	{
		if( updtIcon && !gRun.appStart )
		{
			DEBUGLOGS("updating theme icon");
			make_theme_icon(pWindow);
		}

		bool drewDate = false;

		if( drawDate && gCfg.showDate )
		{
			DEBUGLOGS("updating date surface");
			draw::update_date_surf(false, false, true);
			drewDate = true;
			drawDraw = true;
		}

		if( drawBkgnd && !(drewDate && gCfg.faceDate) )
		{
			DEBUGLOGS("updating bkgnd surface");
			draw::update_bkgnd();
			drawDraw = true;
		}

		if( drawDraw )
		{
			DEBUGLOGS("requesting redraw");
#if _USEGTK
			gtk_widget_queue_draw(pWindow);
#endif
		}
	}

#if 1
	if( gCfg.doSounds && !gRun.appStart && !gRun.appStop )
	{
		if( gCfg.doAlarms )
		{
			DEBUGLOGS("checking alarms");
			snd::play_alarm(false);
		}

		if( gCfg.doChimes )
		{
			DEBUGLOGS("checking chimes");
			snd::play_chime(newMinute, newHour);
		}
	}
#endif
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool update_ts_text(bool newHour)
{
	static const double tbndv   =    5.0;
//	static const double qbndv   =   15.0;
	static const double tbhlf   =   tbndv*0.5;
//	static const double qbhlf   =   qbndv*0.5;
//	static const char*  hsfxs[] = { " am", " pm", "" };
//	static const char*  hsfxs[] = { " a.m.", " p.m.", "" };
	static const char*  hsfxs[] = { " o'clock", " o'clock", "" };
//	static const char*  hsfxs[] = { " in the am", " in the pm", "" };
//	static const char*  hsfxs[] = { " in the morn", " after noon", "" };
//	static const char*  hsfxs[] = { " in the morn", " in the after noon", "" };
//	static const char*  hsfxs[] = { " after midnight", " before dawn", " in the morn", " before noon", " after noon", " in the evening", " before dark", " after dark" };
//	static const char*  hlbls[] = { "midnight", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "noon" };
	static const char*  hlbls[] = { "midnight", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "noon" };
//	static const char*  qlbls[] = { "just past", "quarter past", "half past", "quarter 'til", "almost", "" };
//	static const char*  qlbls[] = { "just past", "about quarter past", "about half past", "about quarter 'til", "coming up on", "" };
//	static const char*  qlbls[] = { "just past", "appx. quarter past", "appx. half past", "appx. quarter 'til", "coming up on", "" };
//	static const char*  qlbls[] = { "just past", "around quarter past", "around half past", "around quarter 'til", "coming up on", "" };
	static const char*  tlbls[] = { "just past", "five past", "ten past", "quarter past", "twenty past", "twenty-five past", "half past", "twenty-five 'til", "twenty 'til", "quarter 'til", "ten 'til", "five 'til", "almost", "" };
//	static const char*  fixes[] = { "past", "'til" };
	static int          hb4     =   -1; // force setting on 1st use
//	static int          qb4     =   -1; // ditto
	static int          tb4     =   -1; // ditto
	static int          mb4     =   -1; // ditto

	DEBUGLOGB;

	bool ret = false;

//	if( gCfg.showDate )
	if( gCfg.showDate || (gCfg.showTTips && gRun.isMousing) )
	{
		int hor  = gRun.timeCtm.tm_hour;
		int min  = gRun.timeCtm.tm_min;
//		int mb4  = gRun.timeBtm.tm_min;
		int sfx  = hor / 12, hdx = hor % 12, /*qtr = 5,*/ t12 = 13; // default hour quarter/twelve to 'exact'

		DEBUGLOGP("hor is %d, min is %d, sfx is %d, hdx is %d\n", hor, min, sfx, hdx);

		// NOTE: newHour forcing does not apply in the following
		//       if it is reapplied then a new test for the value of t12 needs
		//       to be added to get it back to its 13 default whenever min == 0
		//       hdx might also need further 'massaging' in that case as well

		if( min != 0 )
//		if( newHour || min != 0 )
		{
#if 0
			for( size_t bnd = 0; bnd < vectsz(qlbls)-1; bnd++ )
			{
				if( (double)min >= qbndv*bnd-qbhlf && (double)min <= qbndv*bnd+qbhlf )
				{
					qtr  = bnd;
					hdx += qtr >=  3 && qtr !=  5 ? 1 : 0; // show next hour for any 'til' times thru 'almost'
					hdx  = hdx == 12 ?  0 : hdx;           // handle index wrap around from eleven to midnight/noon
					break;
				}
			}
#endif
			for( size_t bnd = 0; bnd < vectsz(tlbls)-1; bnd++ )
			{
				if( (double)min >= tbndv*bnd-tbhlf && (double)min <= tbndv*bnd+tbhlf )
				{
					t12  = bnd;
					hdx += t12 >=  7 && t12 != 13 ? 1 : 0; // show next hour for any 'til' times thru 'almost'
					hdx  = hdx == 12 ? 0 : hdx;            // handle index wrap around from eleven to midnight/noon
					break;
				}
			}
		}

		DEBUGLOGP("bef: hdx is %d, hor is %d\n", hdx, hor);
//		hdx = hdx == 0 && hor <= 12 ? 12 : hdx;
		hdx = hdx == 0 && hor > 0 && hor < 13 ? 12 : hdx;
		DEBUGLOGP("aft: hdx is %d, hor is %d\n", hdx, hor);

//		if( hdx != hb4 || qtr != qb4 )
//		if( hdx != hb4 || t12 != tb4 )
		if( newHour || hdx != hb4 || t12 != tb4 )
		{
			DEBUGLOGS("chging face fuzzy time(1&2) related lines");

			sfx = hdx == 0 || hdx == 12 ? 2 : sfx; // no suffix label for midnight and noon

//			if( hdx != hb4 || t12 != tb4 )
//				ret  = true;

			strvcpy(gRun.cfaceTxta1, hlbls[hdx]); hb4 = hdx;
			strvcat(gRun.cfaceTxta1, hsfxs[sfx]);

//			strvcpy(gRun.cfaceTxta2, qlbls[qtr]); qb4 = qtr;
			strvcpy(gRun.cfaceTxta2, tlbls[t12]); tb4 = t12;
//			strvcpy(gRun.cfaceTxta2, tlbls[0]);   tb4 = t12;

			ret = true;
		}

		if( newHour || min != mb4 )
		{
			static const char* flb4    =   NULL;
			static const char* flbls[] = { "", "just past", "just past", "almost", "almost" };
			const        char* flbl    =   min > 2 && min < 58 ? flbls[min % 5] : "";

			if( flbl != flb4 )
			{
				DEBUGLOGS("chging face fuzzy time(3) related line");
				strvcpy(gRun.cfaceTxta3, flbl); mb4 = min;
				ret = true;
			}
		}

		const char* fmtDate = gCfg.show24Hrs ? "%a %d %b" : "%a %b %d";

		if( newHour )
		{
			DEBUGLOGS("chging face date related lines");
			strfmtdt(gRun.cfaceTxtb1, vectsz(gRun.cfaceTxtb1), fmtDate, &gRun.timeCtm); // TODO: change fmt to something coming from gCfg
			strfmtdt(gRun.cfaceTxtb2, vectsz(gRun.cfaceTxtb2), "%Y",    &gRun.timeCtm); // ditto

			if( *gCfg.tzShoName )
			strvcpy (gRun.cfaceTxtb3, gCfg.tzShoName);
			else
			strfmtdt(gRun.cfaceTxtb3, vectsz(gRun.cfaceTxtb3), "%Z",    &gRun.timeCtm); // ditto
		}

		DEBUGLOGP("txt1: %s\n", gRun.cfaceTxta3);
		DEBUGLOGP("txt2: %s\n", gRun.cfaceTxta2);
		DEBUGLOGP("txt3: %s\n", gRun.cfaceTxta1);
		DEBUGLOGP("txt4: %s\n", gRun.cfaceTxtb1);
		DEBUGLOGP("txt5: %s\n", gRun.cfaceTxtb2);
		DEBUGLOGP("txt6: %s\n", gRun.cfaceTxtb3);
	}

	// TODO: always update these since they show up in the title bar, dock, tooltip?, ...
	DEBUGLOGS("chging title date/time related lines");
	strfmtdt(gRun.ttlDateTxt, vectsz(gRun.ttlDateTxt), gCfg.show24Hrs ? gCfg.fmtDate24 : gCfg.fmtDate12, &gRun.timeCtm);
	strfmtdt(gRun.ttlTimeTxt, vectsz(gRun.ttlTimeTxt), gCfg.show24Hrs ? gCfg.fmtTime24 : gCfg.fmtTime12, &gRun.timeCtm);

	DEBUGLOGP("date: %s\n", gRun.ttlDateTxt);
	DEBUGLOGP("time: %s\n", gRun.ttlTimeTxt);
	DEBUGLOGP("ret:  %s\n", ret ? "true" : "false");

	DEBUGLOGE;
	return ret;
}

#ifdef _USEMTHREADS
// -----------------------------------------------------------------------------
struct UpdWndDim
{
	PWidget* pWindow;
	int      width;
	int      height;
	int      clockW;
	int      clockH;
	bool     updgui;
};

// -----------------------------------------------------------------------------
static gpointer update_wnd_dim_func(gpointer data)
{
	if( gRun.appStop )
		return 0;

	DEBUGLOGB;

	UpdWndDim* pUWD    = (UpdWndDim*)data;
	PWidget*   pWidget =  pUWD->pWindow;
	int        width   =  pUWD->width;
	int        height  =  pUWD->height;
	int        clockW  =  pUWD->clockW;
	int        clockH  =  pUWD->clockH;
	double     scaleX  = (double)width /(double)clockW;
	double     scaleY  = (double)height/(double)clockH;
#if _USEGTK
	GdkWindow* pWindow =  gtk_widget_get_window(pWidget);
#else
	void*      pWindow =  NULL;
#endif
	bool       yield   =  true;

#if _USEGTK
	prefs::open(true); // to prevent any changes made here from doing any settings processing
#endif

	if( gRun.drawScaleX != scaleX || gRun.drawScaleY != scaleY )
	{
		DEBUGLOGS("bef draw scale chg/render setting");
		DEBUGLOGP("  old is (%2.2f, %2.2f), new is (%2.2f, %2.2f)\n", gRun.drawScaleX, gRun.drawScaleY, scaleX, scaleY);
		g_yield_thread(yield);
		gRun.drawScaleX = scaleX;
		gRun.drawScaleY = scaleY;
		draw::render_set(NULL, false, false, clockW, clockH);
		DEBUGLOGS("aft draw scale chg/render setting");
	}

	DEBUGLOGP(" req window dims of (%d, %d)\n",       width,           height);
//	DEBUGLOGP(" cfg clock  dims of (%d, %d)\n",       gCfg.clockW,     gCfg.clockH);
	DEBUGLOGP(" cfg clock  dims of (%d, %d)\n",       clockW,          clockH);
//	DEBUGLOGP(" req drawScales  of (%2.2f, %2.2f)\n", gRun.drawScaleX, gRun.drawScaleY);
	DEBUGLOGP(" req drawScales  of (%2.2f, %2.2f)\n", scaleX,          scaleY);

	// resize using 'stretched' clock surfs after main thread pausing

	gint oldW, oldH;
//	gint oldW, oldH, newX, newY, newW, newH;
//	gint oldW, oldH, newX, newY, newW, newH, newD;

#if _USEGTK
	g_yield_thread(yield);
	g_sync_threads_gui_beg();
	gtk_window_get_size(GTK_WINDOW(pWidget), &oldW, &oldH);
	DEBUGLOGP(" old window dims of (%d, %d)\n", oldW, oldH);

	if( width != oldW || height != oldH )
	{
		DEBUGLOGS("bef window resizing");
		gtk_window_resize(GTK_WINDOW(pWidget), width, height);
		DEBUGLOGS("aft window resizing");
#if 0
		gRun.drawScaleX = scaleX;
		gRun.drawScaleY = scaleY;
		draw::render_set(NULL, false, false, clockW, clockH);
#endif
	}
#endif

	if( gRun.appStop )
		return 0;

#if _USEGTK
	DEBUGLOGS("bef 2nd gdk updates processing");
	gtk_widget_queue_draw(pWidget);
	gdk_window_process_updates(pWindow, TRUE);
	DEBUGLOGS("aft 2nd gdk updates processing");
#endif
	g_sync_threads_gui_end();

	// TODO: this doesn't work - nothing's updated yet - way to change?
	//       does queue draw request fix things for this? seems to fix other issues
#if 0
	g_yield_thread(yield);
	g_sync_threads_gui_beg();
	gtk_window_get_position(GTK_WINDOW(pWidget), &newX, &newY);
	gtk_window_get_size    (GTK_WINDOW(pWidget), &newW, &newH);
//	gdk_window_get_geometry(pWindow, &newX, &newY, &newW, &newH, &newD);
	DEBUGLOGP(" new window locs of (%d, %d)\n", newX, newY);
	DEBUGLOGP(" new window dims of (%d, %d)\n", newW, newH);
	g_sync_threads_gui_end();
#endif

	if( gRun.appStop )
		return 0;

	g_expri_thread(+10);
	g_yield_thread(yield);
	gRun.updateSurfs = true;
	DEBUGLOGS("bef surfaces updating");
	update_shapes(pWidget, width, height, true, false, false); // surfs only
	DEBUGLOGS("aft surfaces updating");
	g_yield_thread(yield);
	g_expri_thread(-10);

	if( gRun.appStop )
		return 0;

	draw::lock(true);
	DEBUGLOGS("bef surfaces swapping");

//	gCfg.clockX     = newX;
//	gCfg.clockY     = newY;
//	gCfg.clockW     = newW;
//	gCfg.clockH     = newH;
	gCfg.clockW     = width;
	gCfg.clockH     = height;
	gRun.drawScaleX = gRun.drawScaleY = 1;
//	draw::render_set(pWidget, true, false);

	draw::update_surfs_swap();
	draw::render_set(pWidget, true, false);

	DEBUGLOGS("aft surfaces swapping");
	draw::lock(false);

	if( gRun.appStop )
		return 0;

	g_sync_threads_gui_beg();
#if _USEGTK
	DEBUGLOGS("bef 3rd gdk updates processing");
	gtk_widget_queue_draw(pWidget);
	gdk_window_process_updates(pWindow, TRUE); // configure event shows up way down here (for me on xfce)
	DEBUGLOGS("aft 3rd gdk updates processing");
#endif
	g_sync_threads_gui_end();

	if( gRun.appStop )
		return 0;

	g_expri_thread(0);
	g_yield_thread(yield);
	DEBUGLOGS("bef shape masks updating");
	update_shapes(pWidget, width, height, false, true, true);  // masks only
	DEBUGLOGS("aft shape masks updating");
	g_yield_thread(yield);

	if( gRun.appStop )
		return 0;
#if _USEGTK
	g_sync_threads_gui_beg();
#if _USEGTK
	DEBUGLOGS("bef 4th gdk updates processing");
	gtk_widget_queue_draw(pWidget);
	gdk_window_process_updates(pWindow, TRUE);
	DEBUGLOGS("aft 4th gdk updates processing");
#endif
	g_sync_threads_gui_end();
#endif

#if _USEGTK
	if( !gRun.scrsaver )
	{
		g_yield_thread(yield);
		g_sync_threads_gui_beg();
		DEBUGLOGS("bef cursor resetting");
		change_cursor(pWidget, GDK_FLEUR);
		DEBUGLOGS("aft cursor resetting");
		g_sync_threads_gui_end();
	}
#endif

	if( gRun.appStop )
		return 0;

#if _USEGTK
	if( pUWD->updgui )
	{
		g_sync_threads_gui_beg();
		DEBUGLOGS("bef settings value updates");

		Config    tcfg = gCfg;
		cfg::cnvp(tcfg,  true); // make 0 corner vals relative to the cfg corner
//		tcfg.clockX  =   newX;
//		tcfg.clockY  =   newY;
		cfg::cnvp(tcfg,  true); // make cfg corner vals relative to the 0 corner

		PWidget*    pWidget;
		const char* lbls[] = { prefs::PREFSTR_X, prefs::PREFSTR_Y, prefs::PREFSTR_WIDTH, prefs::PREFSTR_HEIGHT };
		int         vals[] = { tcfg.clockX,      tcfg.clockY,      tcfg.clockW,          tcfg.clockH };

		for( size_t w = 0; w < vectsz(lbls); w++ )
		{
			if( pWidget = glade::pWidget(lbls[w]) )
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), vals[w]);
		}

//		if( pWidget = glade::pWidget(prefs::PREFSTR_X) )
//			gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockX);
//		if( pWidget = glade::pWidget(prefs::PREFSTR_Y) )
//			gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockY);
//		if( pWidget = glade::pWidget(prefs::PREFSTR_WIDTH) )
//			gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockW);
//		if( pWidget = glade::pWidget(prefs::PREFSTR_HEIGHT) )
//			gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockH);

		DEBUGLOGS("aft settings value updates");
		g_sync_threads_gui_end();
	}
#endif

#if _USEGTK
	prefs::open(false);
#endif

	gRun.updating = false;

	DEBUGLOGE;
	return 0;
}
#endif // _USEMTHREADS

// -----------------------------------------------------------------------------
void update_wnd_dim(PWidget* pWindow, int width, int height, int clockW, int clockH, bool async, bool updgui)
{
	if( gRun.appStop )
		return;

	DEBUGLOGB;
	DEBUGLOGP(" new dims (%d, %d), old dims (%d, %d)\n", width, height, clockW, clockH);

	gRun.updating = true;

#if _USEGTK
	if( !gRun.scrsaver )
		change_cursor(pWindow, GDK_WATCH);
#endif

#ifdef _USEMTHREADS
	if( async )
	{
		static UpdWndDim uwd;

		uwd.pWindow = pWindow;
		uwd.width   = width;
		uwd.height  = height;
		uwd.clockW  = clockW;
		uwd.clockH  = clockH;
		uwd.updgui  = updgui;

		GThread* pThread = g_thread_try_new(__func__, update_wnd_dim_func, &uwd, NULL);

		if( pThread )
		{
			DEBUGLOGS("updating window dimensions asynchronously");
			g_thread_unref(pThread);
			DEBUGLOGR(1);
			return;
		}
	}
#endif // _USEMTHREADS

	DEBUGLOGS("updating window dimensions synchronously");

	// TODO: combine most of this & above thread func into one func

//	gRun.drawScaleX  = (double)width /(double)gCfg.clockW;
//	gRun.drawScaleY  = (double)height/(double)gCfg.clockH;
	gRun.drawScaleX  = (double)width /(double)clockW;
	gRun.drawScaleY  = (double)height/(double)clockH;
	draw::render_set();

	if( width != clockW || height != clockH )
	{
#if _USEGTK
		DEBUGLOGS("bef window resizing");
		gtk_window_resize(GTK_WINDOW(pWindow), width, height);
		gdk_window_process_updates(gtk_widget_get_window(pWindow), TRUE);
		DEBUGLOGS("aft window resizing");
#endif
	}

	gRun.drawScaleX  = gRun.drawScaleY = 1;
	gRun.updateSurfs = true;
	gCfg.clockW      = width;
	gCfg.clockH      = height;
	draw::render_set();

	DEBUGLOGS("bef surfaces and shape masks updating");
	update_shapes(pWindow, width, height, true, true, false);
	draw::update_surfs_swap();
	DEBUGLOGS("aft surfaces and shape masks updating");

#if _USEGTK
	gtk_widget_queue_draw(pWindow);
	gdk_window_process_updates(gtk_widget_get_window(pWindow), TRUE);

	if( !gRun.scrsaver )
		change_cursor(pWindow, GDK_FLEUR);
#endif

	gRun.updating = false;

	// TODO: need to update open gui settings if requested to do so (add parm)

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void update_wnd_pos(PWidget* pWindow, int x, int y)
{
	if( gRun.appStop )
		return;

	DEBUGLOGB;

	gCfg.clockX = x;
	gCfg.clockY = y;

#if _USEGTK
	gtk_window_move(GTK_WINDOW(pWindow), x, y);
#endif

	DEBUGLOGE;
}

#if _USEGTK
#if 0
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
GdkPixbuf* get_screenshot()
{
	gint       wx,  wy, ww, wh;
	GdkWindow* rw = gdk_get_default_root_window();

	gdk_window_get_origin(rw, &wx, &wy);
	gdk_drawable_get_size(rw, &ww, &wh);      

	return gdk_pixbuf_get_from_drawable(NULL, rw, NULL, wx, wy, 0, 0, ww, wh);
}
#endif
#if 0
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <cairo.h>
#include "platform.h"

int main (int argc, char* argv[])
{
	gdk_init(&argc, &argv);

	GdkWindow*       root_win = gdk_get_default_root_window();
	gint             width    = gdk_window_get_width (root_win);
	gint             height   = gdk_window_get_height(root_win);
	cairo_surface_t* surface  = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	GdkPixbuf*       pb       = gdk_pixbuf_get_from_window(root_win, 0, 0, width, height);
	cairo_t*         cr       = cairo_create(surface);        

	gdk_cairo_set_source_pixbuf(cr, pb, 0, 0);  
	cairo_paint(cr);  

	cairo_surface_write_to_png(surface, "image.png");

	cairo_destroy(cr);

//	gdk_pixbuf_unref(pb);
	g_object_unref(pb);

	cairo_surface_destroy(surface);

	return 0;
}
#endif
#if 0
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <cairo.h>
#include "platform.h"
#include <pango/pango.h>

static void do_drawing(cairo_t* cr);

static gboolean on_draw_event(PWidget* widget, cairo_t* cr, gpointer user_data)
{      
	do_drawing(cr);  
	return FALSE;
}

static void do_drawing(cairo_t* cr)
{
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
}

static void setup(PWidget* win)
{        
	gtk_widget_set_app_paintable(win, TRUE);
	gtk_window_set_type_hint(GTK_WINDOW(win), GDK_WINDOW_TYPE_HINT_DOCK);
	gtk_window_set_keep_below(GTK_WINDOW(win), TRUE);

	GdkScreen* screen = gdk_screen_get_default();
	GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
  
	if( visual != NULL && gdk_screen_is_composited(screen) )
	{
		gtk_widget_set_visual(win, visual);
	}
}

int main(int argc, char* argv[])
{
	gtk_init(&argc, &argv);

	GdkColor              color;
	PWidget*              window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	PWidget*              lbl    = gtk_label_new("ZetCode, tutorials for programmers");
	PangoFontDescription* fd     = pango_font_description_from_string("Serif 20");

	setup(window);
	gtk_widget_modify_font(lbl, fd);  
	gtk_container_add(GTK_CONTAINER(window), lbl);  

	gdk_color_parse("white", &color);
	gtk_widget_modify_fg(lbl, GTK_STATE_NORMAL, &color);

	g_signal_connect(G_OBJECT(window), "draw",    G_CALLBACK(on_draw_event), NULL); 
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 350, 250); 
	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
#endif
#if 0
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
//	struct GdkEventScroll       { GdkEventType type; GdkWindow *window; gint8 send_event; guint32 time; gdouble x; gdouble y; guint state; GdkScrollDirection direction; GdkDevice *device; gdouble x_root, y_root; };
	static GdkEventScroll seu = { GDK_SCROLL,        0,                 TRUE,             0,            0,         0,         0,           GDK_SCROLL_UP,                0,                 0,              0 };
	static GdkEventScroll sed = { GDK_SCROLL,        0,                 TRUE,             0,            0,         0,         0,           GDK_SCROLL_DOWN,              0,                 0,              0 };
	static gboolean       ret =   FALSE;

	seu.window = sed.window = gtk_widget_get_window(pWindow);
	g_signal_emit_by_name(pWindow, "scroll-event", (GdkEvent*)&seu, &ret);
	g_signal_emit_by_name(pWindow, "scroll-event", (GdkEvent*)&sed, &ret);
#endif

#endif // _USEGTK

