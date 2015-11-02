/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "gui"

#include <math.h>           // for fabs
#include "cfgdef.h"
#include "global.h"         // _USEWKSPACE is in here
#include <stdlib.h>
#include <gdk/gdkkeysyms.h> // for GDK_... defines

#include "debug.h"          // for debugging prints

#include "draw.h"
#include "chart.h"
#include "clock.h"
#include "themes.h"
#include "utility.h"
#include "clockGUI.h"
#include "settings.h"
#include "loadTheme.h"
#include "x.h"              // g_window_space

#ifdef   _USEWKSPACE
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#define   WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
typedef void (*TOGBTNCB)(GtkToggleButton*, gpointer);

// -----------------------------------------------------------------------------
namespace gui
{
static bool getGladeXml(const char* name, const char* root=NULL, GCallback callBack=NULL);

static void initToggleBtn(const char* name, const char* event, int value, TOGBTNCB callback);

static void stopApp();

static void     wndMoveEnter(GtkWidget* pWidget, GdkEventButton* pButton);
static void     wndMoveExit (GtkWidget* pWidget);
static gboolean wndMoveTime (GtkWidget* pWidget);

static void     wndSizeEnter(GtkWidget* pWidget, GdkEventButton* pButton, GdkWindowEdge edge);
static void     wndSizeExit (GtkWidget* pWidget);
static gboolean wndSizeTime (GtkWidget* pWidget);

// -----------------------------------------------------------------------------
/*#ifdef _USEWKSPACE
static void on_active_workspace_changed(WnckScreen* screen, WnckWorkspace* prevSpace, gpointer userData);
#endif*/

// this is currently not an event handler (so not sure why it's called on_...)
//static void on_alpha_screen_changed(GtkWidget* pWidget, GdkScreen* pOldScreen, GtkWidget* pLabel);

static gboolean on_button_press      (GtkWidget* pWidget, GdkEventButton* pButton, GdkWindowEdge edge);
static void     on_composited_changed(GtkWidget* pWidget, gpointer userData);
static gboolean on_configure         (GtkWidget* pWidget, GdkEventConfigure* pEvent, gpointer userData);
static gint     on_delete            (GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData);
static void     on_destroy           (GtkWidget* pWidget, gpointer userData);

#ifdef _ENABLE_DND
static void     on_drag_data_received(GtkWidget* pWidget, GdkDragContext* pContext, gint x, gint y, GtkSelectionData* selData, guint info, guint _time, gpointer userData);
#define TGT_SEL_STR 0
#endif

static gboolean on_enter_notify (GtkWidget* pWidget, GdkEventCrossing* pCrossing, gpointer userData);
static gboolean on_expose       (GtkWidget* pWidget, GdkEventExpose* pExpose);
static gboolean on_focus_in     (GtkWidget* pWidget);
static gboolean on_key_press    (GtkWidget* pWidget, GdkEventKey* pKey, gpointer userData);
static gboolean on_leave_notify (GtkWidget* pWidget, GdkEventCrossing* pCrossing, gpointer userData);
static gboolean on_motion_notify(GtkWidget* pWidget, GdkEventMotion* pEvent, gpointer userData);
static gboolean on_tooltip_show (GtkWidget* pWidget, gint x, gint y, gboolean keyboard_mode, GtkTooltip* pTooltip, gpointer userData);
static gboolean on_wheel_scroll (GtkWidget* pWidget, GdkEventScroll* pScroll, gpointer userData);
static gboolean on_window_state (GtkWidget* pWidget, GdkEventWindowState* pState, gpointer userData);

static void on_popupmenu_done(GtkMenuShell* pMenuShell, gpointer userData);

static void on_alarm_activate(GtkMenuItem* pMenuItem, gpointer userData);
static void on_chart_activate(GtkMenuItem* pMenuItem, gpointer userData);
static void on_chime_activate(GtkMenuItem* pMenuItem, gpointer userData);
static void on_date_activate (GtkMenuItem* pMenuItem, gpointer userData);
static void on_help_activate (GtkMenuItem* pMenuItem, gpointer userData);
static void on_info_activate (GtkMenuItem* pMenuItem, gpointer userData);
static void on_prefs_activate(GtkMenuItem* pMenuItem, gpointer userData);
static void on_quit_activate (GtkMenuItem* pMenuItem, gpointer userData);

static gboolean on_prefs_close(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData);
static void     on_popup_close(GtkDialog* pDialog, gpointer  userData);

static GtkWidget* popup_activate(char type, const char* name, const char* widget);

static void loadMenuPopup();

/*#ifdef _ENABLE_DND
// target side drag signals
void     (*drag_leave)        (GtkWidget* widget, GdkDragContext* context, guint time_);
gboolean (*drag_motion)       (GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time_);
gboolean (*drag_drop)         (GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time_);
void     (*drag_data_received)(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* selection_data, guint info, guint time_);
gboolean (*drag_failed)       (GtkWidget* widget, GdkDragContext* context, GtkDragResult result);
#endif*/

// -----------------------------------------------------------------------------
static const gchar* get_logo_filename()
{
	return PKGDATA_DIR "/pixmaps/cairo-clock-logo.png";
}

// -----------------------------------------------------------------------------
static GladeXML*      g_pGladeXml = NULL;
GladeXML*               pGladeXml = NULL;

static GtkWindow*     g_pPopupDlg = NULL;
static bool           g_inPopup   = false;
static char           g_nmPopup   = ' ';

static bool           g_wndMovein = false;
static guint          g_wndMoveT  = 0; // timer id

static bool           g_wndSizein = false;
static guint          g_wndSizeT  = 0; // timer id
/*
#ifdef _USEWKSPACE
static WnckWindow*    g_clockWnd  = NULL;
static WnckWorkspace* g_clockWrk  = NULL;
#endif
*/
} // namespace gui

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool gui::init()
{
	DEBUGLOGB;

	bool ok = true;

/*	if( gRun.scrsaver == false )
		ok  = getGladeXml("main");

	if( g_pGladeXml )
	{
		gRun.pMainWindow =  glade_xml_get_widget(g_pGladeXml, "mainWindow");
//		dnit();
	}*/

	if( gRun.pMainWindow == NULL )
	{
		if( gRun.pMainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL) )
		{
			gtk_window_set_title(GTK_WINDOW(gRun.pMainWindow), gRun.appName);
			gtk_window_set_decorated(GTK_WINDOW(gRun.pMainWindow), FALSE);
			gtk_widget_set_double_buffered(gRun.pMainWindow, FALSE);
//			gtk_widget_set_double_buffered(gRun.pMainWindow, TRUE);
			gtk_widget_set_can_focus(gRun.pMainWindow, FALSE);
		}
	}

	if( g_pGladeXml )
		dnit();

	if( gRun.pMainWindow )
	{
		// make the top-level window listen for events for which it doesn't do by default
//		gtk_widget_add_events(gRun.pMainWindow, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK);
//		gtk_widget_add_events(gRun.pMainWindow, GDK_BUTTON_PRESS_MASK | GDK_SCROLL_MASK);

		#define CLOCK_EVENTS_MASK (GDK_EXPOSURE_MASK       | GDK_ENTER_NOTIFY_MASK        | \
								   GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | \
								   GDK_BUTTON_PRESS_MASK   | GDK_BUTTON_RELEASE_MASK)

		gtk_widget_set_events(gRun.pMainWindow, gtk_widget_get_events(gRun.pMainWindow) | CLOCK_EVENTS_MASK);

		// main window's event processing

		g_signal_connect(G_OBJECT(gRun.pMainWindow), "button-press-event",  G_CALLBACK(on_button_press),       NULL);
		g_signal_connect(G_OBJECT(gRun.pMainWindow), "composited-changed",  G_CALLBACK(on_composited_changed), NULL);
		g_signal_connect(G_OBJECT(gRun.pMainWindow), "configure-event",     G_CALLBACK(on_configure),          NULL);
		g_signal_connect(G_OBJECT(gRun.pMainWindow), "delete_event",        G_CALLBACK(on_delete),             NULL);
		g_signal_connect(G_OBJECT(gRun.pMainWindow), "destroy_event",       G_CALLBACK(on_destroy),            NULL);
