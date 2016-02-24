/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP     "cgui"

#define  _ENABLE_DND   // enable support for drag-n-dropping theme archives onto the clock window

#include <math.h>      // for fabs
#include <stdlib.h>    // for ?
#include <limits.h>    // for PATH_MAX
#include "platform.h"  // for GDK_... key defines
#include "clockGUI.h"  //

#include "cfgdef.h"    // for ?
#include "global.h"    // for Runtime struct, MIN_CLOCKW/H, global funcs, ...
#include "basecpp.h"   // some useful macros and functions

#include "chart.h"     // for ?
#include "copts.h"     // for copts
#include "config.h"    // for Config struct, CornerType enum, ...
#include "themes.h"    // for ?
#include "helpGUI.h"   // for hgui::init
#include "infoGUI.h"   // for igui::init
#include "utility.h"   // for g_main_end and g_del_dir
#include "settings.h"  // for ?
#include "tzoneGUI.h"  // for ?
#include "loadTheme.h" // for ?
#include "loadGUI.h"   // for lgui::okayGUI(), lgui::pWidget, ...
#include "debug.h"     // for debugging prints
#include "draw.h"      // for draw::grab, draw::render_set, draw::render, ...
#include "x.h"         // for workspace related (and _USEWKSPACE is in here)

#if _USEGTK
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
typedef void (*TOGBTNCB)(GtkToggleButton*, gpointer);

// -----------------------------------------------------------------------------
namespace cgui
{

static void initToggleBtn(const char* name, const char* event, int value, TOGBTNCB callback);

static void send_client_msg(const char* msg);
static void stopApp();

static bool theme_import(GtkWidget* pWidget, const char* uri);

static void     wndMoveEnter(GtkWidget* pWidget, GdkEventButton* pButton);
static void     wndMoveExit (GtkWidget* pWidget);
static gboolean wndMoveTime (GtkWidget* pWidget);

static void     wndSizeEnter(GtkWidget* pWidget, GdkEventButton* pButton, GdkWindowEdge edge);
static void     wndSizeExit (GtkWidget* pWidget);
static gboolean wndSizeTime (GtkWidget* pWidget);

// -----------------------------------------------------------------------------
#if _USEWKSPACE
#if 0
static void on_workspace_active_changed(WnckScreen* screen, WnckWorkspace* prevSpace, gpointer userData);
#endif
static void on_workspace_bkgrnd_changed(WnckScreen* screen, gpointer userData);
#endif

static void     on_activate_focus    (GtkWindow* pWidget, gpointer userData);
static gboolean on_button_press      (GtkWidget* pWidget, GdkEventButton* pButton, GdkWindowEdge edge);
#if !GTK_CHECK_VERSION(3,0,0)
static gboolean on_client_msg        (GtkWidget* pWidget, GdkEventClient* pEventClient, gpointer userData);
#endif
static void     on_composited_changed(GtkWidget* pWidget, gpointer userData);
static gboolean on_configure         (GtkWidget* pWidget, GdkEventConfigure* pEvent, gpointer userData);
#if 0
static gboolean on_damage            (GtkWidget* pWidget, GdkEventExpose* pExpose, gpointer userData);
#endif
static gboolean on_delete            (GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData);
static gboolean on_destroy           (GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData);

#ifdef _ENABLE_DND
static void     on_drag_data_received(GtkWidget* pWidget, GdkDragContext* pContext, gint x, gint y, GtkSelectionData* selData, guint info, guint _time, gpointer userData);
#define TGT_SEL_STR 0
#endif

static gboolean on_enter_notify(GtkWidget* pWidget, GdkEventCrossing* pCrossing, gpointer userData);

#if 0
static gboolean on_event(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData);
#endif

#if GTK_CHECK_VERSION(3,0,0)
static gboolean on_damage       (GtkWidget* pWidget, GdkEventExpose* pExpose, gpointer userData);
static gboolean on_draw         (GtkWidget* pWidget, cairo_t* cr, gpointer userData);
#else
static gboolean on_expose       (GtkWidget* pWidget, GdkEventExpose* pExpose, gpointer userData);
#endif

#if 0
static gboolean on_focus_in      (GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData);
#endif
static gboolean on_key_press     (GtkWidget* pWidget, GdkEventKey* pKey, gpointer userData);
static gboolean on_leave_notify  (GtkWidget* pWidget, GdkEventCrossing* pCrossing, gpointer userData);
static gboolean on_map           (GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData);
static gboolean on_motion_notify (GtkWidget* pWidget, GdkEventMotion* pEvent, gpointer userData);
static gboolean on_query_tooltip (GtkWidget* pWidget, gint x, gint y, gboolean keyboard_mode, GtkTooltip* pTooltip, gpointer userData);
static void     on_screen_changed(GtkWidget* pWidget, GdkScreen* pPrevScreen, gpointer userData);
static gboolean on_unmap         (GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData);
static gboolean on_wheel_scroll  (GtkWidget* pWidget, GdkEventScroll* pScroll, gpointer userData);
static gboolean on_window_state  (GtkWidget* pWidget, GdkEventWindowState* pState, gpointer userData);

static void on_popupmenu_done(GtkMenuShell* pMenuShell, gpointer userData);

static void on_alarm_activate(GtkMenuItem* pMenuItem, gpointer userData);
static void on_chart_activate(GtkMenuItem* pMenuItem, gpointer userData);
static void on_chime_activate(GtkMenuItem* pMenuItem, gpointer userData);
static void on_date_activate (GtkMenuItem* pMenuItem, gpointer userData);
static void on_help_activate (GtkMenuItem* pMenuItem, gpointer userData);
static void on_info_activate (GtkMenuItem* pMenuItem, gpointer userData);
static void on_prefs_activate(GtkMenuItem* pMenuItem, gpointer userData);
static void on_quit_activate (GtkMenuItem* pMenuItem, gpointer userData);
static void on_tzone_activate(GtkMenuItem* pMenuItem, gpointer userData);

static gboolean on_popup_close(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData);
static gboolean on_prefs_close(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData);

static GtkWidget* popup_activate(char type, const char* name, const char* widget);

static void loadMenuPopupFunc();
static bool loadMenuPopup();

// -----------------------------------------------------------------------------
static GtkWindow*  g_pPopupDlg = NULL;
static bool        g_inPopup   = false;
static char        g_nmPopup   = ' ';

static bool        g_wndMovein = false;
static guint       g_wndMoveT  = 0; // timer id

static bool        g_wndSizein = false;
static guint       g_wndSizeT  = 0; // timer id

static const char* g_ttLabel   = APP_NAME "ttip-label"; // object data id for tooltip label widget pointer

// client msgs to other clocks
static const char* g_ceRedock  = APP_NAME ".redock";    // call docks_send_update

} // namespace cgui

