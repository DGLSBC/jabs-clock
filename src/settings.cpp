/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP     "pref"

#include "global.h"    // _USEWKSPACE is in here
#include "debug.h"     // for debugging prints

#include "draw.h"
#include "chart.h"
#include "clock.h"
//#include "tzone.h"
#include "config.h"
#include "themes.h"
#include "clockGUI.h"
#include "settings.h"
#include "utility.h"

#ifdef   _USEWKSPACE
#define   WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#endif

#define   OPENING_RET   if( g_opening )     return

/*******************************************************************************
**
** Settings dialog box related events
**
*******************************************************************************/

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
namespace prefs
{

static void gtk_combo_box_clear(GtkComboBox* pComboBox, int cnt=0);

static void on_button_toggled(GtkToggleButton* pToggButton, gpointer window, gint& flag, bool redraw, bool update);

static void on_ani_startup_toggled  (GtkToggleButton* pToggButton, gpointer window);
static void on_ani_type_toggled     (GtkToggleButton* pToggButton, gpointer window);
static void on_ani_value_changed    (GtkRange*        pRange,      gpointer window);
static void on_close_clicked        (GtkButton*       pButton,     gpointer window);
static void on_corner_toggled       (GtkToggleButton* pToggButton, gpointer window);
static void on_do_sounds_toggled    (GtkToggleButton* pToggButton, gpointer window);
static void on_height_value_changed (GtkSpinButton*   pSpinButton, gpointer window);
static void on_help_clicked         (GtkButton*       pButton,     gpointer window);
static void on_lock_dims_toggled    (GtkToggleButton* pToggButton, gpointer window);
static void on_opacity_toggled      (GtkToggleButton* pToggButton, gpointer window);
static void on_opacity_value_changed(GtkRange*        pRange,      gpointer window);
static void on_startup_size_changed (GtkComboBox*     pComboBox,   gpointer window);
static void on_theme_changed        (GtkComboBox*     pComboBox,   gpointer window);
static void on_unsticky_toggled     (GtkToggleButton* pToggButton, gpointer window);
static void on_width_value_changed  (GtkSpinButton*   pSpinButton, gpointer window);
static void on_ws_value_changed     (GtkRange*        pRange,      gpointer window);
static void on_x_value_changed      (GtkSpinButton*   pSpinButton, gpointer window);
static void on_y_value_changed      (GtkSpinButton*   pSpinButton, gpointer window);

static void on_fmt12Hr_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window);
static void on_fmt24Hr_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window);
static void on_fmtDate_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window);
static void on_fmtTime_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window);

static void on_font_face_list_changed  (GtkTreeSelection*  pTreeSelection,  gpointer window);
static void on_font_family_list_changed(GtkTreeSelection*  pTreeSelection,  gpointer window);
static void on_font_size_list_changed  (GtkTreeSelection*  pTreeSelection,  gpointer window);
static void on_font_size_entry_activate(GtkEntry*          pEntry,          gpointer window);
static void on_font_color_sel_changed  (GtkColorSelection* pColorSelection, gpointer window);

static gboolean on_settings_close(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData);

}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
typedef void (*TOGBTNCB)(GtkToggleButton*, gpointer);

//static gboolean on_settings_close(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData);