//		g_signal_connect(G_OBJECT(gRun.pMainWindow), "drag_end_event",      G_CALLBACK(on_drag_end),           NULL);
		g_signal_connect(G_OBJECT(gRun.pMainWindow), "enter-notify-event",  G_CALLBACK(on_enter_notify),       NULL);
		g_signal_connect(G_OBJECT(gRun.pMainWindow), "expose-event",        G_CALLBACK(on_expose),             NULL);
		g_signal_connect(G_OBJECT(gRun.pMainWindow), "focus-in-event",      G_CALLBACK(on_focus_in),           NULL);
		g_signal_connect(G_OBJECT(gRun.pMainWindow), "key-press-event",     G_CALLBACK(on_key_press),          NULL);
		g_signal_connect(G_OBJECT(gRun.pMainWindow), "leave-notify-event",  G_CALLBACK(on_leave_notify),       NULL);
		g_signal_connect(G_OBJECT(gRun.pMainWindow), "query-tooltip",       G_CALLBACK(on_tooltip_show),       NULL);
		g_signal_connect(G_OBJECT(gRun.pMainWindow), "scroll-event",        G_CALLBACK(on_wheel_scroll),       NULL);
//		g_signal_connect(G_OBJECT(gRun.pMainWindow), "window-state-event",  G_CALLBACK(on_window_state),       NULL);

		if( gRun.scrsaver )
		g_signal_connect(G_OBJECT(gRun.pMainWindow), "motion-notify-event", G_CALLBACK(on_motion_notify),      NULL);
	}

	// TODO: move all of this somewhere else more appropriate?

	// set the initial theme to be used at startup

	DEBUGLOGP("startup theme is '%s'\n", gCfg.themeFile);

	 ThemeEntry* pTE    = NULL;
	bool         direct = gCfg.themePath[0] && gCfg.themeFile[0];

	if( direct )
	{
		DEBUGLOGS("  startup theme directly accessible");

		pTE        = (ThemeEntry*)g_malloc0(sizeof(ThemeEntry));
		pTE->pPath =  g_string_new(gCfg.themePath);
		pTE->pFile =  g_string_new(gCfg.themeFile);
	}
	else
	if( gCfg.themeFile[0] )
	{
		DEBUGLOGS("  startup theme must be searched for");
		DEBUGLOGS("  creating current theme list");

		ThemeEntry     te;
		ThemeList*     tl;
		theme_list_get(tl);

		for( te = theme_list_beg(tl); !theme_list_end(tl); te = theme_list_nxt(tl) )
		{
			if( strcmp(gCfg.themeFile, te.pFile->str) == 0 ||
				strcmp(gCfg.themeFile, te.pName->str) == 0 )
			{
				DEBUGLOGP("  theme found - %s, %s, %s\n", te.pName->str, te.pPath->str, te.pFile->str);

				pTE        = (ThemeEntry*)g_malloc0(sizeof(ThemeEntry));
				pTE->pPath =  g_string_new(te.pPath->str);
				pTE->pFile =  g_string_new(te.pFile->str);
				direct     =  true;
				break;
			}
		}

		DEBUGLOGS("  destroying current theme list");
		theme_list_del(tl);
	}

	if( pTE )
	{
//		strvcpy(gCfg.themePath, pTE->pPath->str);
//		strvcpy(gCfg.themeFile, pTE->pFile->str);

		change_theme(pTE, NULL);

		if( direct )
		{
			g_string_free(pTE->pFile, TRUE);
			g_string_free(pTE->pPath, TRUE);
			g_free(pTE);
		}
	}

	DEBUGLOGE;
	return ok;
}

