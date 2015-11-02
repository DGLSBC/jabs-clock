/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "draw"

#include "cfgdef.h"       // for ?
#include "global.h"       // for runtime data
#include "debug.h"        // for debugging prints

#include <math.h>         // for fabs
#include <cairo.h>        // for drawing primitives
#include <librsvg/rsvg.h> // for svg loading/rendering/etc.

#include "draw.h"         //
#include "config.h"       // for config data
#include "utility.h"      // for ?
#include "handAnim.h"     // for ?

#if CAIRO_HAS_FT_FONT
#include <cairo-ft.h>
#include <ft2build.h>
#include  FT_FREETYPE_H
#endif

#undef   _DRAWINPMASK
#ifdef   _DRAWINPMASK
static    GdkBitmap* g_pShapeBitmap = NULL;
#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
enum SurfaceType
{
	SURF_CBASE = 0,
	SURF_BKGND,
	SURF_FRGND,
	SURF_HHAND,
	SURF_MHAND,
	SURF_SHAND,
	SURF_HHSHD,
	SURF_MHSHD,
	SURF_SHSHD,
//	SURF_CDATE, // not using yet
	SURF_COUNT
//	SURF_COUNT=SURF_SHAND+1 // not using shadow & date surfaces for now
};

#define minv(a, b) (a) < (b) ? (a) : (b)
#define maxv(a, b) (a) > (b) ? (a) : (b)