static bool g_opening = false;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static void initToggleBtn(const char* name, const char* event, int value, TOGBTNCB callback)
{
	GtkWidget* pWidget = glade_xml_get_widget(gui::pGladeXml, name);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pWidget), value);
	g_signal_connect(G_OBJECT(pWidget), event, G_CALLBACK(callback), gRun.pMainWindow);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::begin()
{
	DEBUGLOGB;

/*	// TODO: all of this tzone stuff needs to become part of the run or cfg struct
	const char* abb1 = "America/Chicago";
	const char* abb2 = "America/Los_Angeles";*/

//	tzn::beg();
/*
	DEBUGLOGS("bef tzn::get calls");

	char tzc1[128];
	char tzc2[128];
	int  off1 = 0;
	int  off2 = 0;
	int  utc1 = 0;
	int  utc2 = 0;
	bool oka1 = tzn::get(abb1, off1, utc1, tzc1, vectsz(tzc1));
	bool oka2 = tzn::get(abb2, off2, utc2, tzc2, vectsz(tzc2));
*/
	DEBUGLOGS("aft tzn::get calls");

/*	if( oka1 )
		DEBUGLOGP("found timezone1 %s - label is %s, utc offset is %d\n", abb1, tzc1, utc1);
	else
		DEBUGLOGP("didn't find timezone1 %s\n", abb1);*/

/*	if( oka2 )
		DEBUGLOGP("found timezone2 %s - label is %s, utc offset is %d\n", abb2, tzc2, utc2);
	else
		DEBUGLOGP("didn't find timezone2 %s\n", abb2);*/

/*	if( oka1 && oka2 )
	{
		float of1c = (float) off1         /3600.0f;
		float of2c = (float) off2         /3600.0f;
		float utcd = (float)(utc2 -  utc1)/3600.0f; // 3600 converts from seconds to hours
		bool  less =         utc2 <  utc1;
		bool  same =         utc2 == utc1;
		const char* szo1 = "offset from";
		const char* szo2 = less ? "behind" : (same ? "offset from" : "ahead of");
		DEBUGLOGP("%s is %f (%f) hours %s %s\n", tzc1, 0.0f, of1c, szo1, tzc1);
		DEBUGLOGP("%s is %f (%f) hours %s %s\n", tzc2, utcd, of2c, szo2, tzc1);
	}
*/
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::end()
{
	DEBUGLOGB;

//	tzn::end();

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::open(bool opening)
{
	g_opening = opening;
}

// -----------------------------------------------------------------------------
static void on_custom_anim_activate(GtkExpander* pExpander, gpointer user_data)
{
	DEBUGLOGB;

	if( !gtk_expander_get_expanded(pExpander) )
	{
		GtkWidget* pWidget = glade_xml_get_widget(gui::pGladeXml, "drawingareaAnimationCustom");
		chart_init(pWidget);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::init(GtkWidget* pPropsDialog, GtkWindow*& g_pPopupDlg, bool& g_inPopup, char& g_nmPopup)
{
	DEBUGLOGB;

/*	GTimeVal            ct;
	g_get_current_time(&ct);
	DEBUGLOGP("  time (%d.%d)\n", (int)ct.tv_sec, (int)ct.tv_usec);*/

	GtkWidget* pWidget;

	open(true);

	// dialog box related
	{
		pWidget = glade_xml_get_widget(gui::pGladeXml, "buttonHelp");
//		g_signal_connect(G_OBJECT(pWidget), "clicked", G_CALLBACK(on_help_clicked), gRun.pMainWindow);
		g_signal_connect(G_OBJECT(pWidget), "clicked", G_CALLBACK(on_help_clicked), pPropsDialog);

		pWidget = glade_xml_get_widget(gui::pGladeXml, "buttonClose");
		g_signal_connect(G_OBJECT(pWidget), "clicked", G_CALLBACK(on_close_clicked), gRun.pMainWindow);
	}

	// geometry related
	{
		Settings  tcfg      = gCfg;
		int       sw        = gdk_screen_get_width (gdk_screen_get_default());
		int       sh        = gdk_screen_get_height(gdk_screen_get_default());
		int       tssEnable = TRUE, szIndex = SIZE_CUSTOM; // default

		cfg::cnvp(tcfg, true);

		if( gCfg.clockW  == gCfg.clockH )
		{
			switch( gCfg.clockW )
			{
			case  50: tssEnable = FALSE; szIndex = SIZE_SMALL;  break;
			case 100: tssEnable = FALSE; szIndex = SIZE_MEDIUM; break;
			case 200: tssEnable = FALSE; szIndex = SIZE_LARGE;  break;
			}
		}

		pWidget = glade_xml_get_widget(gui::pGladeXml, "comboboxStartupSize");
		gtk_combo_box_set_active(GTK_COMBO_BOX(pWidget), szIndex);
		g_signal_connect(G_OBJECT(pWidget), "changed", G_CALLBACK(on_startup_size_changed), gRun.pMainWindow);

		pWidget = glade_xml_get_widget(gui::pGladeXml, "tableStartupSize");
		gtk_widget_set_sensitive(pWidget, tssEnable);

		pWidget = glade_xml_get_widget(gui::pGladeXml, "spinbuttonWidth");
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(pWidget), MIN_CWIDTH, sw);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), gCfg.clockW);
		g_signal_connect(G_OBJECT(pWidget), "value-changed", G_CALLBACK(on_width_value_changed), gRun.pMainWindow);

		pWidget = glade_xml_get_widget(gui::pGladeXml, "spinbuttonHeight");
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(pWidget), MIN_CHEIGHT, sh);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), gCfg.clockH);
		g_signal_connect(G_OBJECT(pWidget), "value-changed", G_CALLBACK(on_height_value_changed), gRun.pMainWindow);

		pWidget = glade_xml_get_widget(gui::pGladeXml, "spinbuttonX");
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(pWidget), -sw, sw);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockX);
		g_signal_connect(G_OBJECT(pWidget), "value-changed", G_CALLBACK(on_x_value_changed), gRun.pMainWindow);

		pWidget = glade_xml_get_widget(gui::pGladeXml, "spinbuttonY");
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(pWidget), -sh, sh);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), tcfg.clockY);
		g_signal_connect(G_OBJECT(pWidget), "value-changed", G_CALLBACK(on_y_value_changed), gRun.pMainWindow);

		initToggleBtn("togglebuttonLockDims", "toggled", gRun.lockDims,                   on_lock_dims_toggled);
		initToggleBtn("radiobuttonTopLeft",   "toggled", gCfg.clockC == CORNER_TOP_LEFT,  on_corner_toggled);
		initToggleBtn("radiobuttonTopRight",  "toggled", gCfg.clockC == CORNER_TOP_RIGHT, on_corner_toggled);
		initToggleBtn("radiobuttonBotLeft",   "toggled", gCfg.clockC == CORNER_BOT_LEFT,  on_corner_toggled);
		initToggleBtn("radiobuttonBotRight",  "toggled", gCfg.clockC == CORNER_BOT_RIGHT, on_corner_toggled);
		initToggleBtn("radiobuttonCenter",    "toggled", gCfg.clockC == CORNER_CENTER,    on_corner_toggled);
	}

	// clock face related
	{
		ThemeEntry     te;
		ThemeList*     tl;
		theme_list_get(tl);

		pWidget = glade_xml_get_widget(gui::pGladeXml, "comboboxTheme");

		gtk_combo_box_clear             (GTK_COMBO_BOX(pWidget));
		gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(pWidget),  TRUE);
		gtk_combo_box_set_wrap_width    (GTK_COMBO_BOX(pWidget), (theme_list_cnt(tl)-1)/15+1);

		bool     trunc;
		GString* pName;
		int      themeNameMax  = 20;
		int      themeListCnt  =  0;
		int      themeListActv = -1;

		DEBUGLOGP("theme count is %d\n", theme_list_cnt(tl));

		for( te = theme_list_beg(tl); !theme_list_end(tl); te = theme_list_nxt(tl) )
		{
			pName = te.pName;
			trunc = pName->len > themeNameMax;

			if( trunc )
			{
				pName = g_string_new_len(pName->str, themeNameMax);
				g_string_append(pName, " ...");
			}

			gtk_combo_box_append_text(GTK_COMBO_BOX(pWidget), pName->str);

			if( trunc )
				g_string_free(pName, TRUE);

			DEBUGLOGP("added list theme %s\n", te.pName->str);

			if( strcmp(gCfg.themePath, te.pPath->str) == 0 &&
				strcmp(gCfg.themeFile, te.pFile->str) == 0 )
				themeListActv = themeListCnt;

			themeListCnt++;
		}

		theme_list_del(tl);

		if( themeListActv != -1 )
			gtk_combo_box_set_active(GTK_COMBO_BOX(pWidget), themeListActv);

		g_signal_connect(G_OBJECT(pWidget),  "changed", G_CALLBACK(on_theme_changed), gRun.pMainWindow);

		initToggleBtn("checkbuttonSeconds",  "toggled", gCfg.showSeconds, on_seconds_toggled);
		initToggleBtn("checkbutton24h",      "toggled", gCfg.show24Hrs,   on_24h_toggled);
		initToggleBtn("checkbuttonShowDate", "toggled", gCfg.showDate,    on_show_date_toggled);
		initToggleBtn("checkbuttonFaceDate", "toggled", gCfg.faceDate,    on_face_date_toggled);

		pWidget = glade_xml_get_widget(gui::pGladeXml, "entryDateFormat");
		gtk_entry_set_text(GTK_ENTRY(pWidget), gCfg.fmtDate);
		g_signal_connect(G_OBJECT(pWidget), "focus-out-event", G_CALLBACK(on_fmtDate_focus_lost), gRun.pMainWindow);

		pWidget = glade_xml_get_widget(gui::pGladeXml, "entryTimeFormat");
		gtk_entry_set_text(GTK_ENTRY(pWidget), gCfg.fmtTime);
		g_signal_connect(G_OBJECT(pWidget), "focus-out-event", G_CALLBACK(on_fmtTime_focus_lost), gRun.pMainWindow);

		pWidget = glade_xml_get_widget(gui::pGladeXml, "entry12HrFormat");
		gtk_entry_set_text(GTK_ENTRY(pWidget), gCfg.fmt12Hrs);
		g_signal_connect(G_OBJECT(pWidget), "focus-out-event", G_CALLBACK(on_fmt12Hr_focus_lost), gRun.pMainWindow);

		pWidget = glade_xml_get_widget(gui::pGladeXml, "entry24HrFormat");
		gtk_entry_set_text(GTK_ENTRY(pWidget), gCfg.fmt24Hrs);
		g_signal_connect(G_OBJECT(pWidget), "focus-out-event", G_CALLBACK(on_fmt24Hr_focus_lost), gRun.pMainWindow);
	}

	// hand animation related
	{
		set_ani_rate(gCfg.refreshRate);
		pWidget = glade_xml_get_widget(gui::pGladeXml, "hscaleSmoothness");
//		gtk_range_set_range(GTK_RANGE(pWidget), MIN_REFRESH_RATE, MAX_REFRESH_RATE);
//		gtk_range_set_value(GTK_RANGE(pWidget), gCfg.refreshRate);
		g_signal_connect(G_OBJECT(pWidget), "value-changed", G_CALLBACK(on_ani_value_changed), gRun.pMainWindow);

		initToggleBtn("radiobuttonOriginal", "toggled", gCfg.shandType == ANIM_ORIG,  on_ani_type_toggled);
		initToggleBtn("radiobuttonFlick",    "toggled", gCfg.shandType == ANIM_FLICK, on_ani_type_toggled);
		initToggleBtn("radiobuttonSweep",    "toggled", gCfg.shandType == ANIM_SWEEP, on_ani_type_toggled);
//		initToggleBtn("radiobuttonCustom",   "toggled", gCfg.shandType == ANIM_CUSTO, on_ani_type_toggled);

		pWidget = glade_xml_get_widget(gui::pGladeXml, "expanderAnimationCustom");
		g_signal_connect(G_OBJECT(pWidget), "activate", G_CALLBACK(on_custom_anim_activate), gRun.pMainWindow);
	}

	// date font related
	{
		char              fname[128];
		GtkWidget*        pSubWidget;
		GtkTreeSelection* pSelWidget;

/*		snprintf(fname, vectsz(fname), "%s %s %s %d", "FreeSerif", "Bold", "Italic", gCfg.fontSize);
		DEBUGLOGP("fname is \"%s\"\n", fname);*/

		pWidget = glade_xml_get_widget(gui::pGladeXml, "fontselectionCDF");
		DEBUGLOGP("font selected is '%s'\n", gtk_font_selection_get_font_name(GTK_FONT_SELECTION(pWidget)));
//		gtk_font_selection_set_font_name   (GTK_FONT_SELECTION(pWidget), fname);
		gtk_font_selection_set_font_name   (GTK_FONT_SELECTION(pWidget), gCfg.fontName);
		gtk_font_selection_set_preview_text(GTK_FONT_SELECTION(pWidget), gRun.acDate);
		DEBUGLOGP("font selected is '%s'\n", gtk_font_selection_get_font_name(GTK_FONT_SELECTION(pWidget)));

		pSubWidget = gtk_font_selection_get_face_list(GTK_FONT_SELECTION(pWidget));
		pSelWidget = gtk_tree_view_get_selection(GTK_TREE_VIEW(pSubWidget));
		g_signal_connect(G_OBJECT(pSelWidget), "changed", G_CALLBACK(on_font_face_list_changed), gRun.pMainWindow);

		pSubWidget = gtk_font_selection_get_family_list(GTK_FONT_SELECTION(pWidget));
		pSelWidget = gtk_tree_view_get_selection(GTK_TREE_VIEW (pSubWidget));
		g_signal_connect(G_OBJECT(pSelWidget), "changed", G_CALLBACK(on_font_family_list_changed), gRun.pMainWindow);

		pSubWidget = gtk_font_selection_get_size_list(GTK_FONT_SELECTION(pWidget));
		pSelWidget = gtk_tree_view_get_selection(GTK_TREE_VIEW(pSubWidget));
		g_signal_connect(G_OBJECT(pSelWidget), "changed", G_CALLBACK(on_font_size_list_changed), gRun.pMainWindow);

		pSubWidget = gtk_font_selection_get_size_entry(GTK_FONT_SELECTION(pWidget));
		g_signal_connect(G_OBJECT(pSubWidget), "activate", G_CALLBACK(on_font_size_entry_activate), gRun.pMainWindow);
	}

	// date color related
	{
		GdkColor csc;
		pWidget   = glade_xml_get_widget(gui::pGladeXml, "colorselectionCDC");

		gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(pWidget), &csc);
		DEBUGLOGP("color selected is %d-%d-%d\n", (int)csc.red, (int)csc.green, (int)csc.blue);

		csc.red   = (guint16)(gCfg.dateTextRed*65535);
		csc.green = (guint16)(gCfg.dateTextGrn*65535);
		csc.blue  = (guint16)(gCfg.dateTextBlu*65535);

		gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(pWidget), &csc);

