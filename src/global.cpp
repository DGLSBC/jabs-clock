/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "glbl"

#include "global.h"
#include "cfgdef.h"
#include "debug.h"
#include "utility.h"
#include <glib/gstdio.h> // for g_unlink
#include "settings.h"
#include "draw.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static void         docks_send_update();
static const gchar* get_icon_filename();

// -----------------------------------------------------------------------------
static time_t g_timeCur   = 0;
static time_t g_timeBef   = 0;
static gint   g_timeDif   = 0;
static bool   g_icoBlding = false;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void change_ani_rate(GtkWidget* pWindow, int refreshRate, bool force, bool setgui)
{
	refreshRate = refreshRate < 1                ? 1                :  refreshRate;
	refreshRate = refreshRate > MAX_REFRESH_RATE ? MAX_REFRESH_RATE :  refreshRate;

	bool nuRate = gCfg.refreshRate != refreshRate;

	if( nuRate || force )
	{
		// timer runs 10 times faster than the frame rate so we can pick better
		// points in the animation curve, i.e., fewer points are picked in the
		// flat part(s) of the curve and more points are picked in the curvy
		// part(s) of the curve, and it is at these points that the frames
		// are drawn

		gRun.refreshFrame = 0;
		gCfg.refreshRate  = refreshRate;

//		refreshRate      *= 10;
		refreshRate       = 1000/refreshRate;
		refreshRate       = refreshRate == 0 ? 1 : refreshRate;

//		if( nuRate || !gRun.drawTimerId )
		{
/*			if( gRun.drawTimerId )
			{
				GSource* pS = g_main_context_find_source_by_id(NULL, gRun.drawTimerId);
				gint64   rt = g_source_get_ready_time(pS);
				DEBUGLOGP("ready time is %u\n", (unsigned int)rt);
			}*/

			if( gRun.drawTimerId )
				g_source_remove(gRun.drawTimerId);

//			DEBUGLOGP("changing refreshRate to %d\n", gCfg.refreshRate);

//			int refreshRate  = gCfg.showSeconds ? 1000/gCfg.refreshRate : 1000/2;
			int refreshRate  = gCfg.showSeconds && !gRun.textonly ? 1000/gCfg.refreshRate : 1000/2;
			gRun.drawTimerId = g_timeout_add_full(drawPrio, refreshRate, (GSourceFunc)draw_time_handler, (gpointer)pWindow, NULL);

//			DEBUGLOGP("render timer set to %d\n", refreshRate);
		}

		if( setgui )
			prefs::set_ani_rate(gCfg.refreshRate);
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#ifdef _USEMTHREADS
struct ChangeTheme
{
	GtkWidget* pWindow;
	bool       updateSurfs;
};

// -----------------------------------------------------------------------------
static void change_theme_end(gpointer data, const char* path, const char* file, bool valid)
{
	DEBUGLOGB;

	ChangeTheme* pCT = (ChangeTheme*)data;

	gdk_threads_enter();
	GtkWidget* pWindow = pCT->pWindow;
	bool       gotWind = pWindow && gtk_widget_get_has_window(pWindow);
	gdk_threads_leave();

	if( gotWind )
	{
		DEBUGLOGS("queueing redraw");

		gdk_threads_enter();
		gtk_widget_queue_draw(pWindow);
		gdk_threads_leave();

		if( pCT->updateSurfs ) // TODO: when would this ever NOT be set when theme changing?
		{
			DEBUGLOGS("updating surfaces");

			gRun.updateSurfs = true;
			update_input_shape(pWindow, gCfg.clockW, gCfg.clockH, true, false, false); // surfs only

			// swap the new surfs for the old after pausing the main thread

			gdk_threads_enter();
			draw::update_surfs_swap(gCfg.clockW, gCfg.clockH);
			gdk_threads_leave();

			update_input_shape(pWindow, gCfg.clockW, gCfg.clockH, false, true, true);  // masks only

			gdk_threads_enter();
			gtk_widget_queue_draw(pWindow);
			gdk_window_process_updates(pWindow->window, TRUE);
			gdk_threads_leave();

//			cfg::save(); // TODO: needed here?
		}
	}

	if( valid )
	{
		DEBUGLOGS("valid theme\n");
		DEBUGLOGP("  path is %s\n", path);
		DEBUGLOGP("  file is %s\n", file);

		strvcpy(gCfg.themePath, path);
		strvcpy(gCfg.themeFile, file);

		if( !gRun.appStart )
		{
			gdk_threads_enter();
			make_theme_icon(pWindow);
			gdk_threads_leave();

//			gRun.renderUp = true; // TODO: needed here?
		}

		cfg::save();
	}

	DEBUGLOGE;
}
#endif // _USEMTHREADS

// -----------------------------------------------------------------------------
void change_theme(ThemeEntry* pEntry, GtkWidget* pWindow)
{
	DEBUGLOGB;

	if( !pEntry )
	{
		DEBUGLOGS("return(1)");
		return;
	}

#ifdef _USEMTHREADS
	if( pWindow )
	{
		static ChangeTheme ct;

		ct.pWindow     = pWindow;
		ct.updateSurfs = gRun.updateSurfs;

		bool valid = draw::update_theme(pEntry->pPath->str, pEntry->pFile->str, change_theme_end, &ct);

		DEBUGLOGP("theme updating start is %s\n", valid ? "'valid'" : "not 'valid'");
		DEBUGLOGS("exit(1)");

		return;
	}
#endif // _USEMTHREADS

	bool valid = draw::update_theme(pEntry->pPath->str, pEntry->pFile->str, NULL, NULL);

	if( pWindow && gtk_widget_get_has_window(pWindow) )
	{
		DEBUGLOGS("queueing redraw");

		gtk_widget_queue_draw(pWindow);

		if( gRun.updateSurfs ) // TODO: when would this ever NOT be set when theme changing?
		{
			DEBUGLOGS("updating surfaces");
			gdk_window_process_updates(pWindow->window, TRUE);
		}
	}

	if( valid )
	{
		DEBUGLOGS("valid theme\n");
		strvcpy(gCfg.themePath, pEntry->pPath->str);
		strvcpy(gCfg.themeFile, pEntry->pFile->str);

		if( !gRun.appStart )
			make_theme_icon(pWindow);
	}

	DEBUGLOGP("theme is %s\n", valid ? "'valid'" : "not 'valid' (no drawable elements)");
	DEBUGLOGS("exit(2)");
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void docks_send_update()
{
	DEBUGLOGB;

	static const char* dbus   =  "org.cairodock.CairoDock";
	static const char* objpth = "/org/cairodock/CairoDock";
	static const char* iface  =  dbus;

	GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL, dbus, objpth, iface, NULL, NULL);
	if(        !proxy ) return;

	char     tstr[64];
/*	const char* tfmt = "%I:%M"; // TODO: yet another cfg format string to implement */
	const char* tfmt = "%X";
	strftime(tstr, vectsz(tstr), tfmt, &gRun.timeCtm);
	int      toff = *tstr == '0' ? 1 : 0;

	if( strlen(tstr) > 5 )
		tstr[5] = '\0';

	// TODO: figure out if all dbus target servers can be determined at runtime
	//       and have similar support for dbus-spec'd on-icon labeling strings
	//       (cairo-dock (done), awn?, docky?, plank?, others?)

	const char* vafmt = "(ss)";
	const char* clasn = "class=" APP_NAME;
	GVariant*   parms = proxy ? g_variant_new(vafmt, tstr+toff, clasn) : NULL;
	GVariant*   rcode = parms ? g_dbus_proxy_call_sync(proxy, "SetQuickInfo", parms, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL) : NULL;
	if(         rcode ) g_variant_unref(rcode);

	if( !g_icoBlding && get_theme_icon_filename() )
	{
		GVariant* parms = proxy ? g_variant_new(vafmt, get_theme_icon_filename(false), clasn) : NULL;
		GVariant* rcode = parms ? g_dbus_proxy_call_sync(proxy, "SetIcon", parms, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL) : NULL;
		if(       rcode ) g_variant_unref(rcode);
	}

	g_object_unref(proxy);
	proxy = NULL;

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
gboolean draw_time_handler(GtkWidget* pWidget)
{
	if( gRun.appStop )
		return FALSE;

	DEBUGLOGB;

	bool forceDraw = gRun.appStart || gRun.renderUp;

	if( forceDraw )
	{
		if( gRun.renderUp || ((gRun.drawScaleX = gRun.drawScaleY += 0.01) >= 1.00) )
		{
			gRun.drawScaleX = gRun.drawScaleY  = 1;
			gRun.appStart   = false;
			gRun.renderUp   = false;

			change_ani_rate(pWidget, gCfg.refreshRate, true, true);
			make_theme_icon(pWidget);
		}

		gdk_window_set_opacity(gRun.pMainWindow->window, gCfg.opacity*gRun.drawScaleX);
	}

//	if( forceDraw || ++gRun.refreshFrame >= 10 )
	{
		static int ct = 0;
//		if( ct < 10 ) DEBUGLOGP("rendering(%d)\n", ++ct);
		DEBUGLOGP("rendering(%d)\n", ++ct);

//		draw::render(pWidget, gRun.drawScaleX, gRun.drawScaleY, gRun.renderIt, gRun.appStart);

		gtk_widget_queue_draw(pWidget);
		gdk_window_process_updates(pWidget->window, TRUE);

//		GdkRegion* pRegion  = gdk_window_get_update_area(pWidget->window);
//		if(        pRegion )  gdk_region_destroy(pRegion);

		gRun.refreshFrame   = 0;
	}

//	draw::render(pWidget, forceDraw);

	DEBUGLOGE;

	return TRUE;
}

// -----------------------------------------------------------------------------
const char* get_icon_filename()
{
	static const char* aico = DATA_DIR "/pixmaps/cairo-clock.png";

	const char* ret = NULL;

	if( g_isa_file(aico) )
	{
		ret = aico;
	}
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

			gtk_icon_info_free(pII);
		}
	}

	DEBUGLOGP("ret=*%s*\n", ret);

	return ret;
}

// -----------------------------------------------------------------------------
const gchar* get_theme_icon_filename(bool check, bool update)
{
	static gchar  path[PATH_MAX];
	static gchar  anmp[PATH_MAX];
	static int    pid   = 0;
	static bool   first = true;
	const  gchar* retp  = path;

	if( first )
	{
		first            = false;
		pid              = getpid();
		const char* tdir = get_user_appnm_path();

		sprintf(path, "%s/theme%d.png", tdir ? tdir : ".", pid);
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

// -----------------------------------------------------------------------------
gboolean info_time_handler(GtkWidget* pWidget)
{
	if( gRun.appStop )
		return FALSE;

	DEBUGLOGB;

#ifdef _TESTABORTED
	static int cnt = 2*5; // try and cause abort after 5 secs (since called every 1/2 sec)

	if( --cnt == 0 )
	{
		DEBUGLOGS("bef invalid change_theme call");
/*		ThemeEntry    te = { NULL, NULL };
		change_theme(&te, pWidget);*/
/*		abort();*/
		DEBUGLOGS("aft invalid change_theme call");
	}
#endif

	update_ts_info();

	DEBUGLOGE;
	return TRUE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static gpointer make_theme_icon_func(gpointer data);

// -----------------------------------------------------------------------------
void make_theme_icon(GtkWidget* pWindow)
{
	DEBUGLOGB;

	if( g_icoBlding == false && !gRun.scrsaver )
	{
		g_icoBlding =  true;
		DEBUGLOGP("creating theme ico image file for theme %s\n", gCfg.themeFile);

#ifdef _USEMTHREADS
/*		GThread* pThread = g_thread_try_new(__func__, make_theme_icon_func, (gpointer)pWindow, NULL);

		if( pThread )
			g_thread_unref(pThread)
		else*/
			make_theme_icon_func(pWindow);
#else
		make_theme_icon_func(pWindow);
#endif
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
gpointer make_theme_icon_func(gpointer data)
{
	DEBUGLOGB;

	if( false ) // TODO: change to setting for not making theme-based icons
	{
		g_icoBlding = false; // put here so can send ico update requests to docks

		docks_send_update();
		return 0;
	}

	const gchar* iconPath = get_theme_icon_filename(false, true);

	if( iconPath )
	{
		draw::make_icon(iconPath);

		g_icoBlding = false; // put here so can send ico update requests to docks

		set_window_icon((GtkWidget*)data);
		docks_send_update();

		DEBUGLOGS("exit(2)");
	}
	else
	{
		DEBUGLOGP("exit(1) - can't create the theme ico image file:\n*%s*\n", iconPath);
	}

	return 0;
}

// -----------------------------------------------------------------------------
void set_window_icon(GtkWidget* pWindow)
{
	if( pWindow && !g_icoBlding && get_theme_icon_filename() )
		gtk_window_set_icon_from_file(GTK_WINDOW(pWindow), get_theme_icon_filename(false), NULL);
}

// -----------------------------------------------------------------------------
void update_colormap(GtkWidget* pWidget)
{
	DEBUGLOGB;

	GdkScreen*   pScreen   = gtk_widget_get_screen(pWidget);
	GdkColormap* pColormap = gdk_screen_get_rgba_colormap(pScreen);
      
	if( pColormap == NULL )
		pColormap =  gdk_screen_get_rgb_colormap(pScreen);

	gtk_widget_set_colormap(pWidget, pColormap);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void update_input_shape(GtkWidget* pWindow, int width, int height, bool dosurfs, bool domask, bool lock)
{
	DEBUGLOGB;

	if( dosurfs )
	{
		DEBUGLOGS("updating surfs");
		draw::update_surfs(pWindow, width, height);
	}

	if( domask )
	{
		if( lock )
			gdk_threads_enter();

		GdkBitmap* pShapeMask1 = NULL;
		GdkBitmap* pShapeMask2 = NULL;
		bool       shapeWidget = !gtk_widget_is_composited(pWindow);

		if( lock )
			gdk_threads_leave();

		DEBUGLOGS("updating shape masks");

		if( gRun.clickthru && gCfg.showInTasks ) // TODO: replace with cfg opt to turn on click-thru?
		{
			pShapeMask1 =               draw::make_mask(width, height, false);
			pShapeMask2 = shapeWidget ? draw::make_mask(width, height, false) : NULL;
		}
		else
		if( gRun.scrsaver == false )
		{
			pShapeMask1 =               draw::make_mask(width, height, true);
			pShapeMask2 = shapeWidget ? draw::make_mask(width, height, true)  : NULL;
		}

		if( lock )
			gdk_threads_enter();

		gtk_widget_input_shape_combine_mask(pWindow, NULL, 0, 0);
		gtk_widget_shape_combine_mask      (pWindow, NULL, 0, 0);

		if( pShapeMask1 )
		{
			gtk_widget_input_shape_combine_mask(pWindow, pShapeMask1, 0, 0);
			g_object_unref((gpointer)pShapeMask1);
		}

		if( pShapeMask2 )
		{
			gtk_widget_shape_combine_mask(pWindow, pShapeMask2, 0, 0);
			g_object_unref((gpointer)pShapeMask2);
		}

		if( lock )
			gdk_threads_leave();
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
/*#include <canberra-gtk.h>

// -----------------------------------------------------------------------------
void ca_finish_cb(ca_context* c, uint32_t id, int error_code, void* userData)
{
	bool* b = (bool*)userData;
	*b      =  true;
}*/

// -----------------------------------------------------------------------------
void update_ts_info(bool forceTTip, bool forceDate, bool forceTime)
{
	if( gRun.appStop )
		return;

	DEBUGLOGB;

	static tm g_timeBtm;

	g_timeBef = g_timeCur;

	if( !forceTTip && !forceDate && !forceTime && (g_timeDif = (g_timeCur = time(NULL))-g_timeBef) == 0 )
	{
		DEBUGLOGE;
		return;
	}

//	double gcfg_hoff =  5.0;
	double gcfg_hoff =  0.0;
	g_timeBtm        =  gRun.timeCtm;
	gRun.timeCtm     = *localtime(&g_timeCur);
	gRun.hours       =  gRun.timeCtm.tm_hour;
	gRun.hours       =  gCfg.show24Hrs ? gRun.hours : gRun.hours % 12;
	gRun.minutes     =  gRun.timeCtm.tm_min;
	gRun.seconds     =  gRun.timeCtm.tm_sec;
//	gRun.angleHour   = (gRun.seconds +  60.0*gRun.minutes + 3600.0*gRun.hours)*360.0;
//	gRun.angleHour  /=  gCfg.show24Hrs ? 86400.0 : 43200.0;
//	gRun.angleHour   = (gRun.minutes +  60.0*gRun.hours)*0.5;
	gRun.angleHour   = (gRun.minutes +  60.0*(gRun.hours+gcfg_hoff))*0.5;
	gRun.angleHour  *=  gCfg.show24Hrs ? 0.5 : 1.0;
	gRun.angleMinute =  gRun.minutes*6.0;
	gRun.angleSecond =  gRun.seconds*6.0;

	DEBUGLOGP("time change: %s hr display, hour %d (%f deg)\n",
		gCfg.show24Hrs ? "24" : "12", gRun.hours, gRun.angleHour);

	bool drawBkgnd = false;
	bool drawDate  = false;

	// NOTE: I don't think this is needed anymore, given the new code below that
	//       updates the background surface on H:M change (except it would also
	//       need to check for a non-bkgrnd date and, if necessary, update the
	//       foreground surface too)

	if( gCfg.showDate )
	{
		if( forceDate ||
		    gRun.timeCtm.tm_year != g_timeBtm.tm_year ||
		    gRun.timeCtm.tm_mon  != g_timeBtm.tm_mon  ||
		    gRun.timeCtm.tm_mday != g_timeBtm.tm_mday )
		{
			gchar    tstr[1024];
			strftime(tstr, vectsz(tstr), gCfg.fmtDate, &gRun.timeCtm);
			strvcpy (gRun.acDate, tstr);

			int      toff = gCfg.fmtTime[1] == 'I' && *gRun.acTime == '0' ? 1 : 0;
			snprintf(tstr, vectsz(tstr), "%s %s", gRun.acDate, gRun.acTime+toff);

			gtk_window_set_title(GTK_WINDOW(gRun.pMainWindow), tstr);
//			draw::update_date_surf();
			drawDate = true;

			DEBUGLOGS("date change so updating title & surf w/date on it");
		}
	}

	if( (true || forceTTip) && gtk_widget_get_tooltip_window(gRun.pMainWindow) ) // TODO: chg to chk cfg tooltip display val
	{
		gchar    tstr[1024];
		strftime(tstr, vectsz(tstr), gCfg.show24Hrs ? gCfg.fmt24Hrs : gCfg.fmt12Hrs, &gRun.timeCtm);

		DEBUGLOGP("ttip=%s\n\ndate string=%s\ntimes (bef, cur, dif): %d, %d, %d\n",
			tstr, gRun.acDate, (int)g_timeBef, (int)g_timeCur, (int)g_timeDif);

		gtk_widget_set_tooltip_text(gRun.pMainWindow, tstr);
		DEBUGLOGS("updating tooltip text");
	}

	if( true ) // TODO: is this for gCfg.showDate testing? if not, what?
	{
		if( forceTime || gRun.timeCtm.tm_min != g_timeBtm.tm_min || gRun.timeCtm.tm_hour != g_timeBtm.tm_hour )
		{
			if( gCfg.showDate ) // TODO: add use of appropriate 'fuzzy time' cfg var
			{
				static const double tbndv   =    5.0;
//				static const double qbndv   =   15.0;
				static const double tbhlf   =   tbndv*0.5;
//				static const double qbhlf   =   qbndv*0.5;
//				static const char*  hsfxs[] = { " am", " pm", "" };
//				static const char*  hsfxs[] = { " a.m.", " p.m.", "" };
				static const char*  hsfxs[] = { " o'clock", " o'clock", "" };
//				static const char*  hsfxs[] = { " in the am", " in the pm", "" };
//				static const char*  hsfxs[] = { " in the morn", " after noon", "" };
//				static const char*  hsfxs[] = { " in the morn", " in the after noon", "" };
//				static const char*  hsfxs[] = { " after midnight", " before dawn", " in the morn", " before noon", " after noon", " in the evening", " before dark", " after dark" };
//				static const char*  hlbls[] = { "midnight", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "noon" };
				static const char*  hlbls[] = { "midnight", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "noon" };
//				static const char*  qlbls[] = { "just past", "quarter past", "half past", "quarter 'til", "almost", "" };
//				static const char*  qlbls[] = { "just past", "about quarter past", "about half past", "about quarter 'til", "coming up on", "" };
//				static const char*  qlbls[] = { "just past", "appx. quarter past", "appx. half past", "appx. quarter 'til", "coming up on", "" };
//				static const char*  qlbls[] = { "just past", "around quarter past", "around half past", "around quarter 'til", "coming up on", "" };
				static const char*  tlbls[] = { "just past", "five past", "ten past", "quarter past", "twenty past", "twenty-five past", "half past", "twenty-five 'til", "twenty 'til", "quarter 'til", "ten 'til", "five 'til", "almost", "" };
//				static const char*  fixes[] = { "past", "'til" };
				static int          hb4     =   -1; // force setting on 1st use
//				static int          qb4     =   -1; // ditto
				static int          tb4     =   -1; // ditto

				int hor  = gRun.timeCtm.tm_hour, min = gRun.timeCtm.tm_min;
				int sfx  = hor / 12, hdx = hor % 12, /*qtr = 5,*/ t12 = 13; // default hour quarter/twelve to 'exact'

				if( min != 0 )
				{
/*					for( size_t bnd = 0; bnd < vectsz(qlbls)-1; bnd++ )
					{
						if( (double)min >= qbndv*bnd-qbhlf && (double)min <= qbndv*bnd+qbhlf )
						{
							qtr  = bnd;
							hdx += qtr >=  3 && qtr !=  5 ? 1 : 0; // show next hour for any 'til' times thru 'almost'
							hdx  = hdx == 12 ?  0 : hdx;           // handle index wrap around from eleven to midnight/noon
							break;
						}
					}*/

					for( size_t bnd = 0; bnd < vectsz(tlbls)-1; bnd++ )
					{
						if( (double)min >= tbndv*bnd-tbhlf && (double)min <= tbndv*bnd+tbhlf )
						{
							t12  = bnd;
							hdx += t12 >=  7 && t12 != 13 ? 1 : 0; // show next hour for any 'til' times thru 'almost'
							hdx  = hdx == 12 ?  0 : hdx;           // handle index wrap around from eleven to midnight/noon
							break;
						}
					}
				}

				hdx = hdx == 0 && hor <= 12 ? 12 : hdx;

//				if( hdx != hb4 || qtr != qb4 )
				if( hdx != hb4 || t12 != tb4 )
				{
					sfx  = hdx == 0   || hdx == 12 ? 2 : sfx; // no suffix label for midnight and noon
					strvcpy(gRun.acTxt1, hlbls[hdx]); hb4 = hdx;
					strvcat(gRun.acTxt1, hsfxs[sfx]);
//					strvcpy(gRun.acTxt2, qlbls[qtr]); qb4 = qtr;
					strvcpy(gRun.acTxt2, tlbls[t12]); tb4 = t12;
				}
			}
			else
			{
//				strftime(gRun.acTxt1, vectsz(gRun.acTxt1), "%A",         &gRun.timeCtm); // TODO: change fmt to something coming from gCfg
//				strftime(gRun.acTxt2, vectsz(gRun.acTxt2), "%Z",         &gRun.timeCtm); // TODO: change fmt to something coming from gCfg
			}

			if( gCfg.showDate )
			{
//				strftime(gRun.acTxt3, vectsz(gRun.acTxt3), "%B %e",      &gRun.timeCtm);
//				strftime(gRun.acTxt3, vectsz(gRun.acTxt3), gCfg.fmtDate, &gRun.timeCtm);
				strftime(gRun.acTxt3, vectsz(gRun.acTxt3), "%a %b %d",   &gRun.timeCtm); // TODO: change fmt to something coming from gCfg
				strftime(gRun.acTxt4, vectsz(gRun.acTxt4), "%Y",         &gRun.timeCtm); // TODO: change fmt to something coming from gCfg
//				strftime(gRun.acDate, vectsz(gRun.acDate), gCfg.fmtDate, &gRun.timeCtm);
//				strftime(gRun.acTime, vectsz(gRun.acTime), gCfg.fmtTime, &gRun.timeCtm);
			}

			strftime(gRun.acDate, vectsz(gRun.acDate), gCfg.fmtDate, &gRun.timeCtm);
			strftime(gRun.acTime, vectsz(gRun.acTime), gCfg.fmtTime, &gRun.timeCtm);

			if( forceTime || gCfg.showDate )
			{
				// TODO: make this more efficient since don't need to always redraw here

//				draw::update_bkgnd();
//				draw::update_date_surf();
				drawDate  = true;
			}

			if( (gRun.optHorHand && !gRun.useHorSurf) || (gRun.optMinHand && !gRun.useMinSurf) )
			{
				drawBkgnd = !drawDate;
			}

			if( drawBkgnd )
				draw::update_bkgnd();

			if( drawDate )
				draw::update_date_surf();

			gchar    tstr[1024];
			int      toff = gCfg.fmtTime[1] == 'I' && *gRun.acTime == '0' ? 1 : 0;
			snprintf(tstr, vectsz(tstr), "%s %s", gRun.acDate, gRun.acTime+toff);

			gtk_window_set_title(GTK_WINDOW(gRun.pMainWindow), tstr);
			make_theme_icon(gRun.pMainWindow);

//			// TODO: make this more efficient since don't need to redraw all
//			gtk_widget_queue_draw(gRun.pMainWindow);

//			docks_send_update();

			DEBUGLOGS("time change so updating bkgnd, title, & icon");
		}
	}

/*	if( gCfg.doSounds )
	{
		static const char* rcpath = "/." APP_NAME "/sounds/westminster_chimes/"; // TODO: put sound theme dir in cfg
		const char*        spath =  get_home_subpath(rcpath, strlen(rcpath));

		if( spath && g_isa_dir(spath) )
		{
			static bool        playIt         =  true;
			static int         playCnt        = -1;
			static int         playMin        =  0;
			static const char* currFile       =  NULL;
			static bool        gCfg_onceChime =  true; // TODO: put in cfg - one chime per hour if true, otherwise will chime H time per hour, where H is the hour #
			static const char* gCfg_qtr0Chime = "hrChime.wav";  // TODO: put all of these in cfg
			static const char* gCfg_qtr1Chime = "qtrChime.wav";
			static const char* gCfg_qtr2Chime = "halfChime.wav";
			static const char* gCfg_qtr3Chime = "qtr3Chime.wav";
			static const char* gCfg_alrmChime = "alarm.wav";

			if( !gRun.appStart && gCfg.doAlarms )
			{
				// TODO: put times in cfg
				static unsigned alrmMask[] = {  0x00ffffff,                0x00ffffff };
				static unsigned alrmTime[] = { (10 << 16) + (0 << 8) + 0, (14 << 16) + (0 << 8) + 0 }; // 10a & 2p for mom's walking

				unsigned currTime = (gRun.timeCtm.tm_hour << 16) + (gRun.timeCtm.tm_min << 8) + gRun.timeCtm.tm_sec;

				for( size_t a = 0; a < vectsz(alrmTime); a++ )
				{
					if( (alrmMask[a] & currTime) == alrmTime[a] )
					{
						static bool  pi = true;
						ca_proplist* pl = NULL;
						ca_proplist_create(&pl);
						ca_proplist_sets(pl, CA_PROP_MEDIA_FILENAME, gCfg_alrmChime);
						ca_context_play_full(ca_gtk_context_get(), 0, pl, ca_finish_cb, &(pi=false));
						ca_proplist_destroy(pl);
						break;
					}
				}
			}

			if( !gRun.appStart && playIt )
			{
//				bool hourChng = gRun.timeCtm.tm_hour != g_timeBtm.tm_hour && gCfg.doChimes;
				bool minuChng = gRun.timeCtm.tm_min  != g_timeBtm.tm_min  && gCfg.doChimes;

				bool qtr0File = minuChng && gRun.timeCtm.tm_min ==  0;
				bool qtr1File = minuChng && gRun.timeCtm.tm_min == 15;
				bool qtr2File = minuChng && gRun.timeCtm.tm_min == 30;
				bool qtr3File = minuChng && gRun.timeCtm.tm_min == 45;

				if( playCnt >= 0 || qtr0File || qtr1File || qtr2File || qtr3File )
				{
					const char* sndName = qtr0File ? gCfg_qtr0Chime : (qtr1File ? gCfg_qtr1Chime : (qtr2File ? gCfg_qtr2Chime : (qtr3File ? gCfg_qtr3Chime : currFile)));

					char    sndFile[PATH_MAX];
					strvcpy(sndFile, spath);
					strvcat(sndFile, sndName);

					if( g_isa_file(sndFile) )
					{
						ca_proplist* pl = NULL;
						ca_proplist_create(&pl);
						ca_proplist_sets(pl, CA_PROP_MEDIA_FILENAME, sndFile);
						ca_context_play_full(ca_gtk_context_get(), 0, pl, ca_finish_cb, &(playIt=false));
						ca_proplist_destroy(pl);

						currFile = sndFile;
					}

					// TODO: do these only need to be done if 'isa file'?

					if(   playCnt <=  0 )
						  playCnt  =  gCfg_onceChime ? 1 : gRun.timeCtm.tm_hour;

					if( --playCnt ==  0 )
						  playCnt  = -1;
				}
			}
		}

		if( spath )
			delete [] spath;
	}*/

	DEBUGLOGE;
}

#ifdef _USEMTHREADS
// -----------------------------------------------------------------------------
struct UpdWndDim
{
	GtkWidget* pWindow;
	int        width;
	int        height;
};

// -----------------------------------------------------------------------------
static gpointer update_wnd_dim_func(gpointer data)
{
	DEBUGLOGB;

	g_make_nicer(10);

	UpdWndDim* pUWD    = (UpdWndDim*)data;
	GtkWidget* pWindow =  pUWD->pWindow;
	int        width   =  pUWD->width;
	int        height  =  pUWD->height;

	gRun.drawScaleX    = (double)width /(double)gCfg.clockW;
	gRun.drawScaleY    = (double)height/(double)gCfg.clockH;

	DEBUGLOGP(" new window dims of (%d, %d)\n",       width,           height);
	DEBUGLOGP(" cfg clock  dims of (%d, %d)\n",       gCfg.clockW,     gCfg.clockH);
	DEBUGLOGP(" new drawScales  of (%4.4f, %4.4f)\n", gRun.drawScaleX, gRun.drawScaleY);

	// resize using 'stretched' clock surfs after main thread pausing

	gdk_threads_enter();
	gtk_window_resize(GTK_WINDOW(pWindow), width, height);
	gdk_window_process_updates(pWindow->window, TRUE);
	gdk_threads_leave();

	gRun.updateSurfs = true;
	update_input_shape(pWindow, width, height, true, false, false); // surfs only
	gRun.drawScaleX  = gRun.drawScaleY = 1;

	// swap new surfs for old after main thread pausing

	gdk_threads_enter();
	draw::update_surfs_swap(width, height);
	gdk_threads_leave();

	update_input_shape(pWindow, width, height, false, true, true);  // masks only

	gCfg.clockW = width;
	gCfg.clockH = height;

	gdk_threads_enter();
	gtk_widget_queue_draw(pWindow);
	gdk_window_process_updates(pWindow->window, TRUE);
	gdk_threads_leave();

	DEBUGLOGE;
	return 0;
}
#endif // _USEMTHREADS

// -----------------------------------------------------------------------------
void update_wnd_dim(GtkWidget* pWindow, int width, int height, bool async)
{
	DEBUGLOGB;
	DEBUGLOGP(" new dims of (%d, %d)\n", width, height);
//	DEBUGLOGS("updating surfs/shapes, resizing, & redrawing window");

#ifdef _USEMTHREADS
	if( async )
	{
		static UpdWndDim uwd;

		uwd.pWindow = pWindow;
		uwd.width   = width;
		uwd.height  = height;

		GThread* pThread = g_thread_try_new(__func__, update_wnd_dim_func, &uwd, NULL);

		if( pThread )
		{
			DEBUGLOGS("updating window dimensions asynchronously");
			g_thread_unref(pThread);
			DEBUGLOGE;
			return;
		}
	}
#endif // _USEMTHREADS

	DEBUGLOGS("updating window dimensions synchronously");

	gRun.drawScaleX  = (double)width /(double)gCfg.clockW;
	gRun.drawScaleY  = (double)height/(double)gCfg.clockH;

	gtk_window_resize(GTK_WINDOW(pWindow), width, height);
	gdk_window_process_updates(pWindow->window, TRUE);

	gRun.drawScaleX  =  gRun.drawScaleY = 1;
	gRun.updateSurfs =  true;
	gCfg.clockW      =  width;
	gCfg.clockH      =  height;

	update_input_shape(pWindow, width, height, true, true, false);
	draw::update_surfs_swap(gCfg.clockW, gCfg.clockH);

	gtk_widget_queue_draw(pWindow);
	gdk_window_process_updates(pWindow->window, TRUE);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void update_wnd_pos(GtkWidget* pWindow, int x, int y)
{
	DEBUGLOGB;

	gCfg.clockX = x;
	gCfg.clockY = y;

	gtk_window_move(GTK_WINDOW(pWindow), x, y);

	DEBUGLOGE;
}
/*
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
GdkPixbuf* get_screenshot()
{
	gint       wx,  wy, ww, wh;
	GdkWindow* rw = gdk_get_default_root_window();

	gdk_window_get_origin(rw, &wx, &wy);
	gdk_drawable_get_size(rw, &ww, &wh);      

	return gdk_pixbuf_get_from_drawable(NULL, rw, NULL, wx, wy, 0, 0, ww, wh);
}*/
/*
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <cairo.h>
#include <gdk/gdk.h>

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
}*/
/*
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <cairo.h>
#include <gtk/gtk.h>
#include <pango/pango.h>

static void do_drawing(cairo_t* cr);

static gboolean on_draw_event(GtkWidget* widget, cairo_t* cr, gpointer user_data)
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

static void setup(GtkWidget* win)
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
	GtkWidget*            window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget*            lbl    = gtk_label_new("ZetCode, tutorials for programmers");
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
}*/
/*
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
//	struct GdkEventScroll       { GdkEventType type; GdkWindow *window; gint8 send_event; guint32 time; gdouble x; gdouble y; guint state; GdkScrollDirection direction; GdkDevice *device; gdouble x_root, y_root; };
	static GdkEventScroll seu = { GDK_SCROLL,        0,                 TRUE,             0,            0,         0,         0,           GDK_SCROLL_UP,                0,                 0,              0 };
	static GdkEventScroll sed = { GDK_SCROLL,        0,                 TRUE,             0,            0,         0,         0,           GDK_SCROLL_DOWN,              0,                 0,              0 };
	static gboolean       ret =   FALSE;

	seu.window = sed.window = pWindow->window;
	g_signal_emit_by_name(pWindow, "scroll-event", (GdkEvent*)&seu, &ret);
	g_signal_emit_by_name(pWindow, "scroll-event", (GdkEvent*)&sed, &ret);*/