static const double pi2Rad  =  G_PI/180.0; //
static const double g_xDiv  =  2.0;        // TODO: eventually replace these two with
static const double g_yDiv  =  8.0;        //       gRun.bboxNNNHand usages
/*
static const double g_coffX = -0.75;
static const double g_coffY =  0.75;
static const double g_hhoff =  0;
static const double g_mhoff =  0;
static const double g_shoff =  0;
*/
static const double g_coffX = -1.00;
static const double g_coffY =  1.00;
static const double g_hhoff =  0.35;
static const double g_mhoff =  0.30;
static const double g_shoff =  0.25;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
namespace draw
{

// -----------------------------------------------------------------------------
struct DrawSurf
{
	SurfaceType      type;
	cairo_surface_t* pCairoSurf;
	cairo_pattern_t* pCairoPtrn;
	gint             width;
	gint             height;
	double           svgRatW;
	double           svgRatH;
};

// -----------------------------------------------------------------------------
struct UpdateTheme
{
	UpdateThemeDone callBack;   // where to go once the theme updating has finished
	gpointer        data;       // what caller wants passed to the callback
	char            path[1024]; // theme file path
	char            name[64];   // theme name
	bool            okay;       // whether the updating was a success
};

// -----------------------------------------------------------------------------
static void render(cairo_t* pContext, double scaleX, double scaleY, bool renderIt, bool appStart, int width, int height, bool animate);

static bool render_cbase(cairo_t* pContext, int width, int height, bool bright=false);
static bool render_bkgnd(cairo_t* pContext, int width, int height, bool bright=false, bool date  =true, bool temp  =false);
static bool render_frgnd(cairo_t* pContext, int width, int height, bool bright=false);
static bool render_hhand(cairo_t* pContext, int width, int height, bool bright=false, bool center=true, bool rotate=false, bool hand=true, bool shadow=true);
static bool render_mhand(cairo_t* pContext, int width, int height, bool bright=false, bool center=true, bool rotate=false, bool hand=true, bool shadow=true);
static bool render_shand(cairo_t* pContext, int width, int height, bool bright=false, bool center=true, bool rotate=false, bool hand=true, bool shadow=true);

static bool render_chand(cairo_t* pContext, int handElem, int shadElem, int width, int height, bool bright, double hoff, bool center=true, double rotate=-1);

static void render_date(cairo_t* pContext);
static void render_date(cairo_t* pContext, int width, int height, bool bright);
static bool render_hand(cairo_t* pContext, int handElem, int shadElem, double hoffX, double hoffY, double angle);

static void render_hand_hl(cairo_t* pContext, int width, int height, double scaleX, double scaleY, bool appStart, double rotate, bool useSurf, bool optHand, enum SurfaceType st, int handElem, int shadElem, double hoff, double* bbox=NULL, double alf=1.0);

static void render_surf(cairo_t* pContext, enum SurfaceType st, cairo_operator_t op1, int op2=-1, double alf=1.0);

static bool update_surface(cairo_t* pSourceContext, int width, int height, SurfaceType type);
static bool update_surf   (cairo_t* pContext, SurfaceType st, int width, int height);

static gpointer update_theme_func(gpointer data);
static bool     update_theme_internal(const char* path, const char* name);

/*
double refreshTime[MAX_REFRESH_RATE+2];
double refreshFrac[MAX_REFRESH_RATE+2];
double refreshCumm[MAX_REFRESH_RATE+2];
*/
double refreshTime[100+2];
double refreshFrac[100+2];
double refreshCumm[100+2];
int    refreshCount =  0;

// -----------------------------------------------------------------------------
static DrawSurf              g_surfs[SURF_COUNT];
static DrawSurf              g_temps[SURF_COUNT];
static gint                  g_pCairoSurfLock = 0; // per surface bit locking

static cairo_font_face_t*    g_pTextFontFace  = NULL;
static cairo_font_options_t* g_pTextOptions   = NULL;

static RsvgHandle*           g_pSvgHandle[CLOCK_ELEMENTS];
static RsvgDimensionData     g_svgClock       = { 0, 0 };
static gint                  g_pSvgHandleLock = 0; // per element bit locking
static cairo_region_t*       g_pMaskRgn       = NULL;

static gint                  g_surfaceW       = 0;
static gint                  g_surfaceH       = 0;
/*
static double                g_wratio         = 1;
static double                g_hratio         = 1;
*/
#if CAIRO_HAS_FT_FONT
static FT_Face    ft_face    = 0;
static FT_Library ft_library = 0;
#endif
/*
static PangoLayout* g_pPFLayout = NULL;
*/

// -----------------------------------------------------------------------------
static inline void SurfLockBeg(int b) { g_bit_lock  (&g_pCairoSurfLock, b); }
static inline void SurfLockEnd(int b) { g_bit_unlock(&g_pCairoSurfLock, b); }

static inline void SvgLockBeg (int b) { g_bit_lock  (&g_pSvgHandleLock, b); }
static inline void SvgLockEnd (int b) { g_bit_unlock(&g_pSvgHandleLock, b); }

/*
static void g_lockbeg(int& a, int b)
{
//	if( g_thread_self() == ? ) return; // use TLS value(s) here (1/lock-bit)?
	DEBUGLOGP("Locking %s (%d)", &a == &g_pCairoSurfLock ? "surf" : "svg", b);
	fflush(stdout);
	g_bit_lock(&a, b);
	printf(" done\n");
	fflush(stdout);
}

static void g_lockend(int& a, int b)
{
//	if( g_thread_self() == ? ) return; // use TLS value(s) here (1/lock-bit)?
	DEBUGLOGP("UNLocking %s (%d)", &a == &g_pCairoSurfLock ? "surf" : "svg", b);
	fflush(stdout);
	g_bit_unlock(&a, b);
	printf(" done\n");
	fflush(stdout);
}

static inline void SurfLockBeg(int b) { g_lockbeg(g_pCairoSurfLock, b); }
static inline void SurfLockEnd(int b) { g_lockend(g_pCairoSurfLock, b); }

static inline void SvgLockBeg (int b) { g_lockbeg(g_pSvgHandleLock, b); }
static inline void SvgLockEnd (int b) { g_lockend(g_pSvgHandleLock, b); }
*/
/*
static inline void SurfLockBeg(int b) {}
static inline void SurfLockEnd(int b) {}

static inline void SvgLockBeg (int b) {}
static inline void SvgLockEnd (int b) {}
*/
} // draw

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::init()
{
	DEBUGLOGB;
//	DEBUGLOGF("entry\n");

	for( size_t st =  0; st < SURF_COUNT; st++ )
	{
		SurfLockBeg(st);
		g_surfs[st].type       = (SurfaceType)st;
		g_surfs[st].pCairoSurf =  NULL;
		g_surfs[st].pCairoPtrn =  NULL;
		g_surfs[st].width      =  0;
		g_surfs[st].height     =  0;
		g_surfs[st].svgRatW    =  0;
		g_surfs[st].svgRatH    =  0;
		g_temps[st]            =  g_surfs[st];
		SurfLockEnd(st);
	}

	for( size_t e =  0; e < CLOCK_ELEMENTS; e++ )
	{
		SvgLockBeg  (e);
		g_pSvgHandle[e] = NULL;
		SvgLockEnd  (e);
	}

//	DEBUGLOGF("exit\n");
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::beg(bool init)
{
	DEBUGLOGB;

	if( init )
	{
		DEBUGLOGS("initing");

		for( size_t st = 0; st < SURF_COUNT; st++ )
		{
			SurfLockBeg(st);
			g_surfs[st].pCairoPtrn = NULL;
			g_surfs[st].pCairoSurf = NULL;
			g_temps[st]            = g_surfs[st];
			SurfLockEnd(st);
		}

		if( g_svgClock.width == 0 || g_svgClock.height == 0 )
		{
			g_svgClock.width  = 100;
			g_svgClock.height = 100;

			DEBUGLOGS("setting svgs dims to 100");
		}
	}

	end(true);
	reset_ani();

	DEBUGLOGS("bef font processing calls");

	// TODO: does this need to go earlier (if clock might already be displayed?)

	g_pTextOptions = cairo_font_options_create();
	cairo_font_options_set_antialias (g_pTextOptions, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_hint_style(g_pTextOptions, CAIRO_HINT_STYLE_FULL);

	// TODO: put into a func? also, does this need to go earlier (if clock might already be displayed?)

#if CAIRO_HAS_FT_FONT
	if( *gCfg.fontFace && g_isa_file(gCfg.fontFace) )
	{
		static const cairo_user_data_key_t key = { 0 };

		DEBUGLOGP("using cfg fontFace of %s\n", gCfg.fontFace);

		FT_Init_FreeType(&ft_library);
		FT_New_Face     ( ft_library, gCfg.fontFace, 0, &ft_face);

		g_pTextFontFace = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
		cairo_font_face_set_user_data(g_pTextFontFace, &key, ft_face, (cairo_destroy_func_t)FT_Done_Face);

		DEBUGLOGP("font path is    \"%s\"\n", gCfg.fontFace);
		DEBUGLOGP("font name is    \"%s\"\n", gCfg.fontName);
		DEBUGLOGP("font ps name is \"%s\"\n", FT_Get_Postscript_Name(ft_face));
	}
#endif // CAIRO_HAS_FT_FONT
/*
	cairo_t*              pContext = gdk_cairo_create(gRun.pMainWindow->window);
	PangoFontDescription* pPFDesc  = pango_font_description_from_string(gCfg.fontName);
	g_pPFLayout                    = pango_cairo_create_layout(pContext);

	if( pContext && pPFDesc && g_pPFLayout )
	{
		DEBUGLOGP("after font processing calls: succeeded (font: %s)\n", gCfg.fontName);

		cairo_set_font_size(pContext, gCfg.fontSize);
		pango_cairo_context_set_font_options(pContext, g_pTextOptions);
		pango_layout_set_font_description(g_pPFLayout, pPFDesc);
//		pango_layout_set_text(g_pPFLayout, gRun.acTxt3, -1);
		pango_font_description_free(pPFDesc);
		cairo_destroy(pContext);
		pContext = NULL;
		pPFDesc  = NULL;
	}
	else
		DEBUGLOGP("after font processing calls: failed\n(context okay: %s, desc okay: %s, layout okay: %s)\n",
			pContext ? "yes" : "no", pPFDesc ? "yes" : "no", g_pPFLayout ? "yes" : "no");*/

	DEBUGLOGS("aft font processing calls");
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::end(bool init)
{
	DEBUGLOGB;

	if( !init )
	{
		for( size_t e = 0; e < CLOCK_ELEMENTS; e++ )
		{
			SvgLockBeg(e);

			if( g_pSvgHandle[e] )
			{
//				rsvg_handle_free(g_pSvgHandle[e]); // deprecated
				g_object_unref(g_pSvgHandle[e]);
				g_pSvgHandle[e] = NULL;
			}

			SvgLockEnd(e);

			cairo_region_destroy(g_pMaskRgn);
			g_pMaskRgn = NULL;
		}

//		rsvg_term(); // deprecated
	}

	hand_anim_end();

	if( g_pTextOptions )
		cairo_font_options_destroy(g_pTextOptions);
	g_pTextOptions  = NULL;

	if( g_pTextFontFace )
		cairo_font_face_destroy(g_pTextFontFace);
	g_pTextFontFace = NULL;

#if CAIRO_HAS_FT_FONT
	if( ft_face )
		FT_Done_Face(ft_face);

	if( ft_library )
		FT_Done_FreeType(ft_library);
#endif // CAIRO_HAS_FT_FONT
/*
	if( g_pPFLayout )
		g_object_unref(g_pPFLayout);
	g_pPFLayout = NULL;*/

	for( size_t st = 0; st < SURF_COUNT; st++ )
	{
		SurfLockBeg(st);

		if( g_surfs[st].pCairoPtrn )
			cairo_pattern_destroy(g_surfs[st].pCairoPtrn);
		g_surfs[st].pCairoPtrn = NULL;

		if( g_surfs[st].pCairoSurf )
			cairo_surface_destroy(g_surfs[st].pCairoSurf);
		g_surfs[st].pCairoSurf = NULL;

		if( g_temps[st].pCairoPtrn )
			cairo_pattern_destroy(g_temps[st].pCairoPtrn);
		g_temps[st].pCairoPtrn = NULL;

		if( g_temps[st].pCairoSurf )
			cairo_surface_destroy(g_temps[st].pCairoSurf);
		g_temps[st].pCairoSurf = NULL;

		SurfLockEnd(st);
	}

	gRun.optHorHand = true;
	gRun.optMinHand = true;
	gRun.optSecHand = true;
	gRun.useHorSurf = true;
	gRun.useMinSurf = true;
	gRun.useSecSurf = true;
/*	gRun.optHorHand = false;
	gRun.optMinHand = false;
	gRun.optSecHand = false;
	gRun.useHorSurf = false;
	gRun.useMinSurf = false;
	gRun.useSecSurf = false;*/

	g_surfaceW = 0;
	g_surfaceH = 0;

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::chg()
{
/*	PangoFontDescription* pPFDesc = pango_font_description_from_string(gCfg.fontName);
	pango_layout_set_font_description(g_pPFLayout, pPFDesc);
	pango_font_description_free(pPFDesc);
	pPFDesc = NULL;*/
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool draw::make_icon(const char* iconPath)
{
	DEBUGLOGB;

//#if CAIRO_HAS_SVG_SURFACE
#if CAIRO_HAS_PNG_FUNCTIONS
	DEBUGLOGS("creating theme ico image file for valid current theme");
	DEBUGLOGP("  %s\n", gCfg.themeFile);

	// makes a 160x160 svg with 128x128 internal svg with 534x534 embedded png image

//	const double     isz   =  48.0;
	const double     isz   = 128.0;
	const double     ppi   =  72.0;
	const double     ext   = isz*ppi; // icon width & height extent size in points per inch
//	cairo_surface_t* pSurf = cairo_svg_surface_create("/home/me/.cairo-clock/theme.svg", isz, isz);
//	cairo_surface_t* pSurf = cairo_svg_surface_create(NULL, isz, isz);
	cairo_surface_t* pSurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, isz, isz);

	if( pSurf )
	{
		DEBUGLOGS("creating new image surface tied to the theme ico image file");

		cairo_t* pContext = cairo_create(pSurf);

		if( pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS )
		{
			DEBUGLOGS("successfully created context on theme ico image file's surface");

			// TODO: fix following non-multi-thread usable ratio fiddling

/*			double wrt = g_wratio, hrt = g_hratio;

			g_wratio   = (double)isz/(double)g_svgClock.width;
			g_hratio   = (double)isz/(double)g_svgClock.height;*/

			render_cbase(pContext, isz, isz, false);
//			render_bkgnd(pContext, isz, isz, false, false);

//			if( !(gRun.optHorHand && !gRun.useHorSurf) )
				render_hhand(pContext, isz, isz, false, true, true);

//			if( !(gRun.optMinHand && !gRun.useMinSurf) )
				render_mhand(pContext, isz, isz, false, true, true);

			render_frgnd(pContext, isz, isz);

//			render(pContext, isz, isz, 1.0);

/*			g_wratio = wrt;
			g_hratio = hrt;*/

			cairo_destroy(pContext);
		}

		if( cairo_surface_write_to_png(pSurf, iconPath) == CAIRO_STATUS_SUCCESS )
		{
			DEBUGLOGP("successfully created the theme ico image file:\n*%s*\n", iconPath);
		}
		else
		{
			DEBUGLOGP("didn't create the theme ico image file:\n*%s*\n", iconPath);
		}

		cairo_surface_destroy(pSurf);
		pSurf = NULL;
	}

	DEBUGLOGE;

	return true;
#else
	DEBUGLOGE;
	return false;
#endif // CAIRO_HAS_PNG_FUNCTIONS
//#endif // CAIRO_HAS_SVG_SURFACE
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
GdkBitmap* draw::make_mask(int width, int height, bool shaped)
{
	DEBUGLOGB;

	bool       okay       =  false;
	GdkBitmap* pShapeMask = (GdkBitmap*)gdk_pixmap_new(NULL, width, height, 1);
	cairo_t*   pContext   =  pShapeMask ? gdk_cairo_create(pShapeMask) : NULL;

	if( pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS )
	{
		cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
		cairo_paint(pContext);

		if( shaped )
		{
			SvgLockBeg(CLOCK_MASK);
			bool maskd = g_pSvgHandle[CLOCK_MASK];

			if( maskd )
			{
				DEBUGLOGS("mask svg is available");

//				cairo_scale(pContext, g_wratio, g_hratio);
				cairo_scale(pContext, (double)width/(double)g_svgClock.width, (double)height/(double)g_svgClock.height);

				cairo_set_operator(pContext, CAIRO_OPERATOR_OVER);

				okay = rsvg_handle_render_cairo(g_pSvgHandle[CLOCK_MASK], pContext) == TRUE;
			}

			SvgLockEnd(CLOCK_MASK);

			if( maskd == false )
			{
				DEBUGLOGS("mask svg not available");

				if( g_pMaskRgn )
				{
					DEBUGLOGS("using a scaled, theme load time created, outline mask");

#if GTK_CHECK_VERSION(3,0,0)
					// TODO: will this work the same as below?
					cairo_scale(pContext, width, height);
					gdk_cairo_region(pContext, g_pMaskRgn);
#else
					cairo_rectangle_int_t cr;
					int                   nr = cairo_region_num_rectangles(g_pMaskRgn);

					DEBUGLOGP("  %d rects make up the outline mask region\n", nr);

					for( size_t r = 0; r < nr; r++ )
					{
						cairo_region_get_rectangle(g_pMaskRgn, r, &cr);
//						cairo_rectangle(pContext, (double)cr.x*g_wratio, (double)cr.y*g_hratio, (double)cr.width*g_wratio, (double)cr.height*g_hratio);
						cairo_rectangle(pContext, (double)cr.x     *(double)width /(double)g_svgClock.width,
												  (double)cr.y     *(double)height/(double)g_svgClock.height,
												  (double)cr.width *(double)width /(double)g_svgClock.width,
												  (double)cr.height*(double)height/(double)g_svgClock.height);
					}
#endif
					cairo_set_source_rgba(pContext, 1, 1, 1, 1);
					cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);
					cairo_fill(pContext);
				}
				else
				{
					DEBUGLOGS("falling back to a simple scaled circular mask");

					cairo_scale(pContext, width, height);
					cairo_set_source_rgba(pContext, 1, 1, 1, 1);
					cairo_set_operator(pContext, CAIRO_OPERATOR_OVER);
					cairo_arc (pContext, 0.5, 0.5, 0.5, 0, 2*G_PI);
					cairo_fill(pContext);
//					cairo_stroke(pContext);
				}

				okay = true;
			}

			if( !okay )
			{
				DEBUGLOGS("all types of mask creation failed");
				DEBUGLOGS("falling back to a \"everything's clickable\" rectangle");

				cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);
				cairo_set_source_rgba(pContext, 1, 1, 1, 1);
				cairo_paint(pContext);
			}
		}
		else
		{
			okay = true;
		}
	}
	else
	{
		if( pShapeMask )
		{
			DEBUGLOGS("no context created");
			g_object_unref((gpointer)pShapeMask);
			pShapeMask = NULL;
		}
		else
		{
			DEBUGLOGS("no mask pixmap created");
		}
	}

	if( pContext )
		cairo_destroy(pContext);

	DEBUGLOGE;
	DEBUGLOGP("  returned shapeMask is %s\n", okay ? "valid" : (pShapeMask ? "a full square" : "null"));

	return pShapeMask;
}
/*
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::render(GtkWidget* pWidget, bool forceDraw)
{
	static cairo_matrix_t g_mat;
	static bool           g_bboxZero;

	DEBUGLOGP("(%d) entry\n", 1);

//	bool fullDraw = gRun.seconds        >= 59 || gRun.seconds        <= 1 || forceDraw;
//	fullDraw     |= gRun.bboxSecHand[0] ==  0 && gRun.bboxSecHand[1] == 0 &&
//	                gRun.bboxSecHand[2] ==  0 && gRun.bboxSecHand[3] == 0;

	bool fullDraw = forceDraw || g_bboxZero || gRun.seconds >= 59 || gRun.seconds <= 1;

	if( fullDraw )
	{
		gtk_widget_queue_draw(pWidget);
	}
	else
	{
		const int    ndr =  1;                    // # of hand drawing rect segments
		const double a2r =  G_PI/180.0;           // angle to radians conversion factor
//		const double cww =  gCfg.clockW;          // clock window width
//		const double cwh =  gCfg.clockH;          // clock window height
		const double ang =  gRun.angleSecond;     // second hand rotation angle
//		const double fcw =  gRun.svgClock.width;  // svg hand file width (total)
//		const double fch =  gRun.svgClock.height; // svg hand file height (total)
		const double lft =  gRun.bboxSecHand[0];  // svg hand file object(s) left position
		const double top =  gRun.bboxSecHand[1];  // svg hand file object(s) top position
		const double rgt =  gRun.bboxSecHand[2];  // svg hand file object(s) right position
		const double bot =  gRun.bboxSecHand[3];  // svg hand file object(s) bottom position
		const double drw = (rgt-lft)/ndr;         // svg hand file object(s) drawing rect segment width

		int     dx,  dy,  dw,  dh;
		double ulx, uly, urx, ury, llx, lly, lrx, lry;

//		cairo_matrix_init_scale(&g_mat,  cww/fcw, cwh/fch);
//		cairo_matrix_translate (&g_mat,  fcw/2,   fch/2);
//		cairo_matrix_rotate    (&g_mat, (ang-90) *a2r);

		cairo_matrix_t       mat = g_mat;
		cairo_matrix_rotate(&mat, (ang-90)*a2r);

		for( int d = 0; d < ndr; d++ )
		{
			ulx = lft+drw*d; uly = top; urx = ulx+drw; ury = top;
			llx = ulx;       lly = bot; lrx = urx;     lry = bot;

			cairo_matrix_transform_point(&mat, &ulx, &uly);
			cairo_matrix_transform_point(&mat, &urx, &ury);
			cairo_matrix_transform_point(&mat, &llx, &lly);
			cairo_matrix_transform_point(&mat, &lrx, &lry);

			#define minbox(a,b) (a) < (b) ? (a) : (b)
			#define maxbox(a,b) (a) > (b) ? (a) : (b)

			dx = minbox(ulx, minbox(urx, minbox(llx, lrx)));
			dy = minbox(uly, minbox(ury, minbox(lly, lry)));
			dw = maxbox(ulx, maxbox(urx, maxbox(llx, lrx))) - dx;
			dh = maxbox(uly, maxbox(ury, maxbox(lly, lry))) - dy;

			if( ang > 180 && ang < 270 ) dw -= dx;
			if( ang > 270 && ang < 360 ) dh -= dy;

//			gtk_widget_queue_draw_area(pWidget, dx, dy, dw, dh);

			GdkRectangle ir = { dx, dy, dw, dh };
			gdk_window_invalidate_rect(pWidget->window, &ir, FALSE);

			render(pWidget, gRun.drawScaleX, gRun.drawScaleY, gRun.renderIt, gRun.appStart);

			GdkRegion* pRegion  = gdk_window_get_update_area(pWidget->window);
			if(        pRegion )  gdk_region_destroy(pRegion);
		}
	}

	DEBUGLOGP("(%d) exit\n", 1);
}
*/
// -----------------------------------------------------------------------------
void draw::render(GtkWidget* pWidget, double scaleX, double scaleY, bool renderIt, bool appStart)
{
	DEBUGLOGP("(%d) entry\n", 2);

	if( renderIt )
	{
		cairo_t* pContext = gdk_cairo_create(pWidget ? pWidget->window : gRun.pMainWindow->window);

		if( pContext )
		{
			render(pContext, scaleX, scaleY, true, appStart, gCfg.clockW, gCfg.clockH, appStart);
			cairo_destroy(pContext);
		}
	}

	DEBUGLOGP("(%d) entry\n", 2);
}

// -----------------------------------------------------------------------------
void draw::render(GtkWidget* pWidget, double scaleX, double scaleY, bool renderIt, bool appStart, int width, int height, bool animate)
{
	DEBUGLOGP("(%d) entry\n", 3);

	cairo_t* pContext = gdk_cairo_create(pWidget->window);

	if( pContext )
	{
		render(pContext, scaleX, scaleY, renderIt, appStart, width, height, animate);
		cairo_destroy(pContext);
	}

	DEBUGLOGP("(%d) exit\n", 3);
}

// -----------------------------------------------------------------------------
void draw::render(cairo_t* pContext, double scaleX, double scaleY, bool renderIt, bool appStart, int width, int height, bool animate)
{
	DEBUGLOGP("(%d) entry\n", 4);

	if( !renderIt )
	{
		DEBUGLOGP("(%d) exit(%d)\n", 4, 1);
		return;
	}

	static int ct = 0;
//	if( ct < 10 ) DEBUGLOGP("(2) rendering(%d)\n", ++ct);
	DEBUGLOGP("(2) rendering(%d)\n", ++ct);

	bool gcfg_aniskip = false;
	bool gcfg_hndonly = false;

	static int skipm = 2, skipc = skipm;
	if( gcfg_aniskip && --skipc ) return;
	skipc = skipm;

#undef _RENDERALL
//#ifdef _RENDERALL
	GTimeVal            tval;
	g_get_current_time(&tval);

	glong  tvsec  =  tval.tv_sec %   60;
	glong  tvmin  = (tval.tv_sec % 3600) / 60;
	double fHalfX =  g_svgClock.width *0.5;
	double fHalfY =  g_svgClock.height*0.5;
	double xlatX  =  animate ? width *(1.0-scaleX)*0.5 : 0;
	double xlatY  =  animate ? height*(1.0-scaleY)*0.5 : 0;
	double angAdj = (double)(tval.tv_usec % 1000000)*1.0e-6;
//	double angHor =  gRun.angleHour;
	double angMin = (double)(tvmin)*6.0;
	double angSec = (double)(tvsec)*6.0;
	bool   adjMin =  tvsec >= 59;

	angAdj        = gcfg_aniskip ? angAdj : hand_anim_get(angAdj);
	angAdj       *= 6.0;

//	gRun.hours       = tvhor;
	gRun.minutes     = tvmin;
	gRun.seconds     = tvsec;
//	gRun.angleHour   = angHor;
	gRun.angleMinute = angMin + (adjMin ? angAdj : 0);
	gRun.angleSecond = angSec +  angAdj;

	if( gRun.nodisplay || gcfg_hndonly )
	{
		cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
		cairo_paint(pContext);

		if( gRun.nodisplay )
		{
			DEBUGLOGP("(%d) exit(%d)\n", 4, 2);
			return;
		}
	}

	cairo_matrix_t icm;
	cairo_identity_matrix(pContext);
	cairo_scale(pContext, scaleX, scaleY);
	cairo_translate(pContext, xlatX, xlatY);
	cairo_get_matrix(pContext, &icm);

	if( gRun.textonly )
	{
		DEBUGLOGS("textonly rendering");
		render_surf(pContext, gCfg.faceDate ? SURF_BKGND : SURF_FRGND, CAIRO_OPERATOR_SOURCE, CAIRO_OPERATOR_CLEAR);

		DEBUGLOGP("(%d) exit(%d)\n", 4, 3);
		return;
	}

	if( !gcfg_hndonly )
		render_surf(pContext, SURF_BKGND, CAIRO_OPERATOR_SOURCE, CAIRO_OPERATOR_CLEAR);

	if( gCfg.showHours )
	{
/*		// 2nd timezone hour hand angle = circle-in-degs / hours-per-circle * hours-offset-from-local
		double grun_angleHour2 = gRun.angleHour+360.0/12.0*5.0;

		render_hand_hl(pContext, width, height, scaleX, scaleY, appStart, grun_angleHour2,  gRun.useHorSurf, gRun.optHorHand, SURF_HHSHD, CLOCK_HOUR_HAND,   CLOCK_HOUR_HAND_SHADOW,   g_hhoff, NULL,             0.25);
		render_hand_hl(pContext, width, height, scaleX, scaleY, appStart, grun_angleHour2,  gRun.useHorSurf, gRun.optHorHand, SURF_HHAND, CLOCK_HOUR_HAND,   CLOCK_HOUR_HAND_SHADOW,   g_hhoff, gRun.bboxHorHand, 0.25);
*/
		render_hand_hl(pContext, width, height, scaleX, scaleY, appStart, gRun.angleHour,   gRun.useHorSurf, gRun.optHorHand, SURF_HHSHD, CLOCK_HOUR_HAND,   CLOCK_HOUR_HAND_SHADOW,   g_hhoff);
		render_hand_hl(pContext, width, height, scaleX, scaleY, appStart, gRun.angleHour,   gRun.useHorSurf, gRun.optHorHand, SURF_HHAND, CLOCK_HOUR_HAND,   CLOCK_HOUR_HAND_SHADOW,   g_hhoff, gRun.bboxHorHand);
	}

	if( gCfg.showMinutes )
	{
		render_hand_hl(pContext, width, height, scaleX, scaleY, appStart, gRun.angleMinute, gRun.useMinSurf, gRun.optMinHand, SURF_MHSHD, CLOCK_MINUTE_HAND, CLOCK_MINUTE_HAND_SHADOW, g_mhoff);
		render_hand_hl(pContext, width, height, scaleX, scaleY, appStart, gRun.angleMinute, gRun.useMinSurf, gRun.optMinHand, SURF_MHAND, CLOCK_MINUTE_HAND, CLOCK_MINUTE_HAND_SHADOW, g_mhoff, gRun.bboxMinHand);
	}

	if( gCfg.showSeconds )
	{
		render_hand_hl(pContext, width, height, scaleX, scaleY, appStart, gRun.angleSecond, gRun.useSecSurf, gRun.optSecHand, SURF_SHSHD, CLOCK_SECOND_HAND, CLOCK_SECOND_HAND_SHADOW, g_shoff);
		render_hand_hl(pContext, width, height, scaleX, scaleY, appStart, gRun.angleSecond, gRun.useSecSurf, gRun.optSecHand, SURF_SHAND, CLOCK_SECOND_HAND, CLOCK_SECOND_HAND_SHADOW, g_shoff, gRun.bboxSecHand);
	}

	cairo_set_matrix(pContext, &icm);

	if( !gcfg_hndonly )
		render_surf(pContext, SURF_FRGND, CAIRO_OPERATOR_OVER);

/*	if( !appStart && gRun.evalDraws ) // TODO: turning this on now crashes the app
	{
		double xb, yb, xe, ye;

		cairo_new_path(pContext);
		cairo_set_line_width(pContext, 0.25);
//		cairo_clip_extents(pContext, &xb, &yb, &xe, &ye);
		cairo_set_source_rgba(pContext, 1, 0.2, 0.2, 0.5);
		cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);
//		cairo_rectangle(pContext, xb+1, yb+1, xe-xb-2, ye-yb-2);

		cairo_rectangle_list_t* pRects = cairo_copy_clip_rectangle_list(pContext);

		if( pRects )
		{
			if( pRects->status == CAIRO_STATUS_SUCCESS )
			{
				for( int r = 0; r < pRects->num_rectangles; r++ )
					cairo_rectangle(pContext, pRects->rectangles[r].x+1, pRects->rectangles[r].y+1, pRects->rectangles[r].width-2, pRects->rectangles[r].height-2);
			}

			cairo_rectangle_list_destroy(pRects);
		}

		cairo_stroke(pContext);
	}*/

#ifdef _DRAWINPMASK
	if( g_pShapeBitmap )
	{
		DEBUGLOGS("drawing shape bitmap");

		cairo_save(pContext);
		cairo_scale(pContext, 1.0, 1.0);
		cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);
		gdk_cairo_set_source_pixmap(pContext, g_pShapeBitmap, 0.0f, 0.0f);
//		gdk_drawable_set_colormap(g_pShapeBitmap, gdk_colormap_get_system());
//		cairo_set_source_rgb(pContext, 1.0, 1.0, 1.0);
		cairo_paint(pContext);
		cairo_restore(pContext);
	}
	else
	{
		DEBUGLOGS("not drawing shape bitmap since it's null");
	}
#endif

	DEBUGLOGP("(%d) exit(%d)\n", 4, 4);
}

// -----------------------------------------------------------------------------
bool draw::render_cbase(cairo_t* pContext, int width, int height, bool bright)
{
	DEBUGLOGB;

	bool okay = false;
//	bool comp = gdk_screen_is_composited(gdk_screen_get_default()) == TRUE;
	bool comp = gdk_screen_is_composited(gtk_widget_get_screen(gRun.pMainWindow)) == TRUE;
//	bool comp = gdk_display_supports_composite(gtk_widget_get_display(gRun.pMainWindow)) == TRUE;
//	bool comp = gdk_window_get_composited(gRun.pMainWindow->window) == TRUE;

	if( comp )
	{
		DEBUGLOGS("window is composited");
/*
		gint       wx,  wy, ww, wh;
		GdkWindow* rw = gdk_get_default_root_window();

		gdk_window_get_origin(rw, &wx, &wy);
		gdk_drawable_get_size(rw, &ww, &wh);

		DEBUGLOGP("def root win dims: (%d, %d) x (%d, %d)\n", wx, wy, ww, wh);
*/
		if( gRun.scrsaver )
		{
			cairo_set_source_rgba(pContext, 0, 0, 0, 1.0);
			cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);
		}
		else
		{
			cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
		}

		cairo_paint(pContext);
	}
	else
	{
		DEBUGLOGS("window is NOT composited");
/*
		gint       wx,  wy, ww, wh;
		GdkWindow* rw = gdk_get_default_root_window();

		gdk_window_get_origin(rw, &wx, &wy);
		gdk_drawable_get_size(rw, &ww, &wh);

		DEBUGLOGP("def root win dims: (%d, %d) x (%d, %d)\n", wx, wy, ww, wh);

		guint      nBytes  = 0;
//		GdkPixbuf* pBuffer = gdk_pixbuf_get_from_drawable(NULL, rw, NULL, wx, wy, 0, 0, ww, wh);
		GdkPixbuf* pBuffer = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 32, gCfg.clockW, gCfg.clockH);
//		pBuffer            = gdk_pixbuf_get_from_drawable(pBuffer, rw, NULL, gCfg.clockX, gCfg.clockY, 0, 0, gCfg.clockW, gCfg.clockH);
		pBuffer            = gdk_pixbuf_get_from_drawable(pBuffer, rw, NULL, wx, wy, 0, 0, gCfg.clockW, gCfg.clockH);
//		GdkPixbuf* pBuffer = gdk_pixbuf_get_from_window(rw, gCfg.clockX, gCfg.clockY, gCfg.clockW, gCfg.clockH);
		guchar*    pBBytes = gdk_pixbuf_get_pixels_with_length(pBuffer, &nBytes);

//		cairo_surface_t* pSurface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, ww, wh);
		cairo_surface_t* pSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, gCfg.clockW, gCfg.clockH);
		guchar*          pSBytes  = cairo_image_surface_get_data(pSurface);

//		memcpy(pSBytes, pBBytes, nBytes);

		for( size_t y = 0; y < gCfg.clockH; y++ )
		{
			for( size_t x = 0; x < gCfg.clockW; x++ )
			{
				pSBytes[0] = 0; // blue
				pSBytes[1] = 0; // green
				pSBytes[2] = 0; // red
				pSBytes[3] = 0; // alpha
				pSBytes[2] = pBBytes[0]; // red
//				pSBytes[2] = pBBytes[1];
//				pSBytes[3] = pBBytes[2];
				pBBytes   += 4;
				pSBytes   += 4;
			}
		}

		cairo_surface_mark_dirty(pSurface);
		cairo_set_source_surface(pContext, pSurface, 0.0, 0.0);
		cairo_set_operator      (pContext, CAIRO_OPERATOR_SOURCE);
		cairo_paint             (pContext);
		cairo_surface_destroy   (pSurface);

		g_object_unref(pBuffer);
		pBuffer = NULL;
*/
		SvgLockBeg(CLOCK_MASK);

		bool okay  = false;
		bool maskd = g_pSvgHandle[CLOCK_MASK];

		if( maskd )
		{
			cairo_save(pContext);
//			cairo_scale(pContext, g_wratio, g_hratio);
			cairo_scale(pContext, (double)width/(double)g_svgClock.width, (double)height/(double)g_svgClock.height);
			cairo_set_operator(pContext, bright ? CAIRO_OPERATOR_LIGHTEN : CAIRO_OPERATOR_OVER);

			g_bit_lock(&g_pSvgHandleLock, CLOCK_MASK);
			okay = rsvg_handle_render_cairo(g_pSvgHandle[CLOCK_MASK], pContext) == TRUE;

			cairo_restore(pContext);
		}

		SvgLockEnd(CLOCK_MASK);

		if( !okay )
		{
			// TODO: this really needs to change - but to what?
//			gdouble red =  87.0/255.0, green = 115.0/255.0, blue =  54.0/255.0;
			gdouble red = 128.0/255.0, green = 128.0/255.0, blue = 128.0/255.0;

			cairo_set_source_rgba(pContext, red, green, blue, 1.0);
			cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);
			cairo_paint(pContext);
		}
	}

//	cairo_set_operator(pContext, CAIRO_OPERATOR_OVER);

	cairo_save (pContext);
//	cairo_scale(pContext, g_wratio, g_hratio);
	cairo_scale(pContext, (double)width/(double)g_svgClock.width, (double)height/(double)g_svgClock.height);
//	cairo_set_source_rgba(pContext, 1.0, 1.0, 1.0, 0.0); // transparent white
	cairo_set_operator(pContext, bright ? CAIRO_OPERATOR_LIGHTEN : CAIRO_OPERATOR_OVER);
//	cairo_paint(pContext);

	// draw the lowest layer elements

	ClockElement cm           =   gCfg.show24Hrs ?   CLOCK_MARKS_24H : CLOCK_MARKS;
	ClockElement ClockElems[] = { CLOCK_DROP_SHADOW, CLOCK_FACE, cm };

	ClockElement le;

	for( size_t e = 0; e < vectsz(ClockElems); e++ )
	{
		le =  ClockElems[e];

		SvgLockBeg(le);

		if( g_pSvgHandle[le] )
		{
			if( rsvg_handle_render_cairo(g_pSvgHandle[le], pContext) )
			{
				DEBUGLOGP("successfully rendered clock element %d\n", (int)le);
				okay = true;
			}
		}

		SvgLockEnd(le);
	}

	cairo_restore(pContext);

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
bool draw::render_bkgnd(cairo_t* pContext, int width, int height, bool bright, bool date, bool temp)
{
	DEBUGLOGB;

	double wratio = temp ? 1 : (double)width /(double)gCfg.clockW;
	double hratio = temp ? 1 : (double)height/(double)gCfg.clockH;

	cairo_save (pContext);
//	cairo_scale(pContext, (double)width/(double)gCfg.clockW, (double)height/(double)gCfg.clockH);
	cairo_scale(pContext, wratio, hratio);

	SurfLockBeg(SURF_CBASE);

	cairo_surface_t* pCBaseSurf  = temp ?    g_temps[SURF_CBASE].pCairoSurf : g_surfs[SURF_CBASE].pCairoSurf;
	bool okay =      pCBaseSurf != NULL && !(gCfg.faceDate && date && gRun.textonly);

	cairo_set_operator(pContext, okay ? CAIRO_OPERATOR_SOURCE : CAIRO_OPERATOR_CLEAR);

	if( okay )
		cairo_set_source_surface(pContext, pCBaseSurf, 0.0, 0.0);

	SurfLockEnd(SURF_CBASE);

	cairo_paint(pContext);

//	cairo_scale(pContext, g_wratio, g_hratio);
	cairo_scale(pContext, (double)width/(double)g_svgClock.width, (double)height/(double)g_svgClock.height);
	cairo_set_operator(pContext, bright ? CAIRO_OPERATOR_LIGHTEN : CAIRO_OPERATOR_OVER);

	if( gCfg.faceDate && date )
	{
		render_date(pContext, width, height, bright);
		okay = true;
	}

	cairo_restore(pContext);

	if( (gRun.optHorHand && !gRun.useHorSurf) || (gRun.optMinHand && !gRun.useMinSurf) )
	{
		DEBUGLOGS("drawing hour & minute hands into the given context - begin");

/*		double sourceW  = g_svgClock.width;
		double sourceH  = g_svgClock.height;
		double horAngle = gRun.angleHour  *pi2Rad;
		double minAngle = gRun.angleMinute*pi2Rad;
		double fHalfX   = sourceW*0.5;
		double fHalfY   = sourceH*0.5;
*/
		DEBUGLOGP("angleHour/Min/Sec=%f, %f, %f\n", gRun.angleHour, gRun.angleMinute, gRun.angleSecond);

		cairo_save(pContext);
/*		cairo_scale(pContext, (double)width/sourceW, (double)height/sourceH);
		cairo_translate(pContext, fHalfX, fHalfY); // rendered hand's origin is at the clock's center
		cairo_rotate(pContext, -G_PI*0.5);         // rendered hand's zero angle is at 12 o'clock
*/
		if( gRun.optHorHand && !gRun.useHorSurf )
		{
//			render_hand(pContext, CLOCK_HOUR_HAND,   CLOCK_HOUR_HAND_SHADOW,   g_hhoff, -g_hhoff, horAngle);
			render_hhand(pContext, width, height, false, true, true);
			okay = true;
		}

		if( gRun.optMinHand && !gRun.useMinSurf )
		{
//			render_hand(pContext, CLOCK_MINUTE_HAND, CLOCK_MINUTE_HAND_SHADOW, g_mhoff, -g_mhoff, minAngle);
			render_mhand(pContext, width, height, false, true, true);
			okay = true;
		}

		cairo_restore(pContext);

		DEBUGLOGS("drawing hour & minute hands into the given context - end");
	}

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
bool draw::render_chand(cairo_t* pContext, int handElem, int shadElem, int width, int height, bool bright, double hoff, bool center, double rotate)
{
	DEBUGLOGB;

	cairo_save(pContext);
	cairo_set_source_rgba(pContext, 1.0, 1.0, 1.0, 0.0); // transparent white
	cairo_set_operator(pContext, bright ? CAIRO_OPERATOR_LIGHTEN : CAIRO_OPERATOR_OVER);
	cairo_paint(pContext);

	double hoffX  =  hoff;
	double hoffY  = -hoff;
	double fHalfX =  center ? width *0.5 : width *0.5/g_xDiv;
	double fHalfY =  center ? height*0.5 : height*0.5/g_yDiv;

	cairo_translate(pContext, fHalfX, fHalfY); // rendered hand's origin is at the clock's center
//	cairo_scale(pContext,   g_wratio, g_hratio);
	cairo_scale(pContext,  (double)width/(double)g_svgClock.width, (double)height/(double)g_svgClock.height);
	cairo_rotate(pContext, -G_PI*0.5);         // rendered hand's zero angle is at 12 o'clock

	if( rotate <  0.0 )     // indicates a clock hand shadow - TODO: change this to something more appropriate
	{
		hoffX  = -g_coffX; // zero sum the offsets since the offset has already been applied
		hoffY  = -g_coffY;
		rotate =  90.0;    // assumes rendering into a memory surface for later animation, so minimize needed area
	}

	bool okay  =  render_hand(pContext, handElem, shadElem, hoffX, hoffY, rotate*pi2Rad);

	cairo_restore(pContext);

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
static double draw_render_text(cairo_t* pContext, const char* text, double sizeFact, double hiteFact, double hiteAddO, double hiteSign)
{
	double extH = 0;

	if( text && *text )
	{
		cairo_text_extents_t  ext;
		cairo_set_font_size  (pContext,  gCfg.fontSize*sizeFact);
		cairo_text_extents   (pContext,  text, &ext);
		cairo_move_to        (pContext, -ext.width*0.50,  (hiteAddO+ext.height*hiteFact)*hiteSign*gCfg.fontOffY);
		cairo_text_path      (pContext,  text);
		cairo_set_source_rgba(pContext,  gCfg.dateTextRed, gCfg.dateTextGrn, gCfg.dateTextBlu, gCfg.dateTextAlf);
		cairo_fill_preserve  (pContext);
		cairo_set_source_rgba(pContext,  gCfg.dateShdoRed, gCfg.dateShdoGrn, gCfg.dateShdoBlu, gCfg.dateShdoAlf);
		cairo_set_line_width (pContext,  gCfg.dateShdoWid);
		cairo_stroke         (pContext);

//		pango_cairo_update_layout(pContext,  g_pPFLayout);
//		pango_layout_set_text  (g_pPFLayout, text, -1);
//		pango_cairo_show_layout  (pContext,  g_pPFLayout);

		extH = ext.height;
	}

	return extH;
}

// -----------------------------------------------------------------------------
void draw::render_date(cairo_t* pContext)
{
	DEBUGLOGB;

	cairo_save  (pContext);
	cairo_rotate(pContext, 90.0*pi2Rad);

	DEBUGLOGS("bef if tests");

	if( g_pTextOptions )
		cairo_set_font_options(pContext, g_pTextOptions);

	if( g_pTextFontFace )
		cairo_set_font_face   (pContext, g_pTextFontFace);

	DEBUGLOGS("aft if tests");
	DEBUGLOGP("bef text rendering: date string is *%s*\n", gRun.acDate);
	DEBUGLOGP("bef text rendering: date string is *%s* & *%s*\n", gRun.acTxt3, gRun.acTxt4);

	double ext1H = draw_render_text(pContext, gRun.acTxt1, 1.00, 0.75, 0.00,       -1.00);
	double ext2H = draw_render_text(pContext, gRun.acTxt2, 0.66, 1.00, ext1H*1.00, -1.00);
	double ext3H = draw_render_text(pContext, gRun.acTxt3, 1.00, 1.00, 0.00,       +1.00);
	double ext4H = draw_render_text(pContext, gRun.acTxt4, 0.66, 1.00, ext3H*1.25, +1.00);

	cairo_restore(pContext);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::render_date(cairo_t* pContext, int width, int height, bool bright)
{
	DEBUGLOGB;

//	if( gCfg.showDate && *gRun.acDate )
	if( gCfg.showDate && (*gRun.acTxt3 || *gRun.acTxt4) )
	{
		DEBUGLOGS("drawing date into the given context - begin");

		cairo_save(pContext);
		cairo_translate(pContext, g_svgClock.width*0.5, g_svgClock.height*0.5);
		cairo_rotate(pContext, -G_PI*0.5);
		render_date(pContext);
		cairo_restore(pContext);

		DEBUGLOGS("drawing date into the given context - end");
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool draw::render_frgnd(cairo_t* pContext, int width, int height, bool bright)
{
	DEBUGLOGB;

	bool okay = false;

//	cairo_scale(pContext, g_wratio, g_hratio);
	cairo_scale(pContext, (double)width/(double)g_svgClock.width, (double)height/(double)g_svgClock.height);
	cairo_set_source_rgba(pContext, 1.0, 1.0, 1.0, 0.0); // transparent white
	cairo_set_operator(pContext, bright ? CAIRO_OPERATOR_LIGHTEN : CAIRO_OPERATOR_OVER);
	cairo_paint(pContext);

	// draw the upper layer elements

	if( !gCfg.faceDate )
	{
		render_date(pContext, width, height, bright);
		okay = true;
	}

	static const ClockElement ClockElems[] = { CLOCK_FACE_SHADOW, CLOCK_GLASS, CLOCK_FRAME };

	ClockElement le;

	for( size_t e = 0; e < vectsz(ClockElems); e++ )
	{
		le = ClockElems[e];

		SvgLockBeg(le);

		if( g_pSvgHandle[le] )
		{
			if( rsvg_handle_render_cairo(g_pSvgHandle[le], pContext) )
			{
//				DEBUGLOGP("successfully rendered clock element %d\n", (int)le);
				okay = true;
			}
		}

		SvgLockEnd(le);
	}

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
bool draw::render_hand(cairo_t* pContext, int handElem, int shadElem, double hoffX, double hoffY, double angle)
{
	DEBUGLOGB;
	DEBUGLOGP("  for %d (%d)\n", handElem, shadElem);

	bool okay = false;

	cairo_save(pContext);

	if( shadElem != -1 && g_pSvgHandle[shadElem] )
	{
//		DEBUGLOGP("rendering clock hand shadow element %d\n", shadElem);

		cairo_save(pContext);
		cairo_translate(pContext, g_coffX+hoffX, g_coffY+hoffY);
		cairo_rotate(pContext, angle);
		cairo_set_operator(pContext, CAIRO_OPERATOR_OVER);

		SvgLockBeg(shadElem);

		if( rsvg_handle_render_cairo(g_pSvgHandle[shadElem], pContext) )
		{
//			DEBUGLOGP("successfully rendered clock element %d\n", shadElem);
			okay = true;
		}

		SvgLockEnd(shadElem);

		cairo_restore(pContext);
	}

	if( handElem != -1 && g_pSvgHandle[handElem] )
	{
//		DEBUGLOGP("rendering clock hand element %d\n", handElem);

		cairo_rotate(pContext, angle);
		cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);

		SvgLockBeg(handElem);

		if( rsvg_handle_render_cairo(g_pSvgHandle[handElem], pContext) )
		{
//			DEBUGLOGP("successfully rendered clock element %d\n", handElem);
			okay = true;
		}

		SvgLockEnd(handElem);
	}

	cairo_restore(pContext);

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
void draw::render_hand_hl(cairo_t* pContext, int width, int height, double scaleX, double scaleY, bool appStart, double rotate, bool useSurf, bool optHand, enum SurfaceType st, int handElem, int shadElem, double hoff, double* bbox, double alf)
{
	DEBUGLOGB;

	cairo_save(pContext);

	SurfLockBeg(st);

	double angRad =  rotate*pi2Rad;
	double fHalfX =  g_svgClock.width *0.5;
	double fHalfY =  g_svgClock.height*0.5;
	useSurf      |=  g_surfs[st].pCairoSurf != NULL;

	SurfLockEnd(st);

	if( useSurf )
	{
		double cx =  width *0.5;
		double cy =  height*0.5;
		double ox = -width /g_xDiv*0.5;
		double oy = -height/g_yDiv*0.5;

//		cairo_identity_matrix(pContext);
		cairo_translate(pContext,  cx, cy);
		cairo_rotate   (pContext, -G_PI*0.5); // rendered hand's zero angle is at 12 o'clock

		if( !bbox ) // indicates a shadow element - TODO: change to something proper
		{
			const double hoffX =  hoff;
			const double hoffY = -hoff;
//			const double sx    =  g_wratio;
//			const double sy    =  g_hratio;
			const double sx    = (double)width /(double)g_svgClock.width;
			const double sy    = (double)height/(double)g_svgClock.height;

			cairo_translate(pContext, sx*(g_coffX+hoffX), sy*(g_coffY+hoffY));
		}

		cairo_rotate   (pContext, angRad);
		cairo_translate(pContext, ox, oy);

		render_surf(pContext, st, CAIRO_OPERATOR_OVER, -1, alf);

/*		if( !appStart && gRun.evalDraws ) // TODO: turning this on now crashes the app
		{
			cairo_identity_matrix(pContext);
			cairo_scale(pContext, g_wratio, g_hratio);
//			cairo_init_scale(pContext, g_wratio, g_hratio);
			cairo_translate(pContext, fHalfX, fHalfY);  // rendered hand's origin is at the clock's center
			cairo_rotate(pContext, -G_PI*0.5+angRad); // rendered hand's zero angle is at 12 o'clock

			cairo_new_path(pContext);
			cairo_set_line_width(pContext, 0.25);
			cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);
			cairo_set_source_rgba(pContext, 0.2, 1, 0.2, 0.5);
			cairo_rectangle(pContext, bbox[0], bbox[1], bbox[2]-bbox[0]+1, bbox[3]-bbox[1]+1);
			cairo_stroke(pContext);
		}*/
	}
	else
	if( !optHand )
	{
//		cairo_identity_matrix(pContext);

//		cairo_scale(pContext, g_wratio, g_hratio);
		cairo_scale(pContext, (double)width/(double)g_svgClock.width, (double)height/(double)g_svgClock.height);
		cairo_translate(pContext, fHalfX, fHalfY); // rendered hand's origin is at the clock's center
		cairo_rotate(pContext, -G_PI*0.5);         // rendered hand's zero angle is at 12 o'clock
		render_hand(pContext, handElem, shadElem, hoff, -hoff, angRad);
	}

//	SurfLockEnd(st);

	cairo_restore(pContext);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool draw::render_hhand(cairo_t* pContext, int width, int height, bool bright, bool center, bool rotate, bool hand, bool shadow)
{
	DEBUGLOGB;

	int    handElem = hand   ? CLOCK_HOUR_HAND          : -1;
	int    shadElem = shadow ? CLOCK_HOUR_HAND_SHADOW   : -1;
	double angle    = rotate ? gRun.angleHour           : -1;

	bool   ret      = render_chand(pContext, handElem, shadElem, width, height, bright, 0, center, angle);

	return ret;
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool draw::render_mhand(cairo_t* pContext, int width, int height, bool bright, bool center, bool rotate, bool hand, bool shadow)
{
	DEBUGLOGB;

	int    handElem = hand   ? CLOCK_MINUTE_HAND        : -1;
	int    shadElem = shadow ? CLOCK_MINUTE_HAND_SHADOW : -1;
	double angle    = rotate ? gRun.angleMinute         : -1;

	bool   ret      = render_chand(pContext, handElem, shadElem, width, height, bright, 0, center, angle);

	return ret;
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool draw::render_shand(cairo_t* pContext, int width, int height, bool bright, bool center, bool rotate, bool hand, bool shadow)
{
	DEBUGLOGB;

	int    handElem = hand   ? CLOCK_SECOND_HAND        : -1;
	int    shadElem = shadow ? CLOCK_SECOND_HAND_SHADOW : -1;
	double angle    = rotate ? gRun.angleSecond         : -1;

	bool   ret      = render_chand(pContext, handElem, shadElem, width, height, bright, 0, center, angle);

	return ret;
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::render_surf(cairo_t* pContext, enum SurfaceType st, cairo_operator_t op1, int op2, double alf)
{
	DEBUGLOGB;
	DEBUGLOGP("  surface type is %d\n", (int)st);

	bool paint = true;

	SurfLockBeg(st);

	if( g_surfs[st].pCairoSurf )
	{
		DEBUGLOGS("surface available");
		cairo_set_operator(pContext, op1);

		if( g_surfs[st].pCairoPtrn )
		{
			DEBUGLOGS("pattern available");
			cairo_set_source(pContext, g_surfs[st].pCairoPtrn);
		}
		else
		{
			DEBUGLOGS("no pattern available");
			cairo_set_source_surface(pContext, g_surfs[st].pCairoSurf, 0.0, 0.0);
		}
	}
	else
	{
		DEBUGLOGS("no surface available");

		if( paint = op2 != -1 )
			cairo_set_operator(pContext, (cairo_operator_t)op2);
	}

	SurfLockEnd(st);

	if( paint )
	{
		DEBUGLOGS("bef paint");
/*		if( alf > 0.9999 && alf < 1.0001 )
		{*/
			cairo_paint(pContext);
/*		}
		else
		{
			cairo_pattern_t*      cp = cairo_pattern_create_rgba(alf, alf, alf, alf);
			cairo_mask(pContext,  cp);
			cairo_pattern_destroy(cp);
		}*/
/*		cairo_paint_with_alpha(pContext, alf);*/
		DEBUGLOGS("aft paint");
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::reset_ani()
{
	static double timeX[] =
	{
		0.000000000, 0.033333333, 0.066666667, 0.100000000, 0.133333333,
		0.166666667, 0.200000000, 0.233333333, 0.266666667, 0.300000000,
		0.333333333, 0.366666667, 0.400000000, 0.433333333, 0.466666667,
		0.500000000, 0.533333333, 0.566666667, 0.600000000, 0.633333333,
		0.666666667, 0.700000000, 0.733333333, 0.766666667, 0.800000000,
		0.833333333, 0.866666667, 0.900000000, 0.933333333, 0.966666667, 1.0
	};
	static double bounceY[] =
	{
		0.000,       0.000,       0.000,       0.000,       0.000,
		0.010,       0.010,       0.020,       0.030,       0.050,
		0.080,       0.110,       0.150,       0.200,       0.260,
		0.320,       0.400,       0.480,       0.570,       0.660,
		0.760,       0.850,       0.940,       1.020,       1.090,
		1.150,       1.180,       1.190,       1.160,       1.100,       1.0
	};
	static double flickY[] =
	{
		0.000,       0.000,       0.000,       0.000,       0.000,
		0.000,       0.000,       0.000,       0.000,       0.000,
		0.000,       0.000,       0.000,       0.000,       0.000,
		0.000,       0.000,       0.000,       0.000,       0.000,
		0.000,       0.005,       0.042,       0.096,       0.200,
		0.450,       1.000,       1.200,       1.180,       1.100,       1.0
	};
	static double smoothY[] =
	{
		0.000000000, 0.033333333, 0.066666667, 0.100000000, 0.133333333,
		0.166666667, 0.200000000, 0.233333333, 0.266666667, 0.300000000,
		0.333333333, 0.366666667, 0.400000000, 0.433333333, 0.466666667,
		0.500000000, 0.533333333, 0.566666667, 0.600000000, 0.633333333,
		0.666666667, 0.700000000, 0.733333333, 0.766666667, 0.800000000,
		0.833333333, 0.866666667, 0.900000000, 0.933333333, 0.966666667, 1.0
	};

/*	static double timeX[] = { 0.0, 0.5, 1.0 }; // min of 3 pts for gsl::interp funcs use
	static double rotaY[] = { 0.0, 0.5, 1.0 };*/

	static const int ntimes = sizeof(timeX)/sizeof(timeX[0]);

	DEBUGLOGB;

	hand_anim_beg();

	double* rotaY = NULL;

	switch( gCfg.shandType )
	{
	default:
	case ANIM_FLICK: rotaY = flickY;  break; // curve from watching an analog clock
	case ANIM_SWEEP: rotaY = smoothY; break; // continuous (smooth)
	case ANIM_ORIG:  rotaY = bounceY; break; // MacSlow's cairo-clock animation curve
	}

	hand_anim_set(timeX, rotaY, ntimes);

	update_ani();

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::update_ani()
{
	DEBUGLOGB;

	const double* timeX;
	const double* angrY;
	int           ntimes = hand_anim_get(&timeX, &angrY);

	refreshCount = 0;

/*	int  xindx[vectsz(refreshTime)];
	int  cntpv   = 0;
	bool off0    = false;

	for( int i = 0; i < ntimes; i++ )
	{
		if( fabs(angrY[i]) > 0.01 )
		{
			if( !off0 )
				cntpv++;

			xindx[refreshCount++] = i;
			off0 = true;
		}
		else
		if( off0 )
		{
			off0 = false;
		}
	}*/

/*	int      b = 0;
	for( int i = 0; i < ntimes; i++ )
//	for( int i = 0; i < refreshCount; i++ )
	{
		if( fabs(angrY[i]) > 0.01 )
//		if( fabs(refreshTime[i]) > 0.01 )
		{
			b  = i - (i == 0 ? 0 : 1);
			break;
		}
	}*/

	int      b = 0;
	for( int i = 0; i < ntimes; i++ )
	{
		if( fabs(angrY[i]) > 0.01 )
		{
			b  = i - (i == 0 ? 0 : 1);

			while( b > 0 )
			{
				bool   more = false;
				double tinc = timeX[b] - timeX[b-1];
				double tdel = tinc/100.0;
				double time = timeX[b] + tdel;

				for( int j = 1; j < 100; j++, time+=tdel )
				{
					if( fabs(hand_anim_get(time)) > 0.01 )
					{
						more = true;
						b--;
						break;
					}
				}

				if( !more )
					break;
			}

			break;
		}
	}

	double tf =  0;
//	int    nt =  vectsz(refreshTime);
	int    nt =  gCfg.refreshRate + 2;
//	double td = (1.0 - timeX[b])/double(nt-1);
	double td = (1.0 - timeX[b])/double(nt-2);

	refreshTime[0] = 0;
	refreshFrac[0] = 0;
	refreshCumm[0] = 0;

//	DEBUGLOGP("nt=%d, b=%d, td=%f\n", nt, b, td);
//	DEBUGLOGP("%2.2d: %4.4f, %4.4f, %4.4f\n", 0, refreshTime[0], refreshFrac[0], refreshCumm[0]);

	for( int i = 1; i < nt-1; i++ )
//	for( int i = 1; i < nt; i++ )
	{
		refreshTime[i] = timeX[b] + td*(i-1);
		refreshFrac[i] = hand_anim_get(refreshTime[i]);
		tf            += refreshFrac[i];
		refreshCumm[i] = tf;
//		DEBUGLOGP("%2.2d: %4.4f, %4.4f, %4.4f\n", i, refreshTime[i], refreshFrac[i], refreshCumm[i]);
	}

	refreshTime[nt-1] = 1;
	refreshFrac[nt-1] = 1;
	tf               += refreshFrac[nt-1];
	refreshCumm[nt-1] = tf;

//	DEBUGLOGP("%2.2d: %4.4f, %4.4f, %4.4f\n", nt-1, refreshTime[nt-1], refreshFrac[nt-1], refreshCumm[nt-1]);
//	DEBUGLOGS("");

	for( int i = 0; i < nt; i++ )
	{
//		refreshFrac[i] /= tf;
		refreshCumm[i] /= tf;
//		DEBUGLOGP("%2.2d: %4.4f, %4.4f, %4.4f\n", i, refreshTime[i], refreshFrac[i], refreshCumm[i]);
	}

	// TODO: form a new set of data & pass into whatever interp funcs hand_anim
	//       uses and then sample from that to get a more exact (correct?)
	//       distribution time values that are highly concentrated around the
	//       peaks and valleys of the original animation curve

	// TODO: then use these in a new rendering time stepping loop (using
	//       nanosleep or something similiar) instead of using a rendering timer
	//       IOW, vary the time at which the rendering frames occur so that a
	//       more accurate representation of the animation curve is realized

	refreshCount = nt;

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::update_bkgnd()
{
	DEBUGLOGB;

	if( (gRun.optHorHand && !gRun.useHorSurf) || (gRun.optMinHand && !gRun.useMinSurf) )
	{
		DEBUGLOGS("updating background drawing surface");

		SurfLockBeg(SURF_BKGND);

		cairo_t* pContext = g_surfs[SURF_BKGND].pCairoSurf ? cairo_create(g_surfs[SURF_BKGND].pCairoSurf) : NULL;

		if( pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS )
		{
			render_bkgnd(pContext, gCfg.clockW, gCfg.clockH);
			cairo_destroy(pContext);
			pContext = NULL;
		}

		SurfLockEnd(SURF_BKGND);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::update_date_surf()
{
	DEBUGLOGB;

	SurfaceType st = gCfg.faceDate ? SURF_BKGND : SURF_FRGND;

	SurfLockBeg(st);

	cairo_surface_t* pSurface = g_surfs[st].pCairoSurf;
	cairo_t*         pContext = pSurface ? cairo_create(pSurface) : NULL;

	if( pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS )
	{
		if( gCfg.faceDate )
		{
			render_bkgnd(pContext, gCfg.clockW, gCfg.clockH);
		}
		else
		{
			cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
			cairo_paint(pContext);

			render_frgnd(pContext, gCfg.clockW, gCfg.clockH);
		}

		cairo_destroy(pContext);
		pContext = NULL;
	}

	SurfLockEnd(st);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool draw::update_surface(cairo_t* pSourceContext, int width, int height, SurfaceType type)
{
	DEBUGLOGB;

	g_temps[type].pCairoSurf =  NULL;
	g_temps[type].width      =  width;
	g_temps[type].height     =  height;
	g_temps[type].svgRatW    = (double)width/ (double)g_svgClock.width;
	g_temps[type].svgRatH    = (double)height/(double)g_svgClock.height;

	bool hand = type == SURF_HHAND || type == SURF_MHAND || type == SURF_SHAND  ||
	            type == SURF_HHSHD || type == SURF_MHSHD || type == SURF_SHSHD;

	DEBUGLOGS("destroying old and creating a new surface");
	DEBUGLOGP("creating a new surface of type %d\n", type);

//	int handW = width;
//	if( hand )  width  /= (int)g_xDiv;

	int handH = height;
	if( hand )  height /= (int)g_yDiv;

	g_temps[type].pCairoSurf = cairo_surface_create_similar(cairo_get_target(pSourceContext), CAIRO_CONTENT_COLOR_ALPHA, width, height);

//	if( hand )  width   =  handW;
	if( hand )  height  =  handH;

	if( !g_temps[type].pCairoSurf || cairo_surface_status(g_temps[type].pCairoSurf) != CAIRO_STATUS_SUCCESS )
	{
		g_temps[type].pCairoSurf = NULL;

		DEBUGLOGE;
		return false;
	}

//	DEBUGLOGP("created new surface of type %d\n", type);

	cairo_t* pContext = cairo_create(g_temps[type].pCairoSurf);

	if( !pContext || cairo_status(pContext) != CAIRO_STATUS_SUCCESS )
	{
//		DEBUGLOGP("failed to create a context using the new surface of type %d\n", type);
//		DEBUGLOGP("new surface of type %d was valid - so what went wrong?\n", type);

		cairo_surface_destroy(g_temps[type].pCairoSurf);
		g_temps[type].pCairoSurf = NULL;

		DEBUGLOGE;
		return false;
	}

//	DEBUGLOGP("clearing the context using the new surface of type %d\n", type);

	cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
	cairo_paint(pContext);

	bool    okay = false;
	switch( type )
	{
	case SURF_CBASE: okay = render_cbase(pContext, width, height); break;
	case SURF_BKGND: okay = render_bkgnd(pContext, width, height,  false, true,  true); break;
	case SURF_FRGND: okay = render_frgnd(pContext, width, height); break;
	case SURF_HHAND: okay = render_hhand(pContext, width, height,  false, false, false, true,  false); break;
	case SURF_MHAND: okay = render_mhand(pContext, width, height,  false, false, false, true,  false); break;
	case SURF_SHAND: okay = render_shand(pContext, width, height,  false, false, false, true,  false); break;
	case SURF_HHSHD: okay = render_hhand(pContext, width, height,  false, false, false, false, true);  break;
	case SURF_MHSHD: okay = render_mhand(pContext, width, height,  false, false, false, false, true);  break;
	case SURF_SHSHD: okay = render_shand(pContext, width, height,  false, false, false, false, true);  break;
	}

	if( !okay )
	{
//		DEBUGLOGP("bad render to new surface of type %d\n", type);
	}
	else
	{
//		DEBUGLOGP("good render to new surface of type %d\n", type);
	}

	cairo_destroy(pContext);
	pContext = NULL;

	DEBUGLOGE;
	return true;
}

// -----------------------------------------------------------------------------
bool draw::update_surf(cairo_t* pContext, SurfaceType st, int width, int height)
{
	DEBUGLOGB;

//	TODO: need 'asyncing' bool passed in (and pass it to update_surface as well?)
//        use this to determine whether to do any surface use locking

	g_thread_yield();

	bool okay = update_surface(pContext, width, height, st);

	DEBUGLOGE;

//	return g_temps[st].pCairoSurf != NULL;
//	return okay;
	return true;
}

// -----------------------------------------------------------------------------
void draw::update_surfs(GtkWidget* pWidget, int width, int height)
{
	DEBUGLOGB;
	DEBUGLOGS("  updating all drawing surfaces & resetting flag");

	cairo_t* pContext = gdk_cairo_create(pWidget->window);
	if( !pContext || cairo_status(pContext) != CAIRO_STATUS_SUCCESS )
		return;

//	TODO: these need to be local vars and passed around until all surfs are
//        rebuilt and swapped new for old below

/*	g_wratio = (double)width/ (double)g_svgClock.width;
	g_hratio = (double)height/(double)g_svgClock.height;*/
	double g_wratio = (double)width/ (double)g_svgClock.width;
	double g_hratio = (double)height/(double)g_svgClock.height;

	DEBUGLOGP("surfaces=%dx%d, svgs=%dx%d, ratios=%4.4f & %4.4f\n",
		width, height, g_svgClock.width, g_svgClock.height, (float)g_wratio, (float)g_hratio);

	bool cokay = update_surf(pContext, SURF_CBASE, width, height);
	bool bokay = update_surf(pContext, SURF_BKGND, width, height);
	bool fokay = update_surf(pContext, SURF_FRGND, width, height);

	if( gRun.useHorSurf )
	{
		update_surf(pContext, SURF_HHAND, width, height);
		update_surf(pContext, SURF_HHSHD, width, height);
	}

	if( gRun.useMinSurf )
	{
		update_surf(pContext, SURF_MHAND, width, height);
		update_surf(pContext, SURF_MHSHD, width, height);
	}

	if( gRun.useSecSurf )
	{
		update_surf(pContext, SURF_SHAND, width, height);
		update_surf(pContext, SURF_SHSHD, width, height);
	}

	if( bokay && fokay )
	{
		DEBUGLOGP("setting surface dims to (%d,%d)\n", width, height);

		g_surfaceW = width;
		g_surfaceH = height;
	}

	cairo_destroy(pContext);
	pContext = NULL;

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::update_surfs_swap(int width, int height)
{
	for( size_t st = 0; st < SURF_COUNT; st++ )
	{
		g_temps[st].pCairoPtrn = g_temps[st].pCairoSurf ? cairo_pattern_create_for_surface(g_temps[st].pCairoSurf) : NULL;
	}

	for( size_t st = 0; st < SURF_COUNT; st++ )
	{
		SurfLockBeg(st);

		if( g_surfs[st].pCairoPtrn )
			cairo_pattern_destroy(g_surfs[st].pCairoPtrn);

		if( g_surfs[st].pCairoSurf && (g_temps[st].pCairoSurf != g_surfs[st].pCairoSurf) )
			cairo_surface_destroy(g_surfs[st].pCairoSurf);

		g_surfs[st]            =  g_temps[st];

		g_temps[st].pCairoSurf =  NULL;
		g_temps[st].pCairoPtrn =  NULL;
		g_temps[st].width      =  0;
		g_temps[st].height     =  0;
		g_temps[st].svgRatW    =  0;
		g_temps[st].svgRatH    =  0;

		SurfLockEnd(st);
	}

	gRun.updateSurfs = false;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include "loadTheme.h" // for svg clock element bounding box retrieval
#include "svgData.h"

// -----------------------------------------------------------------------------
bool draw::update_theme(const char* path, const char* name, UpdateThemeDone callBack, gpointer cbData)
{
	DEBUGLOGB;

	if( callBack )
	{
		static UpdateTheme ut;

		DEBUGLOGS("attempting to async update theme");
		DEBUGLOGP("path is %s\n", path);
		DEBUGLOGP("name is %s\n", name);

		ut.callBack = callBack;
		ut.data     = cbData;
		ut.okay     = false;

		strvcpy(ut.path, path);
		strvcpy(ut.name, name);

		GThread* pThread = g_thread_try_new(__func__, update_theme_func, &ut, NULL);

		if( pThread )
		{
			DEBUGLOGS("update theme thread created");
			g_thread_unref(pThread);
			return true;
		}
	}

	DEBUGLOGS("attempting to sync update theme");

	bool ret = update_theme_internal(path, name);

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
gpointer draw::update_theme_func(gpointer data)
{
	DEBUGLOGB;

	g_make_nicer(10);

	UpdateTheme* pUT = (UpdateTheme*)data;

	DEBUGLOGS("before internal update_theme call");
	DEBUGLOGP("  path is %s\n", pUT->path);
	DEBUGLOGP("  name is %s\n", pUT->name);

	gdk_threads_enter();
	pUT->okay = update_theme_internal(pUT->path, pUT->name);
	gdk_threads_leave();

	DEBUGLOGP("after internal update_theme call (%s)\n", pUT->okay ? "succeeded" : "failed");
	DEBUGLOGS("before callBack call");

	pUT->callBack(pUT->data, pUT->path, pUT->name, pUT->okay);

	DEBUGLOGS("after callBack call");

	DEBUGLOGE;
	return 0;
}

// -----------------------------------------------------------------------------
bool draw::update_theme_internal(const char* path, const char* name)
{
	struct svgEle
	{
		const char* name;
		int         type;
	};

	static svgEle svgs[] =
	{
		{ "drop-shadow",        CLOCK_DROP_SHADOW },
		{ "face",               CLOCK_FACE },
		{ "marks",              CLOCK_MARKS },
		{ "marks-24h",          CLOCK_MARKS_24H },
		{ "hour-hand-shadow",   CLOCK_HOUR_HAND_SHADOW },
		{ "minute-hand-shadow", CLOCK_MINUTE_HAND_SHADOW },
		{ "second-hand-shadow", CLOCK_SECOND_HAND_SHADOW },
		{ "hour-hand",          CLOCK_HOUR_HAND },
		{ "minute-hand",        CLOCK_MINUTE_HAND },
		{ "second-hand",        CLOCK_SECOND_HAND },
		{ "face-shadow",        CLOCK_FACE_SHADOW },
		{ "glass",              CLOCK_GLASS },
		{ "frame",              CLOCK_FRAME },
		{ "mask",               CLOCK_MASK },
	};

	DEBUGLOGB;
	DEBUGLOGP("  name=%s\n", name);

	if( !path || !name )
	{
		DEBUGLOGE;
		DEBUGLOGS("  (1)");
		return false;
	}

	RsvgDimensionData cdim, mdim;
	gchar             fpath[PATH_MAX];

	bool intern = strcmp(path, INTERNAL_THEME) == 0;
	bool valid  = intern;
	bool maskd  = false;

	mdim.width = mdim.height = intern ? 100 : 0;

	DEBUGLOGP("loading %s theme's elements\n", name);

	for( size_t s = 0; s < vectsz(svgs); s++ )
	{
		g_thread_yield();

		int         e     = svgs[s].type;
		const char* ename = svgs[s].name;

		SvgLockBeg(e);

		if( g_pSvgHandle[e] )
		{
//			rsvg_handle_free(g_pSvgHandle[e]); // deprecated
			g_object_unref(g_pSvgHandle[e]);
			g_pSvgHandle[e] = NULL;
		}

		SvgLockEnd(e);

		if( intern )
		{
			for( size_t t = 0; t < CLOCK_ELEMENTS; t++ )
			{
				if( g_svgDefDTyp[t] == -1 )
					break;

				if( g_svgDefDTyp[t] ==  e )
				{
					guint8* pRSvgData = (guint8*)g_svgDefData[t];
					gsize   pRSvgDLen = (gsize)  g_svgDefDLen[t];

					SvgLockBeg(e);
					g_pSvgHandle[e]   =  rsvg_handle_new_from_data(pRSvgData, pRSvgDLen, NULL);
					SvgLockEnd(e);
					break;
				}
			}
		}
		else
		{
			const char pfmt[] = "%s/%s/clock-%s.svg%c";

			snprintf(fpath, vectsz(fpath), pfmt, path, name, ename, 'z');

			if( !g_isa_file(fpath) )
				snprintf(fpath, vectsz(fpath), pfmt, path, name, ename, '\0');

			SvgLockBeg(e);
			g_pSvgHandle[e] = rsvg_handle_new_from_file(fpath, NULL);
			SvgLockEnd(e);
		}

		SvgLockBeg(e);

		if( (ClockElement)e == CLOCK_MASK && g_pSvgHandle[e] )
			maskd = true;

		SvgLockEnd(e);
	}

	if( g_pMaskRgn )
		cairo_region_destroy(g_pMaskRgn);

	g_pMaskRgn = maskd ? NULL : cairo_region_create();

	for( size_t s = 0; s < vectsz(svgs); s++ )
	{
		g_thread_yield();

		int         e     = svgs[s].type;
		const char* ename = svgs[s].name;

		SvgLockBeg (e);
		bool        goodh = g_pSvgHandle[e] != NULL;
		SvgLockEnd (e);

		if( goodh )
		{
			float iw, ih, bb[4];
			bool  blank = false;

			DEBUGLOGP("svg %s was loaded %sternal\n", ename, intern ? "in" : "ex");

//			if( hand )
			if( loadTheme(fpath, iw, ih, bb) )
			{
				DEBUGLOGP("svg element %s was loaded\n", ename);
				DEBUGLOGP("cw=%f, ch=%f, xmin=%f, ymin=%f, xmax=%f, ymax=%f, xext=%f, yext=%f\n",
					   iw, ih, bb[0], bb[1], bb[2], bb[3], bb[2]-bb[0], bb[3]-bb[1]);

//				blank = iw == 0 || ih == 0 ||  bb[0] == bb[2] || bb[1] == bb[3];
				blank = iw == 0 || ih == 0 || (bb[0] == bb[2] && bb[1] == bb[3]);
			}
			else
			{
				SvgLockBeg(e);

				cdim.width = cdim.height = 0;
				rsvg_handle_get_dimensions(g_pSvgHandle[e], &cdim);
				blank      = cdim.width == 0 || cdim.height == 0;

				SvgLockEnd(e);

				if( mdim.width  < cdim.width )
					mdim.width  = cdim.width;

				if( mdim.height < cdim.height )
					mdim.height = cdim.height;
			}

			blank = true; // assume they're all blank for now

			if( blank )
			{
				// fallback to using the pixel buffer since nanosvg seems to miss
				// getting some types of objects (like pngs and other embeddeds)

				SvgLockBeg(e);
				GdkPixbuf* pBuffer = rsvg_handle_get_pixbuf(g_pSvgHandle[e]);
				SvgLockEnd(e);

				if( pBuffer )
				{
					DEBUGLOGS("got GdkPixbuf");

					guint   nBytes = 0;
					guchar* pBytes = gdk_pixbuf_get_pixels_with_length(pBuffer, &nBytes);

					if( pBytes && nBytes )
					{
						DEBUGLOGS("got pixels buffer data");

						for( guint b = 0; b < nBytes; b++ )
						{
							if( pBytes[b] != 0 )
							{
								blank = false;
								bb[0] = bb[1] = bb[2] = bb[3] = 0; // indicate a full window redraw
								break;
							}
						}

						ClockElement ce = (ClockElement)e;
						bool         m1 = !maskd && !blank && gdk_pixbuf_get_n_channels(pBuffer) == 4 && gdk_pixbuf_get_bits_per_sample(pBuffer) == 8;
						bool         m2 =  ce == CLOCK_DROP_SHADOW  || ce == CLOCK_FACE  ||
										   ce == CLOCK_FACE_SHADOW  || ce == CLOCK_GLASS ||
										   ce == CLOCK_FRAME;
						bool         m3 = !m2 && (ce == CLOCK_MARKS || ce == CLOCK_MARKS_24H);

						if( m1 && (m2 || m3) )
						{
							// rescan to get outline (min/max of each scanline)
							// and 'add' to the current mask region

							int    xlen =  gdk_pixbuf_get_width    (pBuffer);
							int    ylen =  gdk_pixbuf_get_height   (pBuffer);
							int    rlen =  gdk_pixbuf_get_rowstride(pBuffer);
							guint* pixs = (guint*)pBytes;

							DEBUGLOGP("  outline svg %s parms (%d, %d, %d)\n", ename, xlen, ylen, rlen);

//							int xbb = xlen, xbe = 0, ybb = ylen, ybe = 0, nrs = 0;

							for( int y = 0; y < ylen; y++ )
							{
								int xbeg = -1, xend = -1;

								for( int x = 0; x < xlen; x++ ) // find first non-zero pixel
								{
									// TODO: need better test since can include a lot of 'shady' pixels
//									if( pixs[x] ) // non-zero pixel?
									if( ((guchar*)(&pixs[x]))[0] >= 128 )
									{
										xbeg = x;
										break;
									}
								}

								if( xbeg != -1 )
								{
									xend  =  xbeg + 1; // at least one non-zero pixel found

									for( int x = xlen-1; x >= 0; x-- ) // find last non-zero pixel
									{
										// TODO: need better test since can include a lot of 'shady' pixels
//										if( pixs[x] ) // non-zero pixel?
										if( ((guchar*)(&pixs[x]))[0] >= 128 )
										{
											xend = x + 1;
											break;
										}
									}
								}

								if( xbeg != -1 && xend != -1 )
								{
									cairo_rectangle_int_t r = { xbeg, y, xend-xbeg, 1 };
									cairo_region_union_rectangle(g_pMaskRgn, &r);
//									nrs++;

//									xbb = xbeg < xbb ? xbeg : xbb;
//									xbe = xend > xbe ? xend : xbe;
//									ybb = y    < ybb ? y    : ybb;
//									ybe = y    > ybe ? y    : ybe;
								}

								pixs = (guint*)((guchar*)pixs + rlen); // to next scanline
							}

//							DEBUGLOGP("    found %d rects within box of (%d, %d)->(%d, %d)\n",
//								nrs, xbb, ybb, xbe, ybe);
						}
					}

//					gdk_pixbuf_unref(pBuffer);
					g_object_unref(pBuffer);
					pBuffer = NULL;
				}
			}

			if( blank )
			{
				DEBUGLOGP("svg element %s was loaded\n", ename);
				DEBUGLOGP("cw=%f, ch=%f, xmin=%f, ymin=%f, xmax=%f, ymax=%f, xext=%f, yext=%f\n",
					iw, ih, bb[0], bb[1], bb[2], bb[3], bb[2]-bb[0], bb[3]-bb[1]);
				DEBUGLOGP("discarding %s's %s element (zero area or no objects)\n", name, ename);

				SvgLockBeg(e);

//				rsvg_handle_free(g_pSvgHandle[e]); // deprecated
				g_object_unref(g_pSvgHandle[e]);
				g_pSvgHandle[e] = NULL;

				SvgLockEnd(e);
			}
			else
			{
				valid = true;

				if( mdim.width  < iw )
					mdim.width  = iw;

				if( mdim.height < ih )
					mdim.height = ih;

				DEBUGLOGP("svg element %s was loaded\n", ename);

/*				if( e == CLOCK_HOUR_HAND || e == CLOCK_MINUTE_HAND || e == CLOCK_SECOND_HAND )
				{
					for( size_t b = 0; b < vectsz(bb); b++ )
					{
						switch( e )
						{
						case CLOCK_HOUR_HAND:   gRun.bboxHorHand[b] = bb[b]; break;
						case CLOCK_MINUTE_HAND: gRun.bboxMinHand[b] = bb[b]; break;
						case CLOCK_SECOND_HAND: gRun.bboxSecHand[b] = bb[b]; break;
						}
					}

					DEBUGLOGP("svg element %s was loaded\n", ename);
					DEBUGLOGP("cw=%f, ch=%f, xmin=%f, ymin=%f, xmax=%f, ymax=%f, xext=%f, yext=%f\n",
						iw, ih, bb[0], bb[1], bb[2], bb[3], bb[2]-bb[0], bb[3]-bb[1]);
				}*/
			}
		}
		else
		{
			DEBUGLOGP("(1) svg element %s was NOT loaded\n", ename);
		}

/*		if( g_pSvgHandle[e] )
		{
			DEBUGLOGP("svg element %s was loaded\n", ename);

//			static const int elems[] = { CLOCK_DROP_SHADOW, CLOCK_FACE, CLOCK_MARKS, CLOCK_FACE_SHADOW, CLOCK_GLASS, CLOCK_FRAME };
			static const int dimElems[] = { CLOCK_DROP_SHADOW, CLOCK_FACE, CLOCK_FACE_SHADOW };

			for( size_t d = 0; d < vectsz(dimElems); d++ )
			{
				if( dimElems[d] == e )
				{
					cdim.width = cdim.height = 0;
					rsvg_handle_get_dimensions(g_pSvgHandle[e], &cdim);

					if( mdim.width  < cdim.width )
						mdim.width  = cdim.width;

					if( mdim.height < cdim.height )
						mdim.height = cdim.height;
				}
			}

			GdkPixbuf* pBuffer = rsvg_handle_get_pixbuf(g_pSvgHandle[e]);

			if( pBuffer )
			{
				DEBUGLOGS("got GdkPixbuf");

				guint         xmin   = 0;
				guint         ymin   = 0;
				guint         xmax   = mdim.width;
				guint         ymax   = mdim.height;
				guint         nBytes = 0;
				guchar*       pBytes = gdk_pixbuf_get_pixels_with_length(pBuffer, &nBytes);

				if( pBytes && nBytes )
				{
					DEBUGLOGS("got pixels buffer data");

					bool blank = true;
					for( guint b = 0; b < nBytes; b++ )
					{
						if( pBytes[b] != 0 )
						{
							blank = false;
							break;
						}
					}

					if( blank )
					{
						DEBUGLOGP("discarding %s's %s element (fully transparent)\n", name, ename);

//						rsvg_handle_free(g_pSvgHandle[e]); // deprecated
						g_object_unref(g_pSvgHandle[e]);
						g_pSvgHandle[e] = NULL;
					}
					else
					{
						valid = true;
						DEBUGLOGS("pixels buffer data is NOT blank");

						if( hand )
						{
							guint p, x, y;
	
							cdim.width = cdim.height = 0;
							rsvg_handle_get_dimensions(g_pSvgHandle[e], &cdim);

							xmin = cdim.width;
							ymin = cdim.height;
							xmax = 0;
							ymax = 0;

							for( guint b = 0; b < nBytes; b++ )
							{
								if( pBytes[b] != 0 )
								{
									p = b / 4;
									x = p % cdim.width;
									y = p / cdim.width;

									if( xmin > x ) xmin = x;
									if( ymin > y ) ymin = y;
									if( xmax < x ) xmax = x;
									if( ymax < y ) ymax = y;
								}
							}
						}
					}

//					g_free(pPixels);
				}
				else
				{
					DEBUGLOGS("didn't get pixels buffer data");
				}

				if( hand )
				{
					DEBUGLOGP("svg element %s was loaded\n", ename);
					DEBUGLOGP("cw=%d, ch=%d, xmin=%d, ymin=%d, xmax=%d, ymax=%d\n",
						(int)cdim.width, (int)cdim.height, (int)xmin, (int)ymin, (int)xmax, (int)ymax);
				}

//				gdk_pixbuf_unref(pBuffer);
				g_object_unref(pBuffer);
				pBuffer = NULL;
			}
			else
			{
				DEBUGLOGS("didn't get GdkPixbuf");
			}
		}
		else
		{
			DEBUGLOGP("svg element %s was NOT loaded\n", ename);
		}*/

//		free(fpath);
	}

	if( g_pMaskRgn )
	{
		if( cairo_region_num_rectangles(g_pMaskRgn) <= 0 )
		{
			DEBUGLOGS("  created scanline based mask is empty so discarding it");
			cairo_region_destroy(g_pMaskRgn);
			g_pMaskRgn = NULL;
		}
		else
		{
//			cairo_rectangle_int_t br;
//			cairo_region_get_extents(g_pMaskRgn, &br);

//			DEBUGLOGP("  mask has %d rects within (%d, %d)->(%d, %d) bbox\n",
//				cairo_region_num_rectangles(g_pMaskRgn), br.x, br.y, br.x+br.width-1, br.y+br.height-1);
			DEBUGLOGS("  created scanline based mask is not empty so keeping it");
		}
	}
	else
	{
		DEBUGLOGS("  mask svg supplied so alls well");
	}

	if( mdim.width && mdim.height )
		g_svgClock       = mdim;
	else
		g_svgClock.width = g_svgClock.height = 100;

	DEBUGLOGP("  global clock dims of (%d, %d) set\n", g_svgClock.width, g_svgClock.height);

/*	if( valid )
	{
		g_bboxZero = gRun.bboxSecHand[0] == 0 && gRun.bboxSecHand[1] == 0 &&
		             gRun.bboxSecHand[2] == 0 && gRun.bboxSecHand[3] == 0;

//		cairo_matrix_init_scale(&g_mat, gCfg.clockW/g_svgClock.width, gCfg.clockH/g_svgClock.height);
//		cairo_matrix_translate (&g_mat, g_svgClock.width/2,           g_svgClock.height/2);

		const double cww = gCfg.clockW;       // clock window width
		const double cwh = gCfg.clockH;       // clock window height
		const double fcw = g_svgClock.width;  // svg hand file width (total)
		const double fch = g_svgClock.height; // svg hand file height (total)
//
		cairo_matrix_init_scale(&g_mat, cww/fcw, cwh/fch);
		cairo_matrix_translate (&g_mat, fcw/2,   fch/2);
	}*/

	DEBUGLOGP(" theme is %s\n\n", valid ? "'valid'" : "not 'valid' (no drawable elements)");

	DEBUGLOGE;
	DEBUGLOGS("  (2)");

	return valid;
}
/*
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prt_main_surf_type(GtkWidget* pWidget)
{
	static const int   t_surfs[] =
	{
		CAIRO_SURFACE_TYPE_IMAGE, CAIRO_SURFACE_TYPE_PDF,       CAIRO_SURFACE_TYPE_PS,             CAIRO_SURFACE_TYPE_XLIB,         CAIRO_SURFACE_TYPE_XCB,
		CAIRO_SURFACE_TYPE_GLITZ, CAIRO_SURFACE_TYPE_QUARTZ,    CAIRO_SURFACE_TYPE_WIN32,          CAIRO_SURFACE_TYPE_BEOS,         CAIRO_SURFACE_TYPE_DIRECTFB,
		CAIRO_SURFACE_TYPE_SVG,   CAIRO_SURFACE_TYPE_OS2,       CAIRO_SURFACE_TYPE_WIN32_PRINTING, CAIRO_SURFACE_TYPE_QUARTZ_IMAGE, CAIRO_SURFACE_TYPE_SCRIPT,
		CAIRO_SURFACE_TYPE_QT,    CAIRO_SURFACE_TYPE_RECORDING, CAIRO_SURFACE_TYPE_VG,             CAIRO_SURFACE_TYPE_GL,           CAIRO_SURFACE_TYPE_DRM,
		CAIRO_SURFACE_TYPE_TEE,   CAIRO_SURFACE_TYPE_XML,       CAIRO_SURFACE_TYPE_SKIA,           CAIRO_SURFACE_TYPE_SUBSURFACE,
		CAIRO_SURFACE_TYPE_COGL
	};

	static const char* t_names[] =
	{
		"CairoSurfaceTypeImage", "CairoSurfaceTypePDF",       "CairoSurfaceTypePS",            "CairoSurfaceTypeXLIB",        "CairoSurfaceTypeXCB",
		"CairoSurfaceTypeGlitz", "CairoSurfaceTypeQuartz",    "CairoSurfaceTypeWin32",         "CairoSurfaceTypeBEOS",        "CairoSurfaceTypeDirectFB",
		"CairoSurfaceTypeSVG",   "CairoSurfaceTypeOS/2",      "CairoSurfaceTypeWin32Printing", "CairoSurfaceTypeQuartzImage", "CairoSurfaceTypeScript",
		"CairoSurfaceTypeQT",    "CairoSurfaceTypeRecording", "CairoSurfaceTypeVG",            "CairoSurfaceTypeGL",          "CairoSurfaceTypeDRM",
		"CairoSurfaceTypeTee",   "CairoSurfaceTypeXML",       "CairoSurfaceTypeSKIA",          "CairoSurfaceTypeSubSurface",
		"CairoSurfaceTypeCOGL"
	};

	cairo_t*         pContext = pWidget  ?  gdk_cairo_create(pWidget->window)    :  NULL;
	cairo_surface_t* pSurface = pContext ?  cairo_get_target(pContext)           :  NULL;
	int              st       = pSurface ? (int)cairo_surface_get_type(pSurface) : -1;

	if( st != -1 )
	{
		for( size_t t = 0; t < vectsz(t_surfs); t++ )
		{
			if( t_surfs[t] == st )
			{
				DEBUGLOGP("surface type is %s (%d)\n", t_names[t], t_surfs[t]);
				break;
			}
		}
	}

	if( pSurface )
		cairo_surface_finish(pSurface);

	if( pContext )
		cairo_destroy(pContext);
}
*/