/*		csc.red   = csc.green = csc.blue = 0;
		gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(pWidget), &csc);
		DEBUGLOGP("color selected is %d-%d-%d\n", (int)csc.red, (int)csc.green, (int)csc.blue);*/

		g_signal_connect(G_OBJECT(pWidget), "color-changed", G_CALLBACK(on_font_color_sel_changed), gRun.pMainWindow);
	}

	// extras
	{
		initToggleBtn("checkbuttonKeepOnTop",       "toggled", gCfg.keepOnTop,    on_keep_on_top_toggled);
		initToggleBtn("checkbuttonKeepOnBot",       "toggled", gCfg.keepOnBot,    on_keep_on_bot_toggled);
		initToggleBtn("checkbuttonAppearInPager",   "toggled", gCfg.showInPager,  on_show_in_pager_toggled);
		initToggleBtn("checkbuttonAppearInTaskbar", "toggled", gCfg.showInTasks,  on_show_in_taskbar_toggled);
		initToggleBtn("checkbuttonSticky",          "toggled", cclock::sticky(),  on_sticky_toggled);

		initToggleBtn("checkbuttonAniStartup",      "toggled", gCfg.aniStartup,   on_ani_startup_toggled);
		initToggleBtn("checkbuttonPlaySounds",      "toggled", gCfg.doSounds,     on_do_sounds_toggled);
		initToggleBtn("checkbuttonUnSticky",        "toggled", gCfg.clockWS != 0, on_unsticky_toggled);
		initToggleBtn("checkbuttonOpacity",         "toggled", gCfg.opacity != 1, on_opacity_toggled);

		int nw  = 0;
#ifdef _USEWKSPACE
		nw      = wnck_screen_get_workspace_count(wnck_screen_get_default());
#endif
		pWidget = glade_xml_get_widget(gui::pGladeXml, "hscaleUnSticky");
		gtk_range_set_range(GTK_RANGE(pWidget), 0, nw);
		gtk_range_set_value(GTK_RANGE(pWidget), gCfg.clockWS);
		g_signal_connect(G_OBJECT(pWidget), "value-changed", G_CALLBACK(on_ws_value_changed), gRun.pMainWindow);

#ifndef _USEWKSPACE
		pWidget = glade_xml_get_widget(gui::pGladeXml, "checkbuttonUnSticky");
		gtk_widget_set_sensitive(pWidget, FALSE);
		pWidget = glade_xml_get_widget(gui::pGladeXml, "hscaleUnSticky");
		gtk_widget_set_sensitive(pWidget, FALSE);
#endif
		pWidget = glade_xml_get_widget(gui::pGladeXml, "hscaleOpacity");
		gtk_range_set_range(GTK_RANGE(pWidget), 0, 100);
		gtk_range_set_value(GTK_RANGE(pWidget), gCfg.opacity*100.0);
		g_signal_connect(G_OBJECT(pWidget), "value-changed", G_CALLBACK(on_opacity_value_changed), gRun.pMainWindow);
	}

	if( pPropsDialog )
	{
/*		g_get_current_time(&ct);
		DEBUGLOGP("before show call (%d.%d)\n", (int)ct.tv_sec, (int)ct.tv_usec);*/

		g_signal_connect(G_OBJECT(pPropsDialog), "delete_event", G_CALLBACK(on_settings_close), NULL);
		gtk_widget_show(pPropsDialog);

/*		g_get_current_time(&ct);
		DEBUGLOGP("after show call (%d.%d)\n", (int)ct.tv_sec, (int)ct.tv_usec);*/

		g_pPopupDlg = GTK_WINDOW(pPropsDialog);
		g_inPopup   = true;
		g_nmPopup   = 's';
	}

	open(false);

