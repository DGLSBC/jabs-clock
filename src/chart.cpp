/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "chart"

#undef   _USEGSL
#define  _USESPLINE

#include <math.h>           // for roundf
#include <stdlib.h>         // for ?

#include "chart.h"          //
#include "config.h"         // for Config struct, CornerType enum, ...
#include "global.h"         //
#include "handAnim.h"       //
#include "settings.h"       //
#include "glade.h"          // for glade::pWidget, ...
#include "debug.h"          // for debugging prints
#include "draw.h"           // for _USEREFRESHER and draw::... funcs/vars

#ifdef   _USEGSL
#include <gsl/gsl_interp.h> // for spline interpolation
#endif

#ifdef   _USESPLINE
#include "cspline.h"        // for static implementation
#endif

#if _USEGTK
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static gboolean on_chart_button_press  (GtkWidget* pWidget, GdkEventButton* pButton, gpointer data);
static gboolean on_chart_button_release(GtkWidget* pWidget, GdkEventButton* pButton, gpointer data);
static gboolean on_chart_close         (GtkWidget* pWidget, GdkEvent*       pEvent,  gpointer data);
static gboolean on_chart_expose        (GtkWidget* pWidget, GdkEventExpose* pExpose, gpointer data);
static gboolean on_chart_motion_notify (GtkWidget* pWidget, GdkEventMotion* pMotion, gpointer data);

static void on_close_clicked        (GtkButton*       pButton,     gpointer window);
static void on_help_clicked         (GtkButton*       pButton,     gpointer window);
static void on_refresh_value_changed(GtkSpinButton*   pSpinButton, gpointer window);
static void on_revert_clicked       (GtkButton*       pButton,     gpointer window);
static void on_save_clicked         (GtkButton*       pButton,     gpointer window);
static void on_show_toggled         (GtkToggleButton* pToggButton, gpointer window);
static void on_skip_value_changed   (GtkSpinButton*   pSpinButton, gpointer window);

static void set_fps_label();

