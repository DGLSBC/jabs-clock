/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "chart"

#include "global.h"
#include "chart.h"
#include "draw.h"
#include "handAnim.h"
#include "debug.h"          // for debugging prints

#include <gsl/gsl_interp.h> // for spline interpolation

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static gboolean on_chart_button_press  (GtkWidget* pWidget, GdkEventButton* pEvent, gpointer data);
static gboolean on_chart_button_release(GtkWidget* pWidget, GdkEventButton* pEvent, gpointer data);
static gboolean on_chart_expose        (GtkWidget* pWidget, GdkEventExpose* pEvent, gpointer data);
static gboolean on_chart_motion_notify (GtkWidget* pWidget, GdkEventMotion* pEvent, gpointer data);

static GdkCursorType gCursorType =  GDK_BLANK_CURSOR;
static int           gMoverPoint = -1;
static int           gHoverPoint = -1;
static int           gHoverX     =  0;
static int           gHoverY     =  0;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void chart_init(GtkWidget* pDrawingArea)
{
//	const double* dpts;
//	float         fpts[1024];
//	int           npts   = hand_anim_get(NULL, &dpts);
//	GtkWidget*    pCurve = glade_xml_get_widget(g_pGladeXml, "curveCurve");

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

	#define CHART_EVENTS_MASK (GDK_EXPOSURE_MASK       | GDK_ENTER_NOTIFY_MASK        | \
							   GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | \
							   GDK_BUTTON_PRESS_MASK   | GDK_BUTTON_RELEASE_MASK)

	gtk_widget_set_events(pDrawingArea, gtk_widget_get_events(pDrawingArea) | CHART_EVENTS_MASK);

	g_signal_connect(G_OBJECT(pDrawingArea), "button-press-event",   G_CALLBACK(on_chart_button_press),   NULL);
	g_signal_connect(G_OBJECT(pDrawingArea), "button-release-event", G_CALLBACK(on_chart_button_release), NULL);
	g_signal_connect(G_OBJECT(pDrawingArea), "expose-event",         G_CALLBACK(on_chart_expose),         NULL);
	g_signal_connect(G_OBJECT(pDrawingArea), "motion-notify-event",  G_CALLBACK(on_chart_motion_notify),  NULL);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