/*	g_get_current_time(&ct);
	DEBUGLOGP("exit (%d.%d)\n", (int)ct.tv_sec, (int)ct.tv_usec);*/

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
gboolean prefs::on_settings_close(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	DEBUGLOGB;

	gtk_widget_hide(pWidget);

	gui::dnit();

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::gtk_combo_box_clear(GtkComboBox* pComboBox, int cnt)
{
	gtk_combo_box_set_active(pComboBox, -1);

	if( cnt )
	{
		while( cnt-- )
			gtk_combo_box_remove_text(GTK_COMBO_BOX(pComboBox), 0);
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
void prefs::on_button_toggled(GtkToggleButton* pToggButton, gpointer window, gint& flag, bool redraw, bool update)
{
	flag = pToggButton ? (gtk_toggle_button_get_active(pToggButton) ? 1 : 0) : !flag;

	if( update )
	{
		gRun.updateSurfs = true;
		update_wnd_dim(GTK_WIDGET(window), gCfg.clockW, gCfg.clockH, true);
	}

	if( redraw )
		gtk_widget_queue_draw(GTK_WIDGET(window));
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prefs::on_24h_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	on_button_toggled(pToggButton, window, gCfg.show24Hrs, true, true);
	update_ts_info(true, true, true);
}

// -----------------------------------------------------------------------------
void prefs::on_ani_startup_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	on_button_toggled(pToggButton, window, gCfg.aniStartup, false, false);
}

// -----------------------------------------------------------------------------
void prefs::on_ani_type_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	if( gtk_toggle_button_get_active(pToggButton) )
	{
		gint         shandType = gCfg.shandType;
		const gchar* name      = gtk_widget_get_name(GTK_WIDGET(pToggButton));

		shandType = strcmp(name, "radiobuttonOriginal") == 0 ? ANIM_ORIG  : shandType;
		shandType = strcmp(name, "radiobuttonFlick")    == 0 ? ANIM_FLICK : shandType;
		shandType = strcmp(name, "radiobuttonSweep")    == 0 ? ANIM_SWEEP : shandType;

		if( gCfg.shandType != shandType )
		{
			gCfg.shandType  = shandType;
			draw::reset_ani();
		}
	}
}

// -----------------------------------------------------------------------------
void prefs::on_ani_value_changed(GtkRange* pRange, gpointer window)
{
	gint refreshRate = (gint)gtk_range_get_value(pRange);
	change_ani_rate(GTK_WIDGET(window), refreshRate, false, false);
	draw::update_ani();
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

			Settings  tcfg = gCfg;
			cfg::cnvp(tcfg,  true);

//			gCfg.clockX = tcfg.clockX;
//			gCfg.clockY = tcfg.clockY;

			GtkWidget* pSpinButtonX = glade_xml_get_widget(gui::pGladeXml, "spinbuttonX");
			GtkWidget* pSpinButtonY = glade_xml_get_widget(gui::pGladeXml, "spinbuttonY");

			gtk_spin_button_set_value(GTK_SPIN_BUTTON(pSpinButtonX), (gdouble)tcfg.clockX);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(pSpinButtonY), (gdouble)tcfg.clockY);

//			gtk_window_move(GTK_WINDOW(window), tcfg.clockX, tcfg.clockY);
		}
	}
}