// -----------------------------------------------------------------------------
void gui::dnit(bool clrPopup)
{
	DEBUGLOGB;

	if( g_pGladeXml )
	{
		DEBUGLOGS("clearing glade object");
		g_clear_object(&g_pGladeXml);
	}

	pGladeXml = g_pGladeXml = NULL;

	if( clrPopup )
	{
		DEBUGLOGS("clearing popup tracking vals");
		g_pPopupDlg = NULL;
		g_inPopup   = false;
		g_nmPopup   = ' ';
	}

	DEBUGLOGE;
}
/*
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
const gchar* gui::getGladeFilename()
{
#ifdef _RELEASE
	return PKGDATA_DIR "/glade/cairo-clock.glade";
#else
//	return "../glade/cairo-clock-new.glade";

	static char uipath[PATH_MAX];
	static bool first = true;

	if( first )
	{
		first              = false;
		const char* anpath = get_user_appnm_path();

		if( anpath )
		{
			strvcpy(uipath,  anpath ? anpath : ".");
			strvcat(uipath, "/glade/cairo-clock.glade");

			if( anpath )
				delete [] anpath;
		}
		else
		{
			strvcpy(uipath, "../glade/cairo-clock-new.glade");
		}
	}

	return uipath;
#endif
}
*/
/*
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
struct GladeLoad
{
	GString*  uipath;   // full path to the glade xml file to be loaded
	GString*  root;     // name of the root glade element from which to start loading
	GCallback callBack; // where to go once glade loading has finished
	bool      okay;     // whether the loading was a success
};

// -----------------------------------------------------------------------------
static gpointer load_glade_func(gpointer data)
{
	DEBUGLOGB;

	if( data )
	{
		GladeLoad* pGD   = (GladeLoad*)data;

		DEBUGLOGP("before worker threaded loading of:\n\t%s\n", pGD->uipath->str);

		gdk_threads_enter();
		gui::g_pGladeXml =  gui::  pGladeXml  = glade_xml_new(pGD->uipath->str, pGD->root->len ? pGD->root->str : NULL, NULL);
		pGD->okay        =  gui::g_pGladeXml != NULL;
		gdk_threads_enter();

		DEBUGLOGP("after loading, which %s\n", pGD->okay ? "succeeded" : "failed");

		g_string_free(pGD->uipath, TRUE);
		g_string_free(pGD->root,   TRUE);

		DEBUGLOGS("before making the callBack call");

		pGD->callBack();

		DEBUGLOGS("after making the callBack call");
	}

	DEBUGLOGE;
	return 0;
}
*/
// -----------------------------------------------------------------------------
bool gui::getGladeXml(const char* name, const char* root, GCallback callBack)
{
	DEBUGLOGB;

	char        uipath[PATH_MAX];
	const char* anpath = get_user_appnm_path();
	bool  okay         = false;

	strvcpy(uipath,  anpath ? anpath : ".");
	strvcat(uipath, "/glade/");
	strvcat(uipath,  name);
	strvcat(uipath, ".glade");

	if( anpath )
		delete [] anpath;

/*	if( callBack )
	{
		static GladeLoad gladeLoad;

		gladeLoad.uipath   = g_string_new(uipath);
		gladeLoad.root     = g_string_new(root ? root : "");
		gladeLoad.callBack = callBack;

		DEBUGLOGP("before async loading via worker thread of %s.glade\n", name);

		GThread* pThread = g_thread_try_new(__func__, load_glade_func, &gladeLoad, NULL);

		DEBUGLOGP("after worker thread creation which %s\n", pThread ? "succeeded" : "failed");

		if( pThread )
		{
			g_thread_unref(pThread);
			okay = true;
		}
	}*/

	if( okay == false )
	{
/*		GTimeVal            ct;
		g_get_current_time(&ct);
		DEBUGLOGP("before sync loading of %s.glade (%d.%d)\n", name, (int)ct.tv_sec, (int)ct.tv_usec);*/

		g_pGladeXml = pGladeXml = glade_xml_new(uipath, root, NULL);
		okay = g_pGladeXml != NULL;

/*		g_get_current_time(&ct);
		DEBUGLOGP("after loading, which %s (%d.%d)\n", ok ? "succeeded" : "failed", (int)ct.tv_sec, (int)ct.tv_usec);*/

		if( callBack )
			callBack();
	}

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void gui::initDragDrop()
{
#ifdef _ENABLE_DND
//	static gchar          target_type[] =     "text/uri-list";
//	static GtkTargetEntry target_list[] = { { target_type, GTK_TARGET_OTHER_APP, TGT_SEL_STR } };

	DEBUGLOGS("bef setting drag destination");
//	gtk_drag_dest_set(gRun.pMainWindow, GTK_DEST_DEFAULT_ALL, target_list, G_N_ELEMENTS(target_list), GDK_ACTION_COPY);
	gtk_drag_dest_set(gRun.pMainWindow, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);
	g_signal_connect (G_OBJECT(gRun.pMainWindow), "drag_data_received", G_CALLBACK(on_drag_data_received), NULL);
	gtk_drag_dest_add_uri_targets(gRun.pMainWindow);
	DEBUGLOGS("aft  setting drag destination");
#endif
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void gui::initToggleBtn(const char* name, const char* event, int value, TOGBTNCB callback)
{
	GtkWidget* pButton = glade_xml_get_widget(g_pGladeXml, name);

	if( pButton )
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pButton), value);
		g_signal_connect(G_OBJECT(pButton), event, G_CALLBACK(callback), gRun.pMainWindow);
	}
	else
	{
		DEBUGLOGP("failed to get/set toggle button %s's value to %d and connect its %s event\n", name, value, event);
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void gui::initWorkspace()
{
	DEBUGLOGB;

#ifdef _USEWKSPACE

	WnckScreen* screen = wnck_screen_get_default();

	if( screen )
	{
		DEBUGLOGS("got default screen");
		wnck_screen_force_update(screen);

		if( !gCfg.sticky && gCfg.clockWS )
		{
			DEBUGLOGP("sticky is off and workspace is %d\n", gCfg.clockWS);
			WnckWindow* window = wnck_window_get(GDK_WINDOW_XWINDOW(gRun.pMainWindow->window));

			if( window && !wnck_window_is_pinned(window) )
			{
				DEBUGLOGS("got non-pinned window");
				WnckWorkspace* trgtWrk = wnck_screen_get_workspace       (screen, gCfg.clockWS-1);
				WnckWorkspace* actvWrk = wnck_screen_get_active_workspace(screen);

				if( trgtWrk && actvWrk && trgtWrk != actvWrk )
				{
					DEBUGLOGS("got target workspace is diff from current so moving window to target");
 					wnck_window_move_to_workspace(window, trgtWrk);
				}
			}
		}
	}

//	g_signal_connect(screen, "active-workspace-changed", G_CALLBACK(on_active_workspace_changed), NULL);

//	gulong wnck_screen_get_background_pixmap(WnckScreen* screen); // can use for non-composited background drawing?

#else  // _USEWKSPACE

	if( !gCfg.sticky && gCfg.clockWS )
		g_window_space(gRun.pMainWindow, gCfg.clockWS-1);

#endif // _USEWKSPACE

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void gui::dnitWorkspace()
{
#ifdef _USEWKSPACE
	DEBUGLOGB;
#if GTK_CHECK_VERSION(3,0,0)
	wnck_shutdown();
#endif
	DEBUGLOGE;
#endif
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void gui::stopApp()
{
	DEBUGLOGB;
	gRun.appStop = true;

	if( gRun.pMainWindow )
	{
		gtk_widget_destroyed(gRun.pMainWindow, &gRun.pMainWindow);
		DEBUGLOGS("main window widget destroyed");
	}

	DEBUGLOGS("quitting");
	g_main_end();
	DEBUGLOGE;
}

/*******************************************************************************
**
** Main window event related
**
*******************************************************************************/

// -----------------------------------------------------------------------------
void gui::loadMenuPopup()
{
	DEBUGLOGB;

	if( !g_pGladeXml )
	{
		DEBUGLOGS("exit(1)");
//		gdk_threads_enter(); // TODO: necessary?
		dnit();
//		gdk_threads_leave();
		return;
	}

	DEBUGLOGS("before popupMenu widget loading");
//	gdk_threads_enter();
	GtkWidget* pPopUpMenu = glade_xml_get_widget(g_pGladeXml, "popUpMenu");
//	gdk_threads_leave();
	DEBUGLOGS("after popupMenu widget loading");

	if( !pPopUpMenu )
	{
		DEBUGLOGS("exit(2)");
		return;
	}

	static const char* mis[] =
	{
		"prefs", "chart", "alarm", "chime", "date", "info", "help", "quit"
	};

	static GCallback   cbs[] =
	{
		G_CALLBACK(on_prefs_activate), G_CALLBACK(on_chart_activate),
		G_CALLBACK(on_alarm_activate), G_CALLBACK(on_chime_activate),
		G_CALLBACK(on_date_activate),  G_CALLBACK(on_info_activate),
		G_CALLBACK(on_help_activate),  G_CALLBACK(on_quit_activate)
	};

	char       tstr[1024];
	GtkWidget* pMenuItem;

	DEBUGLOGS("before popupMenu item widget loading");
	for( size_t i = 0; i < vectsz(mis); i++ )
	{
		snprintf(tstr, vectsz(tstr), "%sMenuItem", mis[i]);

//		gdk_threads_enter();
		pMenuItem = glade_xml_get_widget(g_pGladeXml, tstr);

		if( pMenuItem )
			g_signal_connect(G_OBJECT(pMenuItem), "activate", cbs[i], gRun.pMainWindow);
//		gdk_threads_leave();
	}
	DEBUGLOGS("after popupMenu item widget loading");

	g_signal_connect(G_OBJECT(pPopUpMenu), "selection-done", G_CALLBACK(on_popupmenu_done), gRun.pMainWindow);

	DEBUGLOGS("before popupMenu displaying");
//	gdk_threads_enter();
	gtk_menu_popup(GTK_MENU(pPopUpMenu), NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
//	gdk_threads_leave();
	DEBUGLOGS("after popupMenu displaying");

	DEBUGLOGS("exit(3)");
}
/*
#ifdef _USEWKSPACE
// -----------------------------------------------------------------------------
void gui::on_active_workspace_changed(WnckScreen* screen, WnckWorkspace* prevSpace, gpointer userData)
{
	DEBUGLOGB;

	if( gRun.scrsaver && !gRun.appStart )
	{
		stopApp();
		DEBUGLOGS("exit(1)");
		return;
	}

	if( gCfg.sticky || !gCfg.clockWS )
	{
		DEBUGLOGS("exit(2)");
		return;
	}

	WnckWorkspace* actvWrk = wnck_screen_get_active_workspace(screen);
	WnckWorkspace* trgtWrk = wnck_screen_get_workspace       (screen, gCfg.clockWS-1);

	if( actvWrk != trgtWrk )
	{
		DEBUGLOGS("exit(3)");
		return;
	}

//	g_clockWnd    = wnck_window_get(GDK_WINDOW_XWINDOW(gRun.pMainWindow->window));
//	g_clockWrk    = wnck_window_get_workspace(g_clockWnd);

	WnckWindow*    g_clockWnd = wnck_window_get(GDK_WINDOW_XWINDOW(gRun.pMainWindow->window));
	WnckWorkspace* g_clockWrk = wnck_window_get_workspace(g_clockWnd);

	gRun.renderIt = cclock::sticky() || !g_clockWrk || !actvWrk || (g_clockWrk == actvWrk);
	gRun.renderUp = gRun.renderIt    && !gRun.appStart;

//	if( gRun.scrsaver && !gRun.appStart )
//	{
//		stopApp();
//	}
//	else
//	if( gRun.renderUp && !cclock::sticky() && (g_clockWrk && g_clockWrk != actvWrk) )
	if( gRun.renderIt )
	{
		DEBUGLOGS("queueing redraw request");
//		TODO: best to fire off draw timer so renderIt, etc., are handled properly
		gtk_widget_queue_draw(gRun.pMainWindow);
	}

//	DEBUGLOGP("clockWnd  is %s, clockWrk is %s\n", g_clockWnd    ? "okay" : "null", g_clockWrk ? "okay" : "null");
//	DEBUGLOGP("rendering is %s\n", gRun.renderIt ? "on"   : "off");

//	if( gRun.renderIt )
//	{
////		render_time_handler(gRun.pMainWindow);
//	}
//	else
//	{
//		if( gRun.renderHandlerId )
//		{
//			g_source_remove(gRun.renderHandlerId);
//			gRun.renderHandlerId = 0;
//		}
//	}

//	const char* wsName = wnck_workspace_get_name(actvWrk);
//	notify_notification_update(m_notification, N_SUMMARY, wsName, N_ICON);
//	notify_notification_show  (m_notification, NULL);

	DEBUGLOGS("exit(4)");
}
#endif // _USEWKSPACE*/
/*
// -----------------------------------------------------------------------------
void gui::on_alpha_screen_changed(GtkWidget* pWidget, GdkScreen* pOldScreen, GtkWidget* pLabel)
{
/*	GdkScreen*   pScreen   = gtk_widget_get_screen(pWidget);
	GdkColormap* pColormap = gdk_screen_get_rgba_colormap(pScreen);
      
	if( pColormap == NULL )
		pColormap =  gdk_screen_get_rgb_colormap(pScreen);

	gtk_widget_set_colormap(pWidget, pColormap);*/
/*	update_colormap(pWidget);
}*/

// -----------------------------------------------------------------------------
gboolean gui::on_button_press(GtkWidget* pWidget, GdkEventButton* pButton, GdkWindowEdge edge)
{
	DEBUGLOGB;

	if( pButton->type != GDK_BUTTON_PRESS )
		return FALSE;

	DEBUGLOGP("button %d pressed\n", (int)pButton->button);

	if( gRun.scrsaver )
	{
		stopApp();
		return FALSE;
	}

	if( pButton->button == 1 ) // typically left-click for window moving
	{
		wndMoveExit (pWidget);
		wndMoveEnter(pWidget, pButton);
	}
	else
	if( pButton->button == 2 ) // typically middle-click for window resizing
	{
		wndSizeExit (pWidget);
		wndSizeEnter(pWidget, pButton, edge);
	}
	else
	if( pButton->button == 3 ) // typically right-click for window context menu
	{
		if( g_pGladeXml )
		{
			if( !g_inPopup )
			{
				DEBUGLOGS("clearing previously loaded glade context menu");
				dnit();
			}
		}

		if( g_pGladeXml == NULL )
		{
			DEBUGLOGS("attempting to load/popup glade context menu");

			if( true ) // true for single threaded, false for multi-threaded
			{
				getGladeXml("menu");
				loadMenuPopup();
			}
			else
				getGladeXml("menu", NULL, loadMenuPopup);

/*			if( g_pGladeXml )
			{
				GtkWidget* pPopUpMenu = glade_xml_get_widget(g_pGladeXml, "popUpMenu");

				if( pPopUpMenu )
				{
					static const char* mis[] =
					{ "prefs", "chart", "alarm", "chime", "date", "info", "help", "quit" };

					static GCallback   cbs[] =
					{ G_CALLBACK(on_prefs_activate), G_CALLBACK(on_chart_activate),
					  G_CALLBACK(on_alarm_activate), G_CALLBACK(on_chime_activate),
					  G_CALLBACK(on_date_activate),  G_CALLBACK(on_info_activate),
					  G_CALLBACK(on_help_activate),  G_CALLBACK(on_quit_activate) };

					char       tstr[1024];
					GtkWidget* pMenuItem;

					for( size_t i = 0; i < vectsz(mis); i++ )
					{
						snprintf(tstr, vectsz(tstr), "%sMenuItem", mis[i]);
						pMenuItem = glade_xml_get_widget(g_pGladeXml, tstr);
						g_signal_connect(G_OBJECT(pMenuItem), "activate", cbs[i], gRun.pMainWindow);
					}

					gtk_menu_popup(GTK_MENU(pPopUpMenu), NULL, NULL, NULL, NULL, pButton->button, pButton->time);
				}
				else
				{
					DEBUGLOGS("didn't load/popup glade context menu (3)");
					dnit();
				}
			}
			else
			{
				DEBUGLOGS("didn't load/popup glade context menu (2)");
			}*/
		}
		else
		{
			DEBUGLOGS("didn't load/popup glade context menu (1)");
		}
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
void gui::on_composited_changed(GtkWidget* pWidget, gpointer userData)
{
	DEBUGLOGB;
	DEBUGLOGF("%s\n", "entry");

	update_wnd_dim(pWidget, gCfg.clockW, gCfg.clockH, true);

	DEBUGLOGF("%s\n", "exit");
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// The ::configure-event signal will be emitted when the size, position or
// stacking of the widget's window has changed.
// -----------------------------------------------------------------------------
gboolean gui::on_configure(GtkWidget* pWidget, GdkEventConfigure* pEvent, gpointer userData)
{
	DEBUGLOGB;

	gint newX = pEvent->x;
	gint newY = pEvent->y;
	gint newW = pEvent->width;
	gint newH = pEvent->height;
/*
#ifdef DEBUGLOG
	gint                                          oldX,  oldY;
	gint                                          oldW,  oldH;
	gtk_window_get_position(GTK_WINDOW(pWidget), &oldX, &oldY);
	gtk_window_get_size    (GTK_WINDOW(pWidget), &oldW, &oldH);

	DEBUGLOGP("oldX=%d, oldY=%d, oldW=%d, oldH=%d\n", oldX, oldY, oldW, oldH);
#endif*/
	DEBUGLOGP("newX=%d, newY=%d, newW=%d, newH=%d\n", newX, newY, newW, newH);

	if( newX != gCfg.clockX || newY != gCfg.clockY )
	{
#ifdef  DEBUGLOG
		static int n = 0;
		DEBUGLOGP("position change # %d: newX=%d, newY=%d\n", ++n, newX, newY);

/*		static int      n  =   0;
		static GTimeVal bt = { 0, 0 }, ct = { 0, 0 };

		g_get_current_time(&ct);

		guint32  et = gtk_get_current_event_time();
		gboolean pg = gdk_pointer_is_grabbed();

		DEBUGLOGP("position change # %d (%d, %d, %d.%d, %d.%d)\n", ++n, (int)pg, (int)et, (int)ct.tv_sec, (int)ct.tv_usec, (int)bt.tv_sec, (int)bt.tv_usec);
		bt = ct;*/
#endif
		gCfg.clockX = newX;
		gCfg.clockY = newY;

		if( pGladeXml )
		{
			prefs::open(true);

			GtkWidget* pWidget;

			Settings  tcfg = gCfg;
			cfg::cnvp(tcfg,  true);

			if( pWidget = glade_xml_get_widget(pGladeXml, "spinbuttonX") )
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockX);

			if( pWidget = glade_xml_get_widget(pGladeXml, "spinbuttonY") )
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockY);

			prefs::open(false);
		}
	}

	if( newW != gCfg.clockW || newH != gCfg.clockH )
	{
#ifdef  DEBUGLOG
		static int n = 0;
		DEBUGLOGP("size change # %d: newW=%d, newH=%d\n", ++n, newW, newH);
#endif
		gRun.drawScaleX = (double)newW/(double)gCfg.clockW;
		gRun.drawScaleY = (double)newH/(double)gCfg.clockH;

		DEBUGLOGP("new sz: old(%d %d), new(%d %d), scl(%f %f)\n",
			gCfg.clockW, gCfg.clockH, newW, newH, gRun.drawScaleX, gRun.drawScaleY);

		if( pGladeXml )
		{
			prefs::open(true);

			GtkWidget* pWidget;

			Settings  tcfg = gCfg;
			cfg::cnvp(tcfg,  true);

			if( pWidget = glade_xml_get_widget(pGladeXml, "spinbuttonX") )
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockX);

			if( pWidget = glade_xml_get_widget(pGladeXml, "spinbuttonY") )
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockY);

			if( pWidget = glade_xml_get_widget(pGladeXml, "spinbuttonWidth") )
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), gCfg.clockW);

			if( pWidget = glade_xml_get_widget(pGladeXml, "spinbuttonHeight") )
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), gCfg.clockH);

			prefs::open(false);
		}

		// TODO: can just redraw part of the clock window?
		//       or can do stretching until resizing is over?

////		gtk_widget_queue_draw(pWidget);
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
gint gui::on_delete(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	DEBUGLOGS("entry-exit");
	stopApp();
	return TRUE;
}

// -----------------------------------------------------------------------------
void gui::on_destroy(GtkWidget* pWidget, gpointer userData)
{
	DEBUGLOGS("entry-exit");
	stopApp();
}

#ifdef _ENABLE_DND
// -----------------------------------------------------------------------------
void gui::on_drag_data_received(GtkWidget* pWidget, GdkDragContext* pContext, gint x, gint y, GtkSelectionData* pSelData, guint info, guint _time, gpointer userData)
{
	DEBUGLOGB;

	bool         update = false;
	const gchar* name   = gtk_widget_get_name(pWidget);

	DEBUGLOGP("widget name is %s\n", name);

	if( gdk_drag_context_get_suggested_action(pContext) == GDK_ACTION_COPY &&
		pSelData && gtk_selection_data_get_length(pSelData) > 0 &&
		info == TGT_SEL_STR )
	{
		gchar* strs = (gchar*)gtk_selection_data_get_data(pSelData);

		if( strs )
		{
			DEBUGLOGP("string received is\n*****\n%s\n*****\n", strs);

			gchar*  eol = strs;
			while( (eol = strchr(eol, '\n')) && *eol )
			{
				*eol = '\0';
				 eol++;
			}

			gchar* curs =   strs;
			while( curs && *curs )
			{
				int    snxt = strlen(curs) + 1;
				gchar* path = g_filename_from_uri(curs, NULL, NULL);
				if(   !path ) break;

				g_strstrip(path);

				int plen = strlen(path);
				if( plen > 1 )
				{
					DEBUGLOGP("path received is\n*%s*\n", path);

					char tpath[PATH_MAX]; *tpath = '\0';
					char tname[64];       *tname = '\0';

					if( loadTheme(path, tpath, vectsz(tpath), tname, vectsz(tname)) )
					{
						DEBUGLOGP("path received successfully extracted to\n*%s%s*\n", tpath, tname);

						GString* pP = g_string_new(tpath);
						GString* pN = g_string_new(tname);

						DEBUGLOGP("current theme to use is\n*%s%s*\n", gCfg.themePath, gCfg.themeFile);

						update = true;
						gRun.updateSurfs = true;

						ThemeEntry    te = { pP, pN };
						change_theme(&te,  pWidget);

						g_string_free(pP,  TRUE);
						g_string_free(pN,  TRUE);
					}
					else
					{
						DEBUGLOGS("path received not extracted");
					}

					if( tpath[0] && false )
						g_del_dir(tpath);

					break; // only one dropped theme imported for now
				}

				DEBUGLOGP("slen=%d, plen=%d, curs+snxt is\n*****\n%s\n*****\n", snxt, plen, curs+snxt);

				curs += snxt;

				g_free(path);
			}
		}
		else
		{
			DEBUGLOGS("no string received");
		}
	}

	gtk_drag_finish(pContext, TRUE, FALSE, _time);

	// TODO: need func to just add new theme above instead of rebuild here

/*	if( update )
	{
		// TODO: anything to do here now that a global theme list is no longer kept?
	}*/

	DEBUGLOGE;
}
#endif // _ENABLE_DND

// -----------------------------------------------------------------------------
gboolean gui::on_enter_notify(GtkWidget* pWidget, GdkEventCrossing* pCrossing, gpointer userData)
{
	DEBUGLOGB;

	gdk_window_set_opacity(gRun.pMainWindow->window, 1.0);
//	wndMoveExit(pWidget);

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
// The ::expose-event signal is emitted when an area of a previously obscured
// GdkWindow is made visible and needs to be redrawn.
// -----------------------------------------------------------------------------
gboolean gui::on_expose(GtkWidget* pWidget, GdkEventExpose* pExpose)
{
	DEBUGLOGB;
	DEBUGLOGP("entry - %d upcoming exposes\n", pExpose->count);

#ifdef  DEBUGLOG
	static int ct = 0;
//	if( ct < 10 ) DEBUGLOGP("rendering(%d)\n", ++ct);
	DEBUGLOGP("rendering(%d)\n", ++ct);
#endif

	draw::render(pWidget, gRun.drawScaleX, gRun.drawScaleY, gRun.renderIt, gRun.appStart, gCfg.clockW, gCfg.clockH, gRun.appStart);

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean gui::on_focus_in(GtkWidget* pWidget)
{
	DEBUGLOGB;

	// focus-in is not fired on on all dms/wms/gtklibs/gdklibs/xlibs/who-knows

//	wndMoveExit(pWidget);

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
void gui::on_alarm_activate(GtkMenuItem* pMenuItem, gpointer userData)
{
	GtkWidget* pDlgBox =
		gtk_message_dialog_new((GtkWindow*)userData,
			GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
			"No alarm settings/usage available yet.\n\nSorry.");

	if( pDlgBox )
	{
		gtk_window_set_transient_for(GTK_WINDOW(pDlgBox), (GtkWindow*)userData);
		gtk_window_set_position(GTK_WINDOW(pDlgBox), GTK_WIN_POS_CENTER);
		gtk_window_set_title(GTK_WINDOW(pDlgBox), gRun.appName);
		gtk_dialog_run(GTK_DIALOG(pDlgBox));
		on_popup_close(GTK_DIALOG(pDlgBox), userData);
		gtk_widget_destroy(pDlgBox);
	}
}

// -----------------------------------------------------------------------------
static void on_chart_realize(GtkWidget* pWidget, gpointer data)
{
	GtkWidget* pChart = glade_xml_get_widget(gui::g_pGladeXml, "drawingareaCurve");
	chart_init(pChart);
}

// -----------------------------------------------------------------------------
void gui::on_chart_activate(GtkMenuItem* pMenuItem, gpointer data)
{
	GtkWidget* pDialog = popup_activate('c', "chart", "windowCurve");

	if( pDialog )
	{
		set_window_icon(pDialog);

		g_signal_connect(G_OBJECT(pDialog), "delete_event", G_CALLBACK(on_popup_close),   NULL);
		g_signal_connect(G_OBJECT(pDialog), "realize",      G_CALLBACK(on_chart_realize), NULL);

		gtk_widget_show_all(pDialog);

		g_pPopupDlg = GTK_WINDOW(pDialog);
		g_inPopup   = true;
		g_nmPopup   = 'c';
	}
}

// -----------------------------------------------------------------------------
void gui::on_chime_activate(GtkMenuItem* pMenuItem, gpointer userData)
{
	GtkWidget* pDlgBox =
		gtk_message_dialog_new((GtkWindow*)userData,
			GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
			"No chime settings/usage available yet.\n\nSorry.");

	if( pDlgBox )
	{
		gtk_window_set_transient_for(GTK_WINDOW(pDlgBox), (GtkWindow*)userData);
		gtk_window_set_position(GTK_WINDOW(pDlgBox), GTK_WIN_POS_CENTER);
		gtk_window_set_title(GTK_WINDOW(pDlgBox), gRun.appName);
		gtk_dialog_run(GTK_DIALOG(pDlgBox));
		on_popup_close(GTK_DIALOG(pDlgBox), userData);
		gtk_widget_destroy(pDlgBox);
	}
}

// -----------------------------------------------------------------------------
void gui::on_date_activate(GtkMenuItem* pMenuItem, gpointer data)
{
	GtkWidget* pDialog = popup_activate('d', "date", "windowDate");

	if( pDialog )
	{
		set_window_icon(pDialog);

		GtkCalendar* pDate = (GtkCalendar*)glade_xml_get_widget(gui::g_pGladeXml, "calendar1");

		gtk_calendar_select_month(pDate, gRun.timeCtm.tm_mon, gRun.timeCtm.tm_year+1900);
		gtk_calendar_select_day  (pDate, gRun.timeCtm.tm_mday);

		g_signal_connect(G_OBJECT(pDialog), "delete_event", G_CALLBACK(on_popup_close), NULL);

		gtk_widget_show_all(pDialog);

		g_pPopupDlg = GTK_WINDOW(pDialog);
		g_inPopup   = true;
		g_nmPopup   = 'd';
	}
}

// -----------------------------------------------------------------------------
void gui::on_help_activate(GtkMenuItem* pMenuItem, gpointer userData)
{
	GtkWidget* pDlgBox =
		gtk_message_dialog_new((GtkWindow*)userData,
			GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
			"No help available yet, other than through the command line via -? or --help.\n\nSorry.");

	if( pDlgBox )
	{
		gtk_window_set_transient_for(GTK_WINDOW(pDlgBox), (GtkWindow*)userData);
		gtk_window_set_position(GTK_WINDOW(pDlgBox), GTK_WIN_POS_CENTER);
		gtk_window_set_title(GTK_WINDOW(pDlgBox), gRun.appName);
		gtk_dialog_run(GTK_DIALOG(pDlgBox));
		on_popup_close(GTK_DIALOG(pDlgBox), userData);
		gtk_widget_destroy(pDlgBox);
	}
}

// -----------------------------------------------------------------------------
void gui::on_info_activate(GtkMenuItem* pMenuItem, gpointer userData)
{
	GtkWidget* pDialog = popup_activate('a', "info", "infoDialog");

	if( pDialog )
	{
		set_window_icon(pDialog);

		gtk_about_dialog_set_version (GTK_ABOUT_DIALOG(pDialog), gRun.appVersion);
//		gtk_window_set_icon_from_file(GTK_WINDOW      (pDialog), get_icon_filename(), NULL);
//		gtk_about_dialog_set_logo    (GTK_ABOUT_DIALOG(pDialog), gdk_pixbuf_new_from_file(get_logo_filename(), &pError));
		gtk_about_dialog_set_logo    (GTK_ABOUT_DIALOG(pDialog), gdk_pixbuf_new_from_file(get_logo_filename(), NULL));

		g_signal_connect(G_OBJECT(pDialog), "response", G_CALLBACK(on_popup_close), NULL);

		gtk_widget_show(pDialog);

		g_pPopupDlg = GTK_WINDOW(pDialog);
		g_inPopup   = true;
		g_nmPopup   = 'a';
	}
}

// -----------------------------------------------------------------------------
gboolean gui::on_key_press(GtkWidget* pWidget, GdkEventKey* pKey, gpointer userData)
{
	DEBUGLOGP("entry - keyval is %d\n", (int)pKey->keyval);

	if( pKey->type == GDK_KEY_PRESS )
	{
		if( gRun.scrsaver )
		{
			stopApp();
			return FALSE;
		}

		if( gRun.maximize && (pKey->keyval == 'q' || pKey->keyval == 'Q') )
		{
			pKey->keyval  =  'm';
		}

		switch( pKey->keyval )
		{
		case GDK_Escape:
		case 'q':
		case 'Q': stopApp();
				  break;

		case GDK_Menu: on_prefs_activate(NULL, NULL);
					   break;

		// settings based functionality - TODO: should these be 'cfg-saved'?

		case '2': gCfg.show24Hrs = true;
				  prefs::on_24h_toggled(NULL, pWidget);
				  break;

		case '4': gCfg.show24Hrs = false;
				  prefs::on_24h_toggled(NULL, pWidget);
				  break;

		case 'b': prefs::on_show_in_taskbar_toggled(NULL, pWidget);
				  break;

		case 'B': prefs::on_keep_on_bot_toggled(NULL, pWidget);
				  break;

		case 'c':
		case 'C':
			if( gCfg.clockX != 0 || gCfg.clockY != 0 || gCfg.clockC != CORNER_CENTER )
			{
				gCfg.clockX  = 0;
				gCfg.clockY  = 0;
				gCfg.clockC  = CORNER_CENTER;

				cfg::cnvp(gCfg, true);
				update_wnd_pos(pWidget, gCfg.clockX, gCfg.clockY);
			}
			break;

		case 'd':
		case 'D': prefs::on_show_date_toggled(NULL, pWidget);
				  break;

		case 'e':
//		case 'E':
		case 'E': gRun.evalDraws = !gRun.evalDraws; // TODO: turning this on now crashes the app (fix in draw)
//				  // TODO: for composited testing, otherwise leave commented out
//				  gdk_window_set_composited(pWidget->window, gRun.evalDraws ? FALSE : TRUE);
//				  on_composited_changed(pWidget, NULL);
				  gtk_widget_queue_draw(pWidget);
				  break;

		case 'f':
		case 'F': prefs::on_face_date_toggled(NULL, pWidget);
				  break;

		case 'i':
		case 'I': prefs::on_sticky_toggled(NULL, pWidget);
				  break;

		case 'k': gRun.clickthru = !gRun.clickthru;
				  update_input_shape(pWidget, gCfg.clockW, gCfg.clockH, false, true, false);
				  gtk_widget_queue_draw(pWidget);
				  break;

		case 'K': gRun.nodisplay = !gRun.nodisplay;
				  update_input_shape(pWidget, gCfg.clockW, gCfg.clockH, false, true, false);
				  gtk_widget_queue_draw(pWidget);
				  break;

		case 'm':
		case 'M':
			int cx, cy, cw, ch;

			if( gRun.maximize = !gRun.maximize )
			{
				gRun.prevWinX =  gCfg.clockX;
				gRun.prevWinY =  gCfg.clockY;
				gRun.prevWinW =  gCfg.clockW;
				gRun.prevWinH =  gCfg.clockH;

				int  sw       =  gdk_screen_get_width (gdk_screen_get_default());
				int  sh       =  gdk_screen_get_height(gdk_screen_get_default());

				cx = sw/2-gCfg.clockW/2; cy = sh/2-gCfg.clockH/2;
				cw = sw; ch = sh;

				gtk_window_maximize(GTK_WINDOW(pWidget));
			}
			else
			{
				cx = gRun.prevWinX; cy = gRun.prevWinY;
				cw = gRun.prevWinW; ch = gRun.prevWinH;

				gtk_window_unmaximize(GTK_WINDOW(pWidget));
			}

			// TODO: only shows correctly after switching to another workspace

			update_wnd_pos(pWidget, cx, cy);
			update_wnd_dim(pWidget, cw, ch, true);

			break;

		case 'o':
		case 'O': prefs::on_keep_on_top_toggled(NULL, pWidget);
				  break;

		case 'p':
		case 'P': prefs::on_show_in_pager_toggled(NULL, pWidget);
				  break;

		case 'r': change_ani_rate(pWidget, gCfg.refreshRate-1, false, true);
				  break;
		case 'R': change_ani_rate(pWidget, gCfg.refreshRate+1, false, true);
				  break;

		case 's':
		case 'S': prefs::on_seconds_toggled(NULL, pWidget);
				  break;

		case 'T': gRun.textonly = !gRun.textonly;
				  draw::update_date_surf();
				  gtk_widget_queue_draw(pWidget);
				  change_ani_rate(pWidget, gCfg.refreshRate, true, false);
				  break;
		}
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean gui::on_leave_notify(GtkWidget* pWidget, GdkEventCrossing* pCrossing, gpointer userData)
{
	DEBUGLOGB;

	if( !g_wndMovein )
	{
		gdk_window_set_opacity(gRun.pMainWindow->window, gCfg.opacity);
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean gui::on_motion_notify(GtkWidget* pWidget, GdkEventMotion* pEvent, gpointer userData)
{
	if( gRun.scrsaver && !gRun.appStart )
	{
		static double mouseX, mouseY, delmvX, delmvY;
		static bool  first =  true;

		if( first )
		{
			first  = false;
			mouseX = pEvent->x;
			mouseY = pEvent->y;
			delmvX = gCfg.clockW/20.0;
			delmvY = gCfg.clockH/20.0;
		}
		else
		if( fabs(pEvent->x-mouseX) > delmvX || fabs(pEvent->y-mouseY) > delmvY )
		{
			stopApp();
		}
	}

	return FALSE;
}

// -----------------------------------------------------------------------------
GtkWidget* gui::popup_activate(char type, const char* name, const char* widget)
{
	DEBUGLOGB;

	if( g_inPopup && g_nmPopup == type )
	{
		gtk_window_present(g_pPopupDlg);
		dnit(false);
		return NULL;
	}

	if( g_inPopup || !g_pGladeXml ) // should always be available since this is a glade menu item event
	{
		dnit(!g_inPopup);
		return NULL;
	}

	dnit();

	if( g_pGladeXml == NULL )
		getGladeXml(name);

	GtkWidget* ret = g_pGladeXml != NULL ? glade_xml_get_widget(g_pGladeXml, widget) : NULL;

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
void gui::on_popup_close(GtkDialog* pDialog, gpointer userData)
{
	gtk_widget_hide(GTK_WIDGET(pDialog));
	dnit();
}

// -----------------------------------------------------------------------------
void gui::on_popupmenu_done(GtkMenuShell* pMenuShell, gpointer userData)
{
	DEBUGLOGB;

	if( !g_inPopup )
	{
		DEBUGLOGS("clearing previously loaded glade context menu");
		dnit();
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static void on_custom_anim_activate(GtkExpander* pExpander, gpointer user_data)
{
	DEBUGLOGS("entry/exit");

	if( !gtk_expander_get_expanded(pExpander) )
	{
		GtkWidget* pChart = glade_xml_get_widget(gui::g_pGladeXml, "drawingareaAnimationCustom");
		chart_init(pChart);
	}
}

// -----------------------------------------------------------------------------
void gui::on_prefs_activate(GtkMenuItem* pMenuItem, gpointer data)
{
	DEBUGLOGB;
/*
	https://developer.gnome.org/gio/stable/GSubprocess.html#g-subprocess-new
	GSubprocess* g_subprocess_new(GSubprocessFlags flags, GError** error, const gchar* argv0, ...);
*/
	// TODO: can this use 'popup_activate' function instead of all this? If not, why?

/*	GtkWidget* pDialog = popup_activate('c', "chart", "windowCurve");

	if( pDialog )
	{
		set_window_icon(pDialog);

		g_signal_connect(G_OBJECT(pDialog), "delete_event", G_CALLBACK(on_popup_close),   NULL);
		g_signal_connect(G_OBJECT(pDialog), "realize",      G_CALLBACK(on_chart_realize), NULL);

		gtk_widget_show_all(pDialog);

		g_pPopupDlg = GTK_WINDOW(pDialog);
		g_inPopup   = true;
		g_nmPopup   = 'c';
	}*/

	if( g_inPopup && g_nmPopup == 's' )
	{
		gtk_window_present(g_pPopupDlg);
		dnit(false);
		return;
	}

	if( g_inPopup || !g_pGladeXml ) // should always be available since this is a glade menu item event
	{
		DEBUGLOGS("exit(1)");
		dnit(!g_inPopup);
		return;
	}

	prefs::begin();

	dnit();

	if( g_pGladeXml == NULL )
		getGladeXml("props");

	if( g_pGladeXml )
	{
/*		GTimeVal            ct;
		g_get_current_time(&ct);
		DEBUGLOGP("before sync loading of %s (%d.%d)\n", "settingsDialog", (int)ct.tv_sec, (int)ct.tv_usec);*/

		GtkWidget* pDialog = glade_xml_get_widget(g_pGladeXml, "settingsDialog");

/*		g_get_current_time(&ct);
		DEBUGLOGP("after loading, which %s (%d.%d)\n", pDialog ? "succeeded" : "failed", (int)ct.tv_sec, (int)ct.tv_usec);*/

		if( pDialog )
		{
//			gtk_window_set_icon_from_file(GTK_WINDOW(pDialog), get_icon_filename(), NULL);
			set_window_icon(pDialog);
			prefs::init(pDialog, g_pPopupDlg, g_inPopup, g_nmPopup);
		}
		else
		{
			dnit();
		}
	}

	DEBUGLOGS("exit(2)");
}

// -----------------------------------------------------------------------------
gboolean gui::on_prefs_close(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	gtk_widget_hide(pWidget);
	dnit();

	return FALSE;
}

// -----------------------------------------------------------------------------
void gui::on_quit_activate(GtkMenuItem* pMenuItem, gpointer data)
{
	stopApp();
}

// -----------------------------------------------------------------------------
gboolean gui::on_tooltip_show(GtkWidget* pWidget, gint x, gint y, gboolean keyboard_mode, GtkTooltip* pTooltip, gpointer userData)
{
//	update_ts_info();

	gchar    str[128];
	strftime(str, vectsz(str), gCfg.show24Hrs ? gCfg.fmt24Hrs : gCfg.fmt12Hrs, &gRun.timeCtm);

//	DEBUGLOGP("ttip=%s\n\ndate string=%s\ntimes (bef, cur, dif): %d, %d, %d\n",
//		str, gRun.acDate, (int)g_timeBef, (int)g_timeCur, (int)g_timeDif);

//	gtk_widget_set_tooltip_text(gRun.pMainWindow, str);
	gtk_tooltip_set_text(pTooltip, str);

	return TRUE;
}

// -----------------------------------------------------------------------------
gboolean gui::on_wheel_scroll(GtkWidget* pWidget, GdkEventScroll* pScroll, gpointer userData)
{
	DEBUGLOGB;

	if( gRun.scrsaver )
	{
		stopApp();
		return FALSE;
	}

	const int dincr = (pScroll->state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK ? 10 : 1;
	int       delta =  0;

	switch( pScroll->direction )
	{
	case GDK_SCROLL_LEFT:
	case GDK_SCROLL_UP:
		delta -= dincr;
		break;

	case GDK_SCROLL_RIGHT:
	case GDK_SCROLL_DOWN:
		delta += dincr;
		break;

	DEBUGLOGP("delta is %d\n", delta);

/*	case GDK_SCROLL_SMOOTH:
		{
			double                                    dx,  dy;
			if( gdk_event_get_scroll_deltas(pScroll, &dx, &dy) )
			{
				if( dx < 0.0f || dy < 0.0f )
					delta += dincr;
				else
				if( dx > 0.0f || dy > 0.0f )
					delta -= dincr;
			}
		}
		break;*/
	}

	if( delta )
	{
		if( (pScroll->state & GDK_MOD1_MASK) == GDK_MOD1_MASK ) // font size change (alt key normally)
		{
			DEBUGLOGS("  font size change request");

			gCfg.fontSize += delta;
			draw::update_date_surf();
//			gtk_widget_queue_draw(pWidget);
			cfg::save();

			DEBUGLOGP("changing font size to %d\n", gCfg.fontSize);
		}
/*		else
		if( (pScroll->state & GDK_SUPER_MASK) == GDK_SUPER_MASK ) // ? change (win key normally)
		{
			DEBUGLOGS("win+mouse wheel processing is not currently assigned to any action");
		}*/
		else
		if( (pScroll->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK ) // theme change (ctrl key normally)
		{
			DEBUGLOGS("  theme change request");

			ThemeList*     tl;
			theme_list_get(tl);
			int            ti = -1;
			ThemeEntry     te =  theme_list_fnd(tl, gCfg.themePath, gCfg.themeFile, NULL, &ti);

			DEBUGLOGP("  comparing %d list entries to current theme of %s\n", theme_list_cnt(tl), gCfg.themeFile);

			if( ti != -1 )
			{
				DEBUGLOGP("found current theme at index %d\n", ti);

				ti +=  delta;

				if( ti >= 0 && ti < theme_list_cnt(tl) )
				{
					DEBUGLOGP("changing to new theme at index %d\n", ti);

					gRun.updateSurfs = true;

					te = theme_list_nth(tl, ti);

					DEBUGLOGP("changing to new theme %s\n", te.pName->str);

					change_theme(&te, pWidget);

					if( g_pGladeXml )
					{
						GtkWidget* pWidget = glade_xml_get_widget(g_pGladeXml, "comboboxTheme");

						if( pWidget )
							gtk_combo_box_set_active(GTK_COMBO_BOX(pWidget), ti);
					}
				}
			}

			theme_list_del(tl);
		}
		else
		if( (pScroll->state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK || pScroll->state == 0 ) // window size change
		{
			DEBUGLOGS("  window size change request");

			float adjF =       (float)delta*0.0025f + 1;
			int   newW = (int)((float)gCfg.clockW*adjF);
			int   newH = (int)((float)gCfg.clockH*adjF);

			if( newW == gCfg.clockW )
				newW =  gCfg.clockW + delta;

			if( newH == gCfg.clockH )
				newH =  gCfg.clockH + delta;

			update_wnd_dim(pWidget, newW, newH, false);
			cfg::save();
		}
	}

	DEBUGLOGE;
	return TRUE;
}
/*
// -----------------------------------------------------------------------------
// The ::window-state-event will be emitted when the state of the toplevel
// window associated to the widget changes.
// -----------------------------------------------------------------------------
gboolean gui::on_window_state(GtkWidget* pWidget, GdkEventWindowState* pState, gpointer userData)
{
	bool maxState  = pState->changed_mask     & GDK_WINDOW_STATE_MAXIMIZED == GDK_WINDOW_STATE_MAXIMIZED;
	bool maximized = pState->new_window_state & GDK_WINDOW_STATE_MAXIMIZED == GDK_WINDOW_STATE_MAXIMIZED;

	const char* mc_str = maxState  ? "yes"       : "no";
	const char* ms_str = maximized ? "maximized" : "unmaximized";

	DEBUGLOGP("entry-exit - win max state changed? %s, window is %s\n", mc_str, ms_str);

	return FALSE;
}
*/
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void gui::wndMoveEnter(GtkWidget* pWidget, GdkEventButton* pButton)
{
	g_wndMovein = true;

	DEBUGLOGS("entering window movement dragging");

//#if GTK_CHECK_VERSION(3,0,0)
//	gdk_window_set_event_compression(GTK_WINDOW(gtk_widget_get_toplevel(pWidget)), FALSE);
//#endif

	DEBUGLOGS("left mouse button is now down - creating check timer");

	if( g_wndMoveT )
		g_source_remove(g_wndMoveT);

	g_wndMoveT = g_timeout_add(200, (GSourceFunc)wndMoveTime, (gpointer)pWidget);

	gtk_window_begin_move_drag(GTK_WINDOW(gtk_widget_get_toplevel(pWidget)), pButton->button,
							   pButton->x_root, pButton->y_root, pButton->time);
}

// -----------------------------------------------------------------------------
void gui::wndMoveExit(GtkWidget* pWidget)
{
	if( g_wndMovein )
	{
		g_wndMovein = false;

		DEBUGLOGS("exiting window movement dragging");

//#if GTK_CHECK_VERSION(3,0,0)
//		gdk_window_set_event_compression(GTK_WINDOW(gtk_widget_get_toplevel(pWidget)), TRUE);
//#endif

		DEBUGLOGS("storing new window position");
		cfg::save();
	}
}

// -----------------------------------------------------------------------------
gboolean gui::wndMoveTime(GtkWidget* pWidget)
{
	DEBUGLOGB;

	bool wndMoveDone = false;

	if( g_wndMovein )
	{
		gint            x, y;
		GdkModifierType m           = (GdkModifierType)0;
		GdkWindow*      pWndMouse   =  gdk_window_get_pointer(pWidget->window, &x, &y, &m);

		if( (m & GDK_BUTTON1_MASK) !=  GDK_BUTTON1_MASK )
		{
			DEBUGLOGS("left mouse button is now up during window movement");
			wndMoveDone = true;
		}
	}
	else
	{
		wndMoveDone = true;
	}

	if( wndMoveDone )
	{
		if( g_wndMoveT )
		{
			DEBUGLOGS("killing check timer since window movement is done");
			g_source_remove(g_wndMoveT);
			g_wndMoveT = 0;
		}

		wndMoveExit(pWidget);
	}

	DEBUGLOGE;
	return TRUE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void gui::wndSizeEnter(GtkWidget* pWidget, GdkEventButton* pButton, GdkWindowEdge edge)
{
	g_wndSizein = true;

	DEBUGLOGS("entering window sizing dragging");

//#if GTK_CHECK_VERSION(3,0,0)
//	gdk_window_set_event_compression(GTK_WINDOW(gtk_widget_get_toplevel(pWidget)), FALSE);
//#endif

	DEBUGLOGS("middle mouse button is now down - creating check timer");

	if( g_wndSizeT )
		g_source_remove(g_wndSizeT);

	// TODO: need to add in screen center testing

	double  xc = (double)gCfg.clockW*0.5;
	double  yc = (double)gCfg.clockH*0.5;
	bool    q1 =  pButton->x >= xc && pButton->y <= yc;
	bool    q2 =  pButton->x >= xc && pButton->y >  yc;
	bool    q3 =  pButton->x <  xc && pButton->y >  yc;
	bool    q4 =  pButton->x <  xc && pButton->y <= yc;
	int     pq =  q1 ? 1 : (q2 ? 2 : (q3 ? 3 : 4));

	switch( pq )
	{
	case 1: edge = GDK_WINDOW_EDGE_NORTH_EAST; break;
	case 2: edge = GDK_WINDOW_EDGE_SOUTH_EAST; break;
	case 3: edge = GDK_WINDOW_EDGE_SOUTH_WEST; break;
	case 4: edge = GDK_WINDOW_EDGE_NORTH_WEST; break;
	}

	GdkGeometry geom;
	int         hints = GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE;
	geom.min_width	  = MIN_CWIDTH;
	geom.min_height   = MIN_CHEIGHT;
//	geom.max_width	  = MAX_CWIDTH;
//	geom.max_height   = MAX_CHEIGHT;
	geom.max_width	  = gdk_screen_get_width (gdk_screen_get_default());
	geom.max_height   = gdk_screen_get_height(gdk_screen_get_default());

	if( (pButton->state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK ) // force square window resizing
	{
		hints          |= GDK_HINT_ASPECT;
		geom.min_aspect = 1.0;
		geom.max_aspect = 1.0;
	}

	g_wndSizeT = g_timeout_add(200, (GSourceFunc)wndSizeTime, (gpointer)pWidget);

	DEBUGLOGS("beginning resize drag");

	gtk_window_set_geometry_hints(GTK_WINDOW(gtk_widget_get_toplevel(pWidget)), pWidget, &geom, GdkWindowHints(hints));
	gtk_window_begin_resize_drag (GTK_WINDOW(gtk_widget_get_toplevel(pWidget)), edge, pButton->button, pButton->x_root, pButton->y_root, pButton->time);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void gui::wndSizeExit(GtkWidget* pWidget)
{
	if( g_wndSizein )
	{
		g_wndSizein = false;

		DEBUGLOGS("exiting window sizing dragging");

//#if GTK_CHECK_VERSION(3,0,0)
//		gdk_window_set_event_compression(GTK_WINDOW(gtk_widget_get_toplevel(pWidget)), TRUE);
//#endif

		gint                                      newW,  newH;
		gtk_window_get_size(GTK_WINDOW(pWidget), &newW, &newH);

		DEBUGLOGP("setting new window size (%d, %d)\n", newW, newH);

		update_wnd_dim(pWidget, newW, newH, true);
		cfg::save();
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
gboolean gui::wndSizeTime(GtkWidget* pWidget)
{
	DEBUGLOGB;

	bool wndSizeDone = false;

	if( g_wndSizein )
	{
		gint            x, y;
		GdkModifierType m           = (GdkModifierType)0;
		GdkWindow*      pWndMouse   =  gdk_window_get_pointer(pWidget->window, &x, &y, &m);

		if( (m & GDK_BUTTON2_MASK) !=  GDK_BUTTON2_MASK )
		{
			DEBUGLOGS("left mouse button is now up during window sizing");
			wndSizeDone = true;
		}
	}
	else
	{
		wndSizeDone = true;
	}

	if( wndSizeDone )
	{
		if( g_wndSizeT )
		{
			DEBUGLOGS("killing check timer since window sizing is done");
			g_source_remove(g_wndSizeT);
			g_wndSizeT = 0;
		}

		wndSizeExit(pWidget);
	}

	DEBUGLOGE;
	return TRUE;
}