gboolean on_chart_button_press(GtkWidget* pWidget, GdkEventButton* pEvent, gpointer data)
{
//	printf("%s: entry\n", __func__);

	if( gMoverPoint < 0 && gHoverPoint >= 0 )
		gMoverPoint = gHoverPoint;

//	printf("%s: exit\n", __func__);
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean on_chart_button_release(GtkWidget* pWidget, GdkEventButton* pEvent, gpointer data)
{
//	printf("%s: entry\n", __func__);

	gMoverPoint = -1;

/*	if( pButton->type == GDK_BUTTON_RELEASE )
	{
//		printf("%s: button %d released\n", __func__, (int)pButton->button);

		if( pButton->button == 1 )
		{
			float      fpts[1024];
			double     xpts[1024], dpts[1024];
//			int        npts   = hand_anim_get(NULL, dpts);
			int        npts   = vectsz(fpts);
			GtkWidget* pCurve = glade_xml_get_widget(g_pGladeXml, "curveCurve");

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

//	printf("%s: exit\n", __func__);
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean on_chart_expose(GtkWidget* pWidget, GdkEventExpose* pEvent, gpointer data)
{
//	printf("%s: entry\n", __func__);

	const double* xpts;
	const double* ypts;
	GdkPoint      gpts[1024];
	GdkDrawable*  pWnd = pWidget->window;
	int           npts = hand_anim_get(&xpts, &ypts);
	int           wwid = pWidget->allocation.width;
	int           whgt = pWidget->allocation.height;
	double        ww25 = wwid*1.0/4.0;
	double        ww50 = wwid*2.0/4.0;
	double        ww75 = wwid*3.0/4.0;
	double        wh17 = whgt*1.0/6.0;
	double        wh33 = whgt*2.0/6.0;
	double        wh50 = whgt*3.0/6.0;
	double        wh67 = whgt*4.0/6.0;
	double        wh83 = whgt*5.0/6.0;

	for( size_t p =  0; p < npts; p++ )
	{
		gpts[p].x = (int)(xpts[p]*wwid);
		gpts[p].y =  whgt-(int)(ypts[p]*wh67)-wh17;
	}

/*	if( true ) // use gdk for drawing?
	{
		GtkStyle*    pStyle = pWidget->style;
//		GtkStateType state  = gtk_widget_is_sensitive(pWidget) ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE;
		GtkStateType state  = gtk_widget_get_state(pWidget);
		GdkGC*       pFGC   = pStyle->fg_gc  [state];
		GdkGC*       pBGC   = pStyle->bg_gc  [state];
		GdkGC*       pDGC   = pStyle->dark_gc[state];

		// background
		gtk_paint_flat_box(pStyle, pWnd, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, pWidget, "curve_bg", 0, 0, wwid, whgt);

		// bounding box
		gdk_draw_rectangle(pWnd, pFGC, FALSE, 0, 0, wwid-1, whgt-1);

		// horizontal grid lines
		gdk_draw_line(pWnd, pDGC, 0, (int)wh17, wwid-1, (int)wh17);
		gdk_draw_line(pWnd, pDGC, 0, (int)wh33, wwid-1, (int)wh33);
		gdk_draw_line(pWnd, pDGC, 0, (int)wh50, wwid-1, (int)wh50);
		gdk_draw_line(pWnd, pDGC, 0, (int)wh67, wwid-1, (int)wh67);
		gdk_draw_line(pWnd, pDGC, 0, (int)wh83, wwid-1, (int)wh83);

		// vertical grid lines
		gdk_draw_line(pWnd, pDGC, (int)ww25, 0, (int)ww25, whgt-1);
		gdk_draw_line(pWnd, pDGC, (int)ww50, 0, (int)ww50, whgt-1);
		gdk_draw_line(pWnd, pDGC, (int)ww75, 0, (int)ww75, whgt-1);
	
//		gdk_draw_lines (pWnd, pDGC, gpts, npts);
//		gdk_draw_points(pWnd, pFGC, gpts, npts);

		double y2;
		int    y1, y;

		y2 = hand_anim_get(0);
		y  = whgt-(int)(y2*wh67)-wh17;
		y1 = y;

		for( size_t x = 1; x < wwid; x++ )
		{
			y2 = hand_anim_get((double)x/(double)wwid);
			y  = whgt-(int)(y2*wh67)-wh17;
			gdk_draw_line(pWnd, pDGC, x-1, y1, x, y);
			y1 = y;
		}

		for( size_t p = 0; p < npts; p++ )
			gdk_draw_arc(pWnd, pFGC, TRUE, gpts[p].x-3, gpts[p].y-3, 7, 7, 0*64, 360*64);
	}*/

	if( true ) // use cairo for drawing?
	{
		cairo_t* pContext = gdk_cairo_create(pWnd);

		// background
		{
			cairo_set_line_width (pContext, 1);
			cairo_set_source_rgba(pContext, 1, 1, 1, 1);
			cairo_rectangle      (pContext, 0, 0, wwid, whgt);
			cairo_fill           (pContext);
		}

		// bounding box
		{
			cairo_set_source_rgba(pContext, 0, 0, 0, 1);
			cairo_rectangle      (pContext, 0, 0, wwid, whgt);
			cairo_stroke         (pContext);
		}

		// fixed vertical reference lines for per second times at which redraw occurs (old)
		if( gCfg.refreshRate > 1 )
		{
			double xdel = (double)wwid/(double)gCfg.refreshRate;
			double x    =  xdel;

			cairo_set_source_rgba(pContext, 1, 0, 1, 0.5);

			for( int i  = 1; i < gCfg.refreshRate; i++ )
			{
				cairo_move_to    (pContext, x, 0);
				cairo_rel_line_to(pContext, 0, whgt);
				x += xdel;
			}

			cairo_stroke(pContext);
		}

		// varying vertical reference lines for per second times at which redraw occurs (new)
		{
			cairo_set_line_width (pContext, 2);
			cairo_set_source_rgba(pContext, 0, 1, 0, 0.5);

			double y, y2;
			double yf = whgt*0.05;
//			int    nt = vectsz(draw::refreshTime);
			int    nt = draw::refreshCount;

//			printf("%s: varying vert ref lines nt=%d\n", __func__, nt);

			for( int i = 0; i < nt; i++ )
			{
				y2 = hand_anim_get(draw::refreshTime[i]);
				y  = whgt-(int)(y2*wh67)-wh17;

				cairo_move_to    (pContext, draw::refreshTime[i]*wwid, y-yf);
				cairo_rel_line_to(pContext, 0, yf*2);
			}

			cairo_stroke(pContext);
		}

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

		// the hand animation cummulative curve
		{
			gsl_interp_accel* pAA = gsl_interp_accel_alloc();
			gsl_interp*       pAT = gsl_interp_alloc(gsl_interp_cspline, draw::refreshCount);

			gsl_interp_init       (pAT, draw::refreshTime, draw::refreshCumm, draw::refreshCount);
			gsl_interp_accel_reset(pAA);

//			int    nf = draw::refreshCount;
			double y1 = draw::refreshCumm[0];
//			double y1 = gsl_interp_eval(pAT, draw::refreshTime, draw::refreshCumm, 0, pAA);
			int    y  = whgt-(int)(y1*wh67)-wh17;

			cairo_set_line_width (pContext, 2);
			cairo_set_source_rgba(pContext, 1, 0.2, 0.2, 0.25);
			cairo_move_to        (pContext, 0, y+1);

//			for( int i = 1; i < nf; i++ )
			for( size_t x = 1; x < wwid; x++ )
			{
//				y1 = draw::refreshCumm[i];
				y1 = gsl_interp_eval(pAT, draw::refreshTime, draw::refreshCumm, (double)x/(double)wwid, pAA);
				y  = whgt-(int)(y1*wh67)-wh17;
//				cairo_line_to(pContext, draw::refreshTime[i]*wwid, y+1);
				cairo_line_to(pContext, x, y+1);
			}

			gsl_interp_free      (pAT);
			gsl_interp_accel_free(pAA);

			cairo_stroke(pContext);
		}

		// the hand animation curve
		{
			double y1 = hand_anim_get(0);
			int    y  = whgt-(int)(y1*wh67)-wh17;

			cairo_set_line_width (pContext, 2);
			cairo_set_source_rgba(pContext, 1, 0.2, 0.2, 1);
			cairo_move_to        (pContext, 0, y+1);

			for( size_t x = 1; x < wwid; x++ )
			{
				y1 = hand_anim_get((double)x/(double)wwid);
				y  = whgt-(int)(y1*wh67)-wh17;
				cairo_line_to(pContext, x, y+1);
			}

			cairo_stroke(pContext);
		}

		// the hand animation curve points
		{
			cairo_set_source_rgba(pContext, 0, 0, 0, 0.8);

			for( size_t p = 0; p < npts; p++ )
			{
				cairo_arc (pContext, gpts[p].x+1, gpts[p].y+1, 2, 0, 2*G_PI);
				cairo_fill(pContext);
			}
		}

		cairo_destroy(pContext);
	}

//	printf("%s: exit\n", __func__);
	return FALSE;
}

// -----------------------------------------------------------------------------
gboolean on_chart_motion_notify(GtkWidget* pWidget, GdkEventMotion* pEvent, gpointer data)
{
//	printf("%s: entry\n", __func__);

	const double* xpts;
	const double* ypts;
	int           px, py;
	int           cx   = (int)pEvent->x;
	int           cy   = (int)pEvent->y;
	int           npts = hand_anim_get(&xpts, &ypts);
	int           wwid = pWidget->allocation.width;
	int           whgt = pWidget->allocation.height;
	double        wh17 = whgt*1.0/6.0;
	double        wh67 = whgt*4.0/6.0;

	if( gMoverPoint >= 0 )
	{
		if( cx != gHoverX || cy != gHoverY )
		{
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
			}

			gHoverX = cx;
			gHoverY = cy;
		}

//		printf("%s: exit\n", __func__);
		return FALSE;
	}

	const int     minDist    = 4;
	GdkCursorType cursorType = GDK_TCROSS;

	for( size_t p = 0; p < npts; p++ )
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

	if( gCursorType != cursorType ) // hovering change?
	{
		gCursorType  = cursorType;

		if( cursorType  == GDK_TCROSS ) // no longer hovering?
			gHoverPoint = -1;

		GdkCursor* pCursor = gdk_cursor_new_for_display(gtk_widget_get_display(pWidget), cursorType);
		gdk_window_set_cursor(pWidget->window, pCursor);
		gdk_cursor_unref(pCursor);
	}

//	printf("%s: exit\n", __func__);
	return FALSE;
}