// -----------------------------------------------------------------------------
void prefs::on_do_sounds_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	on_button_toggled(pToggButton, window, gCfg.doSounds, false, false);
}

// -----------------------------------------------------------------------------
void prefs::on_face_date_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	on_button_toggled(pToggButton, window, gCfg.faceDate, true, true);
}

// -----------------------------------------------------------------------------
void prefs::on_fmt12Hr_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	strvcpy(gCfg.fmt12Hrs, gtk_entry_get_text(pEntry));
	update_ts_info(true);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmt24Hr_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	strvcpy(gCfg.fmt24Hrs, gtk_entry_get_text(pEntry));
	update_ts_info(true);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmtDate_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	strvcpy(gCfg.fmtDate, gtk_entry_get_text(pEntry));
	update_ts_info(false, true);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_fmtTime_focus_lost(GtkEntry* pEntry, GdkEventFocus* pEventFocus, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	strvcpy(gCfg.fmtTime, gtk_entry_get_text(pEntry));
	update_ts_info(false, false, true);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
static void on_font_changed()
{
	DEBUGLOGB;

	GtkWidget*            pWidget  = glade_xml_get_widget(gui::pGladeXml, "fontselectionCDF");
	GtkFontSelection*     pFontSel = GTK_FONT_SELECTION(pWidget);
	const char*           fname    = gtk_font_selection_get_font_name  (pFontSel);
	int                   fsize    = gtk_font_selection_get_size       (pFontSel)/1024; // docs don't indicate this, but it works
/*	PangoFontDescription* pPFDesc  = pango_font_description_from_string(fname);

	pPFDesc = pango_font_description_from_string("Sans Bold 12");

	pango_layout_set_text(pPFLayout, gRun.acTxt1, -1);
	pango_layout_set_font_description(pPFLayout, pPFDesc);
	pango_font_description_free(pPFDesc);

//	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
//	pango_cairo_update_layout(cr, pPFLayout);
//	pango_cairo_show_layout(cr, pPFLayout);

	g_object_unref(pPFLayout);*/

	strvcpy(gCfg.fontName, fname);
	gCfg.fontSize = fsize;
	draw::chg();
/*
//	PangoFont*            pPFont   = gtk_font_selection_get_font       (pFontSel);
	PangoFontFace*        pPFFace  = gtk_font_selection_get_face       (pFontSel);
	PangoFontFamily*      pPFFami  = gtk_font_selection_get_family     (pFontSel);
	const char*           pffacenm = pango_font_face_get_face_name     (pPFFace);
	const char*           pffaminm = pango_font_family_get_name        (pPFFami);
//	PangoFontDescription* pPFDesc  = pango_font_describe               (pPFont);
	PangoFontDescription* pPFDesc  = pango_font_description_from_string(fname);
	const char*           pdfilenm = pango_font_description_to_filename(pPFDesc);

	DEBUGLOGP("selected font name/size/pango font/pango family/pango file is:\n%s\n%d\n%s\n%s\n%s\n\n",
			__func__, fname, fsize, pffacenm, pffaminm, pdfilenm);*/

	DEBUGLOGP("selected font name/size is:\n%s (%d)\n\n", fname, fsize);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_font_face_list_changed(GtkTreeSelection* pTreeSelection, gpointer window)
{
	OPENING_RET;
	on_font_changed();
}

// -----------------------------------------------------------------------------
void prefs::on_font_family_list_changed(GtkTreeSelection* pTreeSelection, gpointer window)
{
	OPENING_RET;
	on_font_changed();
}

// -----------------------------------------------------------------------------
void prefs::on_font_size_list_changed(GtkTreeSelection* pTreeSelection, gpointer window)
{
	OPENING_RET;
	on_font_changed();
}

// -----------------------------------------------------------------------------
void prefs::on_font_size_entry_activate(GtkEntry* pEntry, gpointer window)
{
	OPENING_RET;
	DEBUGLOGS("entry/exit");
}

// -----------------------------------------------------------------------------
void prefs::on_font_color_sel_changed(GtkColorSelection* pColorSelection, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	GdkColor                                                csc;
	gtk_color_selection_get_current_color(pColorSelection, &csc);

	gCfg.dateTextRed = (gfloat)csc.red  /65535.0f;
	gCfg.dateTextGrn = (gfloat)csc.green/65535.0f;
	gCfg.dateTextBlu = (gfloat)csc.blue /65535.0f;

	draw::update_date_surf();

	gtk_widget_queue_draw(GTK_WIDGET(window));

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_height_value_changed(GtkSpinButton* pSpinButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	gint                                     oldW,  oldH;
	gtk_window_get_size(GTK_WINDOW(window), &oldW, &oldH);

	gint newH = gtk_spin_button_get_value_as_int(pSpinButton);

//	if( is_power_of_two(newH) ) // why is this necessary? seems to work fine without it
//		gtk_spin_button_set_value(pSpinButton, (gdouble)(++newH));

	if( gRun.lockDims )
	{
		GtkWidget* pWidget = glade_xml_get_widget(gui::pGladeXml, "spinbuttonWidth");
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), newH);
	}

	if( oldH != newH )
	{
		DEBUGLOGS("updating surfaces/resizing & redrawing window");

		gint newW        = gRun.lockDims ? newH : oldW;
/*		gRun.drawScaleX  = gRun.drawScaleY = 1;
		gRun.updateSurfs = true;
		gCfg.clockW      = newW;
		gCfg.clockH      = newH;

		gtk_window_resize(GTK_WINDOW(window), newW, newH);
		update_input_shape(GTK_WIDGET(window), newW, newH, true);
		gtk_widget_queue_draw(GTK_WIDGET(window));*/
		update_wnd_dim(GTK_WIDGET(window), newW, newH, true);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_help_clicked(GtkButton* pButton, gpointer window)
{
	GtkWidget* pPrefsHelpBox =
		gtk_message_dialog_new((GtkWindow*)window,
			GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
			"No help available yet, other than through the command line via -? or --help.\n\nSorry.");

	if( pPrefsHelpBox )
	{
		gtk_window_set_transient_for(GTK_WINDOW(pPrefsHelpBox), (GtkWindow*)window);
		gtk_window_set_position(GTK_WINDOW(pPrefsHelpBox), GTK_WIN_POS_CENTER);
		gtk_window_set_title(GTK_WINDOW(pPrefsHelpBox), gRun.appName);
		gtk_dialog_run(GTK_DIALOG(pPrefsHelpBox));
		gtk_widget_destroy(pPrefsHelpBox);
	}
}

// -----------------------------------------------------------------------------
void prefs::on_keep_on_bot_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	on_button_toggled(pToggButton, window,  gCfg.keepOnBot, false, false);
	cclock::keep_on_bot(GTK_WIDGET(window), gCfg.keepOnBot);
}

// -----------------------------------------------------------------------------
void prefs::on_keep_on_top_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	on_button_toggled(pToggButton, window,  gCfg.keepOnTop, false, false);
	cclock::keep_on_top(GTK_WIDGET(window), gCfg.keepOnTop);
}

// -----------------------------------------------------------------------------
void prefs::on_lock_dims_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	on_button_toggled(pToggButton, window, gRun.lockDims, false, false);
}

