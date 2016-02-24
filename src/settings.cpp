/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP    "pref"

#include "settings.h" //

#include "draw.h"     // for ?
#include "chart.h"    // for ?
#include "copts.h"    // for copts gets/sets
#include "debug.h"    // for debugging prints
#if 0
#include "tzone.h"    // for timezone related
#endif
#include "config.h"   // for Config struct, CornerType enum, ...
#include "global.h"   // for Runtime struct, MIN_CLOCKW/H, SIZE_CUSTOM/..., global funcs, ...
#include "themes.h"   // for ?
#include "loadGUI.h"  // for lgui::okay, lgui::pWidget, ...
#include "clockGUI.h" // for cgui::dnit
#include "utility.h"  // for ?
#include "x.h"        // for g_workspace_count

#define   OPENING           g_opening
#define   OPENING_RET   if( g_opening )   return

/*******************************************************************************
**
** Preferences dialog box related events
**
*******************************************************************************/

#if _USEGTK
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
namespace prefs
{

static void gtk_combo_box_clear(GtkComboBox* pComboBox, int cnt=0);

static void set_geometry_related();     // tab 0
static void set_clock_face_related();   // tab 1
static void set_hand_anim_related();    // tab 2
static void set_face_text_related();    // tab 3
static void set_text_font_related();    // tab 4
static void set_text_color_related();   // tab 5
static void set_text_spacing_related(); // tab 6
static void set_extras_related();       // tab 7

static void on_button_toggled(GtkToggleButton* pToggButton, gpointer window, gint& flag, bool redraw, bool update);

static gboolean on_key_press     (GtkWidget* pWidget, GdkEventKey* pKey,   gpointer userData);
static gboolean on_map_event     (GtkWidget* pWidget, GdkEvent*    pEvent, gpointer userData);
static gboolean on_settings_close(GtkWidget* pWidget, GdkEvent*    pEvent, gpointer userData);

static void on_ani_startup_toggled  (GtkToggleButton*     pToggButton,     gpointer window);
static void on_ani_clock_toggled    (GtkToggleButton*     pToggButton,     gpointer window);
static void on_ani_hand_toggled     (GtkToggleButton*     pToggButton,     gpointer userData);
static void on_ani_type_toggled     (GtkToggleButton*     pToggButton,     gpointer window);
static void on_ani_value_changed    (GtkRange*            pRange,          gpointer window);
static void on_close_clicked        (GtkButton*           pButton,         gpointer window);
static void on_corner_toggled       (GtkToggleButton*     pToggButton,     gpointer window);
static void on_do_sounds_toggled    (GtkToggleButton*     pToggButton,     gpointer window);
static void on_height_value_changed (GtkSpinButton*       pSpinButton,     gpointer window);
static void on_help_clicked         (GtkButton*           pButton,         gpointer window);
static void on_lock_dims_toggled    (GtkToggleButton*     pToggButton,     gpointer window);
static void on_lock_offs_toggled    (GtkToggleButton*     pToggButton,     gpointer window);
static void on_nbook_switch_page    (GtkNotebook*         pNotebook,       gpointer page, guint index, gpointer window);
static void on_opacity_toggled      (GtkToggleButton*     pToggButton,     gpointer window);
static void on_opacity_value_changed(GtkRange*            pRange,          gpointer window);
static void on_save_clicked         (GtkButton*           pButton,         gpointer window);
static void on_startup_size_changed (GtkComboBox*         pComboBox,       gpointer window);
static void on_themes_changed       (GtkComboBox*         pComboBox,       gpointer window);
static void on_themes_popup         (GtkComboBox*         pComboBox,       gpointer window);
static void on_unsticky_toggled     (GtkToggleButton*     pToggButton,     gpointer window);
static void on_width_value_changed  (GtkSpinButton*       pSpinButton,     gpointer window);
static void on_ws_use_toggled       (GtkToggleToolButton* pToggToolButton, gpointer window);
static void on_ws_value_changed     (GtkRange*            pRange,          gpointer window);
static void on_x_value_changed      (GtkSpinButton*       pSpinButton,     gpointer window);
static void on_y_value_changed      (GtkSpinButton*       pSpinButton,     gpointer window);

static gboolean on_exts_btn_release (GtkWidget* pWidget, GdkEvent* pEvent, gpointer window);

static void on_fmtDate12_activate(GtkEntry* pEntry, gpointer window);
static void on_fmtDate24_activate(GtkEntry* pEntry, gpointer window);
static void on_fmtTime12_activate(GtkEntry* pEntry, gpointer window);
static void on_fmtTime24_activate(GtkEntry* pEntry, gpointer window);
static void on_fmtTTip12_activate(GtkEntry* pEntry, gpointer window);
static void on_fmtTTip24_activate(GtkEntry* pEntry, gpointer window);

static void on_fmtDate12_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window);
static void on_fmtDate24_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window);
static void on_fmtTime12_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window);
static void on_fmtTime24_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window);
static void on_fmtTTip12_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window);
static void on_fmtTTip24_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window);

static void on_fmtshow_update(GtkEntry* pEntry, const char* fmtShow);
static void on_fmttext_update(GtkEntry* pEntry, char* fmtText, int maxText, const char* fmtShow, gpointer window, bool ttip=false, bool date=false, bool time=false);

#if !GTK_CHECK_VERSION(3,0,0)
static void on_font_face_list_changed  (GtkTreeSelection* pTreeSelection, gpointer window);
static void on_font_family_list_changed(GtkTreeSelection* pTreeSelection, gpointer window);
static void on_font_size_list_changed  (GtkTreeSelection* pTreeSelection, gpointer window);
static void on_font_size_entry_activate(GtkEntry*         pEntry,         gpointer window);

static void on_font_color_changed    (GtkColorSelection* pColorSelection, gpointer  window);
static void on_font_color_event_after(GtkColorSelection* pColorSelection, GdkEvent* pEvent, gpointer window);

static void on_font_changed(gpointer window);
#endif

static void on_text_spacing_value_changed(GtkSpinButton* pSpinButton, gpointer value);

static void set_wnd_size(GtkWidget* pWidget, int oldW, int oldH, int newW, int newH);

// -----------------------------------------------------------------------------
const char* PREFSTR_24HOUR      = "checkbutton24Hour";
const char* PREFSTR_FACEDATE    = "checkbuttonFaceDate";
const char* PREFSTR_HEIGHT      = "spinbuttonHeight";
const char* PREFSTR_KEEPONBOT   = "checkbuttonKeepOnBot";
const char* PREFSTR_KEEPONTOP   = "checkbuttonKeepOnTop";
const char* PREFSTR_SECONDS     = "checkbuttonSeconds";
const char* PREFSTR_SHOWDATE    = "checkbuttonShowDate";
const char* PREFSTR_SHOWINPAGER = "checkbuttonAppearInPager";
const char* PREFSTR_SHOWINTASKS = "checkbuttonAppearInTaskbar";
const char* PREFSTR_STICKY      = "checkbuttonSticky";
const char* PREFSTR_TEXTONLY    = "checkbuttonTextOnly";
const char* PREFSTR_WIDTH       = "spinbuttonWidth";
const char* PREFSTR_X           = "spinbuttonX";
const char* PREFSTR_Y           = "spinbuttonY";

// -----------------------------------------------------------------------------
static const char* PREFSTR_ANIRATE    = "hscaleSmoothness";
static const char* PREFSTR_ANISTARTUP = "checkbuttonAniStartup";
static const char* PREFSTR_CLICKTHRU  = "checkbuttonClickThru";
static const char* PREFSTR_HANDSONLY  = "checkbuttonHandsOnly";
static const char* PREFSTR_NODISPLAY  = "checkbuttonNoDisplay";
static const char* PREFSTR_OPACITY    = "checkbuttonOpacity";
static const char* PREFSTR_PLAYSOUNDS = "checkbuttonPlaySounds";
static const char* PREFSTR_TAKEFOCUS  = "checkbuttonTakeFocus";
static const char* PREFSTR_THEMES     = "comboboxTheme";
static const char* PREFSTR_UNSTICKY   = "checkbuttonUnSticky";