static GtkAllocation gAlloc;
static GdkCursorType gCursorType =  GDK_BLANK_CURSOR;
static int           gMoverPoint = -1;
static int           gHoverPoint = -1;
static int           gHoverX     =  0;
static int           gHoverY     =  0;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void chart::init(GtkWidget* pDrawingArea)
{
	DEBUGLOGB;

//	const double* dpts;
//	float         fpts[128];
//	int           npts   = hand_anim_get(NULL, &dpts);
//	GtkWidget*    pCurve = gui::pGladeWidget("curveCurve");

//	for( size_t p = 0; p < npts; p++ )
//		fpts[p]   = dpts[p];

//	for( size_t p = 0; p < npts; p++ )
//		printf("%s: point %d is %f\n", __func__, p, fpts[p]);

//	gtk_curve_set_vector    (GTK_CURVE(pCurve), npts, fpts);
//	gtk_curve_set_curve_type(GTK_CURVE(pCurve), GTK_CURVE_TYPE_SPLINE);

	// TODO: this doesn't work - way to get button release on curve widget?

//	gtk_widget_add_events(pCurve, GDK_BUTTON_RELEASE_MASK);
//	g_signal_connect(G_OBJECT(pCurve), "button-release-event", G_CALLBACK(on_chart_button_release), NULL);

	gCursorType =  GDK_BLANK_CURSOR;
	gMoverPoint = -1;
	gHoverPoint = -1;
	gHoverX     =  0;
	gHoverY     =  0;

	gtk_widget_get_allocation(pDrawingArea, &gAlloc);

	draw::g_angChart  = pDrawingArea;
	draw::g_angChartW = gAlloc.width;
	draw::g_angChartH = gAlloc.height;

	#define CHART_EVENTS_MASK (GDK_EXPOSURE_MASK       | GDK_ENTER_NOTIFY_MASK        | \
							   GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | \
							   GDK_BUTTON_PRESS_MASK   | GDK_BUTTON_RELEASE_MASK      | GDK_STRUCTURE_MASK)

	gtk_widget_set_events(pDrawingArea, gtk_widget_get_events(pDrawingArea) | CHART_EVENTS_MASK);

	g_signal_connect(G_OBJECT(pDrawingArea), "button-press-event",   G_CALLBACK(on_chart_button_press),   NULL);
	g_signal_connect(G_OBJECT(pDrawingArea), "button-release-event", G_CALLBACK(on_chart_button_release), NULL);
	g_signal_connect(G_OBJECT(pDrawingArea), "expose-event",         G_CALLBACK(on_chart_expose),         NULL);
	g_signal_connect(G_OBJECT(pDrawingArea), "motion-notify-event",  G_CALLBACK(on_chart_motion_notify),  NULL);
	g_signal_connect(G_OBJECT(pDrawingArea), "destroy_event",        G_CALLBACK(on_chart_close),          NULL);

	GtkWidget * pWidget;
	const char* chgval  = "change-value";
	const char* clicked = "clicked";
	const char* toggled = "toggled";
	const char* valchgd = "value-changed";

	pWidget = glade::pWidget("spinbuttonRefresh");
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(pWidget), MIN_REFRESH_RATE, MAX_REFRESH_RATE);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), gCfg.renderRate);
	g_signal_connect(G_OBJECT(pWidget), valchgd, G_CALLBACK(on_refresh_value_changed), gRun.pMainWindow);

	pWidget = glade::pWidget("spinbuttonSkip");
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(pWidget), 0, 9);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(pWidget), gCfg.refSkipCnt);
	g_signal_connect(G_OBJECT(pWidget), valchgd, G_CALLBACK(on_skip_value_changed), gRun.pMainWindow);

	prefs::initToggleBtn("checkbuttonShow", toggled, draw::g_angChartDo ? TRUE : FALSE, on_show_toggled);

	prefs::initControl("buttonClose",  clicked, G_CALLBACK(on_close_clicked),  pDrawingArea);
	prefs::initControl("buttonHelp",   clicked, G_CALLBACK(on_help_clicked),   pDrawingArea);
	prefs::initControl("buttonRevert", clicked, G_CALLBACK(on_revert_clicked), pDrawingArea);
	prefs::initControl("buttonSave",   clicked, G_CALLBACK(on_save_clicked),   pDrawingArea);

	set_fps_label();

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
gboolean on_chart_button_press(GtkWidget* pWidget, GdkEventButton* pButton, gpointer data)
{
	DEBUGLOGB;

	bool add = false;

	if( pButton->button == 1 && gMoverPoint < 0 ) // move or add a point
	{
		if( gHoverPoint >= 0 )
			gMoverPoint  = gHoverPoint;
//		else
//			add = true;
	}
	else
	if( pButton->button == 2 && gMoverPoint < 0 ) // delete or add a point
	{
		if( gHoverPoint >= 0 )
		{
			gtk_widget_queue_draw(draw::g_angChart);
			hand_anim_del(gHoverPoint);
		}
		else
			add = true;
	}

	if( add )
	{
		const double x    =   pButton->x;
		const double y    =   pButton->y;
		const double wwid =   draw::g_angChartW;
		const double whgt =   draw::g_angChartH;
		const double wh17 =   whgt*1.0/6.0;
		const double wh67 =   whgt*4.0/6.0;
		const double px   =        x       /wwid;
		const double py   = ((whgt-y)-wh17)/wh67;

		DEBUGLOGP("wwid=%f, whgt=%f, wh67=%f, buttonX=%f, buttonY=%f, px=%f, py=%f\n",
				   wwid,    whgt,    wh67,    pButton->x, pButton->y, px,    py);

		gtk_widget_queue_draw(draw::g_angChart);
		hand_anim_add(px, py);
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean on_chart_button_release(GtkWidget* pWidget, GdkEventButton* pButton, gpointer data)
{
	DEBUGLOGB;

	gMoverPoint = -1;

/*	if( pButton->type == GDK_BUTTON_RELEASE )
	{
//		printf("%s: button %d released\n", __func__, (int)pButton->button);

		if( pButton->button == 1 )
		{
			float      fpts[128];
			double     xpts[128], dpts[128];
//			int        npts   = hand_anim_get(NULL, dpts);
			int        npts   = vectsz(fpts);
			GtkWidget* pCurve = gui::pGladeWidget("curveCurve");

			gtk_curve_get_vector(GTK_CURVE(pCurve), npts, fpts);

			for( size_t p =  0; p < npts; p++ )
			{
				xpts[p]   = (double)p/(double)(npts-1);
				dpts[p]   =  fpts[p];
			}

			dpts[0]       =  0; // force beginning and end to required values
			dpts[npts-1]  =  1;

//			hand_anim_set(NULL, dpts, npts);
			hand_anim_set(xpts, dpts, npts);
		}
	}*/

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean on_chart_expose(GtkWidget* pWidget, GdkEventExpose* pExpose, gpointer data)
{
	DEBUGLOGB;

	const double* xpts;
	const double* ypts;
	GtkAllocation alloc;

	gtk_widget_get_allocation(pWidget, &gAlloc);

	alloc             = gAlloc;
	draw::g_angChartW = gAlloc.width;
	draw::g_angChartH = gAlloc.height;

	GdkRectangle  area     = pExpose->area;
	GdkWindow*    pWnd     = gtk_widget_get_window(pWidget);
	cairo_t*      pContext = gdk_cairo_create(pWnd);
	const int     npts     = hand_anim_get(&xpts, &ypts);
	const int     wwid     = alloc.width;
	const int     whgt     = alloc.height;
	const int     xcb      = area.x;
	const int     ycb      = area.y;
	const int     xce      = area.x+area.width;
	const int     yce      = area.y+area.height;
	const double  ww25     = wwid*1.0/4.0;
	const double  ww50     = wwid*2.0/4.0;
	const double  ww75     = wwid*3.0/4.0;
	const double  wh17     = whgt*1.0/6.0;
	const double  wh33     = whgt*2.0/6.0;
	const double  wh50     = whgt*3.0/6.0;
	const double  wh67     = whgt*4.0/6.0;
	const double  wh83     = whgt*5.0/6.0;

	DEBUGLOGP("wwid=%d, whgt=%d\n", wwid, whgt);

#ifdef _DEBUGLOG
	double x1, y1, x2, y2;
	cairo_clip_extents(pContext, &x1, &y1, &x2, &y2);
	DEBUGLOGP("area=(%d, %d, %d, %d), clip=(%d, %d, %d, %d)\n",
		xcb, ycb, xce, yce, (int)x1, (int)y1, (int)x2, (int)y2);
#endif

	cairo_set_line_cap (pContext, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(pContext, CAIRO_LINE_JOIN_ROUND);
	cairo_set_antialias(pContext, CAIRO_ANTIALIAS_BEST);

	// background
	{
		cairo_set_line_width (pContext, 1);
		cairo_set_source_rgba(pContext, 1, 1, 1, 1);
		cairo_rectangle      (pContext, area.x, area.y, area.width, area.height);
		cairo_fill           (pContext);
	}

	// fixed vertical reference lines for per second times at which redraw occurs (old)
//	if( gCfg.renderRate > 1 )
//	if( gCfg.renderRate > gCfg.refSkipCnt )
//	if( gCfg.renderRate > gCfg.refSkipCnt && gCfg.refSkipCnt > 1 )
	{
//		double xdel = (double)wwid/(double)gCfg.renderRate;
//		double xdel = (double)wwid/((double)gCfg.renderRate/(double)gCfg.refSkipCnt);
		double xdel = (double)wwid/double(gCfg.renderRate*(gCfg.refSkipCnt+1));
		double x    =  xdel;

		cairo_set_source_rgba(pContext, 1, 0, 1, 0.5);

//		for( int i = 1; i < gCfg.renderRate; i++ )
		while( x <= (double)wwid )
		{
			if( x >= xcb && x <= xce )
			{
				cairo_move_to    (pContext, x, 0);
				cairo_rel_line_to(pContext, 0, whgt);
			}

			x += xdel;
		}

		cairo_stroke(pContext);
	}

#ifdef _USEREFRESHER
	// varying vertical reference lines for per second times at which redraw occurs (new)
	if( false )
	{
		cairo_set_line_width (pContext, 2);
		cairo_set_source_rgba(pContext, 0, 1, 0, 0.5);

		double y, y2;
		double yf = whgt*0.05;
		int    nt = draw::refreshCount;

		for( int i = 0; i < nt; i++ )
		{
			y2 = hand_anim_get(draw::refreshTime[i]);
			y  = whgt-(int)(y2*wh67)-wh17;

			cairo_move_to    (pContext, draw::refreshTime[i]*wwid, y-yf);
			cairo_rel_line_to(pContext, 0, yf*2);
		}

		cairo_stroke(pContext);
	}
#endif

	cairo_set_line_width (pContext, 1);
	cairo_set_source_rgba(pContext, 0, 0, 0, 0.5);

	// horizontal grid lines
	{
		cairo_move_to(pContext, 0, wh17); cairo_rel_line_to(pContext, wwid, 0);
		cairo_move_to(pContext, 0, wh33); cairo_rel_line_to(pContext, wwid, 0);
		cairo_move_to(pContext, 0, wh50); cairo_rel_line_to(pContext, wwid, 0);
		cairo_move_to(pContext, 0, wh67); cairo_rel_line_to(pContext, wwid, 0);
		cairo_move_to(pContext, 0, wh83); cairo_rel_line_to(pContext, wwid, 0);
		cairo_stroke (pContext);
	}

	// vertical grid lines
	{
		cairo_move_to(pContext, ww25, 0); cairo_rel_line_to(pContext, 0, whgt);
		cairo_move_to(pContext, ww50, 0); cairo_rel_line_to(pContext, 0, whgt);
		cairo_move_to(pContext, ww75, 0); cairo_rel_line_to(pContext, 0, whgt);
		cairo_stroke (pContext);
	}

	// reference line for a continuous (sweep) hand advancement
	{
		cairo_set_line_width (pContext, 2);
		cairo_set_source_rgba(pContext, 0.2, 0.2, 1, 0.5);
		cairo_move_to        (pContext, 0,    whgt-wh17);
		cairo_rel_line_to    (pContext, wwid,     -wh67);
		cairo_stroke         (pContext);
	}

#ifdef _USEREFRESHER
	// the hand animation cummulative curve
	{
#ifdef _USEGSL
		gsl_interp_accel*      pAA = gsl_interp_accel_alloc();
		gsl_interp*            pAT = gsl_interp_alloc(gsl_interp_cspline,  draw::refreshCount);
		gsl_interp_init       (pAT,  draw::refreshTime, draw::refreshCumm, draw::refreshCount);
		gsl_interp_accel_reset(pAA);
#else
#ifdef _USESPLINE
		cspline         curve;
		csp::set_points(curve, draw::refreshTime, draw::refreshCumm, draw::refreshCount);
#else
		int    t  = 0;
#endif
#endif
		double y1 = draw::refreshCumm[0];
		int    y  = whgt-(int)(y1*wh67)-wh17;

		cairo_set_line_width (pContext, 2);
		cairo_set_source_rgba(pContext, 1, 0.2, 0.2, 0.25);
		cairo_move_to        (pContext, 0, y);

		for( size_t x = 1; x < wwid; x++ )
		{
#ifdef _USEGSL
			y1 = gsl_interp_eval(pAT, draw::refreshTime, draw::refreshCumm, (double)x/(double)wwid, pAA);
#else
#ifdef _USESPLINE
			y1 = curve((double)x/(double)wwid);
#else
			double tx = (double)x/(double)wwid;
			if(  draw::refreshTime[t] < tx )
				 while( ++t < draw::refreshCount && draw::refreshTime[t] < tx );
			y1 = draw::refreshCumm[t];
#endif
#endif
			y  = whgt-(int)(y1*wh67)-wh17;
			cairo_line_to(pContext, x, y);
		}
#ifdef _USEGSL
		gsl_interp_free      (pAT);
		gsl_interp_accel_free(pAA);
#endif
		cairo_stroke(pContext);
	}
#endif // _USEREFRESHER

	// the hand animation curve
	{
		int    x  =  xcb;
		int    xb =  x+1;
		int    xe =  xce;
//		int    np =  100;
//		int    xd = (wwid/np)+1;
		int    xd =  1; // increase to reduce # of points to draw
		double y1 =  hand_anim_get((double)x/(double)wwid);
		int    y  =  whgt-(int)(y1*wh67)-wh17;

		cairo_move_to(pContext, x, y);

//		for( x = xb; x < xe; x++ )
		for( x = xb; x < xe; x+=xd )
		{
			y1 = hand_anim_get((double)x/(double)wwid);
			y  = whgt-(int)(y1*wh67)-wh17;
			cairo_line_to(pContext, x, y);
		}

		y1 = hand_anim_get((double)xe/(double)wwid);
		y  = whgt-(int)(y1*wh67)-wh17;
		cairo_line_to(pContext, xe, y);

		cairo_set_line_width (pContext, 3);
		cairo_set_source_rgba(pContext, 1, 0.2, 0.2, 0.4);
/*		cairo_stroke_preserve(pContext);

		cairo_set_line_width (pContext, 1);
		cairo_set_source_rgba(pContext, 1, 0.0, 0.0, 0.6);*/
		cairo_stroke         (pContext);
	}

	// the hand animation curve points
	{
		int x, y;
		for( int p = 0; p < npts; p++ )
		{
			x =      (int)(xpts[p]*wwid);
			y = whgt-(int)(ypts[p]*wh67)-wh17;

			if( x >= xcb && x <= xce && y >= ycb && y <= yce )
			{
				cairo_arc(pContext, x, y, 2, 0, 2*G_PI);
				cairo_new_sub_path(pContext);
			}
		}

		cairo_set_source_rgba(pContext, 0, 0, 0, 0.8);
		cairo_fill(pContext);
	}

	// the current second hand position
	if( draw::g_angChartDo )
	{
		int    x  = draw::g_angAdjX*wwid;
		double y1 = draw::g_angAdjY;
		int    y  = whgt-(int)(y1*wh67)-wh17;

		cairo_set_source_rgba(pContext, 0, 0, 0, 1);
		cairo_arc (pContext, x, y, 4, 0, 2*G_PI);
		cairo_fill(pContext);

		cairo_set_source_rgba(pContext, 1, 0, 0, 1);
		cairo_arc (pContext, x, y, 3, 0, 2*G_PI);
		cairo_fill(pContext);

/*		{
		int x = (int)(gRun.secDrift*(double)wwid);
		int y =  whgt-6*2;

		cairo_set_source_rgba(pContext, 1, 0, 1, 1);
		cairo_arc (pContext, x, y, 6, 0, 2*G_PI);
		cairo_fill(pContext);
		}*/
	}

	// bounding box
//	if( area.x == 0 && area.y == 0 && area.width == wwid && area.height == whgt )
	{
		cairo_set_line_width(pContext, 2);
		cairo_set_source_rgba(pContext, 0, 0, 0, 1);
		cairo_rectangle(pContext, 0, 0, wwid, whgt);
		cairo_stroke(pContext);
	}

#ifdef _DEBUGLOG
	// clipping rect - for debugging purposes only please
	{
		cairo_set_source_rgba(pContext, 0, 1, 0, 0.3);
		cairo_rectangle(pContext, area.x, area.y, area.width, area.height);
		cairo_stroke(pContext);
	}
#endif

	cairo_destroy(pContext);

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean on_chart_motion_notify(GtkWidget* pWidget, GdkEventMotion* pMotion, gpointer data)
{
	DEBUGLOGB;

	const double* xpts;
	const double* ypts;
	int           px, py;
	GtkAllocation alloc;

	gtk_widget_get_allocation(pWidget, &alloc);

	int           cx   = (int)pMotion->x;
	int           cy   = (int)pMotion->y;
	int           npts = hand_anim_get(&xpts, &ypts);
//	int           wwid = pWidget->allocation.width;
//	int           whgt = pWidget->allocation.height;
	int           wwid = alloc.width;
	int           whgt = alloc.height;
	double        wh17 = whgt*1.0/6.0;
	double        wh67 = whgt*4.0/6.0;

	DEBUGLOGP("  mover pt is %d\n", gMoverPoint);

	if( gMoverPoint >= 0 )
	{
		if( cx != gHoverX || cy != gHoverY )
		{
			DEBUGLOGP("  mover pt %d changing to (%d, %d)\n", gMoverPoint, cx, cy);

			bool   okay;
			double xdel = double(cx - gHoverX)/double(wwid);
			double ydel = double(cy - gHoverY)/double(whgt)*1.5;

			bool   xbeg = gMoverPoint == 0;
			bool   xend = gMoverPoint == npts - 1;
			double xnew = xpts[gMoverPoint] + xdel;
			double ynew = ypts[gMoverPoint] - ydel;

			okay  =  xbeg && xnew >= 0;
			okay |= !xbeg && xnew >  xpts[gMoverPoint-1] && !xend && xnew < xpts[gMoverPoint+1];
			okay |=  xend && xnew <= 1;

			if( okay && ynew >= -0.25 && ynew <= 1.25 )
			{
//				printf("%s: #%d (%d) poschg (%d %d)->(%d %d) (%f %f)\n",
//						__func__, gMoverPoint, gHoverPoint, gHoverX, gHoverY, cx, cy, xd, yd);

				hand_anim_chg(gMoverPoint, xnew, ynew);

				draw::update_ani();

				gtk_widget_queue_draw(pWidget);

/*				const int nw = 6;
				const int dw = nw*wwid/(npts-1);
				const int px = xpts[gMoverPoint]*wwid;

				gtk_widget_queue_draw_area(pWidget, px-dw, 0, 2*dw, whgt);*/
			}

			gHoverX = cx;
			gHoverY = cy;
		}

		DEBUGLOGE;
		return FALSE;
	}

	const int     minDist    = 4;
	GdkCursorType cursorType = GDK_TCROSS;

	DEBUGLOGS("  bef checking on hover pt/cursor type change");

	for( size_t p = 1; p < npts-1; p++ )
	{
		px = (int)(xpts[p]*wwid);
		py =  whgt-(int)(ypts[p]*wh67)-wh17;

		if( abs(px-cx)  < minDist && abs(py-cy) < minDist )
		{
			cursorType  = GDK_FLEUR;
			gHoverX     = cx;
			gHoverY     = cy;
			gHoverPoint = p;
			break;
		}
	}

	DEBUGLOGS("  aft checking on hover pt/cursor type change");

	if( gCursorType != cursorType ) // hovering change?
	{
		DEBUGLOGS("  changing cursor type");

		gCursorType  = cursorType;

		if( cursorType  == GDK_TCROSS ) // no longer hovering?
			gHoverPoint = -1;

		change_cursor(pWidget, cursorType);
	}

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean on_chart_close(GtkWidget* pWidget, GdkEvent* pEvent, gpointer data)
{
	DEBUGLOGB;

	draw::g_angChart = NULL;

	DEBUGLOGE;
	return FALSE;
}

// -----------------------------------------------------------------------------
void on_close_clicked(GtkButton* pButton, gpointer window)
{
	DEBUGLOGB;

	draw::g_angChart = NULL;

	// TODO: need to check if anything's been changed before saving

//	cfg::save(); // TODO: this only happens here & not via delete event - is this okay?

	gboolean rc;
	gpointer pDlg = (gpointer)gtk_widget_get_toplevel(GTK_WIDGET(pButton));
	g_signal_emit_by_name(pDlg, "delete_event", NULL, &rc);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void on_help_clicked(GtkButton* pButton, gpointer window)
{
	DEBUGLOGB;

	GtkWindow* pWindow = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(window)));

	GtkWidget* pWidget =
		gtk_message_dialog_new(pWindow,
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
			"No help available yet, other than through the command line via -? or --help.\n\nSorry.");

	if( pWidget )
	{
		gtk_window_set_transient_for(GTK_WINDOW(pWidget), pWindow);
		gtk_window_set_position(GTK_WINDOW(pWidget), GTK_WIN_POS_CENTER);
		gtk_window_set_title(GTK_WINDOW(pWidget), gRun.appName);
		gtk_dialog_run(GTK_DIALOG(pWidget));
		gtk_widget_destroy(pWidget);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void on_refresh_value_changed(GtkSpinButton* pSpinButton, gpointer window)
{
	DEBUGLOGB;

	gint renderRate = gtk_spin_button_get_value_as_int(pSpinButton);

	if( renderRate != gCfg.renderRate )
	{
		DEBUGLOGP("rate is %d\n", renderRate);
		change_ani_rate(GTK_WIDGET(window), renderRate, true, false);
		gtk_widget_queue_draw(draw::g_angChart);
		draw::update_ani();
		set_fps_label();
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void on_revert_clicked(GtkButton* pButton, gpointer window)
{
	DEBUGLOGB;

	gtk_widget_queue_draw(draw::g_angChart);
	draw::reset_ani();

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void on_save_clicked(GtkButton* pButton, gpointer window)
{
	DEBUGLOGB;

//	gint shandType = gCfg.shandType;
//	gCfg.shandType = draw::ANIM_CUSTOM;

//	cfg::save();
	cfg::save(false, true); // TODO: switch to this to force save regardless?

//	gCfg.shandType = shandType;

	GtkWindow* pWindow = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(window)));

	GtkWidget* pWidget =
		gtk_message_dialog_new(pWindow,
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
			"The curve data has been saved as the custom Hand Movement method (aka 'Custom' Motion Technique).");

	if( pWidget )
	{
		gtk_window_set_transient_for(GTK_WINDOW(pWidget), pWindow);
		gtk_window_set_position(GTK_WINDOW(pWidget), GTK_WIN_POS_CENTER);
		gtk_window_set_title(GTK_WINDOW(pWidget), gRun.appName);
		gtk_dialog_run(GTK_DIALOG(pWidget));
		gtk_widget_destroy(pWidget);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void on_show_toggled(GtkToggleButton* pToggButton, gpointer window)
{
	DEBUGLOGB;

	draw::g_angChartDo = gtk_toggle_button_get_active(pToggButton) ? true : false;
	gtk_widget_queue_draw(draw::g_angChart);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void on_skip_value_changed(GtkSpinButton* pSpinButton, gpointer window)
{
	DEBUGLOGB;

	gint skipCnt = gtk_spin_button_get_value_as_int(pSpinButton);

	if( skipCnt != gCfg.refSkipCnt )
	{
		DEBUGLOGP("count is %d\n", skipCnt);
		gCfg.refSkipCur = gCfg.refSkipCnt = skipCnt;
		change_ani_rate(GTK_WIDGET(window), gCfg.renderRate, true, false);
		gtk_widget_queue_draw(draw::g_angChart);
		draw::update_ani();
		set_fps_label();
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void set_fps_label()
{
	int   dtps = gCfg.renderRate*(gCfg.refSkipCnt+1); // desired timeouts/sec
	int   toms = roundf(1000.0f/float(dtps));         // timeout duration (ms)
	float atps = 1000.0f/float(toms);                 // actual  timeouts/sec

	char     txt[32];
	snprintf(txt, vectsz(txt), "( %2.2f tps )", atps);

	GtkWidget* pWidget = glade::pWidget("labelFramesPerSec");
	gtk_label_set_text(GTK_LABEL(pWidget), txt);
}

#endif // _USEGTK