// -----------------------------------------------------------------------------
void prefs::on_opacity_toggled(GtkToggleButton* pToggButton, gpointer window)
{
//	on_button_toggled(pToggButton, window, gRun.useOpacity, false, false);
}

// -----------------------------------------------------------------------------
void prefs::on_opacity_value_changed(GtkRange* pRange, gpointer window)
{
	GtkWidget* pWidget = glade_xml_get_widget(gui::pGladeXml, "hscaleOpacity");
	gCfg.opacity       = gtk_range_get_value(GTK_RANGE(pWidget))/100.0;
	gdk_window_set_opacity(GTK_WIDGET(window)->window, gCfg.opacity);
}

// -----------------------------------------------------------------------------
void prefs::on_seconds_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	on_button_toggled(pToggButton, window, gCfg.showSeconds, true, false);
	change_ani_rate(GTK_WIDGET(window), gCfg.refreshRate, true, false);
}

// -----------------------------------------------------------------------------
void prefs::on_show_date_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	on_button_toggled(pToggButton, window, gCfg.showDate, true, true);
	update_ts_info(true, true, true);
}

// -----------------------------------------------------------------------------
void prefs::on_show_in_pager_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	on_button_toggled(pToggButton, window, gCfg.showInPager, false, false);
	cclock::pagebar_shown(GTK_WIDGET(window), cclock::pagebar_shown());
}

