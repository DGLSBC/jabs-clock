/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "tzng"

#include <set>          //
#include <string>       //
#include <math.h>       //
#include <cairo.h>      //
#include <stdlib.h>     // for abs

#if _USEGTK
#include <cairo-xlib.h> // for testing if not using gdk_cairo_create is feasible
#endif

#include "tzoneGUI.h"   //
#include "clockGUI.h"   // for cgui::setPopup
#include "dayNight.h"   // for sunrise/set calculations
#include "loadGUI.h"    // for lgui::pWidget
#include "utility.h"    // for ?
#include "cfgdef.h"     // for APP_NAME
#include "config.h"     // for Config struct
#include "global.h"     // for set_window_icon
#include "debug.h"      // for debugging prints
#include "tzone.h"      // for timezone related

#if _USEGTK
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
namespace tzng
{

typedef std::set<std::string>             strset;
typedef std::set<std::string>::value_type strsetVT;
typedef std::set<std::string>::iterator   strsetIt;

typedef bool (*CBoxAdd)(tzn::loc_t* pLoc, char*& cbef, char*& cpos);

struct cbData
{
	CBoxAdd      pCBAdd;
	GtkComboBox* pCB;
	GtkEventBox* pEB;
};

static  void sadd(strset& strs, const char* cbef, size_t len);
static  void fill(GtkComboBox* pCB, strset& strs);

static  bool addcb1(tzn::loc_t* pLoc, char*& cbef, char*& cpos);
static  bool addcb2(tzn::loc_t* pLoc, char*& cbef, char*& cpos);
static  bool addcb3(tzn::loc_t* pLoc, char*& cbef, char*& cpos);

static  gboolean on_bld_tzcbox_lists(GtkWidget* pWidget);
static  gboolean on_map_event       (GtkWidget* pWidget,  GdkEvent* pEvent, gpointer userData);

static  void     on_reset_tz  (GtkWidget*   pWidget, GdkEventButton* pButton, gpointer cbType);
static  void     on_tz_changed(GtkComboBox* pComboBox,                        gpointer cbType);
static  void     on_tz_changed_update(bool reset, bool draw);

static bool   g_initing  = true;
static bool   g_cbchgng  = false;
static cbData g_CBData[] =
{
	{ addcb1, NULL, NULL },
	{ addcb2, NULL, NULL },
	{ addcb3, NULL, NULL }
};
static char   g_name[128], g_utcs[128], g_lats[128], g_lngs[128], g_locs[128], g_cmnt[256], g_tzts[128], g_tztl[128], g_suns[128], g_sunl[128], g_tzns[128];

struct label
{
	const char* id;
	GtkLabel*   pLabel;
	char*       text;
	bool        markup;
};

static label  g_labels[] =
{
	{ "labelName",        NULL, g_name, false },
	{ "labelUTCOffset",   NULL, g_utcs, false },
	{ "labelLatitude",    NULL, g_lats, false },
	{ "labelLongitude",   NULL, g_lngs, false },
	{ "labelLocalOffset", NULL, g_locs, false },
	{ "labelComment",     NULL, g_cmnt, false },
	{ "labelTime",        NULL, g_tzts, false },
	{ "labelTimeL",       NULL, g_tztl, false },
	{ "labelSunriseSet",  NULL, g_suns, false },
	{ "labelSunriseSetL", NULL, g_sunl, false },
	{ "labelTZ",          NULL, g_tzns, true  }
};

// -----------------------------------------------------------------------------
static  void     on_map_close_clicked(GtkButton*       pButton,                          gpointer userData);
static  void     on_map_done         (GtkWidget*       pWidget);
static  gboolean on_map_expose       (GtkWidget*       pWidget, GdkEventExpose* pExpose, gpointer userData);
static  gboolean on_map_popup_close  (GtkWidget*       pWidget, GdkEvent*       pEvent,  gpointer userData);
static  gboolean on_map_redraw       (GtkWidget*       pWidget);
static  void     on_map_toggled      (GtkToggleButton* pTogglebutton,                    gpointer userData);

static  double   lat_to_y  (double lat, int mapH);
static  double   lat_to_y  (double lat);
static  double   lon_to_x  (double lon, int mapW);
static  double   deg_to_rad(double deg);

static GtkWidget*       g_pMapWidget = NULL;
static cairo_surface_t* g_pMapImage  = NULL;
static cairo_surface_t* g_pPin1Image = NULL;
static cairo_surface_t* g_pPin2Image = NULL;

struct pin
{
	double lon, lat;
	bool   use;
};

static pin    g_pin[3];
static double g_pin_lon[3]; // 0 is local, 1 is gmt on equator, 2 is user chosen
static double g_pin_lat[3];
static bool   g_pin_use[3];

} // namespace tzng

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void tzng::init(GtkWidget* pDialog, char key)
{
	DEBUGLOGB;

	g_initing = true;

	g_pin_lon[0] = gCfg.tzLngitude; g_pin_lat[0] = gCfg.tzLatitude; g_pin_use[0] = true;  // local position
	g_pin_lon[1] = 0;               g_pin_lat[1] = 0;               g_pin_use[1] = true;  // gmt on equator
	g_pin_lon[2] = 0;               g_pin_lat[2] = 0;               g_pin_use[2] = false; // user selected

	char    ebname[16],                   cbname[16];
	strvcpy(ebname, "eventbox0"); strvcpy(cbname, "combobox0");

	for( size_t cb = 0; cb < vectsz(g_CBData); cb++ )
	{
		ebname[strlen(ebname)-1] = '1' + cb;
		cbname[strlen(cbname)-1] = '1' + cb;

		g_CBData[cb].pCB = GTK_COMBO_BOX(lgui::pWidget(cbname));
		g_CBData[cb].pEB = GTK_EVENT_BOX(lgui::pWidget(ebname));

		gtk_widget_set_sensitive(GTK_WIDGET(g_CBData[cb].pCB), cb == 0 ? TRUE : FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(g_CBData[cb].pEB), cb == 0 ? TRUE : FALSE);

		g_signal_connect(G_OBJECT(g_CBData[cb].pCB), "changed",              G_CALLBACK(on_tz_changed), gpointer(cb));
		g_signal_connect(G_OBJECT(g_CBData[cb].pEB), "button-release-event", G_CALLBACK(on_reset_tz),   gpointer(cb));
	}

	GtkWidget* pWidget = lgui::pWidget("togglebuttonMap");
	g_signal_connect(G_OBJECT(pWidget), "toggled",   G_CALLBACK(on_map_toggled), gpointer(pDialog));

	g_signal_connect(G_OBJECT(pDialog), "map-event", G_CALLBACK(on_map_event),   NULL);

	cgui::setPopup(pDialog, true, key);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void tzng::sadd(strset& strs, const char* cbef, size_t len)
{
	char    text[1024], *cpos;
	strvcpy(text, cbef,  len); text[len] = '\0';
	while( cpos = strchr(text, '_') ) *cpos = ' ';
	strs.insert(strsetVT(text));
}

// -----------------------------------------------------------------------------
void tzng::fill(GtkComboBox* pCB, strset& strs)
{
	GtkListStore* cboxList = GTK_LIST_STORE(gtk_combo_box_get_model(pCB));

	if( cboxList )
		gtk_list_store_clear(cboxList);

#if !GTK_CHECK_VERSION(3,0,0)
	for( strsetIt it = strs.begin(); it != strs.end(); ++it )
		gtk_combo_box_append_text(pCB, it->c_str());
#endif
#if  0
	int ncols = strs.size() < 11 ? 2 : strs.size()/10 + 1;
	if( ncols > 8 ) ncols = 8;

	gtk_combo_box_set_wrap_width(pCB, ncols);
#endif
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool tzng::addcb1(tzn::loc_t* pLoc, char*& cbef, char*& cpos)
{
	return (cpos = strchr(cbef, '/')) != NULL;
}

// -----------------------------------------------------------------------------
bool tzng::addcb2(tzn::loc_t* pLoc, char*& cbef, char*& cpos)
{
	return (cbef = strchr(cbef, '/')) != NULL && (cpos = strchr(++cbef, '/')) != NULL;
}

// -----------------------------------------------------------------------------
bool tzng::addcb3(tzn::loc_t* pLoc, char*& cbef, char*& cpos)
{
	char*    ctmp;
	cbef +=  strlen(pLoc->zone);
	cpos  = (cpos = strrchr(pLoc->zone, '/')) != NULL ? cpos+1 : pLoc->zone;
	ctmp  =  cpos; cpos = cbef; cbef = ctmp;
	return   true;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
gboolean tzng::on_bld_tzcbox_lists(GtkWidget* pWidget)
{
	DEBUGLOGB;

	tzn::beg();

	GPtrArray* pLocs = tzn::getLocs();

	if( pLocs )
	{
		strset      strs;
		tzn::loc_t *pLoc;
		char       *cbef, *cpos;

		DEBUGLOGS("building time zone combo box lists");

		g_cbchgng = true;

//		for( size_t cb = 0; cb < vectsz(g_CBData); cb++ )
		size_t cb = 0;
		{
			strs.clear();

			for( size_t l =  0; l < pLocs->len; l++ )
			{
				if( pLoc  = (tzn::loc_t*)pLocs->pdata[l] )
				{
					cbef  =  pLoc->zone;

					if( g_CBData[cb].pCBAdd(pLoc, cbef, cpos) )
						sadd(strs, cbef, cpos-cbef);
				}
			}

			fill(g_CBData[cb].pCB, strs);
		}

		g_cbchgng = false;
	}

	tzn::end();

	DEBUGLOGE;
	return FALSE; // kills the timer
}

// -----------------------------------------------------------------------------
gboolean tzng::on_map_event(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	DEBUGLOGB;

	if( g_initing )
	{
		g_initing = false;
		g_timeout_add(50, (GSourceFunc)on_bld_tzcbox_lists, (gpointer)pWidget);
		DEBUGLOGS("timer set to build time zone combo box lists");
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
void tzng::on_reset_tz(GtkWidget* pWidget, GdkEventButton* pButton, gpointer cbType)
{
	DEBUGLOGB;

	strset strs;
	size_t type = (size_t)cbType;

	g_cbchgng   =  true;

	gtk_combo_box_set_active(g_CBData[type].pCB, -1);

	for( size_t cb = type+1; cb < vectsz(g_CBData); cb++ )
	{
		gtk_widget_set_sensitive(GTK_WIDGET(g_CBData[cb].pCB), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(g_CBData[cb].pEB), FALSE);
		fill(g_CBData[cb].pCB, strs);
	}

	g_cbchgng   =  false;

	if( type == 0 )
	{
		on_bld_tzcbox_lists(NULL);
		on_tz_changed_update(true, true);
	}
	else
		on_tz_changed(g_CBData[type].pCB, cbType);

//	on_tz_changed(g_CBData[type].pCB, cbType);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void tzng::on_tz_changed(GtkComboBox* pComboBox, gpointer cbType)
{
	if( g_cbchgng ) return;

	DEBUGLOGB;
	g_cbchgng = true;
	tzn::beg();
#if 0
	g_pin_lon[2] = 0;
	g_pin_lat[2] = 0;
	g_pin_use[2] = false;
#endif
	on_tz_changed_update(true, false);

	GPtrArray* pLocs = tzn::getLocs();

	if( pLocs )
	{
		bool        okay;
		strset      strs;
		tzn::loc_t *pLoc;
		char       *cbef, *cpos,  *text;
		char        prfx[128];    *prfx = '\0';
		size_t      type = (size_t)cbType;

		for( size_t cb = type+1; cb < vectsz(g_CBData); cb++ )
		{
			gtk_combo_box_set_active(g_CBData[cb].pCB, -1);
//			gtk_widget_set_sensitive(GTK_WIDGET(g_CBData[cb].pCB), TRUE);
//			gtk_widget_set_sensitive(GTK_WIDGET(g_CBData[cb].pEB), TRUE);
		}

		for( size_t cb = 0; cb < vectsz(g_CBData); cb++ )
		{
#if GTK_CHECK_VERSION(3,0,0)
			text = NULL;
#else
			text = gtk_combo_box_get_active_text(g_CBData[cb].pCB);
#endif
			if( text )
			{
				if( cb )
				strvcat(prfx, "/");
				strvcat(prfx, text);
				g_free (text);
				char*  cpos;
				while( cpos = strchr(prfx, ' ') ) *cpos = '_';
			}

			if( cb > type )
			{
				strs.clear();

				DEBUGLOGP("cb%d prfx is *%s*\n", cb, prfx);

				for( size_t l =  0; l < pLocs->len; l++ )
				{
					if( pLoc  = (tzn::loc_t*)pLocs->pdata[l] )
					{
						cbef  =  pLoc->zone;
						okay  = !(*prfx) || (*prfx && strncmp(cbef, prfx, strlen(prfx)) == 0);

						if( okay && g_CBData[cb].pCBAdd(pLoc, cbef, cpos) )
							sadd(strs, cbef, cpos-cbef);
					}
				}

				gboolean gotd = strs.size() > 0;
				gtk_widget_set_sensitive(GTK_WIDGET(g_CBData[cb].pCB), gotd);
				gtk_widget_set_sensitive(GTK_WIDGET(g_CBData[cb].pEB), gotd);
				fill(g_CBData[cb].pCB, strs);
			}
		}
#if 0
		char  name[128], utcs[128], lats[128], lngs[128], locs[128], cmnt[256], tzts[128], tztl[128], suns[128], sunl[128], tzns[128];

		label labels[] =
		{
			{ "labelName",        NULL, name, false },
			{ "labelUTCOffset",   NULL, utcs, false },
			{ "labelLatitude",    NULL, lats, false },
			{ "labelLongitude",   NULL, lngs, false },
			{ "labelLocalOffset", NULL, locs, false },
			{ "labelComment",     NULL, cmnt, false },
			{ "labelTime",        NULL, tzts, false },
			{ "labelTimeL",       NULL, tztl, false },
			{ "labelSunriseSet",  NULL, suns, false },
			{ "labelSunriseSetL", NULL, sunl, false },
			{ "labelTZ",          NULL, tzns, true  }
		};

		for( size_t l = 0; l < vectsz(g_labels); l++ )
		{
			g_labels[l].pLabel  =  GTK_LABEL(lgui::pWidget(g_labels[l].id));
			g_labels[l].text[0] = '\0';
		}
#endif
		if( okay = type == 2 )
		{
			okay = false;
	
			for( size_t cb = 0; cb < vectsz(g_CBData); cb++ )
			{
#if GTK_CHECK_VERSION(3,0,0)
				if( text = NULL )
#else
				if( text = gtk_combo_box_get_active_text(g_CBData[cb].pCB) )
#endif
				{
					if( cb )
						strvcat(g_name, "/");
					else
						g_name[0] = '\0';

					if( cb == 2 )
						strvcpy(g_tzns, text);

					strvcat(g_name, text);
					g_free (text);
				}
			}

			while( cpos = strchr(g_name, ' ') ) *cpos = '_';

			int    loco, utco;
			double lati, lngi;
			char   path[1024], info[1024];

			if( tzn::get(g_name, &loco, &utco, &lati, &lngi, path, vectsz(path), info, vectsz(info), false) )
			{
				struct tm   rise,      sets;
				char        timr[128], tims[128];
				const char* gCfg_srs = gCfg.show24Hrs ? "%R" : "%-I:%M %P"; // TODO: put into cfg/gui/etc?

				while( cpos = strchr(g_name, '_') ) *cpos = ' ';

				DEBUGLOGP("utco=%d, loco=%d\n", utco, loco);

				snprintf(g_utcs, vectsz(g_utcs), "%+2.2d%2.2d", int(utco/3600), int(abs(utco%3600)/60));
				snprintf(g_lats, vectsz(g_lats), "%+9.4f",    float(lati));
				snprintf(g_lngs, vectsz(g_lngs), "%+9.4f",    float(lngi));
				snprintf(g_locs, vectsz(g_locs), "%+2.2d%2.2d", int(loco/3600), int(abs(loco%3600)/60));

				strvcpy (g_cmnt, info[0] ? info : "<none>");

				if( path[0] )
					strvcpy(g_name, path);

				time_t tt =  time(NULL);
				tm     lt = *localtime(&tt);
				tm     zt =  lt;

				strfmtdt(g_tztl, vectsz(g_tztl), gCfg_srs, &lt);
				DEBUGLOGP("local time: %s\n", asctime(&lt));

				// convert to tzone's local time
				zt.tm_hour += double(loco/3600);
				zt.tm_min  += double(abs(loco%3600)/60);
				mktime(&zt);

				bool tzddif        = zt.tm_mday != lt.tm_mday;
				const char* tzdpfx = tzddif ? (loco > 0.0 ? " (tomorrow)" : " (yesterday)") : "";

				strfmtdt(g_tzts, vectsz(g_tzts), gCfg_srs, &zt);
				strvcat (g_tzts, tzdpfx);
				DEBUGLOGP("tzone time: %s\n", asctime(&zt));

				if( lati != 0.0 || lngi != 0.0 )
				{
					// calculate sunrise/set for tzone coord in system's local time
					DayNight dn(lati, lngi);
					dn.get(rise, sets);

					strfmtdt(timr,   vectsz(timr),   gCfg_srs, &rise);
					strfmtdt(tims,   vectsz(tims),   gCfg_srs, &sets);
					snprintf(g_sunl, vectsz(g_sunl), "%s, %s", timr, tims);
					DEBUGLOGP("tzone sunrise in local time: %s\n", asctime(&rise));
					DEBUGLOGP("tzone sunset  in local time: %s\n", asctime(&sets));

					// convert to tzone's local time
					rise.tm_hour += double (loco/3600);
					rise.tm_min  += double(abs(loco%3600)/60);
					mktime(&rise);
					sets.tm_hour += double (loco/3600);
					sets.tm_min  += double(abs(loco%3600)/60);
					mktime(&sets);

					strfmtdt(timr,   vectsz(timr), gCfg_srs, &rise);
					strfmtdt(tims,   vectsz(tims), gCfg_srs, &sets);
					snprintf(g_suns, vectsz(g_suns), "%s, %s", timr, tims);
					strvcat (g_suns, tzdpfx);
					DEBUGLOGP("tzone sunrise in tzone time: %s\n", asctime(&rise));
					DEBUGLOGP("tzone sunset  in tzone time: %s\n", asctime(&sets));

					g_pin_lon[2] = lngi;
					g_pin_lat[2] = lati;
					g_pin_use[2] = true;
				}
				else
				{
					strvcpy (g_suns, "<unknown>");
					strvcpy (g_sunl, "<unknown>");
#if 0
					g_pin_lon[2] = 0;
					g_pin_lat[2] = 0;
					g_pin_use[2] = false;
#endif
				}

//				if( g_pMapWidget && GTK_IS_WIDGET(g_pMapWidget) && gtk_widget_get_visible(g_pMapWidget) )
//					gtk_widget_queue_draw(g_pMapWidget);

				okay = true;
			}
		}
#if 1
		if( !okay )
		{
/*			for( size_t l = 0; l < vectsz(labels); l++ )
			{
				labels[l].text[0] = '?';
				labels[l].text[1] = '\0';
			}*/

			strvcpy(g_tzns, "Time Zone");
		}
#endif
/*		{
			char     text[64];
			snprintf(text, vectsz(text), "<b>%s Time</b>", g_tzns);
			strvcpy (g_tzns, text);
		}*/
#if 0
		for( size_t l = 0; l < vectsz(g_labels); l++ )
		{
			if( g_labels[l].pLabel && g_labels[l].text )
			{
				if( g_labels[l].markup )
					gtk_label_set_markup(g_labels[l].pLabel, g_labels[l].text);
				else
					gtk_label_set_text  (g_labels[l].pLabel, g_labels[l].text);
			}
		}
#endif
	}

	on_tz_changed_update(false, true);

	tzn::end();
	g_cbchgng = false;
#if 0
	if( g_pMapWidget && GTK_IS_WIDGET(g_pMapWidget) && gtk_widget_get_visible(g_pMapWidget) )
		gtk_widget_queue_draw(g_pMapWidget);
#endif

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void tzng::on_tz_changed_update(bool reset, bool draw)
{
	if( reset )
	{
		g_pin_lon[2] = 0;
		g_pin_lat[2] = 0;
		g_pin_use[2] = false;

		for( size_t l = 0; l < vectsz(g_labels); l++ )
		{
			g_labels[l].pLabel  =  GTK_LABEL(lgui::pWidget(g_labels[l].id));
			g_labels[l].text[0] = '?';
			g_labels[l].text[1] = '\0';
		}

		strvcpy(g_tzns, "Time Zone");
	}

	char     text[64];
	snprintf(text, vectsz(text), "<b>%s Time</b>", g_tzns);
	strvcpy (g_tzns,      text);

	if( draw )
	{
		for( size_t l = 0; l < vectsz(g_labels); l++ )
		{
			if( g_labels[l].pLabel && g_labels[l].text )
			{
				if( g_labels[l].markup )
					gtk_label_set_markup(g_labels[l].pLabel, g_labels[l].text);
				else
					gtk_label_set_text  (g_labels[l].pLabel, g_labels[l].text);
			}
		}

		if( g_pMapWidget && GTK_IS_WIDGET(g_pMapWidget) && gtk_widget_get_visible(g_pMapWidget) )
			gtk_widget_queue_draw(g_pMapWidget);
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void tzng::on_map_close_clicked(GtkButton* pButton, gpointer userData)
{
	DEBUGLOGB;

	gboolean rc;
	gpointer pDlg = (gpointer)gtk_widget_get_toplevel(GTK_WIDGET(pButton));
	g_signal_emit_by_name(pDlg, "delete_event", NULL, &rc);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void tzng::on_map_done(GtkWidget* pWidget)
{
	DEBUGLOGB;

	gtk_widget_hide(pWidget);

	if( g_pPin2Image )
		cairo_surface_destroy(g_pPin2Image);
	g_pPin2Image = NULL;

	if( g_pPin1Image )
		cairo_surface_destroy(g_pPin1Image);
	g_pPin1Image = NULL;

	if( g_pMapImage )
		cairo_surface_destroy(g_pMapImage);
	g_pMapImage = NULL;

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
struct cairo_rgba
{
	cairo_rgba(double _r, double _g, double _b, double _a) { r=_r; g=_g; b=_b; a=_a; }
	double r,  g,  b,  a;
};

struct cairo_line
{
	cairo_line(double _xp, double _yp, double _dx, double _dy) { xp=_xp; yp=_yp; dx=_dx; dy=_dy; }
	double xp, yp, dx, dy;
};

static cairo_rgba gmtc(0.25, 1,    0.25, 0.6);
static cairo_rgba idlc(1,    0.25, 0.25, 0.6);
static cairo_rgba arcc(0.5,  0.5,  1,    0.6);
static cairo_rgba tcnc(0.25, 0.25, 1,    0.6);
static cairo_rgba eqac(0,    0,    0,    0.6);
static cairo_rgba tcsc(0.25, 0.25, 1,    0.6);
static cairo_rgba aacc(0.5,  0.5,  1,    0.6);

// -----------------------------------------------------------------------------
static void drawline(cairo_t* pContext, const cairo_rgba& colr, const cairo_line& line)
{
	cairo_set_source_rgba(pContext, colr.r,  colr.g, colr.b, colr.a);
	cairo_move_to        (pContext, line.xp, line.yp);
	cairo_rel_line_to    (pContext, line.dx, line.dy);
	cairo_stroke         (pContext);
}

// -----------------------------------------------------------------------------
gboolean tzng::on_map_expose(GtkWidget* pWidget, GdkEventExpose* pExpose, gpointer userData)
{
	DEBUGLOGB;

	GtkAllocation                       alloc;
	gtk_widget_get_allocation(pWidget, &alloc);

	GdkRectangle     area     = pExpose->area;
	GdkWindow*       pWnd     = gtk_widget_get_window(pWidget);
#if defined(CAIRO_HAS_XLIB_SURFACE) && defined(CAIRO_XLIB_TEST)
	cairo_surface_t* pSurf    = cairo_xlib_surface_create(GDK_WINDOW_XDISPLAY(pWnd), GDK_WINDOW_XID(pWnd), GDK_VISUAL_XVISUAL(gdk_window_get_visual(pWnd)), alloc.width, alloc.height);
	cairo_t*         pContext = cairo_create(pSurf);
#else
	cairo_t*         pContext = gdk_cairo_create(pWnd);
#endif
	bool             okay     = pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS;

#if 0
	double minX = lon_to_x(-180+12, alloc.width);
	double minY = lat_to_y( +90,    alloc.height);
	double maxX = lon_to_x(+180+12, alloc.width);
	double maxY = lat_to_y( -90,    alloc.height);
	DEBUGLOGPF("min(%d,%d) max(%d,%d) map(%d,%d)\n", int(minX), int(minY), int(maxX), int(maxY), int(alloc.width), int(alloc.height));
#endif

	if( okay )
	{
		cairo_set_line_cap (pContext, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_join(pContext, CAIRO_LINE_JOIN_ROUND);
		cairo_set_antialias(pContext, CAIRO_ANTIALIAS_BEST);
	}

	if( okay ) // background
	{
		cairo_save(pContext);

		if( g_pMapImage )
		{
			double mapW = (double)cairo_image_surface_get_width (g_pMapImage);
			double mapH = (double)cairo_image_surface_get_height(g_pMapImage);
			cairo_scale(pContext, (double)alloc.width /mapW, (double)alloc.height/mapH);
			cairo_set_source_surface(pContext, g_pMapImage, 0.0, 0.0);
			cairo_set_operator(pContext, CAIRO_OPERATOR_OVER);
			cairo_paint(pContext);
		}
		else
		{
			cairo_set_line_width(pContext, 1);
			cairo_set_source_rgba(pContext, 1, 1, 1, 1);
			cairo_rectangle(pContext, area.x, area.y, area.width, area.height);
			cairo_fill(pContext);
		}

		cairo_restore(pContext);
	}

	if( okay ) // map pins
	{
		cairo_save(pContext);

		if( g_pPin1Image && g_pPin2Image )
		{
			cairo_scale(pContext, 1.0, 1.0);
			cairo_set_operator(pContext, CAIRO_OPERATOR_OVER);

			double pin1W = (double)cairo_image_surface_get_width (g_pPin1Image);
			double pin1H = (double)cairo_image_surface_get_height(g_pPin1Image);
			DEBUGLOGP("pin1w/h=%2.2f/%2.2f\n", (float)pin1W, (float)pin1H);

			double pin2W = (double)cairo_image_surface_get_width (g_pPin2Image);
			double pin2H = (double)cairo_image_surface_get_height(g_pPin2Image);
			DEBUGLOGP("pin2w/h=%2.2f/%2.2f\n", (float)pin2W, (float)pin2H);

			for( size_t p = 0; p < vectsz(g_pin_lon); p++ )
			{
				if( !g_pin_use[p] ) continue;

				double pinX = lon_to_x(g_pin_lon[p], alloc.width);
				double pinY = lat_to_y(g_pin_lat[p], alloc.height);
				DEBUGLOGP("p=%d, lon=%2.2f, lat=%2.2f, x=%d, y=%d\n",
				         (int)p, (float)g_pin_lon[p], (float)g_pin_lat[p], (int)pinX, (int)pinY);

				if( pinX < 0.0 )          pinX = 0.0;
				if( pinX > alloc.width )  pinX = alloc.width;
				if( pinY < 0.0 )          pinY = 0.0;
				if( pinY > alloc.height ) pinY = alloc.height;

				cairo_save(pContext);

				if( p == 0 ) // current local location pin
				{
					cairo_translate(pContext, pinX-pin1W*0.5, pinY-pin1H); // coordinate at pin middle bottom
					cairo_set_source_surface(pContext, g_pPin1Image, 0.0, 0.0);
				}
				else
				{
					if( p == 1 ) // gmt-on-the-equator pin and reference lines
					{
						cairo_set_line_width(pContext, 1);

						cairo_line gmtl(pinX, 0, 0,                        alloc.height);
						cairo_line idll(lon_to_x(180, alloc.width), 0, 0,  alloc.height);
						cairo_line arcl(0, lat_to_y(66.56, alloc.height),  alloc.width, 0);
						cairo_line tcnl(0, lat_to_y(23.44, alloc.height),  alloc.width, 0);
						cairo_line eqal(0, pinY,                           alloc.width, 0);
						cairo_line tcsl(0, lat_to_y(-23.44, alloc.height), alloc.width, 0);
						cairo_line aacl(0, lat_to_y(-66.56, alloc.height), alloc.width, 0);

						drawline(pContext, gmtc, gmtl); // gmt (green)
						drawline(pContext, idlc, idll); // international date line (red)
						drawline(pContext, arcc, arcl); // arctic circle (light blue)
						drawline(pContext, tcnc, tcnl); // Tropic of Cancer (blue)
						drawline(pContext, eqac, eqal); // equator (black)
						drawline(pContext, tcsc, tcsl); // Tropic of Capicorn (blue)
						drawline(pContext, aacc, aacl); // antarctic circle (light blue)
					}

					cairo_translate(pContext, pinX-pin2W*0.5, pinY-pin2H);   // coordinate at pin middle bottom
					cairo_set_source_surface(pContext, g_pPin2Image, 0.0, 0.0);
				}

				cairo_paint  (pContext);
				cairo_restore(pContext);
			}
		}

		cairo_restore(pContext);
	}

	if( okay ) // sun's 'noon' position
	{
		DayNight dn(0, 0);
		double              sunRA,  sunDec;
		dn.get(NULL, NULL, &sunRA, &sunDec);

		double sunX = lon_to_x(sunRA,  alloc.width);
		double sunY = lat_to_y(sunDec, alloc.height);
		double arcR = 5.0;

		DEBUGLOGP("sunRA=%f(%d), sunDec=%f(%d)\n", float(sunRA), int(sunX), float(sunDec), int(sunY));

		cairo_save           (pContext);
		cairo_arc            (pContext, sunX, sunY, arcR, 0.0, 2.0*M_PI);
		cairo_set_line_width (pContext, 1.0);
		cairo_set_source_rgba(pContext, 0.8, 0.8, 0.0, 1.0);
		cairo_fill_preserve  (pContext);
		cairo_set_source_rgba(pContext, 0.0, 0.0, 0.0, 0.4);
		cairo_stroke         (pContext);
		cairo_restore        (pContext);
#if 0
		cairo_line l1(lon_to_x(sunRA-90, alloc.width), sunY-5,   0, 10);
		cairo_line l2(lon_to_x(sunRA+90, alloc.width), sunY-5,   0, 10);
		cairo_line l3(sunX-5, lat_to_y(sunDec-90, alloc.height), 10, 0);
		cairo_line l4(sunX-5, lat_to_y(sunDec+90, alloc.height), 10, 0);

		drawline(pContext, eqac, l1);
		drawline(pContext, eqac, l2);
		drawline(pContext, eqac, l3);
		drawline(pContext, eqac, l4);
#endif
	}

	if( okay ) // bounding box
	{
		cairo_set_line_width(pContext, 2);
		cairo_set_source_rgba(pContext, 0, 0, 0, 1);
		cairo_rectangle(pContext, 0, 0, alloc.width, alloc.height);
		cairo_stroke(pContext);
	}

#ifdef _DEBUGLOG
	if( okay ) // clipping rect - for debugging purposes only please
	{
		cairo_set_source_rgba(pContext, 0, 1, 0, 0.3);
		cairo_rectangle(pContext, area.x, area.y, area.width, area.height);
		cairo_stroke(pContext);
	}
#endif

	if( okay )
	{
#if defined(CAIRO_HAS_XLIB_SURFACE) && defined(CAIRO_XLIB_TEST)
		cairo_surface_destroy(pSurf);
#endif
		cairo_destroy(pContext);
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean tzng::on_map_popup_close(GtkWidget* pWidget, GdkEvent* pEvent, gpointer userData)
{
	DEBUGLOGB;

	if( pWidget = lgui::pWidget("togglebuttonMap") )
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pWidget), FALSE);

	DEBUGLOGE;
	return TRUE; // don't destroy the widget - it's just hidden instead via above call
}

// -----------------------------------------------------------------------------
gboolean tzng::on_map_redraw(GtkWidget* pWidget)
{
	bool   okay = g_pMapWidget && GTK_IS_WIDGET(g_pMapWidget) && gtk_widget_get_visible(g_pMapWidget);
	if(    okay ) gtk_widget_queue_draw(g_pMapWidget);
	return okay ? TRUE : FALSE; // false kills the timer
}

// -----------------------------------------------------------------------------
void tzng::on_map_toggled(GtkToggleButton* pTogglebutton, gpointer userData)
{
	DEBUGLOGB;

	static guint redrawTimerID = 0;
	GtkWidget*   pDialog       = lgui::pWidget("tzoneMapDialog");

	if( pDialog )
	{
		DEBUGLOGS("got map widget");

		if( gtk_toggle_button_get_active(pTogglebutton) )
		{
			DEBUGLOGS("displaying its window");
			g_signal_connect(G_OBJECT(pDialog), "delete_event", G_CALLBACK(on_map_popup_close),   userData);

			GtkWidget* pWidget;
			if( pWidget = lgui::pWidget("buttonCloseMap") )
			g_signal_connect(G_OBJECT(pWidget), "clicked",      G_CALLBACK(on_map_close_clicked), userData);

			if( pWidget = lgui::pWidget("drawingareaMap") )
			g_signal_connect(G_OBJECT(pWidget), "expose-event", G_CALLBACK(on_map_expose),        userData);

			{
				g_pMapWidget = pWidget;

				const char* mapr  = "/." APP_NAME "/glade/tzone/bg.png";
				const char* mapf  = get_home_subpath(mapr, strlen(mapr), true);
				g_pMapImage       = cairo_image_surface_create_from_png(mapf);
				DEBUGLOGS(g_pMapImage && cairo_surface_status(g_pMapImage) == CAIRO_STATUS_SUCCESS ? "map image created" : "map image NOT created");
				delete []   mapf;

				const char* pin1r = "/." APP_NAME "/glade/tzone/pin.png";
				const char* pin1f = get_home_subpath(pin1r, strlen(pin1r), true);
				g_pPin1Image      = cairo_image_surface_create_from_png(pin1f);
				DEBUGLOGS(g_pPin1Image && cairo_surface_status(g_pPin1Image) == CAIRO_STATUS_SUCCESS ? "pin1 image created" : "pin1 image NOT created");
				delete []   pin1f;

				const char* pin2r = "/." APP_NAME "/glade/tzone/pin2.png";
				const char* pin2f = get_home_subpath(pin2r, strlen(pin2r), true);
				g_pPin2Image      = cairo_image_surface_create_from_png(pin2f);
				DEBUGLOGS(g_pPin2Image && cairo_surface_status(g_pPin2Image) == CAIRO_STATUS_SUCCESS ? "pin2 image created" : "pin2 image NOT created");
				delete []   pin2f;
			}

			set_window_icon(pDialog);
			gtk_widget_show_all(pDialog);
			gtk_window_set_transient_for(GTK_WINDOW(pDialog), GTK_WINDOW(userData));
			gtk_window_present(GTK_WINDOW(pDialog));

			if( redrawTimerID )
				g_source_remove(redrawTimerID);
			redrawTimerID = g_timeout_add(60000, (GSourceFunc)on_map_redraw, (gpointer)pDialog);
		}
		else
		{
			DEBUGLOGS("hiding its window");
			if( redrawTimerID )
				g_source_remove(redrawTimerID);
			redrawTimerID = 0;

/*			if( GTK_IS_WIDGET(pDialog) )
				on_map_popup_close(pDialog, NULL, userData);*/
			if( GTK_IS_WIDGET(pDialog) )
				on_map_done(pDialog);
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// NOTE: The following functions were taken from the cc-timezone-map.c file of
//       the gente project as found on github and reworked as warranted.
// -----------------------------------------------------------------------------
/*
 * Copyright (C) 2010 Intel, Inc
 *
 * Portions from Ubiquity, Copyright (C) 2009 Canonical Ltd.
 * Written by Evan Dandrea <evand@ubuntu.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Author: Thomas Wood <thomas.wood@intel.com>
 *
 */
// -----------------------------------------------------------------------------
double tzng::lat_to_y(double lat, int mapH)
{
	double bot_lat    = -59.0;
	double top_lat    =  81.0;
	double top_per    =  top_lat/180.0;
//	double y          =  1.25*log(tan(G_PI_4+0.4*deg_to_rad(lat)));
	double y          =  lat_to_y(lat);
	double full_range =  4.6068250867599998;
	double top_offset =  full_range*top_per;
//	double map_range  =  fabs(1.25*log(tan(G_PI_4+0.4*deg_to_rad(bot_lat)))-top_offset);
	double map_range  =  fabs(lat_to_y(bot_lat)-top_offset);
	return fabs(y-top_offset)/map_range*mapH;
}

// -----------------------------------------------------------------------------
double tzng::lat_to_y(double lat)
{
	return 1.25*log(tan(G_PI_4+0.4*deg_to_rad(lat)));
}

// -----------------------------------------------------------------------------
double tzng::lon_to_x(double lon, int mapW)
{
	const double xdegOff = -6.0;
//	if( lon < -180 -  2.0*xdegOff )
//		lon =  180 + (lon-xdegOff);
//	return mapW*(180.0+lon)/360.0 + mapW*xdegOff/180.0;
	double ret = mapW*(180.0+lon)/360.0 + mapW*xdegOff/180.0;
	if( ret < 0 ) ret = mapW + ret;
	return ret;
}

// -----------------------------------------------------------------------------
double tzng::deg_to_rad(double deg)
{
	return 2.0*G_PI*deg/360.0;
}

#endif // _USEGTK