#endif // _USEGTK

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool cgui::init()
{
	DEBUGLOGB;

	if( !gRun.pMainWindow )
	{
		PWidget* pScrnWidget = NULL;

		if( gRun.scrsaver )
		{
#if _USEGTK
			if( pScrnWidget = gtk_window_new(GTK_WINDOW_TOPLEVEL) )
#else
			if( pScrnWidget = new PWidget() )
#endif
			{
				DEBUGLOGS("screen saver widget window created");
#if _USEGTK
				GdkGeometry hints;
				GtkWindow*  pScrnWindow =   GTK_WINDOW(pScrnWidget);
				GdkScreen*  pScreen     =   gtk_window_get_screen(pScrnWindow);
				int         sw          =   hints.min_width  = hints.max_width  = gdk_screen_get_width (pScreen);
				int         sh          =   hints.min_height = hints.max_height = gdk_screen_get_height(pScreen);
#if !GTK_CHECK_VERSION(3,0,0)
				guint16     red         =   guint16(65535.0*gCfg.bkgndRed);
				guint16     green       =   guint16(65535.0*gCfg.bkgndGrn);
				guint16     blue        =   guint16(65535.0*gCfg.bkgndBlu);
				GdkColor    color       = { 0, red, green, blue };
				gtk_widget_modify_bg                (pScrnWidget, GTK_STATE_NORMAL, &color);
#else
				GdkRGBA     color       = {    gCfg.bkgndRed, gCfg.bkgndGrn, gCfg.bkgndBlu, 1 };
				gtk_widget_override_background_color(pScrnWidget, GtkStateFlags(0), &color);
#endif
				gtk_widget_set_app_paintable    (pScrnWidget, TRUE);
				gtk_widget_set_can_focus        (pScrnWidget, FALSE);
				gtk_window_set_decorated        (pScrnWindow, FALSE);
				gtk_window_set_resizable        (pScrnWindow, FALSE);
				gtk_window_set_keep_above       (pScrnWindow, TRUE);
				gtk_window_set_accept_focus     (pScrnWindow, FALSE);
				gtk_window_set_focus_on_map     (pScrnWindow, FALSE);
				gtk_widget_set_double_buffered  (pScrnWidget, FALSE);
				gtk_window_set_skip_pager_hint  (pScrnWindow, TRUE);
				gtk_window_set_skip_taskbar_hint(pScrnWindow, TRUE);
				gtk_window_set_geometry_hints   (pScrnWindow, pScrnWidget, &hints, GdkWindowHints(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
				gtk_window_set_title            (pScrnWindow, APP_NAME);
				gtk_window_resize               (pScrnWindow, sw, sh);
				gtk_window_move                 (pScrnWindow, 0, 0);
				gtk_widget_show_now             (pScrnWidget);

				change_cursor(pScrnWidget, GDK_BLANK_CURSOR);
#endif // _USEGTK
			}
		}

#if _USEGTK
		if( gRun.pMainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL) )
//		if( gRun.pMainWindow = gRun.scrsaver ? gtk_drawing_area_new() : gtk_window_new(GTK_WINDOW_TOPLEVEL) )
#else
		if( gRun.pMainWindow = new PWidget() )
#endif
		{
			DEBUGLOGS("clock widget window created");
#if _USEGTK
			GtkWindow* pMainWindow = GTK_WINDOW(gRun.pMainWindow);

			gtk_widget_set_can_focus    (gRun.pMainWindow, FALSE);
			gtk_widget_set_app_paintable(gRun.pMainWindow, TRUE);
			gtk_window_set_default_size (pMainWindow, gCfg.clockW, gCfg.clockH);

//			if( !gRun.scrsaver )
			{
				gtk_window_set_title(pMainWindow, gRun.appName);
				gtk_window_set_accept_focus(pMainWindow, FALSE);
				gtk_window_set_focus_on_map(pMainWindow, FALSE);
				gtk_window_set_decorated(pMainWindow, FALSE);
				gtk_window_set_resizable(pMainWindow, TRUE);
			}

#if !GTK_CHECK_VERSION(3,0,0)
			gtk_widget_set_double_buffered(gRun.pMainWindow, TRUE);
#endif
//			char     clasn[128];
//			snprintf(clasn, vectsz(clasn), "class=%s%d", APP_NAME, (int)getpid());
//			gtk_window_set_wmclass(pMainWindow, APP_NAME, clasn);

//			if( !gRun.scrsaver )
			{
				GdkGeometry hints;
				GdkScreen* pScreen = gtk_window_get_screen(pMainWindow);
				hints.min_width	   = MIN_CLOCKW;
				hints.min_height   = MIN_CLOCKH;
				hints.max_width	   = gdk_screen_get_width (pScreen);
				hints.max_height   = gdk_screen_get_height(pScreen);
				hints.base_width   = gCfg.clockW;
				hints.base_height  = gCfg.clockH;

				gtk_window_set_geometry_hints(pMainWindow, gRun.pMainWindow, &hints, GdkWindowHints(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_BASE_SIZE));
			}
#ifdef DEBUGLOG
//			if( !gRun.scrsaver )
			{
				gint                                  wndX,  wndY;
				gint                                  wndW,  wndH;
				gtk_window_get_position(pMainWindow, &wndX, &wndY);
				gtk_window_get_size    (pMainWindow, &wndW, &wndH);
				DEBUGLOGP("created clock window: wndX=%d, wndY=%d, wndW=%d, wndH=%d\n", wndX, wndY, wndW, wndH);
				DEBUGLOGP("defaulted clock dims: clkW=%d, clkH=%d\n", gCfg.clockW, gCfg.clockH);
			}
#endif
			if( gRun.scrsaver )
			{
				DEBUGLOGS("bef fullscreen parent setting");
//				gtk_widget_set_has_window(gRun.pMainWindow, TRUE);
//				gtk_widget_set_has_window(gRun.pMainWindow, FALSE);
//				gtk_widget_set_parent(gRun.pMainWindow, pScrnWidget);
				gtk_widget_set_parent_window(gRun.pMainWindow, gtk_widget_get_window(pScrnWidget));
				gtk_window_set_destroy_with_parent(pMainWindow, TRUE);
				gtk_window_set_transient_for(pMainWindow, GTK_WINDOW(pScrnWidget));
				DEBUGLOGS("aft fullscreen parent setting");
			}
			else
			{
				if( gRun.minimize )
					gtk_window_iconify(pMainWindow);

				gtk_widget_set_has_tooltip(gRun.pMainWindow, gCfg.showTTips ? TRUE : FALSE);

				if( gCfg.showTTips )
				{
					if( lgui::getXml("tooltip") && lgui::okayGUI() )
					{
						DEBUGLOGS("loaded tooltip xml successfully");
						GtkWidget* pWidget = lgui::pWidget("windowTooltip");

						if( pWidget )
						{
							DEBUGLOGS("loaded tooltip widget successfully");
							GtkWindow* pTTipWnd = GTK_WINDOW(pWidget);
							gtk_widget_set_tooltip_window(gRun.pMainWindow, pTTipWnd);

							if( pWidget = lgui::pWidget("labelTooltip") )
							{
								DEBUGLOGS("loaded tooltip widget label successfully");
								g_object_set_data(G_OBJECT(pTTipWnd), g_ttLabel, gpointer(pWidget)); 
#if 0
								gtk_widget_realize(pWidget=GTK_WIDGET(pTTipWnd));
								gdk_window_set_back_pixmap(gtk_widget_get_window(pWidget), NULL, FALSE);

								if( pWidget = lgui::pWidget("hboxTooltip") )
								{
								gtk_widget_realize(pWidget);
								gdk_window_set_back_pixmap(gtk_widget_get_window(pWidget), NULL, FALSE);
								}

								if( pWidget = lgui::pWidget("imageTooltip") )
								{
								gtk_widget_realize(pWidget);
								gdk_window_set_back_pixmap(gtk_widget_get_window(pWidget), NULL, FALSE);
								}

								if( pWidget = lgui::pWidget("labelTooltip") )
								{
								gtk_widget_realize(pWidget);
								gdk_window_set_back_pixmap(gtk_widget_get_window(pWidget), NULL, FALSE);
								}
#endif
							}
						}
					}
				}
			}

			gRun.composed = gtk_widget_is_composited(gRun.pMainWindow) == TRUE;

			DEBUGLOGP("clock screen %s composited\n", gRun.composed ? "is" : "is NOT");
#else  // _USEGTK
			gRun.pMainWindow->setWindowTitle(QString(gRun.appName));
			gRun.pMainWindow->move  (gCfg.clockX, gCfg.clockY);
			gRun.pMainWindow->resize(gCfg.clockW, gCfg.clockW);
			gRun.pMainWindow->setAutoFillBackground(false);
//			gRun.composed = QX11Info::isCompositingManagerRunning();
#endif // _USEGTK
		}
	}

	if( !gRun.pMainWindow )
	{
		DEBUGLOGR(1);
		return false;
	}

	if( !gRun.composed && !gRun.scrsaver )
		draw::grab(); // TODO: move into clock window event instead, if can

	if( lgui::okayGUI() )
		dnit();

#if _USEGTK
	// make the top-level window listen for events for which it doesn't do by default

	#define CLOCK_EVENTS_MASK (GDK_EXPOSURE_MASK       | GDK_ENTER_NOTIFY_MASK        | GDK_LEAVE_NOTIFY_MASK  | \
	                           GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_MOTION_MASK | \
	                           GDK_BUTTON_PRESS_MASK   | GDK_BUTTON_RELEASE_MASK      | \
	                           GDK_SCROLL_MASK)

	gtk_widget_set_events(gRun.pMainWindow, gtk_widget_get_events(gRun.pMainWindow) | CLOCK_EVENTS_MASK);

	// main window's event processing

	GObject* pMainObj = G_OBJECT(gRun.pMainWindow);

	g_signal_connect(pMainObj, "activate-focus",      G_CALLBACK(on_activate_focus),     NULL);
	g_signal_connect(pMainObj, "button-press-event",  G_CALLBACK(on_button_press),       NULL);
#if !GTK_CHECK_VERSION(3,0,0)
	g_signal_connect(pMainObj, "client-event",        G_CALLBACK(on_client_msg),         NULL);
#endif
	g_signal_connect(pMainObj, "composited-changed",  G_CALLBACK(on_composited_changed), NULL);
	g_signal_connect(pMainObj, "configure-event",     G_CALLBACK(on_configure),          NULL);
#if 0
	g_signal_connect(pMainObj, "damage-event",        G_CALLBACK(on_damage),             NULL);
#endif
	g_signal_connect(pMainObj, "delete-event",        G_CALLBACK(on_delete),             NULL);
	g_signal_connect(pMainObj, "destroy_event",       G_CALLBACK(on_destroy),            NULL);
	g_signal_connect(pMainObj, "enter-notify-event",  G_CALLBACK(on_enter_notify),       NULL);
#if 0
	g_signal_connect(pMainObj, "event",               G_CALLBACK(on_event),              NULL);
#endif
#if GTK_CHECK_VERSION(3,0,0)
	g_signal_connect(pMainObj, "damage-event",        G_CALLBACK(on_damage),             NULL);
	g_signal_connect(pMainObj, "draw",                G_CALLBACK(on_draw),               NULL);
#else
	g_signal_connect(pMainObj, "expose-event",        G_CALLBACK(on_expose),             NULL);
#endif
#if 0
	g_signal_connect(pMainObj, "focus-in-event",      G_CALLBACK(on_focus_in),           NULL);
#endif
	g_signal_connect(pMainObj, "key-press-event",     G_CALLBACK(on_key_press),          NULL);
	g_signal_connect(pMainObj, "leave-notify-event",  G_CALLBACK(on_leave_notify),       NULL);
	g_signal_connect(pMainObj, "map-event",           G_CALLBACK(on_map),                NULL);
	g_signal_connect(pMainObj, "query-tooltip",       G_CALLBACK(on_query_tooltip),      NULL);
	g_signal_connect(pMainObj, "screen-changed",      G_CALLBACK(on_screen_changed),     NULL);
	g_signal_connect(pMainObj, "unmap-event",         G_CALLBACK(on_unmap),              NULL);
	g_signal_connect(pMainObj, "scroll-event",        G_CALLBACK(on_wheel_scroll),       NULL);
	g_signal_connect(pMainObj, "window-state-event",  G_CALLBACK(on_window_state),       NULL);

	if( gRun.scrsaver )
	g_signal_connect(pMainObj, "motion-notify-event", G_CALLBACK(on_motion_notify),      NULL);
#else
//	gRun.pMainWindow->connect(SIGNAL(showEvent), SLOT(on_map));
#endif // _USEGTK

	// TODO: move all of this somewhere else more appropriate? call from main?
	//       since this is 'putting the cart before the horse'?

	// set the initial theme to be used at startup

	DEBUGLOGP("startup theme path is '%s'\n", gCfg.themePath);
	DEBUGLOGP("startup theme file is '%s'\n", gCfg.themeFile);

	ThemeEntry ts;
	bool       direct = gCfg.themePath[0] && gCfg.themeFile[0];

	memset(&ts, 0, sizeof(ts));

	if( direct )
	{
		DEBUGLOGS("  startup theme directly accessible");

		ts.pPath  = g_string_new(gCfg.themePath);
		ts.pFile  = g_string_new(gCfg.themeFile);
		ts.pName  = g_string_new("");
		ts.pModes = g_string_new("");
	}
	else
	if( gCfg.themeFile[0] )
	{
		DEBUGLOGS("  startup theme must be searched for");
		DEBUGLOGS("  creating current theme list");

		ThemeEntry     te;
		ThemeList*     tl;
		theme_list_get(tl);

		// TODO: can now call a theme_list_find function to replace all of this?

		for( te = theme_list_beg(tl); !theme_list_end(tl); te = theme_list_nxt(tl) )
		{
			if( strcmp(gCfg.themeFile, te.pFile->str) == 0 ||
				strcmp(gCfg.themeFile, te.pName->str) == 0 )
			{
				DEBUGLOGP("  theme found - %s, %s, %s\n", te.pName->str, te.pPath->str, te.pFile->str);
				theme_ntry_cpy(ts, te);
				direct = true;
				break;
			}
		}

		DEBUGLOGS("  destroying current theme list");
		theme_list_del(tl);
	}

	if( ts.pPath && ts.pPath->str && ts.pPath->str[0] )
	{
		DEBUGLOGS("  changing startup theme to one found");
		change_theme(NULL, ts, false);

		if( direct )
			theme_ntry_del(ts);
	}
	else
	{
		DEBUGLOGS("  theme not found in list");
		change_theme(NULL, ts, false);
	}

	DEBUGLOGE;
	return true;
}

// -----------------------------------------------------------------------------
void cgui::dnit(bool clrPopup, bool unloadGUI)
{
	DEBUGLOGB;

#if _USEGTK
//	lgui::dnit();

	if( clrPopup )
	{
		if( g_pPopupDlg )
		{
			if( GTK_IS_WIDGET(g_pPopupDlg) )
			{
				DEBUGLOGS("destroying popup dbox");
				gtk_widget_destroy(GTK_WIDGET(g_pPopupDlg));
			}

//			DEBUGLOGS("unrefing popup dbox");
//			g_object_unref(g_pPopupDlg);

//			DEBUGLOGS("dniting lgui");
//			lgui::dnit();
		}

		DEBUGLOGS("clearing popup tracking vals");
		g_pPopupDlg = NULL;
		g_inPopup   = false;
		g_nmPopup   = ' ';
	}

	DEBUGLOGS("dniting lgui");
	lgui::dnit(unloadGUI);
#endif // _USEGTK

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cgui::initDragDrop()
{
#if _USEGTK
#ifdef _ENABLE_DND
//	static char           target_type[] =     "text/uri-list";
//	static GtkTargetEntry target_list[] = { { target_type, GTK_TARGET_OTHER_APP, TGT_SEL_STR } };

	DEBUGLOGB;
	DEBUGLOGS("bef setting drag destination");

//	gtk_drag_dest_set(gRun.pMainWindow, GTK_DEST_DEFAULT_ALL, target_list, G_N_ELEMENTS(target_list), GDK_ACTION_COPY);
	gtk_drag_dest_set(gRun.pMainWindow, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);
	g_signal_connect (G_OBJECT(gRun.pMainWindow), "drag_data_received", G_CALLBACK(on_drag_data_received), NULL);
	gtk_drag_dest_add_uri_targets(gRun.pMainWindow);

	DEBUGLOGS("aft setting drag destination");
	DEBUGLOGE;
#endif
#endif
}

#if _USEGTK
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cgui::initToggleBtn(const char* name, const char* event, int value, TOGBTNCB callback)
{
	if( lgui::okayGUI() )
	{
		GtkWidget* pButton = lgui::pWidget(name);

		if( pButton )
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pButton), value);
			g_signal_connect(G_OBJECT(pButton), event, G_CALLBACK(callback), gRun.pMainWindow);
		}