// -----------------------------------------------------------------------------
void prefs::on_show_in_taskbar_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	on_button_toggled(pToggButton, window, gCfg.showInTasks, false, false);
	cclock::taskbar_shown(GTK_WIDGET(window), cclock::taskbar_shown());
}

// -----------------------------------------------------------------------------
void prefs::on_startup_size_changed(GtkComboBox* pComboBox, gpointer window)
{
	DEBUGLOGB;

	int  wsz, hsz, tssEnable;
	gint cbi = gtk_combo_box_get_active(pComboBox);

	DEBUGLOGP("cbi=%d\n", cbi);

	if( tssEnable = (cbi == SIZE_CUSTOM) )
	{
		GtkWidget* pWidget;
		pWidget =  glade_xml_get_widget(gui::pGladeXml, "spinbuttonWidth");
		wsz     =  gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pWidget)),
		pWidget =  glade_xml_get_widget(gui::pGladeXml, "spinbuttonHeight");
		hsz     =  gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pWidget));
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
		GtkWidget* pWidget = glade_xml_get_widget(gui::pGladeXml, "tableStartupSize");
/*		gRun.drawScaleX    = gRun.drawScaleY = 1;
		gRun.updateSurfs   = true;
		gCfg.clockW        = wsz;
		gCfg.clockH        = hsz;*/

		gtk_widget_set_sensitive(pWidget, tssEnable);