// -----------------------------------------------------------------------------
static GtkWidget*  g_pWidget;
static bool        g_opening  =   false;
static bool        g_tabset[] = { false, false, false, false, false, false, false, false };
static const char* g_activate =  "activate";
static const char* g_btnreld  =  "button-release-event";
static const char* g_changed  =  "changed";
static const char* g_clicked  =  "clicked";
static const char* g_toggled  =  "toggled";
static const char* g_valchgd  =  "value-changed";

} // namespace prefs

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::gtk_combo_box_clear(GtkComboBox* pComboBox, int cnt)
{
	gtk_combo_box_set_active(pComboBox, -1);

	if( cnt )
	{
#if !GTK_CHECK_VERSION(3,0,0)
		while( cnt-- )
			gtk_combo_box_remove_text(GTK_COMBO_BOX(pComboBox), 0);
#endif
	}
	else
	{
//		gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(pComboBox))));

		GtkTreeModel* cboxList = gtk_combo_box_get_model(GTK_COMBO_BOX(pComboBox));

		if( cboxList )
			gtk_list_store_clear(GTK_LIST_STORE(cboxList));
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
GtkWidget* prefs::initControl(const char* name, const char* event, GCallback callback, GtkWidget* pWindow)
{
	DEBUGLOGB;
	DEBUGLOGP("  %s widget & %s event passed\n", name, event);

	if( g_pWidget = lgui::pWidget(name) )
	{
		DEBUGLOGP("%s widget found, connecting the %s signal event to it\n", name, event);
		g_signal_connect(G_OBJECT(g_pWidget), event, callback, pWindow ? pWindow : gRun.pMainWindow);
	}

	DEBUGLOGE;
	return g_pWidget;
}

// -----------------------------------------------------------------------------
void prefs::initToggleBtn(const char* name, const char* event, int value, TOGBTNCB callback, gpointer userData)
{
	DEBUGLOGB;
	DEBUGLOGP("  %s widget & %s event passed\n", name, event);

	GtkToggleButton* pWidget = ToggleBtnSet(name, value);

	if( pWidget )
	{
		DEBUGLOGP("%s widget found, connecting the %s signal event to it\n", name, event);
		g_signal_connect(G_OBJECT(pWidget), event, G_CALLBACK(callback), userData ? userData : gRun.pMainWindow);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
GtkToggleButton* prefs::ToggleBtnSet(const char* name, int value)
{
	DEBUGLOGB;
	DEBUGLOGP("%s widget passed\n", name);

	GtkToggleButton* pWidget = GTK_TOGGLE_BUTTON(lgui::pWidget(name));

	if( pWidget )
	{
		DEBUGLOGP("%s widget found, setting it to a value of %d\n", name, value);
		gtk_toggle_button_set_active(pWidget, value);
	}

	DEBUGLOGE;
	return pWidget;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::begin()
{
	DEBUGLOGB;
#if 0
	tzn::beg();
#endif
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::end()
{
	DEBUGLOGB;
#if 0
	tzn::end();
#endif
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::open(bool opening)
{
	g_opening = opening;
}

// -----------------------------------------------------------------------------
bool prefs::opend()
{
	return g_opening;
}
/*
// -----------------------------------------------------------------------------
static void on_custom_anim_activate(GtkExpander* pExpander, gpointer user_data)
{
	DEBUGLOGB;

	if( !gtk_expander_get_expanded(pExpander) )
	{
		g_pWidget = lgui::pWidget("drawingareaAnimationCustom");

		if( g_pWidget )
			chart::init(g_pWidget);
	}

	DEBUGLOGE;
}*/

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::init(GtkWidget* pPropsDialog)
{
	DEBUGLOGB;

	if( !lgui::okay() )
	{
		DEBUGLOGR(1);
		return;
	}

#ifdef _DEBUGLOG
	GTimeVal            ctb;
	g_get_current_time(&ctb);
	DEBUGLOGP("  time beg (%d.%d)\n", (int)ctb.tv_sec, (int)ctb.tv_usec);
#endif

	open(true);

	for( size_t  t  = 0; t < vectsz(g_tabset); t++ )
		g_tabset[t] = false;

	if( pPropsDialog )
	{
		g_pWidget = lgui::pWidget("notebookSettings");
		g_signal_connect(G_OBJECT(g_pWidget),    "switch-page",     G_CALLBACK(on_nbook_switch_page), gRun.pMainWindow);
		g_signal_connect(G_OBJECT(pPropsDialog), "map-event",       G_CALLBACK(on_map_event),         gRun.pMainWindow);
		g_signal_connect(G_OBJECT(pPropsDialog), "key_press_event", G_CALLBACK(on_key_press),         pPropsDialog);
		g_signal_connect(G_OBJECT(pPropsDialog), "delete_event",    G_CALLBACK(on_settings_close),    NULL);

		initControl("buttonClose", g_clicked, G_CALLBACK(on_close_clicked), pPropsDialog);
		initControl("buttonHelp",  g_clicked, G_CALLBACK(on_help_clicked),  pPropsDialog);
		initControl("buttonSave",  g_clicked, G_CALLBACK(on_save_clicked),  pPropsDialog);

		cgui::setPopup(pPropsDialog, true, 's');
	}

	open(false);

#ifdef _DEBUGLOG
	GTimeVal            cte;
	g_get_current_time(&cte);
	DEBUGLOGP("  time end (%d.%d)\n",     (int) cte.tv_sec, (int)cte.tv_usec);
	DEBUGLOGP("  elapsed secs of %f\n", (float)(cte.tv_sec-ctb.tv_sec)+(float)(cte.tv_usec-ctb.tv_usec)*1.0e-6);
#endif

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
gboolean prefs::on_key_press(GtkWidget* pWidget, GdkEventKey* pKey, gpointer userData)
{
	DEBUGLOGB;
	DEBUGLOGP("  keyval is %d\n", (int)pKey->keyval);

	if( pKey->type == GDK_KEY_PRESS && pKey->keyval == GDK_KEY_Escape )
		on_settings_close(GTK_WIDGET(userData), NULL, NULL);

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
// The ::map-event signal is emitted when the widget's window is mapped. A
// window is mapped when it becomes visible on the screen.
// -----------------------------------------------------------------------------
gboolean prefs::on_map_event(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	DEBUGLOGP("opening is %s\n", opend() ? "true" : "false");

	if( !opend() )
	{
		DEBUGLOGB;
		on_nbook_switch_page(NULL, NULL, 0, NULL);
		DEBUGLOGE;
	}

	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean prefs::on_settings_close(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	DEBUGLOGB;

	gtk_widget_hide(pWidget);
	cgui::dnit();

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::set_geometry_related()
{
	DEBUGLOGB;

	Config   tcfg    = gCfg;
	int      sw      = gdk_screen_get_width (gdk_screen_get_default());
	int      sh      = gdk_screen_get_height(gdk_screen_get_default());
	gboolean tssOkay = TRUE, szIndex = SIZE_CUSTOM; // default

	cfg::cnvp(tcfg,  true);

	if( gCfg.clockW == gCfg.clockH )
	{
		switch( gCfg.clockW )
		{
		case  50: tssOkay = FALSE; szIndex = SIZE_SMALL;  break;
		case 100: tssOkay = FALSE; szIndex = SIZE_MEDIUM; break;
		case 200: tssOkay = FALSE; szIndex = SIZE_LARGE;  break;
		}
	}

	g_pWidget = initControl("comboboxStartupSize", g_changed, G_CALLBACK(on_startup_size_changed));
	gtk_combo_box_set_active(GTK_COMBO_BOX(g_pWidget), szIndex);
	gtk_widget_grab_focus(g_pWidget);

	g_pWidget = lgui::pWidget("tableStartupSize");
	gtk_widget_set_sensitive(g_pWidget, tssOkay);

	g_pWidget = lgui::pWidget(PREFSTR_WIDTH);
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(g_pWidget),  MIN_CLOCKW, sw);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_pWidget), gCfg.clockW);
	g_signal_connect(G_OBJECT(g_pWidget), g_valchgd, G_CALLBACK(on_width_value_changed),  gRun.pMainWindow);
	g_signal_connect(G_OBJECT(g_pWidget), g_btnreld, G_CALLBACK(on_exts_btn_release),     gRun.pMainWindow);

	g_pWidget = lgui::pWidget(PREFSTR_HEIGHT);
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(g_pWidget),  MIN_CLOCKH, sh);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_pWidget), gCfg.clockH);
	g_signal_connect(G_OBJECT(g_pWidget), g_valchgd, G_CALLBACK(on_height_value_changed), gRun.pMainWindow);
	g_signal_connect(G_OBJECT(g_pWidget), g_btnreld, G_CALLBACK(on_exts_btn_release),     gRun.pMainWindow);

	g_pWidget = lgui::pWidget(PREFSTR_X);
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(g_pWidget), -sw, sw);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_pWidget), tcfg.clockX);
	g_signal_connect(G_OBJECT(g_pWidget), g_valchgd, G_CALLBACK(on_x_value_changed), gRun.pMainWindow);

	g_pWidget = lgui::pWidget(PREFSTR_Y);
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(g_pWidget), -sh, sh);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_pWidget), tcfg.clockY);
	g_signal_connect(G_OBJECT(g_pWidget), g_valchgd, G_CALLBACK(on_y_value_changed), gRun.pMainWindow);

	initToggleBtn("togglebuttonLockDims", g_toggled, gRun.lockDims,                   on_lock_dims_toggled);
	initToggleBtn("togglebuttonLockOffs", g_toggled, gRun.lockOffs,                   on_lock_offs_toggled);
	initToggleBtn("radiobuttonTopLeft",   g_toggled, gCfg.clockC == CORNER_TOP_LEFT,  on_corner_toggled);
	initToggleBtn("radiobuttonTopRight",  g_toggled, gCfg.clockC == CORNER_TOP_RIGHT, on_corner_toggled);
	initToggleBtn("radiobuttonBotLeft",   g_toggled, gCfg.clockC == CORNER_BOT_LEFT,  on_corner_toggled);
	initToggleBtn("radiobuttonBotRight",  g_toggled, gCfg.clockC == CORNER_BOT_RIGHT, on_corner_toggled);
	initToggleBtn("radiobuttonCenter",    g_toggled, gCfg.clockC == CORNER_CENTER,    on_corner_toggled);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::set_clock_face_related()
{
	DEBUGLOGB;

	g_pWidget = lgui::pWidget(PREFSTR_THEMES);
//#if !GTK_CHECK_VERSION(3,0,0)
//	gtk_combo_box_append_text(GTK_COMBO_BOX(g_pWidget), gCfg.themeFile);
//	gtk_combo_box_set_active (GTK_COMBO_BOX(g_pWidget), 0);
//#endif
	on_themes_popup(GTK_COMBO_BOX(g_pWidget), gRun.pMainWindow);
	g_signal_connect(G_OBJECT(g_pWidget), g_changed, G_CALLBACK(on_themes_changed), gRun.pMainWindow);
//	g_signal_connect(G_OBJECT(g_pWidget), "popup",   G_CALLBACK(on_themes_popup),   gRun.pMainWindow);

//	g_pWidget = lgui::pWidget("labelThemeName");
//	gtk_label_set_text(GTK_LABEL(g_pWidget), gCfg.themeFile);
//	gtk_label_set_text(GTK_LABEL(g_pWidget), "");

	initToggleBtn(PREFSTR_SECONDS,   g_toggled, gCfg.showSeconds, on_seconds_toggled);
	initToggleBtn(PREFSTR_24HOUR,    g_toggled, gCfg.show24Hrs,   on_24_hour_toggled);
	initToggleBtn(PREFSTR_SHOWDATE,  g_toggled, gCfg.showDate,    on_show_date_toggled);
	initToggleBtn(PREFSTR_FACEDATE,  g_toggled, gCfg.faceDate,    on_face_date_toggled);
	initToggleBtn(PREFSTR_TEXTONLY,  g_toggled, gCfg.textOnly,    on_text_only_toggled);
	initToggleBtn(PREFSTR_HANDSONLY, g_toggled, gCfg.handsOnly,   on_hands_only_toggled);
	initToggleBtn(PREFSTR_CLICKTHRU, g_toggled, gCfg.clickThru,   on_click_thru_toggled);
	initToggleBtn(PREFSTR_NODISPLAY, g_toggled, gCfg.noDisplay,   on_no_display_toggled);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::set_hand_anim_related()
{
	DEBUGLOGB;

	set_ani_rate (gCfg.renderRate);
	initControl  (PREFSTR_ANIRATE,       g_valchgd,  G_CALLBACK(on_ani_value_changed));

	initToggleBtn("radiobuttonOriginal", g_toggled,  gCfg.shandType == draw::ANIM_ORIG,   on_ani_type_toggled);
	initToggleBtn("radiobuttonFlick",    g_toggled,  gCfg.shandType == draw::ANIM_FLICK,  on_ani_type_toggled);
	initToggleBtn("radiobuttonSweep",    g_toggled,  gCfg.shandType == draw::ANIM_SWEEP,  on_ani_type_toggled);
	initToggleBtn("radiobuttonCustom",   g_toggled,  gCfg.shandType == draw::ANIM_CUSTOM, on_ani_type_toggled);

	initToggleBtn("radiobuttonClockDT1", g_toggled, !gCfg.queuedDraw,                     on_ani_clock_toggled);
	initToggleBtn("radiobuttonClockDT2", g_toggled,  gCfg.queuedDraw,                     on_ani_clock_toggled);
	DEBUGLOGP("gCfg.queuedDraw is %s\n",             gCfg.queuedDraw ? "on" : "off");

	{
		bool               state[ 3];
		char               label[32];
		static const char* handFmt   =   "radiobutton%sDT%d";
		static const char* handLbl[] = { "Alarm", "Hour", "Minute", "Second" };
		static int         handMax[] = {  3,       3,      3,        2 };

		for( size_t h = 0; h < vectsz(handLbl); h++ )
		{
			state[0]  = !gCfg.optHand[h];
			state[1]  =  gCfg.optHand[h] &&  gCfg.useSurf[h];
			state[2]  =  gCfg.optHand[h] && !gCfg.useSurf[h];

			for( size_t s = 0; s < vectsz(state); s++ )
			{
				if( handMax[h] > s )
				{
					snprintf     (label, vectsz(label), handFmt, handLbl[h], s+1);
					initToggleBtn(label, g_toggled, state[s], on_ani_hand_toggled, gpointer(h+1));
				}
			}

			DEBUGLOGP("gCfg.optHand[%d] is %s\n", h, gCfg.optHand[h] ? "on" : "off");
			DEBUGLOGP("gCfg.useSurf[%d] is %s\n", h, gCfg.useSurf[h] ? "on" : "off");
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::set_face_text_related()
{
	DEBUGLOGB;

	static const char* g_focusout = "focus-out-event";

	g_pWidget = lgui::pWidget("entryDate12Format");
	gtk_entry_set_max_length (GTK_ENTRY(g_pWidget),   vectsz(gCfg.fmtDate12));
	gtk_entry_set_text       (GTK_ENTRY(g_pWidget),          gCfg.fmtDate12);
	g_signal_connect(G_OBJECT(g_pWidget), g_activate, G_CALLBACK(on_fmtDate12_activate),   gRun.pMainWindow);
	g_signal_connect(G_OBJECT(g_pWidget), g_focusout, G_CALLBACK(on_fmtDate12_focus_lost), gRun.pMainWindow);
	on_fmtshow_update        (GTK_ENTRY(g_pWidget),  "labelDate12Formatd");

	g_pWidget = lgui::pWidget("entryDate24Format");
	gtk_entry_set_max_length (GTK_ENTRY(g_pWidget),   vectsz(gCfg.fmtDate24));
	gtk_entry_set_text       (GTK_ENTRY(g_pWidget),          gCfg.fmtDate24);
	g_signal_connect(G_OBJECT(g_pWidget), g_activate, G_CALLBACK(on_fmtDate24_activate),   gRun.pMainWindow);
	g_signal_connect(G_OBJECT(g_pWidget), g_focusout, G_CALLBACK(on_fmtDate24_focus_lost), gRun.pMainWindow);
	on_fmtshow_update        (GTK_ENTRY(g_pWidget),  "labelDate24Formatd");

	g_pWidget = lgui::pWidget("entryTime12Format");
	gtk_entry_set_max_length (GTK_ENTRY(g_pWidget),   vectsz(gCfg.fmtTime12));
	gtk_entry_set_text       (GTK_ENTRY(g_pWidget),          gCfg.fmtTime12);
	g_signal_connect(G_OBJECT(g_pWidget), g_activate, G_CALLBACK(on_fmtTime12_activate),   gRun.pMainWindow);
	g_signal_connect(G_OBJECT(g_pWidget), g_focusout, G_CALLBACK(on_fmtTime12_focus_lost), gRun.pMainWindow);
	on_fmtshow_update        (GTK_ENTRY(g_pWidget),  "labelTime12Formatd");

	g_pWidget = lgui::pWidget("entryTime24Format");
	gtk_entry_set_max_length (GTK_ENTRY(g_pWidget),   vectsz(gCfg.fmtTime24));
	gtk_entry_set_text       (GTK_ENTRY(g_pWidget),          gCfg.fmtTime24);
	g_signal_connect(G_OBJECT(g_pWidget), g_activate, G_CALLBACK(on_fmtTime24_activate),   gRun.pMainWindow);
	g_signal_connect(G_OBJECT(g_pWidget), g_focusout, G_CALLBACK(on_fmtTime24_focus_lost), gRun.pMainWindow);
	on_fmtshow_update        (GTK_ENTRY(g_pWidget),  "labelTime24Formatd");

	g_pWidget = lgui::pWidget("entryTTip12Format");
	gtk_entry_set_max_length (GTK_ENTRY(g_pWidget),   vectsz(gCfg.fmtTTip12));
	gtk_entry_set_text       (GTK_ENTRY(g_pWidget),          gCfg.fmtTTip12);
	g_signal_connect(G_OBJECT(g_pWidget), g_activate, G_CALLBACK(on_fmtTTip12_activate),   gRun.pMainWindow);
	g_signal_connect(G_OBJECT(g_pWidget), g_focusout, G_CALLBACK(on_fmtTTip12_focus_lost), gRun.pMainWindow);
	on_fmtshow_update        (GTK_ENTRY(g_pWidget),  "labelTTip12Formatd");

	g_pWidget = lgui::pWidget("entryTTip24Format");
	gtk_entry_set_max_length (GTK_ENTRY(g_pWidget),   vectsz(gCfg.fmtTTip24));
	gtk_entry_set_text       (GTK_ENTRY(g_pWidget),          gCfg.fmtTTip24);
	g_signal_connect(G_OBJECT(g_pWidget), g_activate, G_CALLBACK(on_fmtTTip24_activate),   gRun.pMainWindow);
	g_signal_connect(G_OBJECT(g_pWidget), g_focusout, G_CALLBACK(on_fmtTTip24_focus_lost), gRun.pMainWindow);
	on_fmtshow_update        (GTK_ENTRY(g_pWidget),  "labelTTip24Formatd");

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::set_text_font_related()
{
	DEBUGLOGB;

#if !GTK_CHECK_VERSION(3,0,0)
	char              preview[1024];
	GtkWidget*        pSubWidget;
	GtkTreeSelection* pSelWidget;

	DEBUGLOGP("fname is '%s', size is %d\n", gCfg.fontName, gCfg.fontSize);

	static const char* txts[] = { gRun.cfaceTxta3, gRun.cfaceTxta2, gRun.cfaceTxta1, gRun.cfaceTxtb1, gRun.cfaceTxtb2, gRun.cfaceTxtb3 };

	*preview = '\0';
	for( size_t t = 0; t < vectsz(txts); t++ )
	{
		if( *txts[t] )
		{
			if( *preview )
				strvcat(preview, " ");
			strvcat(preview, txts[t]);
		}
	}

	g_pWidget = lgui::pWidget("fontselectionCDF");
	DEBUGLOGP("font selected is '%s'\n", gtk_font_selection_get_font_name(GTK_FONT_SELECTION(g_pWidget)));
	gtk_font_selection_set_font_name    (GTK_FONT_SELECTION(g_pWidget), gCfg.fontName);
	gtk_font_selection_set_preview_text (GTK_FONT_SELECTION(g_pWidget), preview);
	DEBUGLOGP("font selected is '%s'\n", gtk_font_selection_get_font_name(GTK_FONT_SELECTION(g_pWidget)));

	pSubWidget = gtk_font_selection_get_face_list  (GTK_FONT_SELECTION(g_pWidget));
	pSelWidget = gtk_tree_view_get_selection(GTK_TREE_VIEW(pSubWidget));
	g_signal_connect(G_OBJECT(pSelWidget), g_changed,  G_CALLBACK(on_font_face_list_changed),   gRun.pMainWindow);

	pSubWidget = gtk_font_selection_get_family_list(GTK_FONT_SELECTION(g_pWidget));
	pSelWidget = gtk_tree_view_get_selection(GTK_TREE_VIEW (pSubWidget));
	g_signal_connect(G_OBJECT(pSelWidget), g_changed,  G_CALLBACK(on_font_family_list_changed), gRun.pMainWindow);

	pSubWidget = gtk_font_selection_get_size_list  (GTK_FONT_SELECTION(g_pWidget));
	pSelWidget = gtk_tree_view_get_selection(GTK_TREE_VIEW(pSubWidget));
	g_signal_connect(G_OBJECT(pSelWidget), g_changed,  G_CALLBACK(on_font_size_list_changed),   gRun.pMainWindow);

	pSubWidget = gtk_font_selection_get_size_entry (GTK_FONT_SELECTION(g_pWidget));
	g_signal_connect(G_OBJECT(pSubWidget), g_activate, G_CALLBACK(on_font_size_entry_activate), gRun.pMainWindow);
#endif // !GTK_CHECK_VERSION(3,0,0)

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::set_text_color_related()
{
	DEBUGLOGB;

#if !GTK_CHECK_VERSION(3,0,0)

	GdkColor csc;
	guint16  alf;
	g_pWidget =  lgui::pWidget("colorselectionCDC");

//	gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(g_pWidget), &csc);
//	DEBUGLOGP("color selected is %d-%d-%d\n", (int)csc.red, (int)csc.green, (int)csc.blue);

	csc.red   = (guint16)(gCfg.dateTxtRed*65535.0);
	csc.green = (guint16)(gCfg.dateTxtGrn*65535.0);
	csc.blue  = (guint16)(gCfg.dateTxtBlu*65535.0);
	alf       = (guint16)(gCfg.dateTxtAlf*65535.0);

	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(g_pWidget), &csc);
	gtk_color_selection_set_current_alpha(GTK_COLOR_SELECTION(g_pWidget),  alf);
	gtk_color_selection_set_update_policy(GTK_COLOR_SELECTION(g_pWidget),  GTK_UPDATE_DELAYED);

//	csc.red   = csc.green = csc.blue = 0;
//	gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(g_pWidget), &csc);
//	DEBUGLOGP("color selected is %d-%d-%d\n", (int)csc.red, (int)csc.green, (int)csc.blue);

	g_signal_connect(G_OBJECT(g_pWidget), "color-changed", G_CALLBACK(on_font_color_changed),     gRun.pMainWindow);
	g_signal_connect(G_OBJECT(g_pWidget), "event-after",   G_CALLBACK(on_font_color_event_after), gRun.pMainWindow);

#endif // !GTK_CHECK_VERSION(3,0,0)

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::set_text_spacing_related()
{
	DEBUGLOGB;

	static const char* lbls[] =
	{
		"A3SF",        "A2SF",        "A1SF",        "B1SF",        "B2SF",        "B3SF",
		"A3HF",        "A2HF",        "A1HF",        "B1HF",        "B2HF",        "B3HF",
//		"A3HO",        "A2HO",        "A1HO",        "B1HO",        "B2HO",        "B3HO"
	};

	static double*     vals[] =
	{
		&gCfg.sfTxta3, &gCfg.sfTxta2, &gCfg.sfTxta1, &gCfg.sfTxtb1, &gCfg.sfTxtb2, &gCfg.sfTxtb3,
		&gCfg.hfTxta3, &gCfg.hfTxta2, &gCfg.hfTxta1, &gCfg.hfTxtb1, &gCfg.hfTxtb2, &gCfg.hfTxtb3,
//		&gCfg.haTxta3, &gCfg.haTxta2, &gCfg.haTxta1, &gCfg.haTxtb1, &gCfg.haTxtb2, &gCfg.haTxtb3
	};

	char lbl[64];
	for( size_t l = 0; l < vectsz(lbls); l++ )
	{
		snprintf(lbl, vectsz(lbl), "spinbuttonCTS%s", lbls[l]);

		if( g_pWidget = lgui::pWidget(lbl) )
		{
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_pWidget), double(*vals[l]));
			g_signal_connect(G_OBJECT(g_pWidget), g_valchgd, G_CALLBACK(on_text_spacing_value_changed), vals[l]);
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::set_extras_related()
{
	DEBUGLOGB;

	initToggleBtn(PREFSTR_KEEPONBOT,   g_toggled, gCfg.keepOnBot,    on_keep_on_bot_toggled);
	initToggleBtn(PREFSTR_KEEPONTOP,   g_toggled, gCfg.keepOnTop,    on_keep_on_top_toggled);
	initToggleBtn(PREFSTR_SHOWINPAGER, g_toggled, gCfg.showInPager,  on_show_in_pager_toggled);
	initToggleBtn(PREFSTR_SHOWINTASKS, g_toggled, gCfg.showInTasks,  on_show_in_taskbar_toggled);
	initToggleBtn(PREFSTR_STICKY,      g_toggled, copts::sticky(),   on_sticky_toggled);
	initToggleBtn(PREFSTR_ANISTARTUP,  g_toggled, gCfg.aniStartup,   on_ani_startup_toggled);
	initToggleBtn(PREFSTR_TAKEFOCUS,   g_toggled, gCfg.takeFocus,    on_take_focus_toggled);
	initToggleBtn(PREFSTR_PLAYSOUNDS,  g_toggled, gCfg.doSounds,     on_do_sounds_toggled);
	initToggleBtn(PREFSTR_UNSTICKY,    g_toggled, gCfg.clockWS != 0, on_unsticky_toggled);
	initToggleBtn(PREFSTR_OPACITY,     g_toggled, gCfg.opacity != 1, on_opacity_toggled);

	int nws   = g_workspace_count();

	g_pWidget = lgui::pWidget("hscaleUnSticky");
	gtk_range_set_range(GTK_RANGE(g_pWidget), 0, nws);
	gtk_range_set_value(GTK_RANGE(g_pWidget), gCfg.clockWS);
	g_signal_connect(G_OBJECT(g_pWidget), g_valchgd, G_CALLBACK(on_ws_value_changed), gRun.pMainWindow);

	if( g_pWidget = lgui::pWidget("toolbarWorkspaces") )
	{
//		DEBUGLOGS("located workspaces toolbar");
//		DEBUGLOGP("  cfg clock ws mask is %d\n", gCfg.clockWS);

		GtkWidget*   pIco;
		GtkToolItem* pTBI;
		char         lbl[64];

		for( size_t w = 0; w < nws; w++ )
		{
//			if( pTBI = gtk_toggle_tool_button_new_from_stock(GTK_STOCK_HOME) )
			if( pTBI = gtk_toggle_tool_button_new() )
			{
//				DEBUGLOGS("created a workspace toolbar button");

//				snprintf(lbl, vectsz(lbl), "%d", w+1);
//				snprintf(lbl, vectsz(lbl), "# %d", w+1);
				snprintf(lbl, vectsz(lbl), "workspace %d", w+1);
				gtk_widget_set_tooltip_text(GTK_WIDGET(pTBI), lbl);
//				gtk_tool_button_set_label(GTK_TOOL_BUTTON(pTBI), lbl);

//				pIco = gtk_image_new_from_icon_name("computer", GTK_ICON_SIZE_SMALL_TOOLBAR);
				pIco = gtk_image_new_from_icon_name("user-desktop", GTK_ICON_SIZE_SMALL_TOOLBAR);

				g_signal_connect(G_OBJECT(pTBI), g_toggled, G_CALLBACK(on_ws_use_toggled), gRun.pMainWindow);

				if( (gCfg.clockWS & (1 << w)) == (1 << w) )
					gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(pTBI), TRUE);

				gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(pTBI), pIco);

				gtk_toolbar_insert(GTK_TOOLBAR(g_pWidget), pTBI, -1);

				gtk_widget_set_visible(GTK_WIDGET(pTBI), TRUE);
				gtk_widget_set_visible(pIco, TRUE);
			}
//			else
//				DEBUGLOGS("couldn't create a workspace toolbar button");
		}

//		DEBUGLOGP("toolbar says it has %d buttons\n", gtk_toolbar_get_n_items(GTK_TOOLBAR(g_pWidget)));
	}
//	else
//		DEBUGLOGS("couldn't locate workspaces toolbar");

	if( nws < 2 )
	{
		g_pWidget = lgui::pWidget(PREFSTR_UNSTICKY);
		gtk_widget_set_sensitive(g_pWidget, FALSE);
		g_pWidget = lgui::pWidget("hscaleUnSticky");
		gtk_widget_set_sensitive(g_pWidget, FALSE);
	}

	g_pWidget = lgui::pWidget("hscaleOpacity");
	gtk_range_set_range(GTK_RANGE(g_pWidget), 0, 100);
	gtk_range_set_value(GTK_RANGE(g_pWidget), gCfg.opacity*100.0);
	g_signal_connect(G_OBJECT(g_pWidget), g_valchgd, G_CALLBACK(on_opacity_value_changed), gRun.pMainWindow);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::on_button_toggled(GtkToggleButton* pToggButton, gpointer window, gint& flag, bool redraw, bool update)
{
	flag = pToggButton ? (gtk_toggle_button_get_active(pToggButton) ? 1 : 0) : !flag;

	if( redraw )
		gtk_widget_queue_draw(GTK_WIDGET(window));

	if( update )
	{
		// TODO: change to something else?
		update_wnd_dim(GTK_WIDGET(window), gCfg.clockW, gCfg.clockH, gCfg.clockW, gCfg.clockH, true);
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::on_24_hour_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

//	on_button_toggled(pToggButton, window, gCfg.show24Hrs, true, true);
	on_button_toggled(pToggButton, window, gCfg.show24Hrs, true, false);

	alarm_set(true);
	draw::update_marks();
	update_ts_info(GTK_WIDGET(window), true, true, true);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_ani_startup_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.aniStartup, false, false);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_ani_clock_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	if( gtk_toggle_button_get_active(pToggButton) )
	{
		const char* name       = gtk_widget_get_name(GTK_WIDGET(pToggButton));
		int         btnSeld    = name[strlen(name)-1] - '0';
		int         queuedDraw = btnSeld == 2;

		if( gCfg.queuedDraw != queuedDraw )
		{
			gCfg.queuedDraw  = queuedDraw;

			DEBUGLOGS("clock drawing tech changed");
			change_ani_rate(GTK_WIDGET(window), gCfg.renderRate, true, false);
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_ani_hand_toggled(GtkToggleButton* pToggButton, gpointer userData)
{
	OPENING_RET;
	DEBUGLOGB;

	if( gtk_toggle_button_get_active(pToggButton) )
	{
		DEBUGLOGP("toggled hand button %d is active\n", int(userData));

		int         hndType = (int)userData - 1; // -1 to allow 1 for alarm, which is a 0 index into optHand/useSurf
		int         optHand =  gCfg.optHand[hndType];
		int         useSurf =  gCfg.useSurf[hndType];
		const char* name    =  gtk_widget_get_name(GTK_WIDGET(pToggButton));
		int         btnSeld =  name[strlen(name)-1] - '0';

		DEBUGLOGP("hand type: %d, optHand: %d, useSurf: %d\n", hndType, optHand, useSurf);
		DEBUGLOGP("btnSeld:   %d, name:    %s\n",              btnSeld, name);

		switch( btnSeld )
		{
		case 1: optHand = FALSE; useSurf = FALSE; break; // direct     (rsvg draws to window cairo context every refresh)
		case 2: optHand = TRUE;  useSurf = TRUE;  break; // buffered   (rsvg draws to hand type  surf then surf drawn to window cairo context every refresh)
		case 3: optHand = TRUE;  useSurf = FALSE; break; // clock back (rsvg draws to background surf then surf drawn to window cairo context every minute)
		}

		if( gCfg.optHand[hndType] != optHand || gCfg.useSurf[hndType] != useSurf )
		{
			bool bkgnd             = hndType >= 0 && hndType <= 2; // alarm, hour, or minute hand change
			gCfg.optHand[hndType]  = optHand;
			gCfg.useSurf[hndType]  = useSurf;

			DEBUGLOGP("draw tech chg: optHand: %d, useSurf: %d\n", optHand, useSurf);

			if( bkgnd )
				draw::update_bkgnd(false);

			draw::reset_hands();
			draw::render_set (NULL, true, true);
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_ani_type_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	if( gtk_toggle_button_get_active(pToggButton) )
	{
		int         shandType = gCfg.shandType;
		const char* name      = gtk_widget_get_name(GTK_WIDGET(pToggButton));

		shandType = strcmp(name, "radiobuttonOriginal") == 0 ? draw::ANIM_ORIG   : shandType;
		shandType = strcmp(name, "radiobuttonFlick")    == 0 ? draw::ANIM_FLICK  : shandType;
		shandType = strcmp(name, "radiobuttonSweep")    == 0 ? draw::ANIM_SWEEP  : shandType;
		shandType = strcmp(name, "radiobuttonCustom")   == 0 ? draw::ANIM_CUSTOM : shandType;

		if( gCfg.shandType != shandType )
		{
			gCfg.shandType  = shandType;
			draw::reset_ani();
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_ani_value_changed(GtkRange* pRange, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	gint renderRate = (gint)gtk_range_get_value(pRange);
	change_ani_rate(GTK_WIDGET(window), renderRate, false, false);
	draw::update_ani();

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_click_thru_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.clickThru, false, false);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_close_clicked(GtkButton* pButton, gpointer window)
{
	DEBUGLOGB;

	end();

	// TODO: need to check if anything's been changed before saving

	cfg::save(); // TODO: this only happens here & not via delete event - is this okay?

	gboolean rc;
	gpointer pDlg = (gpointer)gtk_widget_get_toplevel(GTK_WIDGET(pButton));
	g_signal_emit_by_name(pDlg, "delete_event", NULL, &rc);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_corner_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	if( gtk_toggle_button_get_active(pToggButton) )
	{
		gint         clockC = gCfg.clockC;
		const gchar* name   = gtk_widget_get_name(GTK_WIDGET(pToggButton));

		clockC = strcmp(name, "radiobuttonTopLeft")  == 0 ? CORNER_TOP_LEFT  : clockC;
		clockC = strcmp(name, "radiobuttonTopRight") == 0 ? CORNER_TOP_RIGHT : clockC;
		clockC = strcmp(name, "radiobuttonBotLeft")  == 0 ? CORNER_BOT_LEFT  : clockC;
		clockC = strcmp(name, "radiobuttonBotRight") == 0 ? CORNER_BOT_RIGHT : clockC;
		clockC = strcmp(name, "radiobuttonCenter")   == 0 ? CORNER_CENTER    : clockC;

		if( gCfg.clockC != clockC )
		{
			// NOTE: shouldn't need to do anything more than change the corner since the
			//       clockX/Y values are changed at runtime to always be relative to
			//       the (0, 0) coordinate?

			DEBUGLOGP("corner is now set to %s\n", name);

			gCfg.clockC = clockC;

			Config    tcfg = gCfg;
			cfg::cnvp(tcfg,  true);

//			gCfg.clockX = tcfg.clockX;
//			gCfg.clockY = tcfg.clockY;

			GtkWidget* pSpinButtonX = lgui::pWidget(PREFSTR_X);
			GtkWidget* pSpinButtonY = lgui::pWidget(PREFSTR_Y);

			gtk_spin_button_set_value(GTK_SPIN_BUTTON(pSpinButtonX), (gdouble)tcfg.clockX);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(pSpinButtonY), (gdouble)tcfg.clockY);

//			gtk_window_move(GTK_WINDOW(window), tcfg.clockX, tcfg.clockY);
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_do_sounds_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.doSounds, false, false);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
gboolean prefs::on_exts_btn_release(GtkWidget* pWidget, GdkEvent* pEvent, gpointer window)
{
	DEBUGLOGB;

	if( !gRun.updating )
	{
		pWidget   = lgui::pWidget(PREFSTR_WIDTH);
		gint newW = pWidget ? gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pWidget)) : gCfg.clockW;
		pWidget   = lgui::pWidget(PREFSTR_HEIGHT);
		gint newH = pWidget ? gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pWidget)) : gCfg.clockH;

		if( newW != gCfg.clockW || newH != gCfg.clockH )
		{
			DEBUGLOGS("surfs updating/resizing/redrawing wnd");
			DEBUGLOGP("old dims: %dx%d, new dims:%dx%d\n", gCfg.clockW, gCfg.clockH, newW, newH);
			update_wnd_dim(GTK_WIDGET(window), newW, newH, gCfg.clockW, gCfg.clockH, true, false);
		}
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
void prefs::on_face_date_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.faceDate, true, false);

//	update_ts_text(true); // TODO: needed?
	draw::update_date_surf(true);
	draw::render_set();
	gtk_widget_queue_draw(GTK_WIDGET(window));

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::on_fmtDate12_activate(GtkEntry* pEntry, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_fmttext_update(pEntry, gCfg.fmtDate12, vectsz(gCfg.fmtDate12), "labelDate12Formatd", window, false, true);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmtDate12_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_fmttext_update(pEntry, gCfg.fmtDate12, vectsz(gCfg.fmtDate12), "labelDate12Formatd", window, false, true);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmtDate24_activate(GtkEntry* pEntry, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_fmttext_update(pEntry, gCfg.fmtDate24, vectsz(gCfg.fmtDate24), "labelDate24Formatd", window, false, true);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmtDate24_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_fmttext_update(pEntry, gCfg.fmtDate24, vectsz(gCfg.fmtDate24), "labelDate24Formatd", window, false, true);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmtTime12_activate(GtkEntry* pEntry, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_fmttext_update(pEntry, gCfg.fmtTime12, vectsz(gCfg.fmtTime12), "labelTime12Formatd", window, false, false, true);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmtTime12_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_fmttext_update(pEntry, gCfg.fmtTime12, vectsz(gCfg.fmtTime12), "labelTime12Formatd", window, false, false, true);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmtTime24_activate(GtkEntry* pEntry, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_fmttext_update(pEntry, gCfg.fmtTime24, vectsz(gCfg.fmtTime24), "labelTime24Formatd", window, false, false, true);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmtTime24_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_fmttext_update(pEntry, gCfg.fmtTime24, vectsz(gCfg.fmtTime24), "labelTime24Formatd", window, false, false, true);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmtTTip12_activate(GtkEntry* pEntry, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_fmttext_update(pEntry, gCfg.fmtTTip12, vectsz(gCfg.fmtTTip12), "labelTTip12Formatd", window, true);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmtTTip12_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_fmttext_update(pEntry, gCfg.fmtTTip12, vectsz(gCfg.fmtTTip12), "labelTTip12Formatd", window, true);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmtTTip24_activate(GtkEntry* pEntry, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_fmttext_update(pEntry, gCfg.fmtTTip24, vectsz(gCfg.fmtTTip24), "labelTTip24Formatd", window, true);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmtTTip24_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_fmttext_update(pEntry, gCfg.fmtTTip24, vectsz(gCfg.fmtTTip24), "labelTTip24Formatd", window, true);
	DEBUGLOGE;
}
// -----------------------------------------------------------------------------
void prefs::on_fmtshow_update(GtkEntry* pEntry, const char* fmtShow)
{
	const char* str = gtk_entry_get_text(pEntry);

	if( str && fmtShow && (prefs::g_pWidget = lgui::pWidget(fmtShow)) )
	{
		gchar    tstr[1024]; tstr[0] = '\0';
		strfmtdt(tstr, vectsz(tstr), str, &gRun.timeCtm);
		gtk_label_set_markup(GTK_LABEL(prefs::g_pWidget), tstr);
	}
}

// -----------------------------------------------------------------------------
void prefs::on_fmttext_update(GtkEntry* pEntry, char* fmtText, int maxText, const char* fmtShow, gpointer window, bool ttip, bool date, bool time)
{
	const char* str = gtk_entry_get_text(pEntry);

	if( str && strcmp(fmtText, str) )
	{
		strvcpy(fmtText, str, maxText);
		update_ts_info(GTK_WIDGET(window), ttip, date, time);
#if 0
		if( fmtShow && (prefs::g_pWidget = lgui::pWidget(fmtShow)) )
		{
			gchar    tstr[1024]; tstr[0] = '\0';
			strfmtdt(tstr, vectsz(tstr), str, &gRun.timeCtm);
			gtk_label_set_markup(GTK_LABEL(prefs::g_pWidget), tstr);
		}
#endif
		on_fmtshow_update(pEntry, fmtShow);
	}
}

#if !GTK_CHECK_VERSION(3,0,0)
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::on_font_changed(gpointer window)
{
	DEBUGLOGB;

	GtkWidget* pWidget = lgui::pWidget("fontselectionCDF");

	if( pWidget )
	{
		GtkFontSelection* pFontSel = GTK_FONT_SELECTION(pWidget);

		if( pFontSel )
		{
			const char* fname = gtk_font_selection_get_font_name(pFontSel);
			int         fsize = gtk_font_selection_get_size     (pFontSel)/1024; // docs don't indicate this, but it works

			if( fname && fsize && (strcmp(gCfg.fontName, fname) != 0 || fsize != gCfg.fontSize) )
			{
				DEBUGLOGP("selected font name/size is:\n%s (%d)\n", fname, fsize);

				strvcpy(gCfg.fontName, fname);
				gCfg.fontSize = fsize;

				draw::chg();
				draw::update_date_surf();
				gtk_widget_queue_draw(GTK_WIDGET(window));
			}

			if( fname )
				g_free((gpointer)fname);
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_font_face_list_changed(GtkTreeSelection* pTreeSelection, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_font_changed(window);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_font_family_list_changed(GtkTreeSelection* pTreeSelection, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_font_changed(window);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_font_size_list_changed(GtkTreeSelection* pTreeSelection, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	on_font_changed(window);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_font_size_entry_activate(GtkEntry* pEntry, gpointer window)
{
	OPENING_RET;
	DEBUGLOGS("entry/exit");
}
#endif // !GTK_CHECK_VERSION(3,0,0)

#if !GTK_CHECK_VERSION(3,0,0)
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::on_font_color_changed(GtkColorSelection* pColorSelection, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	int                                x, y;
	GdkModifierType                          mask;
	get_cursor_pos(GTK_WIDGET(window), x, y, mask);

	// only update the clock's face date text color when the widget is 'idle',
	//  i.e., when no adjusting is being made and the left mouse button is up

	bool update = !gtk_color_selection_is_adjusting(pColorSelection) && ((mask & GDK_BUTTON1_MASK) != GDK_BUTTON1_MASK);

	if( update )
	{
		GdkColor csc;
		                   gtk_color_selection_get_current_color(pColorSelection, &csc);
		guint16  csc_alf = gtk_color_selection_get_current_alpha(pColorSelection);

		guint16  cur_red = guint16(gCfg.dateTxtRed*255.0);
		guint16  cur_grn = guint16(gCfg.dateTxtGrn*255.0);
		guint16  cur_blu = guint16(gCfg.dateTxtBlu*255.0);
		guint16  cur_alf = guint16(gCfg.dateTxtAlf*255.0);

		guint16  new_red = guint16(guint32(csc.red  *255)/65535);
		guint16  new_grn = guint16(guint32(csc.green*255)/65535);
		guint16  new_blu = guint16(guint32(csc.blue *255)/65535);
		guint16  new_alf = guint16(guint32(csc_alf  *255)/65535);

		if( cur_red != new_red || cur_grn != new_grn || cur_blu != new_blu || cur_alf != new_alf )
		{
			gCfg.dateTxtRed = (double)new_red/255.0;
			gCfg.dateTxtGrn = (double)new_grn/255.0;
			gCfg.dateTxtBlu = (double)new_blu/255.0;
			gCfg.dateTxtAlf = (double)new_alf/255.0;

			DEBUGLOGP("new is (%d, %d, %d, %d)\n", int(new_red), int(new_grn), int(new_blu), int(new_alf));

//			update_ts_text(true); // TODO: needed?
			draw::update_date_surf();
			gtk_widget_queue_draw(GTK_WIDGET(window));
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_font_color_event_after(GtkColorSelection* pColorSelection, GdkEvent* pEvent, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	int                                x, y;
	GdkModifierType                          mask;
	get_cursor_pos(GTK_WIDGET(window), x, y, mask);

	// only update the clock's face date text color when the widget is 'idle',
	//  i.e., when no adjusting is being made and the left mouse button is up

	bool update = !gtk_color_selection_is_adjusting(pColorSelection) && ((mask & GDK_BUTTON1_MASK) != GDK_BUTTON1_MASK);

	if( update )
		on_font_color_changed(pColorSelection, window);

	DEBUGLOGE;
}
#endif // !GTK_CHECK_VERSION(3,0,0)

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::on_hands_only_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.handsOnly, true, false);

	draw::update_bkgnd();
	draw::render_set();

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_height_value_changed(GtkSpinButton* pSpinButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	gint                                     oldW,  oldH;
	gtk_window_get_size(GTK_WINDOW(window), &oldW, &oldH);
	DEBUGLOGP("old dims %d x %d\n",          oldW,  oldH);

	if( !gRun.updating )
	{
		gint newH = gtk_spin_button_get_value_as_int(pSpinButton);
		gint newW = oldW;

		if( gRun.lockDims )
		{
			open(true); // to prevent width value change from doing any processing
			g_pWidget = lgui::pWidget(PREFSTR_WIDTH);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_pWidget), newW = newH);
			open(false);
		}

		set_wnd_size(GTK_WIDGET(window), oldW, oldH, newW, newH);
	}
	else
	{
		gtk_spin_button_set_value(pSpinButton, oldH);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_help_clicked(GtkButton* pButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	GtkWidget* pPrefsHelpBox =
		gtk_message_dialog_new((GtkWindow*)window,
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
			"No help available yet, other than through the command line via -? or --help.\n\nSorry.");

	if( pPrefsHelpBox )
	{
		gtk_window_set_transient_for(GTK_WINDOW(pPrefsHelpBox), (GtkWindow*)window);
		gtk_window_set_position(GTK_WINDOW(pPrefsHelpBox), GTK_WIN_POS_CENTER);
		gtk_window_set_title(GTK_WINDOW(pPrefsHelpBox), gRun.appName);
		gtk_dialog_run(GTK_DIALOG(pPrefsHelpBox));
		gtk_widget_destroy(pPrefsHelpBox);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_keep_on_bot_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.keepOnBot, false, false);
	copts::keep_on_bot(GTK_WIDGET(window), gCfg.keepOnBot);
	ToggleBtnSet      (PREFSTR_KEEPONTOP,  gCfg.keepOnTop);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_keep_on_top_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.keepOnTop, false, false);
	copts::keep_on_top(GTK_WIDGET(window), gCfg.keepOnTop);
	ToggleBtnSet      (PREFSTR_KEEPONBOT,  gCfg.keepOnBot);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_lock_dims_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gRun.lockDims, false, false);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_lock_offs_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gRun.lockOffs, false, false);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_no_display_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.noDisplay, false, false);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_nbook_switch_page(GtkNotebook* pNotebook, gpointer page, guint index, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	DEBUGLOGP("index is %d\n", index);

	if( g_tabset[index] )
	{
		DEBUGLOGR(1);
		return;
	}

	open(true);

	switch( index )
	{
	case 0: set_geometry_related();     break;
	case 1: set_clock_face_related();   break;
	case 2: set_hand_anim_related();    break;
	case 3: set_face_text_related();    break;
	case 4: set_text_font_related();    break;
	case 5: set_text_color_related();   break;
	case 6: set_text_spacing_related(); break;
	case 7: set_extras_related();       break;
	}

	g_tabset[index] = true;

	open(false);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_opacity_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

//	on_button_toggled(pToggButton, window, gRun.useOpacity, false, false);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_opacity_value_changed(GtkRange* pRange, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	GtkWidget* pWidget = lgui::pWidget("hscaleOpacity");
	gCfg.opacity       = gtk_range_get_value(GTK_RANGE(pWidget))/100.0;
	gdk_window_set_opacity(gtk_widget_get_window(GTK_WIDGET(window)), gCfg.opacity);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_save_clicked(GtkButton* pButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	cfg::save(false, true);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_seconds_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.showSeconds, true, false);
	change_ani_rate(GTK_WIDGET(window), gCfg.renderRate, true, false);
	draw::render_set();

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_text_spacing_value_changed(GtkSpinButton* pSpinButton, gpointer value)
{
	OPENING_RET;
	DEBUGLOGB;
	*((double*)value) = (double)gtk_spin_button_get_value(pSpinButton);
	draw::update_date_surf();
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_show_date_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.showDate, true, false);

	if( gCfg.showDate )
		update_ts_text(true); // TODO: needed? could be out of date?

	draw::update_date_surf(false, false, true, true);
	draw::render_set();
	gtk_widget_queue_draw(GTK_WIDGET(window));

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_show_in_pager_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.showInPager, false, false);
	copts::pagebar_shown(GTK_WIDGET(window), copts::pagebar_shown());

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_show_in_taskbar_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.showInTasks, false, false);
	copts::taskbar_shown(GTK_WIDGET(window), copts::taskbar_shown());

	DEBUGLOGS("making theme icon");
	make_theme_icon(GTK_WIDGET(window));

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_startup_size_changed(GtkComboBox* pComboBox, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	gboolean tssOkay;
	int      wsz, hsz;
	gint     cbi = gtk_combo_box_get_active(pComboBox);

	DEBUGLOGP("cbi=%d\n", cbi);

	if( tssOkay = (cbi == SIZE_CUSTOM) )
	{
		open(true); // to prevent width/height value changes from doing any processing
		GtkWidget* pWidget;
		pWidget =  lgui::pWidget(PREFSTR_WIDTH);
		wsz     =  pWidget ? gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pWidget)) : gCfg.clockW;
		pWidget =  lgui::pWidget(PREFSTR_HEIGHT);
		hsz     =  pWidget ? gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pWidget)) : gCfg.clockH;
		open(false);
	}
	else
	{
		switch( cbi )
		{
		case SIZE_SMALL:  wsz = hsz =  50; break;
		case SIZE_MEDIUM: wsz = hsz = 100; break;
		case SIZE_LARGE:  wsz = hsz = 200; break;
		default:          wsz = hsz =   0; break;
		}
	}

	if( wsz && hsz )
	{
		GtkWidget* pWidget = lgui::pWidget("tableStartupSize");
		gtk_widget_set_sensitive(pWidget, tssOkay);

		DEBUGLOGS("updating surfs/resizing/redrawing window");
		DEBUGLOGP("old dims: %dx%d, new dims:%dx%d\n", gCfg.clockW, gCfg.clockH, wsz,  hsz);
		update_wnd_dim(GTK_WIDGET(window), wsz, hsz,   gCfg.clockW, gCfg.clockH, true, false);
	}
	else
	{
		DEBUGLOGS("invalid index retrieved");
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_sticky_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.sticky, true, false);
	copts::sticky(GTK_WIDGET(window), copts::sticky());

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_take_focus_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.takeFocus, false, false);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_text_only_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	on_button_toggled(pToggButton, window, gCfg.textOnly, true, false);
	change_ani_rate(GTK_WIDGET(window), gCfg.renderRate, true, false);

//	update_ts_text(true); // TODO: needed?
	draw::update_date_surf();
	draw::render_set();
	gtk_widget_queue_draw(GTK_WIDGET(window));

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_themes_changed(GtkComboBox* pComboBox, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

//	int index  =  opend() ? -1 : gtk_combo_box_get_active(pComboBox);
	int index  =  gtk_combo_box_get_active(pComboBox);
	if( index != -1 )
	{
#if GTK_CHECK_VERSION(3,0,0)
		gchar* theme = NULL;
#else
		gchar* theme = gtk_combo_box_get_active_text(pComboBox); // TODO: needed?
#endif
		if( theme )
		{
			DEBUGLOGP("changing to new theme %s at index %d\n", theme, index);

			g_free(theme);

			DEBUGLOGS("bef settings pointers");

			ThemeList*     tl;
			theme_list_get(tl);

			ThemeEntry     te = theme_list_nth(tl, index);
			set_theme_info(te);

			DEBUGLOGS("aft settings pointers");

//			DEBUGLOGS("bef copying strings");

//			strvcpy(gCfg.themePath, te.pPath->str);
//			strvcpy(gCfg.themeFile, te.pFile->str);

//			DEBUGLOGS("aft copying strings");

			DEBUGLOGS("bef changing themes and updating the input shape");

//			gRun.updateSurfs = true;

			change_theme(GTK_WIDGET(window), te, true);

//			update_shapes(GTK_WIDGET(window), gCfg.clockW, gCfg.clockH, false);
/*			update_shapes(GTK_WIDGET(window), gCfg.clockW, gCfg.clockH, true);*/

			DEBUGLOGS("aft changing themes and updating the input shape");

			theme_list_del(tl);
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_themes_popup(GtkComboBox* pComboBox, gpointer window)
{
//	OPENING_RET;
	DEBUGLOGB;

	ThemeEntry     te;
	ThemeList*     tl;
	theme_list_get(tl);

	bool       trunc;
	GString*   pName;
	bool       opened        =  opend();
	int        themeListCur  = -1;
	int        themeListIdx  =  0;
//	const int  themeListRows = 15;
	const int  themeListRows = 16;
	const int  themeListCnt  = theme_list_cnt(tl);
	const bool themeListLrg  = themeListCnt >  themeListRows*3;
	const int  themeListCols = themeListLrg ? (themeListCnt-1)/themeListRows+1 : 4;
	const int  themeNameMax  = themeListLrg ?  20 : 30;

	DEBUGLOGP("theme count is %d\n", themeListCnt);

	if( opened == false )
		open(true);

	if( GTK_IS_COMBO_BOX(pComboBox) )
	{
		gtk_combo_box_clear             (pComboBox);
		gtk_combo_box_set_focus_on_click(pComboBox, TRUE);
		gtk_combo_box_set_wrap_width    (pComboBox, themeListCols);
	}

	for( te = theme_list_beg(tl); !theme_list_end(tl); te = theme_list_nxt(tl) )
	{
		pName = te.pName;

		if( trunc = pName->len > themeNameMax )
		{
			pName = g_string_new_len(pName->str, themeNameMax);
			g_string_append(pName, " ...");
		}

#if !GTK_CHECK_VERSION(3,0,0)
		if( GTK_IS_COMBO_BOX(pComboBox) )
			gtk_combo_box_append_text(pComboBox, pName->str);
#endif
		if( trunc )
			g_string_free(pName, TRUE);

		DEBUGLOGP("added list theme %s\n", te.pName->str);

		if( themeListCur == -1 )
		{
			if( strcmp(gCfg.themePath, te.pPath->str) == 0 &&
				strcmp(gCfg.themeFile, te.pFile->str) == 0 )
			{
				themeListCur = themeListIdx;
				set_theme_info(te);
			}
		}

		themeListIdx++;
	}

	theme_list_del(tl);

	if( GTK_IS_COMBO_BOX(pComboBox) )
	{
		if( themeListCur != -1 )
			gtk_combo_box_set_active(pComboBox, themeListCur);
	}

	if( opened == false )
		open(false);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_unsticky_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_width_value_changed(GtkSpinButton* pSpinButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	gint                                     oldW,  oldH;
	gtk_window_get_size(GTK_WINDOW(window), &oldW, &oldH);
	DEBUGLOGP("old dims %d x %d\n",          oldW,  oldH);

	if( !gRun.updating )
	{
		gint newW = gtk_spin_button_get_value_as_int(pSpinButton);
		gint newH = oldH;

		if( gRun.lockDims )
		{
			open(true); // to prevent height value change from doing any processing
			GtkWidget* pWidget = lgui::pWidget(PREFSTR_HEIGHT);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), newH = newW);
			open(false);
		}

		set_wnd_size(GTK_WIDGET(window), oldW, oldH, newW, newH);
	}
	else
	{
		gtk_spin_button_set_value(pSpinButton, oldW);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_ws_use_toggled(GtkToggleToolButton* pToggToolButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	gint wso = gCfg.clockWS;
	bool use = gtk_toggle_tool_button_get_active(pToggToolButton) == TRUE;
	int  wsb = gtk_toolbar_get_item_index(GTK_TOOLBAR(gtk_widget_get_parent(GTK_WIDGET(pToggToolButton))), GTK_TOOL_ITEM(pToggToolButton));
	gint wsm = 1   << wsb;
	gint wsn = use ?  wso | wsm : wso & ~wsm;

#ifdef _DEBUGLOG
	gchar* pTTipTxt = gtk_widget_get_tooltip_text(GTK_WIDGET(pToggToolButton));
	DEBUGLOGP("%s (%d) toggled: now %s\n", pTTipTxt, wsb, use ? "in use" : "NOT in use");
	DEBUGLOGP("old clockWS is %d, new clockWS is %d\n", wso, wsn);
	g_free(pTTipTxt);
#endif

	if( wsn != wso )
	{
		gCfg.clockWS = wsn;
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_ws_value_changed(GtkRange* pRange, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
static void on_pos_val_chgd(GtkSpinButton* pSpinButtonX, GtkSpinButton* pSpinButtonY, gpointer window)
{
	DEBUGLOGB;

	bool chgX = pSpinButtonX != NULL;
	gint newV = gtk_spin_button_get_value_as_int(chgX ? pSpinButtonX : pSpinButtonY);

	Config    tcfg = gCfg;
	cfg::cnvp(tcfg,  true); // make 0 corner vals relative to the cfg corner

	bool chgV = chgX ?  tcfg.clockX != newV : tcfg.clockY != newV;

	if( chgV )
	{
		tcfg.clockX  =  chgX ? newV :  tcfg.clockX;
		tcfg.clockY  = !chgX ? newV :  tcfg.clockY;

		if( gRun.lockOffs )
		{
			prefs::open(true); // to prevent a value change from doing any processing

			if( chgX )
			{
				prefs::g_pWidget = lgui::pWidget(prefs::PREFSTR_Y);
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(prefs::g_pWidget), tcfg.clockY = tcfg.clockX);
			}
			else
			{
				prefs::g_pWidget = lgui::pWidget(prefs::PREFSTR_X);
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(prefs::g_pWidget), tcfg.clockX = tcfg.clockY);
			}

			prefs::open(false);
		}

		cfg::cnvp(tcfg, true); // make cfg corner vals relative to the 0 corner

		gint                                         oldX,  oldY;
		gtk_window_get_position(GTK_WINDOW(window), &oldX, &oldY);

		if( tcfg.clockX != oldX || tcfg.clockY != oldY )
		{
			gCfg.clockX  = tcfg.clockX;
			gCfg.clockY  = tcfg.clockY;

			update_wnd_pos(GTK_WIDGET(window), tcfg.clockX, tcfg.clockY);
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_x_value_changed(GtkSpinButton* pSpinButton, gpointer window)
{
	OPENING_RET;
	on_pos_val_chgd(pSpinButton, NULL, window);
}

// -----------------------------------------------------------------------------
void prefs::on_y_value_changed(GtkSpinButton* pSpinButton, gpointer window)
{
	OPENING_RET;
	on_pos_val_chgd(NULL, pSpinButton, window);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::set_ani_rate(int renderRate)
{
	DEBUGLOGB;

	DEBUGLOGS("bef lgui::pWidget call");
	GtkWidget* pWidget = lgui::pWidget(PREFSTR_ANIRATE);
	DEBUGLOGS("aft lgui::pWidget call");

	if( pWidget )
	{
		DEBUGLOGP("min/max/val: %d, %d, %d\n", MIN_REFRESH_RATE, MAX_REFRESH_RATE, renderRate);

		DEBUGLOGS("bef set range call");
		gtk_range_set_range(GTK_RANGE(pWidget), MIN_REFRESH_RATE, MAX_REFRESH_RATE);
		gtk_range_set_restrict_to_fill_level(GTK_RANGE(pWidget), FALSE);
		gtk_scale_clear_marks(GTK_SCALE(pWidget));

		for( int v = 4; v <= 60; v += 4 )
			gtk_scale_add_mark(GTK_SCALE(pWidget), v, GTK_POS_BOTTOM, NULL);
		DEBUGLOGS("aft set range call");

		DEBUGLOGS("bef set value call");
		gtk_range_set_fill_level(GTK_RANGE(pWidget), renderRate);
		gtk_range_set_value(GTK_RANGE(pWidget), renderRate);
		DEBUGLOGS("aft set value call");
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::set_theme_info(const ThemeEntry& te)
{
	DEBUGLOGB;

	char        text[4096];
	const char* unkn = "<unknown>";
	GtkWidget*  pWidget;

	if( pWidget = lgui::pWidget("labelThemeName") )
	{
		DEBUGLOGS("setting theme name (name (path/file))");
		const char* n = te.pName && te.pName->str ? te.pName->str : unkn;
		const char* p = te.pPath && te.pPath->str ? te.pPath->str : unkn;
		const char* f = te.pFile && te.pFile->str ? te.pFile->str : unkn;
		snprintf(text, vectsz(text), "%s (%s/%s)", n, p, f);
		gtk_label_set_text(GTK_LABEL(pWidget), text);
	}

	if( pWidget = lgui::pWidget("labelThemeInfo") )
	{
		DEBUGLOGS("setting theme info (author, v. version (info))");
		const char* a = te.pAuthor  && te.pAuthor ->str && *te.pAuthor ->str ? te.pAuthor ->str : unkn;
		const char* v = te.pVersion && te.pVersion->str && *te.pVersion->str ? te.pVersion->str : unkn;
		const char* i = te.pInfo    && te.pInfo   ->str && *te.pInfo   ->str ? te.pInfo   ->str : unkn;
		snprintf(text, vectsz(text), "by %s, v. %s (%s)", a, v, i);
		gtk_label_set_text(GTK_LABEL(pWidget), text);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::set_wnd_size(GtkWidget* pWidget, int oldW, int oldH, int newW, int newH)
{
	DEBUGLOGB;

	if( oldW != newW || oldH != newH )
	{
//		DEBUGLOGS("updating surfs/resizing/redrawing window");
		DEBUGLOGS("scaling window to new size");
		DEBUGLOGP("new dims %d x %d\n", newW, newH);
//		update_wnd_dim(pWidget, newW, newH, oldW, oldH, true, false);

		gRun.drawScaleX = (double)newW/(double)gCfg.clockW;
		gRun.drawScaleY = (double)newH/(double)gCfg.clockH;
		draw::render_set  (pWidget);
		gtk_window_resize (GTK_WINDOW(pWidget), newW, newH);

		int                     x, y;
		GdkModifierType               mask;
		get_cursor_pos(pWidget, x, y, mask);

		if( (mask & GDK_BUTTON1_MASK) != GDK_BUTTON1_MASK ) // button's up - must have been keyed in
		{
			DEBUGLOGS("keyed in size chg - on_exts_btn_release called");
			on_exts_btn_release(NULL, NULL, pWidget);
		}
	}

	DEBUGLOGE;
}

#endif // _USEGTK