#if 0
		else
		{
			DEBUGLOGP("failed to get/set toggle button %s's value to %d and connect its %s event\n", name, value, event);
		}
#endif
	}
}
#endif // _USEGTK

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cgui::initWorkspace()
{
	DEBUGLOGB;

	if( !gCfg.sticky && gCfg.clockWS )
	{
#if _USEGTK
#if 0
		int cws =   g_current_workspace(gRun.pMainWindow);
		if( cws == (gCfg.clockWS & cws) )
			g_window_workspace(gRun.pMainWindow, cws);
#else
		g_window_workspace(gRun.pMainWindow, gCfg.clockWS-1);
#endif
#if _USEWKSPACE
		WnckScreen*      screen = wnck_screen_get_default();
#if 0
		g_signal_connect(screen, "active-workspace-changed", G_CALLBACK(on_workspace_active_changed), NULL);
#endif
		g_signal_connect(screen, "background-changed",       G_CALLBACK(on_workspace_bkgrnd_changed), NULL);
#endif // _USEWKSPACE
#endif // _USEGTK
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void cgui::dnitWorkspace()
{
#if _USEWKSPACE
	DEBUGLOGB;
#if GTK_CHECK_VERSION(3,0,0)
	wnck_shutdown();
#endif
	DEBUGLOGE;
#endif
}

#if _USEGTK
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cgui::send_client_msg(const char* msg)
{
	DEBUGLOGB;

#if !GTK_CHECK_VERSION(3,0,0)
	DEBUGLOGS("bef creating a new event");
	GdkEvent* e  =  gdk_event_new(GDK_CLIENT_EVENT);
	DEBUGLOGS("aft creating a new event");

	if( e )
	{
		GdkEventClient* ec = (GdkEventClient*)e;

		DEBUGLOGS("bef creating a new atom");
		GdkAtom a = gdk_atom_intern_static_string(msg);
		DEBUGLOGS("aft creating a new atom");

		if( a )
		{
			// reset all of these just to make sure
			ec->type         = GDK_CLIENT_EVENT;
			ec->window       = NULL;
			ec->send_event   = TRUE;
			ec->message_type =  a;
			ec->data_format  = 32;
			ec->data.l[0]    =  0;
			ec->data.l[1]    =  0;
			ec->data.l[2]    =  0;
			ec->data.l[3]    =  0;
			ec->data.l[4]    =  0;

			DEBUGLOGS("bef sending the client msg event to all");
			gdk_event_send_clientmessage_toall(e);
			DEBUGLOGS("aft sending the client msg event to all");
		}

		gdk_event_free(e);
	}
#endif // !GTK_CHECK_VERSION(3,0,0)

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cgui::stopApp()
{
	DEBUGLOGB;
	gRun.appStop = true;

	if( gCfg.showInTasks )
	{
		DEBUGLOGS("bef sending the 'redock' client msg");
		send_client_msg(g_ceRedock);
		DEBUGLOGS("aft sending the 'redock' client msg");
	}

	if( gRun.pMainWindow )
	{
		if( gRun.scrsaver )
		{
			GtkWindow* pScrnWindow = gtk_window_get_transient_for(GTK_WINDOW(gRun.pMainWindow));
			gtk_widget_destroy(GTK_WIDGET(pScrnWindow));
			DEBUGLOGS("bkgnd window widget destroyed");
			pScrnWindow = NULL;
		}

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
void cgui::loadMenuPopupFunc()
{
	loadMenuPopup();
}

// -----------------------------------------------------------------------------
bool cgui::loadMenuPopup()
{
	DEBUGLOGB;

	if(!lgui::okayGUI() )
	{
		DEBUGLOGS("exit(1)");
//		g_sync_threads_gui_beg(); // TODO: necessary?
		dnit();
//		g_sync_threads_gui_end();
		return false;
	}

	DEBUGLOGS("before popupMenu widget loading");
//	g_sync_threads_gui_beg();
	GtkWidget* pPopUpMenu = lgui::pWidget("popUpMenu");
//	g_sync_threads_gui_end();
	DEBUGLOGS("after popupMenu widget loading");

	if( !pPopUpMenu )
	{
		DEBUGLOGS("exit(2)");
		return false;
	}

	static const char* mis[] =
	{
		"prefs", "chart", "alarm", "chime", "date", "info", "help", "quit", "tzone"
	};

	static GCallback   cbs[] =
	{
		G_CALLBACK(on_prefs_activate), G_CALLBACK(on_chart_activate),
		G_CALLBACK(on_alarm_activate), G_CALLBACK(on_chime_activate),
		G_CALLBACK(on_date_activate),  G_CALLBACK(on_info_activate),
		G_CALLBACK(on_help_activate),  G_CALLBACK(on_quit_activate),
		G_CALLBACK(on_tzone_activate)
	};

	char       tstr[1024];
	GtkWidget* pMenuItem;

	DEBUGLOGS("before popupMenu item widget loading");
	for( size_t i = 0; i < vectsz(mis); i++ )
	{
		snprintf(tstr, vectsz(tstr), "%sMenuItem", mis[i]);

//		g_sync_threads_gui_beg();
		pMenuItem = lgui::pWidget(tstr);

		if( pMenuItem )
			g_signal_connect(G_OBJECT(pMenuItem), "activate", cbs[i], gRun.pMainWindow);
//		g_sync_threads_gui_end();
	}
	DEBUGLOGS("after popupMenu item widget loading");

	g_signal_connect(G_OBJECT(pPopUpMenu), "selection-done", G_CALLBACK(on_popupmenu_done), gRun.pMainWindow);

	DEBUGLOGS("before popupMenu displaying");
//	g_sync_threads_gui_beg();
	gtk_menu_popup(GTK_MENU(pPopUpMenu), NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
//	g_sync_threads_gui_end();
	DEBUGLOGS("after popupMenu displaying");

	DEBUGLOGS("exit(3)");
	return true;
}

// -----------------------------------------------------------------------------
void cgui::on_activate_focus(GtkWindow* pWidget, gpointer userData)
{
	DEBUGLOGB;
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void cgui::on_alarm_activate(GtkMenuItem* pMenuItem, gpointer userData)
{
	DEBUGLOGB;

	GtkWidget* pDialog =
		gtk_message_dialog_new((GtkWindow*)userData,
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
			"No alarm settings/usage available yet.\n\nSorry.");

	if( pDialog )
	{
		gtk_window_set_transient_for(GTK_WINDOW(pDialog), GTK_WINDOW(userData));
		gtk_window_set_position(GTK_WINDOW(pDialog), GTK_WIN_POS_CENTER);
		gtk_window_set_title(GTK_WINDOW(pDialog), gRun.appName);
		gtk_dialog_run(GTK_DIALOG(pDialog));
		on_popup_close(pDialog, NULL, userData);
		if( GTK_IS_WIDGET(pDialog) )
			gtk_widget_destroy(pDialog);
	}

	DEBUGLOGE;
}
#if 0
// -----------------------------------------------------------------------------
void cgui::on_alpha_screen_changed(GtkWidget* pWidget, GdkScreen* pOldScreen, GtkWidget* pLabel)
{
#if 0
	GdkScreen*   pScreen   = gtk_widget_get_screen(pWidget);
	GdkColormap* pColormap = gdk_screen_get_rgba_colormap(pScreen);

	if( pColormap == NULL )
		pColormap =  gdk_screen_get_rgb_colormap(pScreen);

	gtk_widget_set_colormap(pWidget, pColormap);
#endif
	update_colormap(pWidget);
}
#endif
// -----------------------------------------------------------------------------
gboolean cgui::on_button_press(GtkWidget* pWidget, GdkEventButton* pButton, GdkWindowEdge edge)
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
		bool nogo = true;

		if( lgui::okayGUI() )
		{
			if( !g_inPopup )
			{
				DEBUGLOGS("clearing previously loaded lgui context menu");
				dnit();
			}
		}

		if(!lgui::okayGUI() )
		{
			DEBUGLOGS("attempting to load/popup lgui context menu");

			if( true ) // true for single threaded, false for multi-threaded
			{
				nogo = !lgui::getXml("menu") || !loadMenuPopup();
			}
			else
			{
				lgui::getXml("menu", NULL, loadMenuPopupFunc);
				nogo = false;
			}
#if 0
			if( lgui::okayGUI() )
			{
				GtkWidget* pPopUpMenu = lgui::pWidget("popUpMenu");

				if( pPopUpMenu )
				{
					static const char* mis[] =
					{ "prefs", "chart", "alarm", "chime", "date", "info", "help", "quit", "tzone" };

					static GCallback   cbs[] =
					{
						G_CALLBACK(on_prefs_activate), G_CALLBACK(on_chart_activate),
						G_CALLBACK(on_alarm_activate), G_CALLBACK(on_chime_activate),
						G_CALLBACK(on_date_activate),  G_CALLBACK(on_info_activate),
						G_CALLBACK(on_help_activate),  G_CALLBACK(on_quit_activate),
						G_CALLBACK(on_tzone_activate)
					};

					char       tstr[1024];
					GtkWidget* pMenuItem;

					for( size_t i = 0; i < vectsz(mis); i++ )
					{
						snprintf(tstr, vectsz(tstr), "%sMenuItem", mis[i]);
						pMenuItem = lgui::pWidget(tstr);
						g_signal_connect(G_OBJECT(pMenuItem), "activate", cbs[i], gRun.pMainWindow);
					}

					gtk_menu_popup(GTK_MENU(pPopUpMenu), NULL, NULL, NULL, NULL, pButton->button, pButton->time);
				}
				else
				{
					DEBUGLOGS("didn't load/popup lgui context menu (3)");
					dnit();
				}
			}
			else
			{
				DEBUGLOGS("didn't load/popup lgui context menu (2)");
			}
#endif
		}

		if( nogo )
		{
			DEBUGLOGS("didn't load/popup lgui context menu (1)");

			GtkWidget* pPopUpMenu = gtk_menu_new();
			GtkWidget* pMenuItem  = gtk_menu_item_new_with_label("Quit");

			if( pPopUpMenu && pMenuItem )
			{
				DEBUGLOGS("created popup menu and its quit item");

				gtk_widget_show(pMenuItem);
				gtk_widget_show(pPopUpMenu);

				gtk_menu_shell_append(GTK_MENU_SHELL(pPopUpMenu), pMenuItem);

				g_signal_connect(G_OBJECT(pMenuItem),  "activate",       G_CALLBACK(on_quit_activate),  gRun.pMainWindow);
				g_signal_connect(G_OBJECT(pPopUpMenu), "selection-done", G_CALLBACK(on_popupmenu_done), gRun.pMainWindow);

				gtk_menu_popup  (GTK_MENU(pPopUpMenu), NULL, NULL, NULL, NULL, pButton->button, pButton->time);
			}
		}
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
static void on_chart_realize(GtkWidget* pWidget, gpointer data)
{
	DEBUGLOGB;

	if( lgui::okayGUI() )
	{
		GtkWidget* pChart = lgui::pWidget("drawingAreaChart");

		if( pChart )
			chart::init(pChart);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void cgui::on_chart_activate(GtkMenuItem* pMenuItem, gpointer data)
{
	DEBUGLOGB;

	GtkWidget* pDialog = popup_activate('c', "chart", "windowChart");

	if( pDialog )
	{
		g_signal_connect(G_OBJECT(pDialog), "delete_event", G_CALLBACK(on_popup_close),   NULL);
		g_signal_connect(G_OBJECT(pDialog), "realize",      G_CALLBACK(on_chart_realize), NULL);
		setPopup(pDialog, true, 'c');
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void cgui::on_chime_activate(GtkMenuItem* pMenuItem, gpointer userData)
{
	DEBUGLOGB;

	GtkWidget* pDialog =
		gtk_message_dialog_new(GTK_WINDOW(userData),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
			"No chime settings/usage available yet.\n\nSorry.");

	if( pDialog )
	{
		gtk_window_set_transient_for(GTK_WINDOW(pDialog), GTK_WINDOW(userData));
		gtk_window_set_position(GTK_WINDOW(pDialog), GTK_WIN_POS_CENTER);
		gtk_window_set_title(GTK_WINDOW(pDialog), gRun.appName);
		gtk_dialog_run(GTK_DIALOG(pDialog));
		on_popup_close(pDialog, NULL, userData);
		if( GTK_IS_WIDGET(pDialog) )
			gtk_widget_destroy(pDialog);
	}

	DEBUGLOGE;
}

#if !GTK_CHECK_VERSION(3,0,0)
// -----------------------------------------------------------------------------
gboolean cgui::on_client_msg(GtkWidget* pWidget, GdkEventClient* pEventClient, gpointer userData)
{
	DEBUGLOGB;

	if( pEventClient && pEventClient->message_type )
	{
		DEBUGLOGS("received a client event msg");
		char* an = gdk_atom_name(pEventClient->message_type);

		if( an )
		{
			DEBUGLOGP("atom name is '%s'\n", an);

			if( strstr(an, APP_NAME) != 0 )
			{
				DEBUGLOGS("msg is from a " APP_NAME " clock");

				if( strcmp(an, g_ceRedock) == 0 )
				{
					DEBUGLOGS("updating my dock icon & label as suggested");
					docks_send_update();
				}
			}

			g_free(an);
		}
	}

	DEBUGLOGE;
	return FALSE;
}
#endif // !GTK_CHECK_VERSION(3,0,0)

// -----------------------------------------------------------------------------
// The ::composited-changed signal is emitted when the composited status of
// widget s screen changes. See gdk_screen_is_composited().
// -----------------------------------------------------------------------------
void cgui::on_composited_changed(GtkWidget* pWidget, gpointer userData)
{
	if( gRun.appStop )
		return;

	DEBUGLOGB;

//	gRun.composed = gdk_screen_is_composited(gtk_widget_get_screen(pWidget)) == TRUE;
	gRun.composed = gtk_widget_is_composited(pWidget) == TRUE;

	DEBUGLOGP("window screen %s composited\n", gRun.composed ? "is" : "is NOT");

	if( !gRun.appStart && !gRun.updating )
	{
		// TODO: don't really need to do everything that this func does
		//       just need to make the necessary adjustments in a worker thread

		DEBUGLOGS("calling update_wnd_dim async");
//		update_wnd_dim(pWidget, gCfg.clockW, gCfg.clockH, true);
//		update_wnd_dim(pWidget, gCfg.clockW, gCfg.clockH, gCfg.clockW, gCfg.clockH, true);
//		update_wnd_dim(pWidget, gCfg.clockW, gCfg.clockH, gCfg.clockW, gCfg.clockH, false, false);
		update_wnd_dim(pWidget, gCfg.clockW, gCfg.clockH, gCfg.clockW, gCfg.clockH, true,  false);
//		update_wnd_dim(pWidget, gCfg.clockW, gCfg.clockH, false);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// The ::configure-event signal is emitted when the size, position or stacking
// of the widget's window has changed.
// -----------------------------------------------------------------------------
gboolean cgui::on_configure(GtkWidget* pWidget, GdkEventConfigure* pEvent, gpointer userData)
{
	if( gRun.appStop )
		return FALSE;

	DEBUGLOGB;

	Config     tcfg;
	GtkWidget* pWindow = pWidget;
	gint       newX    = pEvent->x;
	gint       newY    = pEvent->y;
	gint       newW    = pEvent->width;
	gint       newH    = pEvent->height;

#ifdef DEBUGLOG
	{
		gint                                              oldX,  oldY;
		gint                                              oldW,  oldH;
		gtk_window_get_position(GTK_WINDOW(pWindow),     &oldX, &oldY);
		gtk_window_get_size    (GTK_WINDOW(pWindow),     &oldW, &oldH);
		DEBUGLOGP("oldX=%d, oldY=%d, oldW=%d, oldH=%d\n", oldX,  oldY, oldW, oldH);
	}
#endif

	bool newPos  = newX != gCfg.clockX ||  newY   != gCfg.clockY;
	bool newSize = newW != gCfg.clockW ||  newH   != gCfg.clockH;
	bool chgGUI  = lgui::okayGUI()     && (newPos || newSize) && !prefs::opend();

	DEBUGLOGP("newX=%d, newY=%d, newW=%d, newH=%d\n", newX, newY, newW, newH);
	DEBUGLOGP("newPos=%s, newSize=%s\n",              newPos ? "yes" : "no", newSize ? "yes" : "no");

	if( newSize )
	{
		if( g_wndSizein )
		{
			gRun.drawScaleX = (double)newW/(double)gCfg.clockW;
			gRun.drawScaleY = (double)newH/(double)gCfg.clockH;
		}
	}

	if( g_wndSizein )
		draw::render_set(pWindow);

	if( chgGUI )
	{
		prefs::open(true);

		tcfg = gCfg;

		if( newSize )
		{
			tcfg.clockW *= gRun.drawScaleX;
			tcfg.clockH *= gRun.drawScaleY;
		}

		cfg::cnvp(tcfg, true);
	}

	if( newPos )
	{
#ifdef  DEBUGLOG
		{
			static int n = 0;
			DEBUGLOGP("position change # %d: newX=%d, newY=%d\n", ++n, newX, newY);
		}
#endif
#ifdef  DEBUGLOG
		{
			static int      n  =   0;
			static GTimeVal bt = { 0, 0 }, ct = { 0, 0 };

			g_get_current_time(&ct);

			guint32  et = gtk_get_current_event_time();
			gboolean pg = gdk_pointer_is_grabbed();

			DEBUGLOGP("position change # %d (%d, %d, %d.%d, %d.%d)\n", ++n, (int)pg, (int)et, (int)ct.tv_sec, (int)ct.tv_usec, (int)bt.tv_sec, (int)bt.tv_usec);
			bt = ct;
		}
#endif
		DEBUGLOGP("new pos: old(%d %d), new(%d %d)\n", gCfg.clockX, gCfg.clockY, newX, newY);

		gCfg.clockX = newX;
		gCfg.clockY = newY;

		if( chgGUI )
		{
			GtkWidget* pWidget;
			if( pWidget = lgui::pWidget("spinbuttonX") )
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockX);

			if( pWidget = lgui::pWidget("spinbuttonY") )
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockY);
		}
	}

	if( newSize )
	{
#ifdef  DEBUGLOG
		{
			static int n = 0;
			DEBUGLOGP("size change # %d: newW=%d, newH=%d\n", ++n, newW, newH);
		}
#endif
		DEBUGLOGP("new dim: old(%d %d), new(%d %d), scl(%f %f)\n", gCfg.clockW, gCfg.clockH, newW, newH, gRun.drawScaleX, gRun.drawScaleY);

		if( chgGUI )
		{
			GtkWidget* pWidget;
			if( pWidget = lgui::pWidget("spinbuttonWidth") )
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockW);

			if( pWidget = lgui::pWidget("spinbuttonHeight") )
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockH);
		}

		DEBUGLOGS("issuing a redraw request");
		gtk_widget_queue_draw(pWindow);
	}

	if( chgGUI )
		prefs::open(false);

	DEBUGLOGE;
	return FALSE;
}

#if 0
// -----------------------------------------------------------------------------
// The ::damage-event signal is emitted when a redirected window belonging to
// widget gets drawn into. The region/area members of the event shows what area
// of the redirected drawable was drawn into.
// -----------------------------------------------------------------------------
#if !GTK_CHECK_VERSION(3,0,0)
gboolean cgui::on_damage(GtkWidget* pWidget, GdkEventExpose* pExpose, gpointer userData)
{
	DEBUGLOGBF;
	gboolean ret = on_expose(pWidget, pExpose, userData);
	DEBUGLOGEF;
	return   ret;
}
#endif
#endif

// -----------------------------------------------------------------------------
void cgui::on_date_activate(GtkMenuItem* pMenuItem, gpointer data)
{
	DEBUGLOGB;

	GtkWidget* pDialog = popup_activate('d', "date", "windowDate");

	if( pDialog )
	{
		GtkCalendar* pDate = (GtkCalendar*)lgui::pWidget("calendar1");

		if( pDate )
		{
			gtk_calendar_select_month(pDate, gRun.timeCtm.tm_mon, gRun.timeCtm.tm_year+1900);
			gtk_calendar_select_day  (pDate, gRun.timeCtm.tm_mday);
		}

		g_signal_connect(G_OBJECT(pDialog), "delete_event", G_CALLBACK(on_popup_close), NULL);
		setPopup(pDialog, true, 'd');
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
gboolean cgui::on_delete(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	DEBUGLOGB;
	stopApp();
	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean cgui::on_destroy(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	DEBUGLOGB;
	stopApp();
	DEBUGLOGE;
	return FALSE;
}

#ifdef _ENABLE_DND
// -----------------------------------------------------------------------------
void cgui::on_drag_data_received(GtkWidget* pWidget, GdkDragContext* pContext, gint x, gint y, GtkSelectionData* pSelData, guint info, guint _time, gpointer userData)
{
	DEBUGLOGB;

//	bool update = false;

	DEBUGLOGP("widget name is %s\n", gtk_widget_get_name(pWidget));

	if( gdk_drag_context_get_suggested_action(pContext) == GDK_ACTION_COPY &&
		pSelData && gtk_selection_data_get_length(pSelData) > 0 &&
		info == TGT_SEL_STR )
	{
		char* strs = (char*)gtk_selection_data_get_data(pSelData);

		if( strs )
		{
			DEBUGLOGP("string received is\n*****\n%s\n*****\n", strs);

			char*   eol = strs;
			while( (eol = strchr(eol, '\n')) && *eol )
			{
				*eol = '\0';
				 eol++;
			}

			char*  curs =   strs;
			while( curs && *curs )
			{
				int snxt = strlen(curs) + 1;
				theme_import(pWidget, curs);
				curs += snxt;
				break; // only one dropped theme imported for now
			}
		}
		else
		{
			DEBUGLOGS("no string received");
		}
	}

	gtk_drag_finish(pContext, TRUE, FALSE, _time);

	// TODO: need func to just add new theme above instead of rebuild here
#if 0
	if( update )
	{
		// TODO: anything to do here now that a global theme list is no longer kept?
	}
#endif
	DEBUGLOGE;
}
#endif // _ENABLE_DND

#if GTK_CHECK_VERSION(3,0,0)
// -----------------------------------------------------------------------------
gboolean cgui::on_draw(GtkWidget* pWidget, cairo_t* cr, gpointer userData)
{
	if( gRun.appStop )
		return FALSE;

	DEBUGLOGB;

	draw::render(cr);

	DEBUGLOGE;
	return FALSE;
}
#endif

// -----------------------------------------------------------------------------
// The ::enter-notify-event is emitted when the pointer enters the widget's
// window.
// -----------------------------------------------------------------------------
gboolean cgui::on_enter_notify(GtkWidget* pWidget, GdkEventCrossing* pCrossing, gpointer userData)
{
	if( gRun.appStop )
		return FALSE;

	DEBUGLOGB;

	gdk_window_set_opacity(gtk_widget_get_window(gRun.pMainWindow), 1.0);
//	wndMoveExit(pWidget);
	gRun.isMousing = true;

	if( gCfg.showTTips )
		update_ts_text(true);

	DEBUGLOGE;
	return FALSE;
}

#if 0
// -----------------------------------------------------------------------------
gboolean cgui::on_event(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	DEBUGLOGE;
	return FALSE;
}
#endif

// -----------------------------------------------------------------------------
// The ::expose-event signal is emitted when an area of a previously obscured
// GdkWindow is made visible and needs to be redrawn.
// -----------------------------------------------------------------------------
#if GTK_CHECK_VERSION(3,0,0)
gboolean cgui::on_damage(GtkWidget* pWidget, GdkEventExpose* pExpose, gpointer userData)
#else
gboolean cgui::on_expose(GtkWidget* pWidget, GdkEventExpose* pExpose, gpointer userData)
#endif
{
	if( gRun.appStop )
		return FALSE;
#if 0
	{
		// the first call is always for a bogus window position and size
		// apparently it's for the "system's" (window manager's?) defaults
		// these values cause issues later on during the startup, namely,
		// there's a 'flash' of something being displayed instead of a solid
		// clock displayed at the correct position and size - so we punt it

		static bool
			first = true;
		if( first )
		{
			first = false;
			return  FALSE;
		}
	}
#endif
	DEBUGLOGB;
//	DEBUGLOGP(  %d upcoming exposes\n", pExpose->count);

//#ifdef DEBUGLOG
	{
		static int ct = 0;
//		if( ct < 10 ) DEBUGLOGP("rendering(%d)\n", ++ct);
		DEBUGLOGP("rendering(%d)\n", ++ct);
	}
//#endif

#ifdef DEBUGLOG
	{
		gint                                              wndX,  wndY;
		gint                                              wndW,  wndH;
		gtk_window_get_position(GTK_WINDOW(pWidget),     &wndX, &wndY);
		gtk_window_get_size    (GTK_WINDOW(pWidget),     &wndW, &wndH);
		DEBUGLOGP("wndX=%d, wndY=%d, wndW=%d, wndH=%d\n", wndX,  wndY, wndW, wndH);
	}
#endif

//	DEBUGLOGZ("bef calling draw::render", 5000);

//	NOTE: from gdk_cairo_create ref doc:
//		Note that due to double-buffering, Cairo contexts created in a GTK+ expose
//		event handler cannot be cached and reused between different expose events.

#if 0
	if( gRun.appStart )
	{
//		draw::render(pWidget);
//		draw::clear(pWidget);
		DEBUGLOGR(1);
		return TRUE;
	}
	else
#endif
//	{
//		draw::render_set(pWidget); // can't do - see above note
		draw::render(pWidget);
//	}

//	DEBUGLOGZ("aft calling draw::render", 5000);

	DEBUGLOGE;
//	return FALSE;
	return TRUE;
}
#if 0
// -----------------------------------------------------------------------------
// The ::focus-in-event signal is emitted when the keyboard focus enters the
// widget's window.
// -----------------------------------------------------------------------------
gboolean cgui::on_focus_in(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	if( gRun.appStop )
		return FALSE;

	DEBUGLOGB;

	// focus-in is not fired off on all dms/wms/gtklibs/gdklibs/xlibs/who-knows
	// seems to work for xfce's dm/wm

	// can't remember why this is needed anyway, so disabled for now

//	wndMoveExit(pWidget);

	DEBUGLOGE;
	return FALSE;
}
#endif
// -----------------------------------------------------------------------------
void cgui::on_help_activate(GtkMenuItem* pMenuItem, gpointer userData)
{
	DEBUGLOGB;

	GtkWidget* pDialog = popup_activate('h', "help", "helpDialog");

	if( pDialog )
	{
		g_signal_connect(G_OBJECT(pDialog), "response", G_CALLBACK(on_popup_close), NULL);
		hgui::init(pDialog, 'h');
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void cgui::on_info_activate(GtkMenuItem* pMenuItem, gpointer userData)
{
	DEBUGLOGB;

	GtkWidget* pDialog = popup_activate('i', "info", "infoDialog");

	if( pDialog )
	{
		g_signal_connect(G_OBJECT(pDialog), "response", G_CALLBACK(on_popup_close), NULL);
		igui::init(pDialog, 'i');
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// The ::key-press-event signal is emitted when a key is pressed.
// -----------------------------------------------------------------------------
gboolean cgui::on_key_press(GtkWidget* pWidget, GdkEventKey* pKey, gpointer userData)
{
	if( gRun.appStop )
		return FALSE;

	DEBUGLOGB;
	DEBUGLOGP("  keyval is %d\n", (int)pKey->keyval);

	if( pKey->type == GDK_KEY_PRESS )
	{
		if( gRun.scrsaver )
		{
			stopApp();
			return FALSE;
		}

		bool altd  = (pKey->state & GDK_MOD1_MASK)    == GDK_MOD1_MASK;
		bool ctrld = (pKey->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK;
		bool shftd = (pKey->state & GDK_SHIFT_MASK)   == GDK_SHIFT_MASK;

		switch( pKey->keyval )
		{
		case HOTKEY_QUIT1:
//		case HOTKEY_QUIT2:
			if( gRun.maximize )
				pKey->keyval = HOTKEY_MAXIMIZE;
		}

		switch( pKey->keyval )
		{
		case GDK_KEY_Escape:
		case HOTKEY_QUIT1:
//		case HOTKEY_QUIT2:
			stopApp();
			break;

		case GDK_KEY_Menu:
			on_prefs_activate(NULL, gRun.pMainWindow);
			break;

		case GDK_KEY_Up:
		case GDK_KEY_Down:
		case GDK_KEY_Left:
		case GDK_KEY_Right:
			if( ctrld )
			{
				GdkEventScroll es;
				bool keyu    = pKey->keyval == GDK_KEY_Up;
				bool keyd    = pKey->keyval == GDK_KEY_Down;
				bool keyl    = pKey->keyval == GDK_KEY_Left;
				bool keyr    = pKey->keyval == GDK_KEY_Right;
				es.state     = pKey->state;
				es.direction = keyu ? GDK_SCROLL_UP : (keyd ? GDK_SCROLL_DOWN : (keyl ? GDK_SCROLL_LEFT : GDK_SCROLL_RIGHT));
				DEBUGLOGS("ctrl+up/down arrow key input - calling on_wheel_scroll");
				on_wheel_scroll(pWidget, &es, NULL);
			}
			break;

		// settings based functionality - TODO: should these be 'cfg-saved'?

		case HOTKEY_HELP:
			// TODO: add in logic to popup main help dbox
			break;

		case HOTKEY_TWELVE:
			if( gCfg.show24Hrs == true )
			{
				prefs::on_24_hour_toggled(NULL, pWidget);
				prefs::ToggleBtnSet(prefs::PREFSTR_24HOUR, gCfg.show24Hrs);
			}
			break;

		case HOTKEY_TWENTYFOUR:
			if( gCfg.show24Hrs == false )
			{
				prefs::on_24_hour_toggled(NULL, pWidget);
				prefs::ToggleBtnSet(prefs::PREFSTR_24HOUR, gCfg.show24Hrs);
			}
			break;

		case HOTKEY_ALARMS:
			gCfg.showAlarms = !gCfg.showAlarms;
			gtk_widget_queue_draw(pWidget);
			draw::render_set();
			break;

		case HOTKEY_ABOVEALL:
			prefs::on_keep_on_top_toggled(NULL, pWidget);
			prefs::ToggleBtnSet(prefs::PREFSTR_KEEPONBOT, gCfg.keepOnBot);
			prefs::ToggleBtnSet(prefs::PREFSTR_KEEPONTOP, gCfg.keepOnTop);
			break;

		case HOTKEY_TASKBAR:
			prefs::on_show_in_taskbar_toggled(NULL, pWidget);
			prefs::ToggleBtnSet(prefs::PREFSTR_SHOWINTASKS, gCfg.showInTasks);
			break;

		case HOTKEY_BELOWALL:
			prefs::on_keep_on_bot_toggled(NULL, pWidget);
			prefs::ToggleBtnSet(prefs::PREFSTR_KEEPONBOT, gCfg.keepOnBot);
			prefs::ToggleBtnSet(prefs::PREFSTR_KEEPONTOP, gCfg.keepOnTop);
			break;

		case HOTKEY_CENTER:
			if( gCfg.clockX != 0 || gCfg.clockY != 0 || gCfg.clockC != CORNER_CENTER )
			{
				gCfg.clockX  = 0;
				gCfg.clockY  = 0;
				gCfg.clockC  = CORNER_CENTER;

				cfg::cnvp(gCfg, true);
				update_wnd_pos(pWidget, gCfg.clockX, gCfg.clockY);
			}
			break;

		case HOTKEY_DATE:
			prefs::on_show_date_toggled(NULL, pWidget);
			prefs::ToggleBtnSet(prefs::PREFSTR_SHOWDATE, gCfg.showDate);
			break;

		case HOTKEY_EVALDRAWS:
			gRun.evalDraws = !gRun.evalDraws;
			gtk_widget_queue_draw(pWidget);
			draw::update_date_surf();
			draw::render_set();
			break;

		case HOTKEY_FACEDATE:
			prefs::on_face_date_toggled(NULL, pWidget);
			prefs::ToggleBtnSet(prefs::PREFSTR_FACEDATE, gCfg.faceDate);
			break;

		case HOTKEY_FOCUS:
			gCfg.takeFocus = !gCfg.takeFocus;
			gtk_widget_set_can_focus(pWidget, gCfg.takeFocus ? FALSE : TRUE);
			gtk_window_set_accept_focus(GTK_WINDOW(pWidget), gCfg.takeFocus ? FALSE : TRUE);
			break;

		case HOTKEY_SCRNGRAB1: // app-generated svg 'icon' with all clock components drawn as should be seen on-screen
		case HOTKEY_SCRNGRAB2: // typical system-generated window 'grab' (png sized to window - includes desktop bits)
			{
//				TODO: start a new thread then draw
//				TODO: add system-generated grab support for 'g' usage

				const char* ppath = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);

				if( ppath )
				{
					char    fpath[PATH_MAX];
					strvcpy(fpath, ppath);
					strvcat(fpath, "/" APP_NAME "-grab.svg");
					DEBUGLOGP("grab path is:\n%s\n", fpath);
					draw::make_icon(fpath, true);
				}
			}
			break;

		case HOTKEY_HANDSONLY:
//			prefs::on_hands_only_toggled(NULL, pWidget);
//			prefs::ToggleBtnSet(prefs::PREFSTR_HANDSONLY, gCfg.handsOnly);
			gCfg.handsOnly  = !gCfg.handsOnly;
			// TODO: these need to be temp chgs and not cfg saved
			//       toggling also only works for those hands that
			//       aren't using their own draw surf to begin with
			gCfg.useSurf[0] = !gCfg.useSurf[0];
			gCfg.useSurf[1] = !gCfg.useSurf[1];
//			gCfg.useSurf[2] = !gCfg.useSurf[2];
			gtk_widget_queue_draw(pWidget);
			draw::update_bkgnd();
			draw::render_set();
			break;

		case HOTKEY_STICKY:
			prefs::on_sticky_toggled(NULL, pWidget);
			prefs::ToggleBtnSet(prefs::PREFSTR_STICKY, copts::sticky());
			break;

		case HOTKEY_CLICKTHRU:
			gCfg.clickThru = !gCfg.clickThru;
			update_shapes(pWidget, gCfg.clockW, gCfg.clockH, false, true, false);
			gtk_widget_queue_draw(pWidget);
			draw::render_set();
			break;

		case HOTKEY_NODISPLAY:
			gCfg.noDisplay = !gCfg.noDisplay;
			update_shapes(pWidget, gCfg.clockW, gCfg.clockH, false, true, false);
			change_ani_rate(pWidget, gCfg.renderRate, true, false);
			gtk_widget_queue_draw(pWidget);
			draw::render_set();
			break;

		case HOTKEY_MINIMIZE:
			if( gCfg.showInPager && gCfg.showInTasks )
			{
				if( gRun.minimize = !gRun.minimize )
					gtk_window_iconify  (GTK_WINDOW(pWidget));
				else
					gtk_window_deiconify(GTK_WINDOW(pWidget));
			}
			break;

		case HOTKEY_MAXIMIZE:
			{
			int        cx,  cy, cw, ch, ow, oh;
			GdkScreen* pScreen =  gtk_window_get_screen(GTK_WINDOW(pWidget));
			int        sw      =  gdk_screen_get_width (pScreen);
			int        sh      =  gdk_screen_get_height(pScreen);

			if( gRun.maximize  = !gRun.maximize )
			{
				gRun.prevWinX  =  gCfg.clockX;
				gRun.prevWinY  =  gCfg.clockY;
				gRun.prevWinW  =  gCfg.clockW;
				gRun.prevWinH  =  gCfg.clockH;

				ow = gCfg.clockW; oh = gCfg.clockH;
				cx = sw/2 - ow/2; cy = sh/2 - oh/2;
				cw = sw;          ch = sh;

				gtk_window_maximize(GTK_WINDOW(pWidget));
			}
			else
			{
				ow = sw;            oh = sh;
				cx = gRun.prevWinX; cy = gRun.prevWinY;
				cw = gRun.prevWinW; ch = gRun.prevWinH;

				gtk_window_unmaximize(GTK_WINDOW(pWidget));
			}

			// TODO: only shows correctly after a workspace switch - still?

			update_wnd_pos(pWidget, cx, cy);
//			update_wnd_dim(pWidget, cw, ch, true);
			update_wnd_dim(pWidget, cw, ch, ow, oh, true);
			}
			break;

		case HOTKEY_OPACITYDN:
			if( gCfg.opacity > 0.0 )
				gdk_window_set_opacity(gtk_widget_get_window(pWidget), gCfg.opacity-=0.02);
			break;

		case HOTKEY_OPACITYUP:
			if( gCfg.opacity < 1.0 )
				gdk_window_set_opacity(gtk_widget_get_window(pWidget), gCfg.opacity+=0.02);
			break;

		case HOTKEY_PAGER:
			prefs::on_show_in_pager_toggled(NULL, pWidget);
			prefs::ToggleBtnSet(prefs::PREFSTR_SHOWINPAGER, gCfg.showInPager);
			break;

		case HOTKEY_DRAWQUEUED:
			gCfg.queuedDraw = !gCfg.queuedDraw;
			change_ani_rate(pWidget, gCfg.renderRate,   true,  true);
			break;

		case HOTKEY_REFRESHDN:
			change_ani_rate(pWidget, gCfg.renderRate-1, false, true);
			break;

		case HOTKEY_REFRESHUP:
			change_ani_rate(pWidget, gCfg.renderRate+1, false, true);
			break;

		case HOTKEY_SECONDS:
			prefs::on_seconds_toggled(NULL, pWidget);
			prefs::ToggleBtnSet(prefs::PREFSTR_SECONDS, gCfg.showSeconds);
			break;

		case HOTKEY_SQUAREUP:
			if( gCfg.clockW != gCfg.clockH )
			{
				int ce = gCfg.clockW < gCfg.clockH ? gCfg.clockW : gCfg.clockH; 
				update_wnd_dim(pWidget, ce, ce, gCfg.clockW, gCfg.clockH, true);
			}
			break;

		case HOTKEY_TEXTONLY:
			if( gCfg.showDate )
			{
				DEBUGLOGS("***HOTKEY_TEXTONLY***");
				// TODO: need to get the prefs calls working and usable
//				prefs::on_text_only_toggled(NULL, pWidget);
//				prefs::ToggleBtnSet(prefs::PREFSTR_TEXTONLY, gCfg.textOnly);
				gCfg.textOnly = !gCfg.textOnly;
				change_ani_rate(pWidget, gCfg.renderRate, true, false);
				draw::update_date_surf(false, false, false, true);
				gtk_widget_queue_draw(pWidget);
//				draw::update_bkgnd(false);
//				draw::update_date_surf();
				draw::render_set();
			}
			break;

		case HOTKEY_PASTE:
			DEBUGLOGS("***HOTKEY_PASTE***");
			if( ctrld )
			{
				DEBUGLOGS("***ctrl+HOTKEY_PASTE***");

				GtkClipboard* pCB = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

				if( pCB )
				{
					DEBUGLOGS("***selection clipboard available***");

					if( gtk_clipboard_wait_is_uris_available(pCB) )
					{
						DEBUGLOGS("***uris available on clipboard***");
						char** ppURIs = gtk_clipboard_wait_for_uris(pCB);

						if( ppURIs )
						{
							DEBUGLOGS("***got uris on clipboard***");
							int i = 0, n = 8; // n is just an arbitrary failsafe

							while( ppURIs[i] )
							{
								DEBUGLOGP("***%s***\n", ppURIs[i]);
								theme_import(pWidget,   ppURIs[i]);
								break;

								if( ++i >= n )
									break;
							}

							DEBUGLOGS("***freeing array of gotten clipboard uris***");
							g_strfreev(ppURIs);
						}
					}

//					g_object_unref(pCB);
				}
			}
			break;
		}
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
// The ::leave-notify-event is emitted when the pointer leaves the widget's
// window.
// -----------------------------------------------------------------------------
gboolean cgui::on_leave_notify(GtkWidget* pWidget, GdkEventCrossing* pCrossing, gpointer userData)
{
	if( gRun.appStop )
		return FALSE;

	DEBUGLOGB;

	if( !g_wndMovein )
		gdk_window_set_opacity(gtk_widget_get_window(gRun.pMainWindow), gCfg.opacity);

	gRun.isMousing = false;

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
// The ::map-event signal is emitted when the widget's window is mapped. A
// window is mapped when it becomes visible on the screen.
// -----------------------------------------------------------------------------
gboolean cgui::on_map(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	if( gRun.appStop )
		return FALSE;

	DEBUGLOGB;

#if 1
	static bool first = true;

	if( first )
	{
		first = false;

		DEBUGLOGS("first time called so setting timers & opacity");
//		int fps = gCfg.showSeconds || gCfg.aniStartup ? 200 : 2;
		gRun.infoTimerId = g_timeout_add_full(infoPrio, 1000/2,   (GSourceFunc)info_time_timer, (gpointer)pWidget, NULL);
//		gRun.drawTimerId = g_timeout_add_full(drawPrio, 1000/fps, (GSourceFunc)draw_anim_timer, (gpointer)pWidget, NULL);
		gRun.drawTimerId = g_timeout_add_full(drawPrio, 1000/200, (GSourceFunc)draw_anim_timer, (gpointer)pWidget, NULL);
//		gRun.drawTimerId = g_timeout_add_full(drawPrio, 1000/50,  (GSourceFunc)draw_anim_timer, (gpointer)pWidget, NULL);
//		DEBUGLOGP("render timer set to %d fps (%d ms)\n", fps, 1000/fps);
		gdk_window_set_opacity(gtk_widget_get_window(pWidget), gCfg.opacity*gRun.animScale);

		initWorkspace(); // TODO: should this be called here now?
	}
#endif

	DEBUGLOGP("renderIt was %s\n", gRun.renderIt ? "true" : "false");

//	if( gRun.renderIt == false && !gCfg.sticky && gCfg.clockWS )
	if( gRun.renderIt == false )
	{
		gRun.renderIt =  true;
		gRun.renderUp = !gRun.appStart;

		DEBUGLOGP("renderIt is now %s\n", gRun.renderIt ? "true" : "false");
		DEBUGLOGP("renderUp is now %s\n", gRun.renderUp ? "true" : "false");

		if( gRun.scrsaver && !gRun.appStart )
		{
			stopApp();
		}
		else
		{
			DEBUGLOGS("turning on rendering - queueing redraw request");
			change_ani_rate(pWidget, gCfg.renderRate, true, false);
			gtk_widget_queue_draw(pWidget);
			draw::render_set(pWidget);
		}
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
// The ::motion-notify-event signal is emitted when the pointer moves over the
// widget's GdkWindow.
// -----------------------------------------------------------------------------
gboolean cgui::on_motion_notify(GtkWidget* pWidget, GdkEventMotion* pEvent, gpointer userData)
{
	if( gRun.appStop )
		return FALSE;

	DEBUGLOGB;

	if( gRun.scrsaver && !gRun.appStart )
	{
		static double mouseX, mouseY, delmvX, delmvY;
		static bool  first =  true;

		DEBUGLOGB;

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

		DEBUGLOGE;
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean cgui::on_popup_close(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	DEBUGLOGB;

	gtk_widget_hide   (pWidget);
	gtk_widget_destroy(pWidget);
	dnit(true, true);

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
void cgui::on_popupmenu_done(GtkMenuShell* pMenuShell, gpointer userData)
{
	DEBUGLOGB;

	if( !g_inPopup )
	{
		DEBUGLOGS("clearing previously loaded lgui context menu");
		dnit();
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cgui::on_prefs_activate(GtkMenuItem* pMenuItem, gpointer userData)
{
	DEBUGLOGB;
#if 0
	https://developer.gnome.org/gio/stable/GSubprocess.html#g-subprocess-new
	GSubprocess* g_subprocess_new(GSubprocessFlags flags, GError** error, const char* argv0, ...);
#endif
	// TODO: can this use 'popup_activate' function instead of all this? If not, why?
#if 0
	GtkWidget* pDialog = popup_activate('c', "chart", "windowChart");

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
#endif
	if( g_inPopup && g_nmPopup == 's' )
	{
		DEBUGLOGS("exit(0)");
		gtk_window_present(g_pPopupDlg);
		dnit(false);
		return;
	}

	// lgui should always be available since this is a lgui menu item event
	// unless called via context menu key use

	bool lguiActv  =   lgui::okayGUI();
//	bool menuKeyd  =  !lguiActv && !userData && !pMenuItem;
	bool menuKeyd  =  !lguiActv && !pMenuItem;

	if( g_inPopup || (!lguiActv && !menuKeyd) )
	{
		DEBUGLOGS("exit(1)");
		dnit(!g_inPopup);
		return;
	}

	prefs::begin();

	if( !menuKeyd )
		dnit();

	if(!lgui::okayGUI() )
	{
		DEBUGLOGS("loading props lgui file");
		lgui::getXml("props");
	}

	if( lgui::okayGUI() )
	{
		DEBUGLOGS("props lgui file successfully loaded");
#if 0
		GTimeVal            ct;
		g_get_current_time(&ct);
		DEBUGLOGP("before sync loading of %s (%d.%d)\n", "settingsDialog", (int)ct.tv_sec, (int)ct.tv_usec);
#endif
		GtkWidget* pDialog = lgui::pWidget("settingsDialog");
#if 0
		g_get_current_time(&ct);
		DEBUGLOGP("after loading, which %s (%d.%d)\n", pDialog ? "succeeded" : "failed", (int)ct.tv_sec, (int)ct.tv_usec);
#endif
		if( pDialog )
		{
			DEBUGLOGS("settings dbox successfully created");
//			set_window_icon(pDialog);
			gtk_window_set_transient_for(GTK_WINDOW(pDialog), GTK_WINDOW(userData));
//			prefs::init(pDialog, g_pPopupDlg, g_inPopup, g_nmPopup);
			prefs::init(pDialog);
		}
		else
		{
			DEBUGLOGS("settings dbox NOT successfully created");
			dnit();
		}
	}

	DEBUGLOGS("exit(2)");
}

// -----------------------------------------------------------------------------
gboolean cgui::on_prefs_close(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	gtk_widget_hide(pWidget);
	dnit();

	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean cgui::on_query_tooltip(GtkWidget* pWidget, gint x, gint y, gboolean keyboard_mode, GtkTooltip* pTooltip, gpointer userData)
{
	if( keyboard_mode )
		return FALSE;

	DEBUGLOGB;
#if 0
	char*       ttag;
	const int   tlen = 2048;
	const char* fmts = "fr";
#endif
	const int tlen = 2048;
	char      tbuf [tlen];
	char      tstr [tlen]; *tstr = '\0';
	strfmtdt (tstr, tlen, gCfg.show24Hrs ? gCfg.fmtTTip24 : gCfg.fmtTTip12, &gRun.timeCtm, tbuf);

	if( *tstr )
	{
#if 0	// NOTE: moved to utility strfmtdt since it already had access to gRun
		char tend[tlen];
		char fmt[]  =  "@?";

		for( size_t f = 0; f < vectsz(fmts); f++ )
		{
			fmt[1] = fmts[f];
			if( (ttag = strstr(tstr, fmt)) != NULL )
			{
				strvcpy(tend, ttag+2); *ttag = '\0';

				switch( fmts[f] )
				{
				case 'f':
					static const char* txts[] = { gRun.cfaceTxta3, gRun.cfaceTxta2, gRun.cfaceTxta1 };

					for( size_t t = 0; t < vectsz(txts); t++ )
					{
						if( *txts[t] )
						{
							if( *tstr )
								strvcat(tstr, " ");
							strvcat(tstr, txts[t]);
						}
					}
					break;

				case 'r':
					if( *gRun.riseSetTxt )
						strvcat(tstr, gRun.riseSetTxt);
					break;
				}

				strvcat(tstr, tend);
			}
		}
#endif
		GtkWindow* pTTipWnd;

		if( pTTipWnd = gtk_widget_get_tooltip_window(pWidget) )
		{
			DEBUGLOGS("updating tooltip text via own ttip window (step 1 of 2)");

			GtkLabel* pLabel = GTK_LABEL(g_object_get_data(G_OBJECT(pTTipWnd), g_ttLabel));

			if( pLabel )
			{
				DEBUGLOGS("updating tooltip text via own ttip window (step 2 of 2)");
				gtk_label_set_markup(pLabel, tstr);
			}
		}
		else
		{
			DEBUGLOGS("updating tooltip text via sys ttip window");
			gtk_tooltip_set_markup(pTooltip, tstr);
		}
	}

	DEBUGLOGE;
	return TRUE;
}

// -----------------------------------------------------------------------------
void cgui::on_quit_activate(GtkMenuItem* pMenuItem, gpointer data)
{
	stopApp();
}

// -----------------------------------------------------------------------------
void cgui::on_screen_changed(GtkWidget* pWidget, GdkScreen* pPrevScreen, gpointer userData)
{
	DEBUGLOGB;
	// TODO: find out when this gets called to see if it might be of some use
	//       in either reducing our reliance on wnck or for other unsupported
	//       requirements (grab the 'new' desktop bkgnd for instance?)
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void cgui::on_tzone_activate(GtkMenuItem* pMenuItem, gpointer userData)
{
	DEBUGLOGB;

	GtkWidget* pDialog = popup_activate('z', "tzone", "tzoneDialog");

	if( pDialog )
	{
		g_signal_connect(G_OBJECT(pDialog), "response", G_CALLBACK(on_popup_close), NULL);
		tzng::init(pDialog, 'z');
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// The ::unmap-event signal is emitted when the widget's window is unmapped. A
// window is unmapped when it becomes invisible on the screen.
// -----------------------------------------------------------------------------
gboolean cgui::on_unmap(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	DEBUGLOGB;

	if( gRun.scrsaver && !gRun.appStart )
	{
		stopApp();
	}
	else
	{
		DEBUGLOGP("renderIt was %s\n", gRun.renderIt ? "true" : "false");

//		if( gRun.renderIt == true && (!gCfg.sticky || gCfg.clockWS) )
		if( gRun.renderIt == true )
		{
			gRun.renderIt =  false;
			gRun.renderUp = !gRun.appStart;

			DEBUGLOGP("renderIt is now %s\n", gRun.renderIt ? "true" : "false");
			DEBUGLOGP("renderUp is now %s\n", gRun.renderUp ? "true" : "false");
			DEBUGLOGS("turning off rendering - queueing redraw request");

			change_ani_rate(pWidget, gCfg.renderRate, true, false);
			gtk_widget_queue_draw(pWidget);
			draw::render_set(pWidget);
		}
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean cgui::on_wheel_scroll(GtkWidget* pWidget, GdkEventScroll* pScroll, gpointer userData)
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
#if 0
	case GDK_SCROLL_SMOOTH:
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
		break;
#endif
	}

	DEBUGLOGP("delta is %d\n", delta);

	if( delta && !gRun.updating )
	{
		if( (pScroll->state & GDK_MOD1_MASK) == GDK_MOD1_MASK ) // font size change (alt key normally)
		{
			DEBUGLOGS("  font size change request");

			if( (gCfg.fontSize+delta) >= 6 && (gCfg.fontSize+delta) <= 18 ) // reasonableness testing
			{
				gCfg.fontSize += delta;
				draw::update_date_surf(false, false, true);
				gtk_widget_queue_draw(pWidget);
				cfg::save();

				DEBUGLOGP("changed font size to %d\n", gCfg.fontSize);
			}
		}
#if 0
		else
		if( (pScroll->state & GDK_SUPER_MASK) == GDK_SUPER_MASK ) // ? change (win key normally)
		{
			DEBUGLOGS("win+mouse wheel processing is not currently assigned to any action");
		}
#endif
		else
		if( (pScroll->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK ) // theme change (ctrl key normally)
		{
			DEBUGLOGS("  theme change request");

			ThemeList*     tl;
			theme_list_get(tl);
			int            ti = -1;
			ThemeEntry     te =  theme_list_fnd(tl, gCfg.themePath, gCfg.themeFile, NULL, &ti);

			DEBUGLOGP("  comparing %d list entries to current theme of %s\n", theme_list_cnt(tl), gCfg.themeFile);

			if( ti == -1 ) // couldn't find the current theme in the list?
			{
				// point to top or bottom depending on roll direction
				ti    = delta > 0 ? 0 : theme_list_cnt(tl)-1;
				delta = 0;
			}

			if( ti != -1 )
			{
				DEBUGLOGP("found current theme at index %d\n", ti);

				ti +=  delta;

				if( ti >= 0 && ti < theme_list_cnt(tl) )
				{
					DEBUGLOGP("changing to new theme at index %d\n", ti);

					te = theme_list_nth(tl, ti);

					DEBUGLOGP("changing to new theme %s\n", te.pName->str);

					change_theme(pWidget, te, true);

					if( lgui::okayGUI() )
					{
						GtkWidget* pWidget = lgui::pWidget("comboboxTheme");

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

			float adjF = (float)delta/100.0f;
			int   newW = gCfg.clockW + (int)((float)gCfg.clockW*adjF);
			int   newH = gCfg.clockH + (int)((float)gCfg.clockH*adjF);

			if( newW  == gCfg.clockW )
				newW   = gCfg.clockW + delta;

			if( newH  == gCfg.clockH )
				newH   = gCfg.clockH + delta;

			update_wnd_dim(pWidget, newW, newH, gCfg.clockW, gCfg.clockH, true);
			cfg::save();
		}
	}

	DEBUGLOGE;
	return TRUE;
}

// -----------------------------------------------------------------------------
// The ::window-state-event is emitted when the state of the toplevel window
// associated to the widget changes.
// -----------------------------------------------------------------------------
gboolean cgui::on_window_state(GtkWidget* pWidget, GdkEventWindowState* pState, gpointer userData)
{
	DEBUGLOGB;

	bool stkState  = (pState->changed_mask     & GDK_WINDOW_STATE_STICKY)    == GDK_WINDOW_STATE_STICKY;
	bool sticky    = (pState->new_window_state & GDK_WINDOW_STATE_STICKY)    == GDK_WINDOW_STATE_STICKY;
#ifdef DEBUGLOG
	{
		const char* sc_str = stkState  ? "yes"        : "no";
		const char* ss_str = sticky    ? "sticky"     : "unstuck";
		DEBUGLOGP("win sticky   changed? %s, to %s\n", sc_str, ss_str);
	}
#endif

	bool hidState  = (pState->changed_mask     & GDK_WINDOW_STATE_WITHDRAWN) == GDK_WINDOW_STATE_WITHDRAWN;
	bool hidden    = (pState->new_window_state & GDK_WINDOW_STATE_WITHDRAWN) == GDK_WINDOW_STATE_WITHDRAWN;
#ifdef DEBUGLOG
	{
		const char* hc_str = hidState  ? "yes"        : "no";
		const char* hs_str = hidden    ? "hidden"     : "visible";
		DEBUGLOGP("win visible  changed? %s, to %s\n", hc_str, hs_str);
	}
#endif

	bool maxState  = (pState->changed_mask     & GDK_WINDOW_STATE_MAXIMIZED) == GDK_WINDOW_STATE_MAXIMIZED;
	bool maximized = (pState->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) == GDK_WINDOW_STATE_MAXIMIZED;
#ifdef DEBUGLOG
	{
		const char* zc_str = maxState  ? "yes"        : "no";
		const char* zs_str = maximized ? "maximized"  : "unmaximized";
		DEBUGLOGP("win maximize changed? %s, to %s\n", zc_str, zs_str);
	}
#endif

	bool minState  = (pState->changed_mask     & GDK_WINDOW_STATE_ICONIFIED) == GDK_WINDOW_STATE_ICONIFIED;
	bool minimized = (pState->new_window_state & GDK_WINDOW_STATE_ICONIFIED) == GDK_WINDOW_STATE_ICONIFIED;
	gRun.minimize  =  minState  ?  minimized   : gRun.minimize;
#ifdef DEBUGLOG
	{
		const char* ic_str = minState  ? "yes"        : "no";
		const char* is_str = minimized ? "minimized"  : "unminimized";
		DEBUGLOGP("win minimize changed? %s, to %s\n", ic_str, is_str);
	}
#endif

	// TODO: on XFCE, consistent sequence of 2 of these calls is made when
	//       hiding the clock window in order to get a desktop bkgnd grab
	//       (first the window is unstuck then it is hidden)
	//       the reverse holds true for when it's reshown
	//
	//       is this consistent across other WMs (all?) to use as way to
	//       trigger bkgnd grabbing w/o having to resort to using a timer?

	DEBUGLOGE;
	return FALSE;
}
#if _USEWKSPACE
#if 0
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cgui::on_workspace_active_changed(WnckScreen* screen, WnckWorkspace* prevSpace, gpointer userData)
{
	DEBUGLOGB;

	if( gRun.scrsaver && !gRun.appStart )
	{
		stopApp();
		DEBUGLOGR(1);
		return;
	}

	if( gCfg.sticky || !gCfg.clockWS )
	{
		DEBUGLOGR(2);
		return;
	}

	WnckWorkspace* actvWrk = wnck_screen_get_active_workspace(screen);
	WnckWorkspace* trgtWrk = wnck_screen_get_workspace       (screen, gCfg.clockWS-1);

	if( actvWrk != trgtWrk )
	{
		DEBUGLOGR(3);
		return;
	}

//	g_clockWnd    = wnck_window_get(GDK_WINDOW_XWINDOW(gtk_widget_get_window(gRun.pMainWindow)));
//	g_clockWrk    = wnck_window_get_workspace(g_clockWnd);

	DEBUGLOGS("setting render It/Up based on new active workspace");

//	WnckWindow*    g_clockWnd = wnck_window_get(GDK_WINDOW_XWINDOW      (gtk_widget_get_window(gRun.pMainWindow)));
	WnckWindow*    g_clockWnd = wnck_window_get(gdk_x11_drawable_get_xid(gtk_widget_get_window(gRun.pMainWindow)));
	WnckWorkspace* g_clockWrk = wnck_window_get_workspace(g_clockWnd);

	gRun.renderIt = copts::sticky() || !g_clockWrk || !actvWrk || (g_clockWrk == actvWrk);
	gRun.renderUp = gRun.renderIt    && !gRun.appStart;

	draw::render_set();

//	if( gRun.scrsaver && !gRun.appStart )
//	{
//		stopApp();
//	}
//	else
//	if( gRun.renderUp && !copts::sticky() && (g_clockWrk && g_clockWrk != actvWrk) )
	if( gRun.renderIt )
	{
		DEBUGLOGS("queueing redraw request");
//		TODO: best to fire off draw timer so renderIt, etc., are handled properly
		gtk_widget_queue_draw(gRun.pMainWindow);
		gtk_widget_show(gRun.pMainWindow);
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

	DEBUGLOGE;
}
#endif

// -----------------------------------------------------------------------------
static gboolean grab_time_timer(GtkWidget* pWidget)
{
	DEBUGLOGB;

	// TODO: need a way to ensure a redraw with the new 'hidden' outline shape
	//       happens before the timer expires?

//	gboolean ret = gtk_widget_get_visible(pWidget); // continue timer until hidden

//	if( !ret )
	{
		DEBUGLOGS("timer killed; getting bkgnd; showing clock");

		draw::grab();

		update_shapes(gRun.pMainWindow, gCfg.clockW, gCfg.clockH, false, true, false);
		gtk_widget_queue_draw(gRun.pMainWindow);
		draw::render_set();
	}

	DEBUGLOGE;
//	return ret;
	return FALSE;
}

// -----------------------------------------------------------------------------
void cgui::on_workspace_bkgrnd_changed(WnckScreen* pScreen, gpointer userData)
{
	DEBUGLOGB;

	// TODO: get this called on active workspace changes even though they all
	//       use the same bkgnd - workaround possible?

//	on_workspace_active_changed(pScreen, NULL, userData);

//	static
//	gulong cbpm = -1;
//	gulong nbpm =  wnck_screen_get_background_pixmap(pScreen);

//	if( cbpm != nbpm )
//	{
//		cbpm  = nbpm;

		if( !gRun.appStart && !gRun.appStop && gRun.pMainWindow && !gRun.composed )
		{
			DEBUGLOGS("starting timer; hiding clock");

			// TODO: need a way to ensure a redraw with the new 'hidden' outline
			//       shape happens before the timer expires?

			gint clickThru = gCfg.clickThru, noDisplay = gCfg.noDisplay;
			gCfg.clickThru =            gCfg.noDisplay = TRUE;

			update_shapes(gRun.pMainWindow, gCfg.clockW, gCfg.clockH, false, true, false);
			gtk_widget_queue_draw(gRun.pMainWindow);
			draw::render_set();

			gCfg.clickThru = clickThru; gCfg.noDisplay = noDisplay;

			g_timeout_add_full(waitPrio, 50, (GSourceFunc)grab_time_timer, (gpointer)gRun.pMainWindow, NULL);
		}
//	}

	DEBUGLOGE;
}
#endif // _USEWKSPACE

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
GtkWidget* cgui::popup_activate(char type, const char* name, const char* widget)
{
	DEBUGLOGB;

	if( g_inPopup && g_nmPopup == type )
	{
		gtk_window_present(g_pPopupDlg);
		dnit(false);
		return NULL;
	}

	// lgui should always be available since this is a lgui menu item event

	if( g_inPopup || !lgui::okayGUI() )
	{
		dnit(!g_inPopup);
		return NULL;
	}

	dnit();

	if(!lgui::okayGUI() )
		lgui::getXml(name);

	GtkWidget* ret = lgui::okayGUI() ? lgui::pWidget(widget) : NULL;

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
void cgui::setPopup(GtkWidget* pPopupDlg, bool inPopup, char key)
{
	DEBUGLOGB;

	DEBUGLOGS("bef show call");

	set_window_icon(pPopupDlg);
	gtk_widget_show_all(pPopupDlg);

	DEBUGLOGS("aft show call");

	g_pPopupDlg = GTK_WINDOW(pPopupDlg);
	g_inPopup   = inPopup;
	g_nmPopup   = key;

	// TODO: these calls (one or both?) cause the popup box to 'follow' the clock
	//       window from workspace to workspace - is this what's wanted?

	gtk_window_set_transient_for(GTK_WINDOW(pPopupDlg), GTK_WINDOW(gRun.pMainWindow));
	gtk_window_present(g_pPopupDlg);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool cgui::theme_import(GtkWidget* pWidget, const char* uri)
{
	DEBUGLOGB;

	bool  ret  = false;
	char* path = g_filename_from_uri(uri, NULL, NULL);
	if(  !path )
	{
		DEBUGLOGE;
		return ret;
	}

	g_strstrip(path);

	int plen = strlen(path);
	if( plen > 1 )
	{
		DEBUGLOGP("path received is\n%s\n", path);

		char tpath[PATH_MAX]; *tpath = '\0';
		char tfile[64];       *tfile = '\0';

		// TODO: support other types of theme importing besides just archived

		if( loadTheme(path, tpath, vectsz(tpath), tfile, vectsz(tfile)) )
		{
			DEBUGLOGP("path received successfully extracted to\n%s%s\n", tpath, tfile);

			// get rid of the trailing / to get what the rest of the app requires
			tpath[strlen(tpath)-1] = '\0';

			ThemeEntry te;
			memset(&te, 0, sizeof(te));
			te.pPath  = g_string_new(tpath);
			te.pFile  = g_string_new(tfile);
			te.pName  = g_string_new("");
			te.pModes = g_string_new("");

			DEBUGLOGP("current theme in use is\n%s/%s\n",   gCfg.themePath, gCfg.themeFile);
			DEBUGLOGP("switching to new theme of\n%s/%s\n", te.pPath->str,  te.pFile->str);

			change_theme(pWidget, te, true);

			theme_ntry_del(te);

			ret = true;
		}
		else
		{
			DEBUGLOGS("path received NOT successfully extracted");
		}

		if( tpath[0] && false )
			g_del_dir(tpath);
	}

//	DEBUGLOGP("slen=%d, plen=%d, curs+snxt is\n*****\n%s\n*****\n", snxt, plen, curs+snxt);

	g_free(path);

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void cgui::wndMoveEnter(GtkWidget* pWidget, GdkEventButton* pButton)
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
void cgui::wndMoveExit(GtkWidget* pWidget)
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
gboolean cgui::wndMoveTime(GtkWidget* pWidget)
{
	DEBUGLOGB;

	bool wndMoveDone = false;

	if( g_wndMovein )
	{
		int                     x, y;
		GdkModifierType               mask;
		get_cursor_pos(pWidget, x, y, mask);

		if( (mask & GDK_BUTTON1_MASK) != GDK_BUTTON1_MASK )
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
void cgui::wndSizeEnter(GtkWidget* pWidget, GdkEventButton* pButton, GdkWindowEdge edge)
{
	g_wndSizein = true;

	DEBUGLOGS("entering window sizing dragging");

//#if GTK_CHECK_VERSION(3,0,0)
//	gdk_window_set_event_compression(GTK_WINDOW(gtk_widget_get_toplevel(pWidget)), FALSE);
//#endif

	#ifndef GDK_WINDOW_EDGE_CENTER
	#define GDK_WINDOW_EDGE_CENTER (GdkWindowEdge)12345
	#endif

	double xcen = (double)gCfg.clockW/2.0, ycen = (double)gCfg.clockH/2.0;
	double xcmx = (double)gCfg.clockW/8.0, ycmx = (double)gCfg.clockH/8.0;
	double xrel = (double)pButton->x-xcen, yrel = (double)pButton->y-ycen;
	double cang =  180.0-atan2(xrel, yrel)*180.0/M_PI; // make cursor pos. angle start from top (0->360)

	DEBUGLOGP("curpos=(%3.f,%3.f), cen=(%3.f,%3.f), rel=(%3.f,%3.f), ang=%3.2f\n",
		(double)pButton->x, (double)pButton->y, xcen, ycen, xrel, yrel, cang);

	if( fabs(xrel) <= xcmx && fabs(yrel) <= ycmx )
	{
		edge = GDK_WINDOW_EDGE_CENTER;
	}
	else
	{
		GdkWindowEdge edgs[] =
		{
			GDK_WINDOW_EDGE_NORTH,      GDK_WINDOW_EDGE_NORTH_EAST, GDK_WINDOW_EDGE_EAST,
			GDK_WINDOW_EDGE_SOUTH_EAST, GDK_WINDOW_EDGE_SOUTH,      GDK_WINDOW_EDGE_SOUTH_WEST,
			GDK_WINDOW_EDGE_WEST,       GDK_WINDOW_EDGE_NORTH_WEST, GDK_WINDOW_EDGE_NORTH
		};

		for( size_t e = 0; e < vectsz(edgs); e++ )
		{
			if( cang >= 45.0*e-22.5 && cang < 45.0*e+22.5 )
			{
				edge  = edgs[e];
				break;
			}
		}
	}

	GdkGeometry geom;
	int         hints   = GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE;
	GdkScreen*  pScreen = gtk_window_get_screen(GTK_WINDOW(pWidget));

	geom.min_width  = MIN_CLOCKW, geom.max_width  = gdk_screen_get_width (pScreen);
	geom.min_height = MIN_CLOCKH, geom.max_height = gdk_screen_get_height(pScreen);

	if( (pButton->state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK ) // force square window resizing
	{
		hints          |= GDK_HINT_ASPECT;
		geom.min_aspect = 1.0;
		geom.max_aspect = 1.0;
	}

	pWidget            = gtk_widget_get_toplevel(pWidget);
	GtkWindow* pWindow = GTK_WINDOW(pWidget);

	if( edge == GDK_WINDOW_EDGE_CENTER )
	{
		// TODO: need to add in center relative resizing
		DEBUGLOGS("center relative resizing currently not supported");
		change_cursor(pWidget, GDK_CROSSHAIR);
	}
	else
	{
		DEBUGLOGS("beginning resize drag");

		gtk_window_begin_resize_drag(pWindow, edge, pButton->button, pButton->x_root, pButton->y_root, pButton->time);
	}

	DEBUGLOGS("middle mouse button is now down - creating check timer");

	gtk_window_set_geometry_hints(pWindow, pWidget, &geom, GdkWindowHints(hints));

	if( g_wndSizeT )
		g_source_remove(g_wndSizeT);

	g_wndSizeT = g_timeout_add(200, (GSourceFunc)wndSizeTime, (gpointer)pWidget);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void cgui::wndSizeExit(GtkWidget* pWidget)
{
	if( g_wndSizein )
	{
		g_wndSizein = false;

		DEBUGLOGS("exiting window sizing dragging");

//#if GTK_CHECK_VERSION(3,0,0)
//		gdk_window_set_event_compression(GTK_WINDOW(gtk_widget_get_toplevel(pWidget)), TRUE);
//#endif

//		change_cursor(gtk_widget_get_toplevel(pWidget), GDK_FLEUR);

		gint                                      newW,  newH;
		gtk_window_get_size(GTK_WINDOW(pWidget), &newW, &newH);

		if( newW != gCfg.clockW || newH != gCfg.clockH )
		{
			DEBUGLOGP("setting new window size (%d, %d)\n", newW, newH);
//			update_wnd_dim(pWidget, newW, newH, true);
			update_wnd_dim(pWidget, newW, newH, gCfg.clockW, gCfg.clockH, true);
			cfg::save();
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
gboolean cgui::wndSizeTime(GtkWidget* pWidget)
{
	DEBUGLOGB;

	bool wndSizeDone = false;

	if( g_wndSizein )
	{
		int                     x, y;
		GdkModifierType               mask;
		get_cursor_pos(pWidget, x, y, mask);

		if( (mask & GDK_BUTTON2_MASK) != GDK_BUTTON2_MASK )
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

#endif // _USEGTK