/*		gtk_window_resize(GTK_WINDOW(window), wsz, hsz);
		update_input_shape(GTK_WIDGET(window), wsz, hsz, true);
		gtk_widget_queue_draw(GTK_WIDGET(window));*/
		update_wnd_dim(GTK_WIDGET(window), wsz, hsz, true);
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
	on_button_toggled(pToggButton, window, gCfg.sticky, true, false);
	cclock::sticky(GTK_WIDGET(window), cclock::sticky());
}

// -----------------------------------------------------------------------------
void prefs::on_theme_changed(GtkComboBox* pComboBox, gpointer window)
{
	DEBUGLOGB;

	int index  =  g_opening ? -1 : gtk_combo_box_get_active(pComboBox);
	if( index != -1 )
	{
		gchar* theme = gtk_combo_box_get_active_text(pComboBox); // TODO: needed?

		if( theme )
		{
			DEBUGLOGP("changing to new theme %s at index %d\n", theme, index);

			g_free(theme);

			DEBUGLOGS("bef settings pointers");

			ThemeList*     tl;
			theme_list_get(tl);

			ThemeEntry te = theme_list_nth(tl, index);

			DEBUGLOGS("aft settings pointers");

//			DEBUGLOGS("bef copying strings");

//			strvcpy(gCfg.themePath, te.pPath->str);
//			strvcpy(gCfg.themeFile, te.pFile->str);

//			DEBUGLOGS("aft copying strings");

			DEBUGLOGS("bef changing themes and updating the input shape");

			gRun.updateSurfs = true;

			change_theme(&te,  GTK_WIDGET(window));

//			update_input_shape(GTK_WIDGET(window), gCfg.clockW, gCfg.clockH, false);
/*			update_input_shape(GTK_WIDGET(window), gCfg.clockW, gCfg.clockH, true);*/

			DEBUGLOGS("aft changing themes and updating the input shape");

			theme_list_del(tl);
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_unsticky_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	OPENING_RET;
}

// -----------------------------------------------------------------------------
void prefs::on_width_value_changed(GtkSpinButton* pSpinButton, gpointer window)
{
	OPENING_RET;
	DEBUGLOGB;

	gint                                     oldW,  oldH;
	gtk_window_get_size(GTK_WINDOW(window), &oldW, &oldH);

	gint newW = gtk_spin_button_get_value_as_int(pSpinButton);

//	if( is_power_of_two(newW) ) // why is this necessary? seems to work fine without it
//		gtk_spin_button_set_value(pSpinButton, (gdouble)(++newW));

	if( gRun.lockDims )
	{
		GtkWidget* pWidget = glade_xml_get_widget(gui::pGladeXml, "spinbuttonHeight");
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), newW);
	}

	if( oldW != newW )
	{
		DEBUGLOGS("updating surfaces/resizing & redrawing window");

		gint newH        = gRun.lockDims ? newW : oldH;
/*		gRun.drawScaleX  = gRun.drawScaleY = 1;
		gRun.updateSurfs = true;
		gCfg.clockW      = newW;
		gCfg.clockH      = newH;

		gtk_window_resize(GTK_WINDOW(window), newW, newH);
		update_input_shape(GTK_WIDGET(window), newW, newH, true);
		gtk_widget_queue_draw(GTK_WIDGET(window));*/
		update_wnd_dim(GTK_WIDGET(window), newW, newH, true);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void prefs::on_ws_value_changed(GtkRange* pRange, gpointer window)
{
	OPENING_RET;
}

// -----------------------------------------------------------------------------
static void on_pos_val_chgd(GtkSpinButton* pSpinButtonX, GtkSpinButton* pSpinButtonY, gpointer window)
{
	bool chgX = pSpinButtonX != NULL;
	gint newV = gtk_spin_button_get_value_as_int(chgX ? pSpinButtonX : pSpinButtonY);

	Settings  tcfg = gCfg;
	cfg::cnvp(tcfg,  true); // make 0 corner vals relative to the cfg corner

	bool chgV = chgX ? tcfg.clockX != newV : tcfg.clockY != newV;

	if( chgV )
	{
		tcfg.clockX  =  chgX ? newV : tcfg.clockX;
		tcfg.clockY  = !chgX ? newV : tcfg.clockY;

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
void prefs::set_ani_rate(int refreshRate)
{
	if( gui::pGladeXml )
	{
		GtkWidget* pWidget = glade_xml_get_widget(gui::pGladeXml, "hscaleSmoothness");

		if( pWidget )
		{
			gtk_range_set_range(GTK_RANGE(pWidget), MIN_REFRESH_RATE, MAX_REFRESH_RATE);
			gtk_range_set_value(GTK_RANGE(pWidget), refreshRate);
		}
	}
}

