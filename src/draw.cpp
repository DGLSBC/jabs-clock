/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "draw"

#include <math.h>         // for fabs
#include <limits.h>       // for PATH_MAX
#include <librsvg/rsvg.h> // for svg loading/rendering/etc.

#include "draw.h"         //
#include "global.h"       // for runtime data
#include "cfgdef.h"       // for ?
#include "config.h"       // for config data
#include "utility.h"      // for thread funcs, ...
#include "handAnim.h"     // for ?
#include "debug.h"        // for debugging prints

#if CAIRO_HAS_SVG_SURFACE
#define  _WRITE_SVG_ICON
#include <cairo-svg.h>
#else
#define  _WRITE_PNG_ICON
#endif

#if _USEKDE
#include <cairo-xlib.h>
#endif

#if      _USEGTK
#define  _USEPANGO     1  // use pango instead of freetype font loading
#else
#define  _USEPANGO     0  // use freetype font loading
#endif

#if !USEPANGO
#if CAIRO_HAS_FT_FONT
#include <cairo-ft.h>
#include <ft2build.h>
#include  FT_FREETYPE_H
#endif
#endif

#define MASKBEGBAD 65535
#define MASKENDBAD     0

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
enum SurfaceType
{
	SURF_BBASE = 0,
	SURF_FCOVR,
	SURF_BKGND,
	SURF_FRGND,
	SURF_AHAND,
	SURF_HHAND,
	SURF_MHAND,
	SURF_SHAND,
	SURF_AHSHD,
	SURF_HHSHD,
	SURF_MHSHD,
	SURF_SHSHD,
//	SURF_CDATE, // not using yet
	SURF_COUNT
};

#define SURF_ALL   (SURF_COUNT+1)     // clock rendering locking via an unused surface lock bit
#define SURF_CNTXT (SURF_COUNT+2)     // rendering context locking via an unused surface lock bit

#define minv(a, b) (a) < (b) ? (a) : (b)
#define maxv(a, b) (a) > (b) ? (a) : (b)

static inline double deg2Rad(double deg) { return deg*M_PI/180.0; } // degrees to radians convertor

static const  double g_xDiv  =  2.00; // TODO: eventually replace these two with
static const  double g_yDiv  =  8.00; //       gRun.bboxNNNHand usages
static const  double g_coffX = -1.00; // base clock hand shadow x-axis offset
static const  double g_coffY =  1.00; // base clock hand shadow y-axis offset
static const  double g_ahoff =  0.40; // additional hand shadow x-axis offset (alarm)
static const  double g_hhoff =  0.35; // (hour)
static const  double g_mhoff =  0.30; // (minute)
static const  double g_shoff =  0.25; // (second)

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
namespace draw
{

// -----------------------------------------------------------------------------
struct DrawSurf
{
	SurfaceType      type;       //
	cairo_surface_t* pCairoSurf; //
	cairo_pattern_t* pCairoPtrn; //
	int              width;      //
	int              height;     //
	int              svgClockW;  // TODO: use in place of g_svgClockW
	int              svgClockH;  // TODO: use in place of g_svgClockH
};

// -----------------------------------------------------------------------------
struct XForm
{
	int            width;
	int            height;
	double         scaleX;
	double         scaleY;
	double         xlateX;
	double         xlateY;
	double         rotate;
	cairo_matrix_t matrix;
};

// -----------------------------------------------------------------------------
struct Hand
{
	bool        draw;
	bool        optBest;
	bool        useSurf;
	SurfaceType handST;
	SurfaceType shadST;
	int         handElem;
	int         shadElem;
	double      hoff;
	double*     bbox;
	double      alfa;

	double      halfW;
	double      halfH;
	double      wratio;
	double      hratio;
	double      cx;
	double      cy;
	double      ox;
	double      oy;

	// one each for shadow(0) and hand(1)
	XForm       transfrm[2];
	SurfaceType surfType[2];
	bool        handOkay[2];
	bool        surfOkay[2];
};

// -----------------------------------------------------------------------------
struct UpdateTheme // TODO: replace all ThemeEntry specific entries w/ThemeEntry field
{
	UpdateThemeDone callBack; // where to go once the theme updating has finished
	ppointer        data;     // what caller wants passed to the callback
	bool            okay;     // whether the updating was a success
	ThemeEntry      te;       // theme file path, name, modes, colors, ...
};

// -----------------------------------------------------------------------------
struct drawTextPath
{
	int         index;
	const char* text;
	double*     sizeFact;
	double*     hiteFact;
	double      hiteSign;
};

// -----------------------------------------------------------------------------
static cairo_surface_t* make_mask_surf(int w, int h);

typedef int (*Render)  (cairo_t* pContext, bool clear);
static  int   render1a (cairo_t* pContext, bool clear);
static  int   render1b (cairo_t* pContext, bool clear);
static  int   render2  (cairo_t* pContext, bool clear);
static  int   render3  (cairo_t* pContext, bool clear);
static  int   render4  (cairo_t* pContext, bool clear);
static  int   render5  (cairo_t* pContext, bool clear);
static  int   render6  (cairo_t* pContext, bool clear);
static  int   renderbeg(cairo_t* pContext, bool lock=false);

static bool render_dsktp(cairo_t* pContext, int width, int height, bool yield, bool bright=false, bool compChk=true);
static bool render_bbase(cairo_t* pContext, int width, int height, bool yield, bool bright=false);
static bool render_fcovr(cairo_t* pContext, int width, int height, bool yield, bool bright=false);
static bool render_bkgnd(cairo_t* pContext, int width, int height, bool yield, bool bright=false, bool date=true, bool temp=false);
static bool render_frgnd(cairo_t* pContext, int width, int height, bool yield, bool bright=false, bool date=true, bool temp=false);
#if 0
static bool render_dsktp(cairo_t* pContext, const DrawSurf& ds, int width, int height, bool yield, bool bright=false, bool compChk=true);
static bool render_bbase(cairo_t* pContext, const DrawSurf& ds, int width, int height, bool yield, bool bright=false);
static bool render_fcovr(cairo_t* pContext, const DrawSurf& ds, int width, int height, bool yield, bool bright=false);
static bool render_bkgnd(cairo_t* pContext, const DrawSurf& ds, int width, int height, bool yield, bool bright=false, bool date=true, bool temp=false);
static bool render_frgnd(cairo_t* pContext, const DrawSurf& ds, int width, int height, bool yield, bool bright=false, bool date=true, bool temp=false);
#endif

static bool render_chand(cairo_t* pContext, int handElem, int shadElem, int width, int height, bool bright, double hoff, bool center=true, double rotate=-1);

static void   render_date_make(cairo_t* pContext, int width, int height, bool yield, bool bright);
static void   render_date     (cairo_t* pContext, int width, int height, bool yield, bool bright);
//static double render_text_path(cairo_t* pContext, bool yield, int index, const char* text, double sizeFact, double hiteFact, double hiteAddO, double hiteSign, double xoff=0, double yoff=0);
static void   render_text_help(cairo_t* pContext, double xbl, double ybl, double w, double h, double xb, double yb);

static double render_text_path(cairo_t* pContext, bool yield, const drawTextPath& dtp, double hiteAddO, double xoff=0, double yoff=0);

static bool render_hand(cairo_t* pContext, int handElem, int shadElem, double hoffX, double hoffY, double angle);

static void render_hand_hl(cairo_t* pContext, const Hand& hand, bool evalDraws);
static bool render_hand_hl_chk(const Hand& hand, bool* handOkay, bool* surfOkay, SurfaceType* surfType);
static void render_hand_hl_chk_set  (Hand& hand);

static bool render_handalm(cairo_t* pContext, int width, int height, bool yield, bool bright, bool center, bool rotate, bool hand, bool shadow, bool offset);
static bool render_handhor(cairo_t* pContext, int width, int height, bool yield, bool bright, bool center, bool rotate, bool hand, bool shadow, bool offset);
static bool render_handmin(cairo_t* pContext, int width, int height, bool yield, bool bright, bool center, bool rotate, bool hand, bool shadow, bool offset);
static bool render_handsec(cairo_t* pContext, int width, int height, bool yield, bool bright, bool center, bool rotate, bool hand, bool shadow, bool offset);

static void render_surf(cairo_t* pContext, SurfaceType st, cairo_operator_t op1, int op2=-1, double alf=1.0);

static bool update_surface(cairo_t* pSourceContext, SurfaceType st, int width, int height, bool hand, bool yield);
static bool update_surf   (cairo_t* pContext,       SurfaceType st, int width, int height, bool hand, bool yield);

static void update_surf_swap(SurfaceType st);

static void update_text_font(cairo_t* pContext=NULL, bool force=false);

static ppointer update_theme_func(ppointer data);
static bool     update_theme_internal(const ThemeEntry& te);

#ifdef _USEREFRESHER
// -----------------------------------------------------------------------------
double refreshTime[MAX_REFRESH_RATE+2];
double refreshFrac[MAX_REFRESH_RATE+2];
double refreshCumm[MAX_REFRESH_RATE+2];
int    refreshCount =  0;
#endif

// -----------------------------------------------------------------------------
#if _USEGTK
static GdkWindow*            g_pClockWindow  =   NULL;
#endif
static Render                g_pRender       =   render1b; // default to testing/drawing everything
static cairo_t*              g_pContext      =   NULL;     // access via SurfLock...(SURF_CNTXT)
static cairo_font_face_t*    g_pTextFontFace =   NULL;
static cairo_font_options_t* g_pTextOptions  =   NULL;
static cairo_path_t*         g_pTextPath[]   = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static cairo_region_t*       g_pMaskRgn      =   NULL;

static DrawSurf         g_surfs[SURF_COUNT];
static DrawSurf         g_temps[SURF_COUNT];
static cairo_surface_t* g_pDesktop       = NULL;
static int              g_pCairoSurfLock = 0; // per surface bit locking
static int              g_surfaceW       = 0;
static int              g_surfaceH       = 0;

//#if _USEGTK
static RsvgHandle* g_pSvgHandle[CLOCK_ELEMENTS];
//#else
//static void*       g_pSvgHandle[CLOCK_ELEMENTS];
//#endif
static int         g_svgClockW      = 0;
static int         g_svgClockH      = 0;
static int         g_pSvgHandleLock = 0; // per element svg bit locking

#if 0
static int* g_pMaskBeg = NULL;
static int* g_pMaskEnd = NULL;
static int  g_nMaskCnt = 0;
#endif

#if !_USEPANGO
#if CAIRO_HAS_FT_FONT
static FT_Face    g_ft_face = 0;
static FT_Library g_ft_libr = 0;
#endif
#endif

static XForm g_rendrxfB = { 0, 0, 1, 1, 0 }; // drawing matrix for the non-hand (base) elements
static XForm g_rendrxfH = { 0, 0, 1, 1, 0 }; // drawing matrix for the hand elements

//	                        draw   optBest useSurf handST      shadST      handElem           shadElem	                hoff     bbox              alf
static Hand  g_almHand  = { false, true,   false,  SURF_AHAND, SURF_AHSHD, CLOCK_ALARM_HAND,  CLOCK_ALARM_HAND_SHADOW,  g_ahoff, gRun.bboxAlmHand, 1.00 };
static Hand  g_tznHand  = { false, true,   false,  SURF_HHAND, SURF_HHSHD, CLOCK_HOUR_HAND,   CLOCK_HOUR_HAND_SHADOW,   g_hhoff, gRun.bboxHorHand, 0.25 };
static Hand  g_horHand  = { false, true,   false,  SURF_HHAND, SURF_HHSHD, CLOCK_HOUR_HAND,   CLOCK_HOUR_HAND_SHADOW,   g_hhoff, gRun.bboxHorHand, 1.00 };
static Hand  g_minHand  = { false, true,   true,   SURF_MHAND, SURF_MHSHD, CLOCK_MINUTE_HAND, CLOCK_MINUTE_HAND_SHADOW, g_mhoff, gRun.bboxMinHand, 1.00 };
static Hand  g_secHand  = { false, true,   true,   SURF_SHAND, SURF_SHSHD, CLOCK_SECOND_HAND, CLOCK_SECOND_HAND_SHADOW, g_shoff, gRun.bboxSecHand, 1.00 };

static int   g_miscellaniLock = 0;

PWidget* g_angChart   = NULL;
bool     g_angChartDo = false;
int      g_angChartW  = 0;
int      g_angChartH  = 0;
double   g_angAdjX    = 0;
double   g_angAdjY    = 0;
int      g_angX1      = 0;
int      g_angY1      = 0;

// -----------------------------------------------------------------------------
static inline void MiscLockBeg(int b) { g_bit_lock  (&g_miscellaniLock, b); }
static inline void MiscLockEnd(int b) { g_bit_unlock(&g_miscellaniLock, b); }

static inline void SurfLockBeg(int b) { g_bit_lock  (&g_pCairoSurfLock, b); }
static inline void SurfLockEnd(int b) { g_bit_unlock(&g_pCairoSurfLock, b); }

static inline void SvgLockBeg (int b) { g_bit_lock  (&g_pSvgHandleLock, b); }
static inline void SvgLockEnd (int b) { g_bit_unlock(&g_pSvgHandleLock, b); }
#if 0
static void g_lockbeg(int& a, int b)
{
//	if( g_thread_self() == ? ) return; // use TLS value(s) here (1/lock-bit)?
	DEBUGLOGP("Locking %s (%d)", &a == &g_pCairoSurfLock ? "surf" : "svg", b);
	fflush(stdout);
	g_bit_lock(&a, b);
	printfs(" done\n");
	fflush(stdout);
}

static void g_lockend(int& a, int b)
{
//	if( g_thread_self() == ? ) return; // use TLS value(s) here (1/lock-bit)?
	DEBUGLOGP("UNLocking %s (%d)", &a == &g_pCairoSurfLock ? "surf" : "svg", b);
	fflush(stdout);
	g_bit_unlock(&a, b);
	printfs(" done\n");
	fflush(stdout);
}

static inline void SurfLockBeg(int b) { g_lockbeg(g_pCairoSurfLock, b); }
static inline void SurfLockEnd(int b) { g_lockend(g_pCairoSurfLock, b); }

static inline void SvgLockBeg (int b) { g_lockbeg(g_pSvgHandleLock, b); }
static inline void SvgLockEnd (int b) { g_lockend(g_pSvgHandleLock, b); }
#endif

#if 0
static inline void SurfLockBeg(int b) {}
static inline void SurfLockEnd(int b) {}

static inline void SvgLockBeg (int b) {}
static inline void SvgLockEnd (int b) {}
#endif

#if 0
#define LockContextBeg() SurfLockBeg(SURF_CNTXT) // old usage
#define LockContextEnd() SurfLockEnd(SURF_CNTXT)
#endif

#define LockAllSurfBeg() MiscLockBeg(SURF_ALL)
#define LockAllSurfEnd() MiscLockEnd(SURF_ALL)
#define LockContextBeg() MiscLockBeg(SURF_CNTXT)
#define LockContextEnd() MiscLockEnd(SURF_CNTXT)

} // draw

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#undef CAIRO_XLIB_TEST

#if _USEGTK
// -----------------------------------------------------------------------------
static cairo_t* cairo_context(GdkWindow* pWnd)
{
#if defined(CAIRO_HAS_XLIB_SURFACE) && defined(CAIRO_XLIB_TEST)
	Drawable         dr       = GDK_WINDOW_XID(pWnd);
	Display*         pD       = GDK_WINDOW_XDISPLAY(pWnd);
	Visual*          pV       = GDK_VISUAL_XVISUAL(gdk_window_get_visual(pWnd));
	cairo_surface_t* pSurf    = cairo_xlib_surface_create(pD, dr, pV, alloc.width, alloc.height);
	cairo_t*         pContext = cairo_create(pSurf);
#else
	cairo_t*         pContext = gdk_cairo_create(pWnd);
#endif
	bool             okay     = pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS;

	if( !okay && pContext )
		cairo_destroy(pContext);

	return okay ? pContext : NULL;
}
#endif // _USEGTK

// -----------------------------------------------------------------------------
static cairo_t* cairo_context(PWidget* pWidget)
{
#if _USEGTK
	return cairo_context(gtk_widget_get_window(pWidget));
#else
	Display*         pD       = pWidget->x11Display();
	Visual*          pV       = (Visual*)(pWidget->x11Visual());
	Drawable         dr       = Drawable(pWidget->x11AppRootWindow());
	cairo_surface_t* pSurf    = cairo_xlib_surface_create(pD, dr, pV, gCfg.clockW, gCfg.clockH);
	cairo_t*         pContext = cairo_create(pSurf);
	bool             okay     = pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS;

	{
	const char* pDs = pD       != NULL  ? "good" : "bad";
	const char* pVs = pV       != NULL  ? "good" : "bad";
	const char* drs = dr       != 0     ? "good" : "bad";
	const char* pSs = pSurf    != NULL  ? "good" : "bad";
	const char* pCs = pContext != NULL  ? "good" : "bad";
	const char* oks = okay     != false ? "good" : "bad";
	DEBUGLOGPF("pD(%s), pV(%s), dr(%s), pS(%s), pC(%s), okay(%s)\n", pDs, pVs, drs, pSs, pCs, oks);
	}

	if( !okay && pContext )
		cairo_destroy(pContext);

	return okay ? pContext : NULL;
#endif
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::init()
{
	DEBUGLOGB;

	LockAllSurfBeg();
	for( size_t st =  0; st < SURF_COUNT; st++ )
	{
		SurfLockBeg(st);
		g_surfs[st].type       = (SurfaceType)st;
		g_surfs[st].pCairoSurf =  NULL;
		g_surfs[st].pCairoPtrn =  NULL;
		g_surfs[st].width      =  0;
		g_surfs[st].height     =  0;
		g_surfs[st].svgClockW  =  0;
		g_surfs[st].svgClockH  =  0;
		g_temps[st]            =  g_surfs[st];
		SurfLockEnd(st);
	}
	LockAllSurfEnd();

	for( size_t e =  0; e < CLOCK_ELEMENTS; e++ )
	{
		SvgLockBeg  (e);
		g_pSvgHandle[e] = NULL;
		SvgLockEnd  (e);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::grab()
{
	DEBUGLOGB;

	if( g_pDesktop )
		cairo_surface_destroy(g_pDesktop);

//	TODO: would it be easier/better to use the following call instead?
//	void  gdk_cairo_set_source_window(cairo_t* cr, GdkWindow* window, double x, double y);
//        yes, but still get all windows on dtop at clock's location, not just dtop bkgnd

#if _USEGTK
	GdkWindow* pRW          = gdk_get_default_root_window();
//	GdkPixbuf* pDesktop     = gdk_pixbuf_get_from_drawable(NULL, pRW, NULL, gCfg.clockX, gCfg.clockY, 0, 0, gCfg.clockW, gCfg.clockH);
	cairo_t*   pSorcContext = gdk_cairo_create(pRW);
#else
	void*      pRW          = NULL;
	cairo_t*   pSorcContext = cairo_create(NULL);
#endif

	g_pDesktop = cairo_surface_create_similar(cairo_get_target(pSorcContext), CAIRO_CONTENT_COLOR_ALPHA, gCfg.clockW, gCfg.clockH);

	cairo_destroy(pSorcContext);
	pSorcContext = NULL;

	cairo_t* pContext = cairo_create(g_pDesktop);

#if _USEGTK
//	gdk_cairo_set_source_pixbuf(pContext, pDesktop, 0, 0);
#endif

	DEBUGLOGP("clockX=%d, clockY=%d\n", gCfg.clockX,  gCfg.clockY);

#if _USEGTK
//	gdk_cairo_set_source_window(pContext, pRW,    0,    0);
//	gdk_cairo_set_source_window(pContext, pRW, -986, -366);
	gdk_cairo_set_source_window(pContext, pRW, -gCfg.clockX, -gCfg.clockY); // why negs req'd to work?
#endif

	cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);
	cairo_paint       (pContext);

	cairo_destroy(pContext);
	pContext = NULL;

//	g_object_unref(pDesktop);
//	pDesktop = NULL;

	DEBUGLOGP("clk xywh=%d/%d/%d/%d,pRW=%p,pDtop=%p\n", gCfg.clockX, gCfg.clockY, gCfg.clockW, gCfg.clockH, pRW, g_pDesktop);
	DEBUGLOGP("desktop surface%screated\n", g_pDesktop ? " " : " NOT ");
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::beg(bool init)
{
	DEBUGLOGB;

	if( init )
	{
		DEBUGLOGP("init: clockX=%d, clockY=%d, clockW=%d, clockH=%d\n", gCfg.clockX, gCfg.clockY, gCfg.clockW, gCfg.clockH);

		LockAllSurfBeg();
		for( size_t st = 0; st < SURF_COUNT; st++ )
		{
			SurfLockBeg(st);
			g_surfs[st].pCairoPtrn = NULL;
			g_surfs[st].pCairoSurf = NULL;
			g_temps[st]            = g_surfs[st];
			SurfLockEnd(st);
		}
		LockAllSurfEnd();

		if( g_svgClockW == 0 || g_svgClockH == 0 )
		{
			g_svgClockW =  100;
			g_svgClockH =  100;

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

#if _USEPANGO
	update_text_font();
#else
#if CAIRO_HAS_FT_FONT
	if( *gCfg.fontFace && g_isa_file(gCfg.fontFace) )
//	if( *gCfg.fontName && *gCfg.fontFace && g_isa_file(gCfg.fontFace) )
//	if( *gCfg.fontName && *gCfg.fontFace )
//	if( *gCfg.fontName )
	{
		static const cairo_user_data_key_t key = { 0 };

		FT_Init_FreeType(&g_ft_libr);
		FT_New_Face     ( g_ft_libr, gCfg.fontFace, 0, &g_ft_face);

		g_pTextFontFace = cairo_ft_font_face_create_for_ft_face(g_ft_face, 0);
		cairo_font_face_set_user_data(g_pTextFontFace, &key, g_ft_face, (cairo_destroy_func_t)FT_Done_Face);

#ifdef DEBUGLOG
		DEBUGLOGPF("confg font path is    \"%s\"\n", gCfg.fontFace);
		DEBUGLOGPF("confg font name is    \"%s\"\n", gCfg.fontName);
		DEBUGLOGPF("confg font size is    \"%d\"\n", gCfg.fontSize);
		DEBUGLOGPF("ftype font ps name is \"%s\"\n", FT_Get_Postscript_Name(g_ft_face));
		DEBUGLOGPF("cairo font is         \"%s\"\n", g_pTextFontFace ? "valid" : "NOT valid");
#endif
	}
#endif // CAIRO_HAS_FT_FONT
#endif // _USEPANGO

	DEBUGLOGS("aft font processing calls");
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::end(bool init)
{
	DEBUGLOGB;

	if( !init )
	{
		draw::lock(true);

		if( g_pContext )
		{
			cairo_destroy(g_pContext);
#if _USEGTK
			g_pClockWindow = NULL;
#endif
			g_pContext = NULL;
		}

		draw::lock(false);

		for( size_t e = 0; e < CLOCK_ELEMENTS; e++ )
		{
			SvgLockBeg(e);

			if( g_pSvgHandle[e] )
			{
#if _USEGTK
//				rsvg_handle_free(g_pSvgHandle[e]); // deprecated
				g_object_unref(g_pSvgHandle[e]);
#endif
				g_pSvgHandle[e] = NULL;
			}

			SvgLockEnd(e);
		}

		if( g_pMaskRgn )
			cairo_region_destroy(g_pMaskRgn);
		g_pMaskRgn = NULL;
#if 0
		if( g_pMaskBeg )
			delete[] g_pMaskBeg;
		g_pMaskBeg = NULL;

		if( g_pMaskEnd )
			delete[] g_pMaskEnd;
		g_pMaskEnd = NULL;

		g_nMaskCnt = 0;
#endif
//		rsvg_term(); // deprecated

		if( g_pDesktop )
		{
			DEBUGLOGS("desktop pixbuf destroyed");
			cairo_surface_destroy(g_pDesktop);
			g_pDesktop = NULL;
		}
	}

	hand_anim_end();

	for( size_t p = 0; p < vectsz(g_pTextPath); p++ )
	{
		if( g_pTextPath[p] )
			cairo_path_destroy(g_pTextPath[p]);
		g_pTextPath[p] = NULL;
	}

	if( g_pTextOptions )
		cairo_font_options_destroy(g_pTextOptions);
	g_pTextOptions  = NULL;

	if( g_pTextFontFace )
		cairo_font_face_destroy(g_pTextFontFace);
	g_pTextFontFace = NULL;

#if !_USEPANGO
#if CAIRO_HAS_FT_FONT
	if( g_ft_face )
		FT_Done_Face(g_ft_face);

	if( g_ft_libr )
		FT_Done_FreeType(g_ft_libr);
#endif // CAIRO_HAS_FT_FONT
#endif

	LockAllSurfBeg();
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
	LockAllSurfEnd();

	reset_hands();

	g_surfaceW = 0;
	g_surfaceH = 0;

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::chg()
{
	DEBUGLOGBF;
	update_text_font(NULL, true);
	DEBUGLOGEF;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::lock(bool lock)
{
	if( lock )
		LockContextBeg();
	else
		LockContextEnd();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool draw::make_icon(const char* iconPath, bool full)
{
	DEBUGLOGB;
	DEBUGLOGS("creating theme ico image file for valid current theme");
	DEBUGLOGP("  %s\n", gCfg.themeFile);

	bool           yield = true;
	g_yield_thread(yield);

	bool             ret   = false;
	double           isz   = 0;
	cairo_surface_t* pSurf = NULL;

	if( gCfg.pngIcon )
	{
#if defined(CAIRO_HAS_PNG_FUNCTIONS) && defined(_WRITE_PNG_ICON)
		isz              = 128.0;
		pSurf            = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, isz, isz);
#endif
	}
	else
	{
#if defined(CAIRO_HAS_SVG_SURFACE) && defined(_WRITE_SVG_ICON)
//		NOTE: creates a nice image, even though the size doesn't agree with the docs,
//		      but it takes 'much' longer to create this versus a png on my little box

		isz              = 100.0;              // yields a 125x125 image - why?
//		isz              = 100.0*100.0/125.0;  // yields a 100x100 image but innards say its 80x80
//		const double ppi =  72.0;
//		const double ext = isz*ppi;            // doesn't work at all - way too big
//		pSurf            = cairo_svg_surface_create(iconPath, ext, ext);
		pSurf            = cairo_svg_surface_create(iconPath, isz, isz);
#endif
	}
#if 0
#if defined(CAIRO_HAS_SVG_SURFACE) && defined(_WRITE_SVG_ICON)

//	NOTE: creates a nice image, even though the size doesn't agree with the docs,
//	      but it takes 'much' longer to create this versus a png on my little box

	const double     isz   = 100.0;              // yields a 125x125 image - why?
//	const double     isz   = 100.0*100.0/125.0;  // yields a 100x100 image but innards say its 80x80
//	const double     ppi   =  72.0;
//	const double     ext   = isz*ppi;            // doesn't work at all - way too big
//	cairo_surface_t* pSurf = cairo_svg_surface_create(iconPath, ext, ext);
	cairo_surface_t* pSurf = cairo_svg_surface_create(iconPath, isz, isz);

#else  // CAIRO_HAS_SVG_SURFACE

#if defined(CAIRO_HAS_PNG_FUNCTIONS) && defined(_WRITE_PNG_ICON)

	const double     isz   = 128.0;
	cairo_surface_t* pSurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, isz, isz);

#else  // CAIRO_HAS_PNG_FUNCTIONS

	// TODO: need some sort of fallback here?

	const double     isz   = 0;
	cairo_surface_t* pSurf = NULL;

#endif // CAIRO_HAS_PNG_FUNCTIONS
#endif // CAIRO_HAS_SVG_SURFACE
#endif
	if( pSurf )
	{
		DEBUGLOGS("creating new image surface tied to the theme ico image file");

		g_yield_thread(yield);
		cairo_t* pContext = cairo_create(pSurf);

		if( pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS )
		{
			DEBUGLOGS("successfully created context on theme ico image file's surface");

			g_yield_thread(yield);
			render_bbase  (pContext, isz, isz, yield);
//			render_bbase  (pContext, g_surfs[SURF_BBASE], isz, isz, yield);

			if( full && gCfg.showDate && gCfg.faceDate )
			{
			g_yield_thread(yield);
			render_date(pContext, isz, isz, yield, false);
			}

			if( full )
			{
			g_yield_thread(yield);
			render_handalm(pContext, isz, isz, yield, false, true, true, true, true, true);
			}

			g_yield_thread(yield);
			render_handhor(pContext, isz, isz, yield, false, true, true, true, true, true);

			g_yield_thread(yield);
			render_handmin(pContext, isz, isz, yield, false, true, true, true, true, true);

			if( full )
			{
			g_yield_thread(yield);
			render_handsec(pContext, isz, isz, yield, false, true, true, true, true, true);
			}

			if( full && gCfg.showDate && !gCfg.faceDate )
			{
			g_yield_thread(yield);
			render_date(pContext, isz, isz, yield, false);
			}

			g_yield_thread(yield);
//			render_frgnd  (pContext, isz, isz, yield);
			render_fcovr  (pContext, isz, isz, yield);

			g_yield_thread(yield);
			cairo_destroy (pContext);
			ret = true;
		}

#if defined(CAIRO_HAS_PNG_FUNCTIONS) && defined(_WRITE_PNG_ICON)
		if( gCfg.pngIcon )
		{
			g_yield_thread(yield);
			ret = cairo_surface_write_to_png(pSurf, iconPath) == CAIRO_STATUS_SUCCESS;
		}
#endif
		DEBUGLOGP("%s the theme ico image file:\n*%s*\n", ret ? "successfully created" : "didn't create", iconPath);

		cairo_surface_destroy(pSurf);
		g_yield_thread(yield);
		pSurf = NULL;
	}

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool draw::make_mask(PWidget* pWindow, int width, int height, bool shaped, bool input, bool lock)
{
	DEBUGLOGB;

	bool ret  = false;
	bool okay = false;

#if _USEGTK
#if     GTK_CHECK_VERSION(3,0,0)

	// TODO: need to make a new region that's scaled to width/height from the svg-derived region
	cairo_region_t* pMaskRgn  = NULL;
	ret = okay =  g_pMaskRgn != NULL;
//	ret = okay =  g_nMaskCnt  > 0 && g_pMaskBeg != NULL && g_pMaskEnd != NULL;

	if( ret )
	{
		if( shaped ) // non-rectangular mask requested?
		{
			pMaskRgn = cairo_region_create();

			if( pMaskRgn && cairo_region_status(pMaskRgn) == CAIRO_STATUS_SUCCESS )
			{
				DEBUGLOGS("using a scaled, theme load time created, outline mask");

				struct cairo_rect : public cairo_rectangle_int_t
				{
					void scale(double ws, double hs)
					{
						x      = (int)((double)x*ws);
						y      = (int)((double)y*hs);
						width  = (int)((double)width *ws);
						height = (int)((double)height*hs);
					}
				};

				cairo_rect cr, crf;
				int        xb, yb, xe, ye;
				int    nr     =  cairo_region_num_rectangles(g_pMaskRgn);
				double wratio = (double)width /(double)g_svgClockW;
				double hratio = (double)height/(double)g_svgClockH;

				DEBUGLOGP("  %d rects in the unscaled outline mask region\n", nr);
//				DEBUGLOGP("  %d rects in the unscaled outline mask region\n", g_nMaskCnt);

				// TODO: make a region that better represents the scaled outline
				//      (see below for 1st failed attempt that won't work using
				//       a cairo region list, since its rects are 'union'ized)

				for( int r = 0; r < nr; r++ )
//				for( int r = 0; r < g_nMaskCnt; r++ )
				{
					cairo_region_get_rectangle(g_pMaskRgn, r, &cr);
					cr.scale(wratio, hratio); cr.height++; // +1 to ensure overlap
					cairo_region_union_rectangle(pMaskRgn,    &cr);
#if 0
					if( r )
					{
						cairo_region_get_rectangle(g_pMaskRgn, r-1, &crf);
						crf.scale(wratio, hratio); crf.height++; // +1 to ensure overlap

						xb = crf.x < cr.x ? crf.x : cr.x;
						yb = crf.y + crf.height;
						xe = crf.x + crf.width > cr.x + cr.width ? crf.x+crf.width : cr.x+cr.width;
						ye = cr .y;

						cr.x      = xb;
						cr.y      = yb;
						cr.width  = xe - xb + 1;
						cr.height = ye - yb + 1;

						if( cr.width > 0 && cr.height > 0 )
							cairo_region_union_rectangle(pMaskRgn, &cr);
					}
#endif
#if 0
					cr.x      = g_pMaskBeg[r];
					cr.y      = r;
					cr.width  = g_pMaskEnd[r] - g_pMaskBeg[r];
					cr.height = 1;

					cr.scale(wratio, hratio);
					cr.height++; // +1 to ensure overlap

					cairo_region_union_rectangle(pMaskRgn, &cr);
#endif
				}
#if 0
				if( nr == 1 )
				{
					cairo_rect cr;
					cairo_region_get_rectangle(g_pMaskRgn, 0, &cr);
					cr.scale(wratio, hratio);
					cairo_region_union_rectangle(pMaskRgn,    &cr);
				}
				else
				if( nr >  1 )
				{
					cairo_rect crb, cre;
					double     ;

					cairo_region_get_rectangle(g_pMaskRgn, 0, &crb);
					crb.scale(wratio, hratio);

					for( int r = 1; r < nr; r++ )
					{
						cairo_region_get_rectangle(g_pMaskRgn, r, &cre);
						cre.scale(wratio, hratio);

						for( int y = crb.y; y < cre.y; y++ )
						{
						}

						crb = cre;
					}
				}
#endif
				ret = true;
			}
		}
	}

#else // GTK_CHECK_VERSION(3,0,0)

#if _USEGTK
	GdkBitmap* pShapeMask = (GdkBitmap*)gdk_pixmap_new(NULL, width, height, 1);
	cairo_t*   pContext   =  pShapeMask ? gdk_cairo_create(pShapeMask) : NULL;
#else
	void*      pShapeMask =  NULL;
	cairo_t*   pContext   =  pShapeMask ? NULL : NULL;
#endif

	if( pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS )
	{
		cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
		cairo_paint(pContext);

		if( shaped ) // non-rectangular mask requested?
		{
			SvgLockBeg(CLOCK_MASK);
			bool maskd = g_pSvgHandle[CLOCK_MASK] != NULL;

			if( maskd )
			{
				DEBUGLOGS("mask svg is available");

				cairo_set_operator(pContext, CAIRO_OPERATOR_OVER);
				cairo_scale(pContext, (double)width/(double)g_svgClockW, (double)height/(double)g_svgClockH);
				okay = rsvg_handle_render_cairo(g_pSvgHandle[CLOCK_MASK], pContext) == TRUE;
				DEBUGLOGP("mask svg %s drawn into shapemask bitmap context\n", okay ? "was" : "was NOT");
			}

			SvgLockEnd(CLOCK_MASK);

			if( maskd == false )
			{
				DEBUGLOGS("mask svg not available");

				if( input )
				{
					int    nr     = g_pMaskRgn ? cairo_region_num_rectangles(g_pMaskRgn) : 0;
					double wratio = nr         ? (double)width /(double)g_svgClockW      : width;
					double hratio = nr         ? (double)height/(double)g_svgClockH      : height;

//					double wratio = g_nMaskCnt ? (double)width /(double)g_svgClockW : width;
//					double hratio = g_nMaskCnt ? (double)height/(double)g_svgClockH : height;

					cairo_scale(pContext, wratio, hratio);
					cairo_set_source_rgba(pContext, 1, 1, 1, 1);
					cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);

					if( nr )
//					if( g_nMaskCnt > 0 && g_pMaskBeg != NULL && g_pMaskEnd != NULL )
					{
						DEBUGLOGS("using a scaled, theme load time created, outline mask");
//						DEBUGLOGP("  %d rects make up the outline mask region\n",      nr);
//						DEBUGLOGP("  %d rects make up the outline mask region\n",      g_nMaskCnt);
						DEBUGLOGP("  svgDims are (%d, %d), clock dims are (%d, %d)\n", g_svgClockW, g_svgClockH, width, height);
						DEBUGLOGP("  scaling ratios are (%2.2f, %2.2f)\n",             wratio, hratio);

						cairo_rectangle_int_t cr;
#if 0
						if( nr == 1 )
						{
							cairo_region_get_rectangle(g_pMaskRgn, 0, &cr);
							cairo_rectangle(pContext, cr.x, cr.y, cr.width, cr.height);
						}
						else
						{
							cairo_region_get_rectangle(g_pMaskRgn, 0, &cr);
							cairo_move_to(pContext, cr.x, cr.y);
							cairo_line_to(pContext, cr.x+cr.width, cr.y);
							cairo_line_to(pContext, cr.x+cr.width, cr.y+cr.height);

//							for( int r = 0; r < nr; r++ )
							for( int r = 1; r < nr; r++ )
//							for( int r = 0; r < g_nMaskCnt; r++ )
							{
								cairo_region_get_rectangle(g_pMaskRgn, r, &cr);

//								if( g_pMaskBeg[r] == MASKBEGBAD && g_pMaskEnd[r] == MASKENDBAD )
//									continue;
//
//								cr.x      = g_pMaskBeg[r];
//								cr.y      = r;
//								cr.width  = g_pMaskEnd[r] - g_pMaskBeg[r];
//								cr.height = 1;

//								DEBUGLOGP("  r: %3.3d, pos: %3.3d, %3.3d, dim: %3.3d, %3.3d\n", r, cr.x, cr.y, cr.width, cr.height);
//								cairo_rectangle(pContext, cr.x, cr.y, cr.width, cr.height);
								cairo_line_to(pContext, cr.x+cr.width, cr.y);
								cairo_line_to(pContext, cr.x+cr.width, cr.y+cr.height);
							}

							for( int r = nr-1; r >= 0; r-- )
							{
								cairo_region_get_rectangle(g_pMaskRgn, r, &cr);
								cairo_line_to(pContext, cr.x, cr.y+cr.height);
								cairo_line_to(pContext, cr.x, cr.y);
							}

							cairo_close_path(pContext);
						}
#endif
						for( int r = 0; r < nr; r++ )
						{
							cairo_region_get_rectangle(g_pMaskRgn, r, &cr);
							cairo_rectangle(pContext, cr.x, cr.y, cr.width, cr.height);
						}
					}
					else
					{
						DEBUGLOGS("falling back to a simple scaled circular mask");
						cairo_arc(pContext, 0.5, 0.5, 0.5, 0, 2*M_PI);
					}

					cairo_fill(pContext);
					okay = true;
				}
				else // input
				{
					DEBUGLOGS("creating a scaled non-input outline mask");

					cairo_surface_t* pSurf = make_mask_surf(width, height);

					if( pSurf )
					{
						DEBUGLOGS("using a scaled, runtime created, non-input outline mask");

						cairo_set_line_width(pContext, 1);
						cairo_set_source_rgba(pContext, 1, 1, 1, 1);
						cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);

						int     xbeg, xend;
						guchar* pBytes =  cairo_image_surface_get_data  (pSurf);
						int     amin   =  50; // mininum % of alpha to be considered visible
						int     xlen   =  cairo_image_surface_get_width (pSurf);
						int     ylen   =  cairo_image_surface_get_height(pSurf);
						int     rlen   =  cairo_image_surface_get_stride(pSurf);
						guint*  pixs   = (guint*)pBytes;

						DEBUGLOGP("outline mask parms (%p, %d, %d, %d)\n", pBytes, xlen, ylen, rlen);

						for( int y = 0; y < ylen; y++ )
						{
							xbeg = xend = -1;

							for( int x = 0; x < xlen; x++ ) // find scanline's first non-zero pixel
							{
								if( ((guchar*)(&pixs[x]))[3] >= amin )
								{
									xbeg = x;
									break;
								}
							}

							if( xbeg != -1 )
							{
								xend  = xbeg + 1; // at least one non-zero pixel found

								for( int x = xlen-1; x >= 0; x-- ) // find scanline's last non-zero pixel
								{
									if( ((guchar*)(&pixs[x]))[3] >= amin )
									{
										xend = x;
										break;
									}
								}
							}

							if( xbeg != -1 && xend != -1 )
							{
								DEBUGLOGP("  scanline %3.3d non-alpha (%3.3d, %3.3d)\n", y, xbeg, xend);
								cairo_move_to(pContext, xbeg, y);
								cairo_line_to(pContext, xend, y);
							}

							pixs = (guint*)((guchar*)pixs + rlen); // to next scanline
						}

						cairo_stroke(pContext);
						cairo_set_source_rgba(pContext, 0, 0, 0, 0);

						for( int x = 0; x < xlen; x++ )
						{
							pixs = (guint*)((guchar*)pBytes + rlen); // to 2nd scanline

							for( int y = 1; y < ylen; y++ ) // find scanrow's first non-zero pixel
							{
								if( ((guchar*)(&pixs[x]))[3] >= amin )
								{
									cairo_move_to(pContext, x, 0);
									cairo_line_to(pContext, x, y);
									break;
								}

								pixs = (guint*)((guchar*)pixs + rlen); // to next scanline
							}
						}

						for( int x = 0; x < xlen; x++ )
						{
							pixs = (guint*)((guchar*)pBytes + rlen*(ylen-1)); // to next-to-last scanline

							for( int y = 1; y < ylen; y++ ) // find scanrow's first non-zero pixel
							{
								if( ((guchar*)(&pixs[x]))[3] >= amin )
								{
									cairo_move_to(pContext, x, ylen-1);
									cairo_line_to(pContext, x, ylen-y);
									break;
								}

								pixs = (guint*)((guchar*)pixs - rlen); // to previous scanline
							}
						}

						cairo_stroke(pContext);
						cairo_surface_destroy(pSurf);
						okay = true;
					}
					else
					{
						DEBUGLOGS("falling back to a simple scaled circular mask");
						cairo_arc(pContext, 0.5, 0.5, 0.5, 0, 2*M_PI);
						okay = true;
					}
				}
			}

			if( !okay )
			{
				DEBUGLOGS("all types of mask creation failed");
				DEBUGLOGS("falling back to an \"everything's clickable/viewable\" rectangle");

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
#if _USEGTK
			g_object_unref((ppointer)pShapeMask);
#endif
			pShapeMask = NULL;
		}
		else
		{
			DEBUGLOGS("no mask pixmap created");
		}
	}

	if( pContext )
		cairo_destroy(pContext);
	pContext = NULL;

	DEBUGLOGP("shapeMask is %s\n", okay ? "valid" : (pShapeMask ? "a full square" : "null"));

	ret = okay && pShapeMask != NULL;

#endif // GTK_CHECK_VERSION(3,0,0)

	if( ret )
	{
		if( lock )
			g_sync_threads_gui_beg();

#if GTK_CHECK_VERSION(3,0,0)

		// TODO: is it really necessary to get rid of the previous mask before
		//       setting a new one? doesn't appear to be.
#if 0
		if( input )
			gtk_widget_input_shape_combine_region(pWindow, NULL);
		else
			gtk_widget_shape_combine_region(pWindow, NULL);
#endif
//		if( pMaskRgn )
		{
			if( input )
				gtk_widget_input_shape_combine_region(pWindow, pMaskRgn);
			else
				gtk_widget_shape_combine_region(pWindow, pMaskRgn);
#if 0
			if( pMaskRgn )
				cairo_region_destroy(pMaskRgn);
			pMaskRgn = NULL;
#endif
		}

#else // GTK_CHECK_VERSION(3,0,0)

		// TODO: is it really necessary to get rid of the previous mask before
		//       setting a new one? doesn't appear to be.
#if 0
		if( input )
			gtk_widget_input_shape_combine_mask(pWindow, NULL, 0, 0);
		else
			gtk_widget_shape_combine_mask(pWindow, NULL, 0, 0);
#endif
//		if( pShapeMask )
		{
			if( input )
				gtk_widget_input_shape_combine_mask(pWindow, pShapeMask, 0, 0);
			else
				gtk_widget_shape_combine_mask(pWindow, pShapeMask, 0, 0);
#if 0
			if( pShapeMask )
				g_object_unref((ppointer)pShapeMask);
			pShapeMask = NULL;
#endif
		}

#endif // GTK_CHECK_VERSION(3,0,0)

		if( lock )
			g_sync_threads_gui_end();
	}

#if GTK_CHECK_VERSION(3,0,0)
	if( pMaskRgn )
		cairo_region_destroy(pMaskRgn);
	pMaskRgn = NULL;
#else // GTK_CHECK_VERSION(3,0,0)
	if( pShapeMask )
		g_object_unref((ppointer)pShapeMask);
	pShapeMask = NULL;
#endif // GTK_CHECK_VERSION(3,0,0)
#endif // _USEGTK

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
cairo_surface_t* draw::make_mask_surf(int w, int h)
{
	DEBUGLOGB;

	cairo_surface_t* pSurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);

	if( pSurf )
	{
		cairo_t* pContext = cairo_create(pSurf);

		if( pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS )
		{
			render_bbase (pContext, w, h, true);
//			render_bbase (pContext, g_surfs[SURF_BBASE], w, h, true);
			render_frgnd (pContext, w, h, true);
			cairo_destroy(pContext);
		}
	}

	DEBUGLOGE;
	return pSurf;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
int draw::render(bool clear, bool lock)
{
	DEBUGLOGB;

	int ret = 0;

	draw::lock(true);

	if( g_pContext && cairo_status(g_pContext) == CAIRO_STATUS_SUCCESS )
	{
#if _USEGTK
#if !GTK_CHECK_VERSION(3,0,0)
//		cairo_reset_clip(g_pContext);
//		gdk_cairo_reset_clip(g_pContext, gtk_widget_get_window(gRun.pMainWindow));
		gdk_cairo_reset_clip(g_pContext, g_pClockWindow);
#endif
#endif
		cairo_identity_matrix(g_pContext);
		render_dsktp(g_pContext, gCfg.clockW, gCfg.clockH, false);
#ifdef _DEBUGLOG
		static int ct = 0;
		if( ++ct < 100 )
		{
			double x1, y1, x2, y2;
			cairo_clip_extents(g_pContext, &x1, &y1, &x2, &y2);
			DEBUGLOGPF("(1:1) clip=(%d, %d, %d, %d)\n", (int)x1, (int)y1, (int)x2, (int)y2);
		}
#endif
		ret = g_pRender(g_pContext, clear);
	}
	else // assume the stored context is invalid and step on it with a new one
	{
		DEBUGLOGS("stored cairo context status != success - replacing it");

		if( g_pContext )
			cairo_destroy(g_pContext);

		// TODO: is mt-threaded gdk locking needed here or is LockContext enough?

		if( lock )
			g_sync_threads_gui_beg();

		DEBUGLOGS("creating/storing a new drawing context");
#if _USEGTK
//		g_pContext = gdk_cairo_create(gtk_widget_get_window(gRun.pMainWindow));
//		g_pContext = gdk_cairo_create(g_pClockWindow);
		g_pContext = cairo_context(g_pClockWindow);
#else
		g_pContext = cairo_context(gRun.pMainWindow);
#endif
		if( lock )
			g_sync_threads_gui_end();

//		if( g_pContext && cairo_status(g_pContext) == CAIRO_STATUS_SUCCESS )
		if( g_pContext )
		{
#if _USEGTK
#if !GTK_CHECK_VERSION(3,0,0)
//			cairo_reset_clip(g_pContext);
//			gdk_cairo_reset_clip(g_pContext, gtk_widget_get_window(gRun.pMainWindow));
			gdk_cairo_reset_clip(g_pContext, g_pClockWindow);
#endif
#endif
			cairo_identity_matrix(g_pContext);
			render_dsktp(g_pContext, gCfg.clockW, gCfg.clockH, false);
#ifdef _DEBUGLOG
			double x1, y1, x2, y2;
			cairo_clip_extents(g_pContext, &x1, &y1, &x2, &y2);
			DEBUGLOGPF("(1:2) clip=(%d, %d, %d, %d)\n", (int)x1, (int)y1, (int)x2, (int)y2);
#endif
			ret = g_pRender(g_pContext, clear);
		}
	}

	draw::lock(false);

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
int draw::render(PWidget* pWidget, bool clear, bool lock)
{
	DEBUGLOGB;

	int ret = 0;

	if( pWidget )
	{
		draw::lock(true);

		// TODO: is mt-threaded gdk locking needed here or is LockContext enough?

		if( lock )
			g_sync_threads_gui_beg();

		DEBUGLOGS("creating/storing a new drawing context");
//#if _USEGTK
//		cairo_t* pContext = gdk_cairo_create(gtk_widget_get_window(pWidget));
//#else
//		cairo_t* pContext = NULL;
//#endif
		cairo_t* pContext = cairo_context(pWidget);

		if( lock )
			g_sync_threads_gui_end();

//		if( pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS )
		if( pContext )
		{
#if _USEGTK
#if !GTK_CHECK_VERSION(3,0,0)
//			cairo_reset_clip(pContext);
//			gdk_cairo_reset_clip(pContext, gtk_widget_get_window(pWidget));
#endif
#endif
//			cairo_identity_matrix(pContext);
			render_dsktp(pContext, gCfg.clockW, gCfg.clockH, false);
#ifdef _DEBUGLOG
			double x1, y1, x2, y2;
			cairo_clip_extents(pContext, &x1, &y1, &x2, &y2);
			DEBUGLOGPF("(2:1) clip=(%d, %d, %d, %d)\n", (int)x1, (int)y1, (int)x2, (int)y2);
#endif
			ret = g_pRender(pContext, clear);

			cairo_destroy(pContext);
		}

		draw::lock(false);
	}
	else
	{
		ret = render(clear);
	}

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
int draw::render(cairo_t* pContext, bool clear)
{
	DEBUGLOGB;

	int ret = 0;

	draw::lock(true);

	if( pContext )
	{
#if _USEGTK
#if !GTK_CHECK_VERSION(3,0,0)
//		cairo_reset_clip(pContext);
//		gdk_cairo_reset_clip(pContext, gtk_widget_get_window(gRun.pMainWindow));
//		gdk_cairo_reset_clip(pContext, g_pClockWindow);
#endif
#endif
//		cairo_identity_matrix(pContext);
		render_dsktp(pContext, gCfg.clockW, gCfg.clockH, false);
#ifdef _DEBUGLOG
		double x1, y1, x2, y2;
		cairo_clip_extents(pContext, &x1, &y1, &x2, &y2);
		DEBUGLOGPF("(3:1) clip=(%d, %d, %d, %d)\n", (int)x1, (int)y1, (int)x2, (int)y2);
#endif
		ret = g_pRender(pContext, clear);
	}
	
	draw::lock(false);

	DEBUGLOGE;
	return ret;
}
#if 0
#if _USEGTK
// -----------------------------------------------------------------------------
int draw::render(PWidget* pWidget, bool forceDraw)
{
	static cairo_matrix_t g_mat;
	static bool           g_bboxZero;

	int ret = 0;

	DEBUGLOGP("(%d) entry\n", 2);

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
		const double a2r =  M_PI/180.0;           // angle to radians conversion factor
//		const double cww =  gCfg.clockW;          // clock window width
//		const double cwh =  gCfg.clockH;          // clock window height
		const double ang =  gRun.angleSec;        // second hand rotation angle
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
			gdk_window_invalidate_rect(gtk_widget_get_window(pWidget), &ir, FALSE);

//			ret = render(pWidget, gRun.drawScaleX, gRun.drawScaleY, gRun.appStart);

			cairo_t* pContext = gdk_cairo_create(gtk_widget_get_window(pWidget));

			if( pContext )
			{
				ret = render(pContext, gRun.drawScaleX, gRun.drawScaleY, gCfg.clockW, gCfg.clockH, gRun.appStart);
				cairo_destroy(pContext);
			}

			GdkRegion* pRegion  = gdk_window_get_update_area(gtk_widget_get_window(pWidget));
			if(        pRegion )  gdk_region_destroy(pRegion);
		}
	}

	DEBUGLOGP("(%d) exit\n", 2);
	return ret;
}
#endif // _USEGTK
#endif
#if 0
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#define renderlog() \
	static int ct = 0; \
/*	if( ct < 10 ) DEBUGLOGPF("rendering(%d)\n", ++ct);*/ \
	DEBUGLOGPF("rendering(%d)\n", ++ct);
#else
#define renderlog()
#endif

// -----------------------------------------------------------------------------
int draw::render1a(cairo_t* pContext, bool clear)
{
	DEBUGLOGB;
	renderlog();

	int ret = renderbeg(pContext);

	LockAllSurfBeg();

	cairo_set_matrix(pContext, &g_rendrxfB.matrix);
//	render_surf     (pContext, SURF_BKGND, CAIRO_OPERATOR_SOURCE, CAIRO_OPERATOR_CLEAR);
	render_surf     (pContext, SURF_BKGND, CAIRO_OPERATOR_OVER,   CAIRO_OPERATOR_OVER);
	render_hand_hl  (pContext,  g_almHand, false);
//	render_hand_hl  (pContext,  g_tznHand, false);
	render_hand_hl  (pContext,  g_horHand, false);
	render_hand_hl  (pContext,  g_minHand, false);
	render_hand_hl  (pContext,  g_secHand, false);
	cairo_set_matrix(pContext, &g_rendrxfB.matrix);
	render_surf     (pContext, SURF_FRGND, CAIRO_OPERATOR_OVER);

	LockAllSurfEnd();

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
int draw::render1b(cairo_t* pContext, bool clear)
{
	DEBUGLOGB;
	renderlog();

	int ret = renderbeg(pContext);

	LockAllSurfBeg();

//	if( gCfg.handsOnly )
//	{
//		cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
//		cairo_paint(pContext);
//	}

	cairo_set_matrix(pContext, &g_rendrxfB.matrix);
#if 0
	if( !gCfg.handsOnly )
	{
		if( clear )
		{
//			render_surf(pContext, SURF_BKGND, CAIRO_OPERATOR_SOURCE, CAIRO_OPERATOR_CLEAR);
			render_surf(pContext, SURF_BKGND, CAIRO_OPERATOR_OVER,   CAIRO_OPERATOR_OVER);
		}
		else
		{
//			render_surf(pContext, SURF_BKGND, CAIRO_OPERATOR_SOURCE, CAIRO_OPERATOR_OVER);
			render_surf(pContext, SURF_BKGND, CAIRO_OPERATOR_OVER,   CAIRO_OPERATOR_OVER);
		}
	}
#endif
	if( !gCfg.handsOnly )
		render_surf(pContext, SURF_BKGND, CAIRO_OPERATOR_OVER, CAIRO_OPERATOR_OVER);

	if( gCfg.showAlarms )
		render_hand_hl(pContext, g_almHand, false);
#if 0
	if( gCfg.showHours )
		render_hand_hl(pContext, g_tznHand, false);
#endif
	if( gCfg.showHours )
		render_hand_hl(pContext, g_horHand, false);

	if( gCfg.showMinutes )
		render_hand_hl(pContext, g_minHand, false);

	if( gCfg.showSeconds )
		render_hand_hl(pContext, g_secHand, false);

	cairo_set_matrix(pContext, &g_rendrxfB.matrix);

	if( !gCfg.handsOnly )
		render_surf(pContext, SURF_FRGND, CAIRO_OPERATOR_OVER);

	LockAllSurfEnd();

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
int draw::render2(cairo_t* pContext, bool clear)
{
	DEBUGLOGB;
	DEBUGLOGS("was told renderIt is off, so nothing to do");
	renderlog();

	int ret = renderbeg(pContext);

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
int draw::render3(cairo_t* pContext, bool clear)
{
	DEBUGLOGB;
	DEBUGLOGS("was told noDisplay is on, so just clearing");
	renderlog();

	int ret = renderbeg(pContext);

//	cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
//	cairo_paint(pContext);

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
int draw::render4(cairo_t* pContext, bool clear)
{
	DEBUGLOGB;
	DEBUGLOGS("was told textOnly is on, so just clearing & drawing text");
	renderlog();

	int ret = renderbeg(pContext);

	LockAllSurfBeg();

	cairo_set_matrix(pContext, &g_rendrxfB.matrix);
//	render_surf(pContext, gCfg.faceDate ? SURF_BKGND : SURF_FRGND, CAIRO_OPERATOR_SOURCE, CAIRO_OPERATOR_CLEAR);
	render_surf(pContext, gCfg.faceDate ? SURF_BKGND : SURF_FRGND, CAIRO_OPERATOR_OVER,   CAIRO_OPERATOR_OVER);

	LockAllSurfEnd();

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
int draw::render5(cairo_t* pContext, bool clear)
{
	DEBUGLOGB;
	DEBUGLOGS("was told handsOnly is on, so just clearing & drawing hands");
	renderlog();

	int ret = renderbeg(pContext);

	LockAllSurfBeg();

//	bool evalDraws = !gRun.appStart && gRun.evalDraws;
	bool evalDraws = false;

//	cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
//	cairo_paint(pContext);

	if( gCfg.showAlarms )
		render_hand_hl(pContext, g_almHand, evalDraws);
#if 0
	if( gCfg.showHours )
		render_hand_hl(pContext, g_tznHand, evalDraws);
#endif
	if( gCfg.showHours )
		render_hand_hl(pContext, g_horHand, evalDraws);

	if( gCfg.showMinutes )
		render_hand_hl(pContext, g_minHand, evalDraws);

	if( gCfg.showSeconds )
		render_hand_hl(pContext, g_secHand, evalDraws);

	LockAllSurfEnd();

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
int draw::render6(cairo_t* pContext, bool clear)
{
	DEBUGLOGB;
	DEBUGLOGS("was told evalDraws is on, so drawing normally then drawing eval rects");
	renderlog();

	int ret = render1b(pContext, clear);

	if( !gRun.appStart && !gRun.updating )
	{
		cairo_rectangle_list_t* pRects = cairo_copy_clip_rectangle_list(pContext);

		if( pRects && pRects->status == CAIRO_STATUS_SUCCESS )
		{
			int nr = pRects->num_rectangles;

			if( nr > 0 )
			{
//				double xb, yb, xe, ye;

				cairo_new_path(pContext);
//				cairo_set_line_width(pContext, 0.25);
				cairo_set_line_width(pContext, 1);
//				cairo_clip_extents(pContext, &xb, &yb, &xe, &ye);
				cairo_set_source_rgba(pContext, 1, 0, 0, 0.75);
				cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);
//				cairo_rectangle(pContext, xb+1, yb+1, xe-xb-2, ye-yb-2);

				for( int r = 0; r < nr; r++ )
					cairo_rectangle(pContext, pRects->rectangles[r].x+1, pRects->rectangles[r].y+1, pRects->rectangles[r].width-2, pRects->rectangles[r].height-2);

//				DEBUGLOGS("drawing eval rects");
				cairo_stroke(pContext);
			}

			cairo_rectangle_list_destroy(pRects);
		}
#if 0
		if( g_pMaskRgn )
//		if( g_nMaskCnt && g_pMaskBeg && g_pMaskEnd )
		{
			cairo_rectangle_int_t  cr;
			int    nr     =  cairo_region_num_rectangles(g_pMaskRgn);
			double wratio = (double)gCfg.clockW/(double)g_svgClockW;
			double hratio = (double)gCfg.clockH/(double)g_svgClockH;

			cairo_set_matrix(pContext, &g_rendrxfB.matrix);
			cairo_scale     (pContext, wratio, hratio);

			cairo_new_path(pContext);
//			cairo_set_line_width(pContext, 1);
			cairo_set_line_width(pContext, 1.0/wratio);
			cairo_set_source_rgba(pContext, 0, 0, 1, 0.75);
			cairo_set_operator(pContext, CAIRO_OPERATOR_OVER);

			static bool first = true;
			if( first )
			{
				first = false;
				cairo_region_get_rectangle(g_pMaskRgn, 0, &cr);
				DEBUGLOGP("r1top (%d, %d)\n", (int)(cr.x*wratio), (int)(cr.y*hratio));
			}

			for( int r = 0; r < nr; r++ )
			{
				cairo_region_get_rectangle(g_pMaskRgn, r, &cr);
//				cairo_rectangle(pContext, cr.x*wratio, cr.y*hratio, cr.width*wratio, cr.height*hratio);
				cairo_rectangle(pContext, cr.x, cr.y, cr.width, cr.height);
			}

			cairo_stroke(pContext);

			cairo_new_path(pContext);
			cairo_set_source_rgba(pContext, 1, 0, 1, 0.75);

			for( int r = 0; r < nr; r++ )
			{
				cairo_region_get_rectangle(g_pMaskRgn, r, &cr);
//				cairo_move_to(pContext, cr.x*wratio,                 cr.y*hratio);
//				cairo_line_to(pContext, cr.x*wratio+cr.width*wratio, cr.y*hratio);
				cairo_move_to(pContext, cr.x,          cr.y);
				cairo_line_to(pContext, cr.x+cr.width, cr.y);
			}

			cairo_stroke(pContext);

			DEBUGLOGE;
			return ret;

			int b = -1, e = -1;

			DEBUGLOGS("bef mask eval find 1st scan line");

			if( g_pMaskRgn )
			{
				b = 0;
			}
#if 0
			else
			{
				for( int r = 0; r < g_nMaskCnt; r++ )
				{
					if( g_pMaskBeg[r] != MASKBEGBAD && g_pMaskEnd[r] != MASKENDBAD )
					{
						b  = r;
						break;
					}
				}
			}
#endif
			DEBUGLOGS("aft mask eval find 1st scan line");

			if( b != -1 )
			{
				double wratio = (double)gCfg.clockW/(double)g_svgClockW;
				double hratio = (double)gCfg.clockH/(double)g_svgClockH;

				// TODO: use what render_set uses to handle anim, resizing, rotation, ...

//				cairo_set_matrix(pContext, &g_rendrxfB.matrix);
//				cairo_scale     (pContext, wratio, hratio);

				cairo_new_path(pContext);
//				cairo_set_line_width(pContext, 1.0/wratio);
				cairo_set_source_rgba(pContext, 0, 1, 0, 0.75);
				cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);

				if( g_pMaskRgn )
				{
					cairo_region_get_rectangle(g_pMaskRgn, b, &cr);
					cairo_move_to(pContext, cr.x,          cr.y);
					cairo_line_to(pContext, cr.x+cr.width, cr.y);
					cairo_line_to(pContext, cr.x+cr.width, cr.y+cr.height);
				}
#if 0
				else
				{
					cairo_move_to(pContext, g_pMaskBeg[b], b);
					cairo_line_to(pContext, g_pMaskEnd[b], b);
					nr = g_nMaskCnt;
				}
#endif
				DEBUGLOGS("bef mask eval draw mask scan line outline right side");

				for( int r = b+1; r < nr; r++ )
				{
					if( g_pMaskRgn )
					{
						cairo_region_get_rectangle(g_pMaskRgn, r, &cr);
						cairo_line_to(pContext, cr.x+cr.width, cr.y);
						cairo_line_to(pContext, cr.x+cr.width, cr.y+cr.height);
						e = r;
					}
#if 0
					else
					{
						if( g_pMaskBeg[r] != MASKBEGBAD && g_pMaskEnd[r] != MASKENDBAD )
						{
							cairo_line_to(pContext, g_pMaskEnd[r], r);
							e = r;
						}
					}
#endif
				}

				DEBUGLOGS("aft mask eval draw mask scan line outline right side");

				if( e != -1 )
				{
					if( g_pMaskRgn )
					{
						cairo_region_get_rectangle(g_pMaskRgn, e, &cr);
						cairo_line_to(pContext, cr.x+cr.width, cr.y+cr.height);
						cairo_line_to(pContext, cr.x, cr.y+cr.height);
						cairo_line_to(pContext, cr.x, cr.y);
					}
#if 0
					else
					{
						cairo_line_to(pContext, g_pMaskBeg[e], e);
					}
#endif
					DEBUGLOGS("bef mask eval draw mask scan line outline left side");

					for( int r = e-1; r >= b; r-- )
					{
						if( g_pMaskRgn )
						{
							cairo_region_get_rectangle(g_pMaskRgn, r, &cr);
							cairo_line_to(pContext, cr.x, cr.y+cr.height);
							cairo_line_to(pContext, cr.x, cr.y);
						}
#if 0
						else
						{
							if( g_pMaskBeg[r] != MASKBEGBAD && g_pMaskEnd[r] != MASKENDBAD )
								cairo_line_to(pContext, g_pMaskBeg[r], r);
						}
#endif
					}

					DEBUGLOGS("aft mask eval draw mask scan line outline left side");
				}

				DEBUGLOGS("bef mask eval draw mask scan line outline drawing");
				cairo_stroke(pContext);
			}
		}
#endif
	}

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
int draw::renderbeg(cairo_t* pContext, bool lock)
{
	DEBUGLOGB;

	GTimeVal            tval;
	g_get_current_time(&tval);

#if 0
	// test for rendering the clock at diff speeds than 'real' time
	//
	// must also turn on surf use for the hands, in the cfg, or
	// somewhere prior to use here, so the 'constant' vars are set
	// (hand surfs should always be created regardless)
	//
	// also helps to smooth out the ani by upping the render rate
	// and to use direct rather than queued rendering func calls
	//
	// the time chg here does not affect the clock face date, alarms, etc.
	{
	gint64 spdup  =  60*(-10); // multiplier may be negative - 60 makes it secs of speed up (60*30 == 1/2 min of speed up per 1 sec of 'real' time)
	gint64 toms   =  1000000;
	gint64 ms64   = (tval.tv_sec*toms + tval.tv_usec)*spdup;
	gint64 tvsec  =  ms64 / toms;
	tval.tv_sec   =  tvsec;
	tval.tv_usec  =  ms64 -(toms*tvsec);
	}
#endif

	// tz...GmtOffs are offsets from GMT time to system local timezone time
	// tz...ShoOffs are offsets from local time to user requested display timezone time

	long   tvsec  = (tval.tv_sec %    60);
//	long   tvmin  = (tval.tv_sec %  3600) /   60;
//	long   tvmin  =((tval.tv_sec %  3600) /   60 + gCfg.tzMinOff) %   60;
//	long   tvhor  =((tval.tv_sec % 86400) / 3600 + gCfg.tzHorOff) % 3600;
	long   tvmin  =((tval.tv_sec %  3600) /   60 + gCfg.tzMinGmtOff+gCfg.tzMinShoOff) %   60;
//	long   tvhor  =((tval.tv_sec % 86400) / 3600 + gCfg.tzHorGmtOff+gCfg.tzHorShoOff) % 3600;

	double angAlm =  gRun.angleAlm;
	double angHor =  gRun.angleHor;
//	double angHor =  double(tvhor)*30.0;
	double angMin =  double(tvmin)* 6.0;
	double angSec =  double(tvsec)* 6.0;

//	bool   adjHor =  false;
	bool   adjMin =  tvsec >= 59;
//	bool   adjSec =  true;
	double angAdj =  double(tval.tv_usec % 1000000)*1.0e-6;

	g_angAdjX     =  angAdj;
	g_angAdjY     =  hand_anim_get(angAdj);
	angAdj        =  g_angAdjY*6.0;

	if( false ) // TODO: replace w/cfg item for 'apply secs to min/hor hands'
	{
//		adjHor    =  false;
		adjMin    =  false;
		angMin   += (angSec+angAdj)/60.0;
//		angHor   +=  angMin/12.0;
	}

//	angHor       +=  angMin/12.0;

//	gRun.alarm    =  tvalm;
//	gRun.hours    =  tvhor;  // NOTE: this is calc'd in 1/2 sec timer
	gRun.minutes  =  tvmin;
	gRun.seconds  =  tvsec;
#if 0
//	gRun.angleAlm =  angAlm;
	gRun.angleHor =  angHor; // NOTE: this is calc'd in 1/2 sec timer
	gRun.angleMin =  angMin + (adjMin ? angAdj : 0);
	gRun.angleSec =  angSec +  angAdj;
#else
//	angAlm       +=  0;
//	angHor       +=  0;
	angMin       +=  adjMin ? angAdj : 0;
	angSec       +=  angAdj;

//	gRun.angleAlm =  angAlm;
//	gRun.angleHor =  angHor;
	gRun.angleMin =  angMin;
	gRun.angleSec =  angSec;
#endif
#if 0
	g_almHand.transfrm[0].rotate = deg2Rad(gRun.angleAlm);
	// 2nd timezone hour hand angle = circle-in-degs / hours-per-circle * hours-offset-from-local
//	g_tznHand.transfrm[0].rotate = deg2Rad(gRun.angleHor+360.0/12.0*5.0);
	g_horHand.transfrm[0].rotate = deg2Rad(gRun.angleHor);
	g_minHand.transfrm[0].rotate = deg2Rad(gRun.angleMin);
	g_secHand.transfrm[0].rotate = deg2Rad(gRun.angleSec);
#else
	g_almHand.transfrm[0].rotate = deg2Rad(angAlm);
	// 2nd timezone hour hand angle = circle-in-degs / hours-per-circle * hours-offset-from-local
//	g_tznHand.transfrm[0].rotate = deg2Rad(angHor+360.0/12.0*5.0);
	g_horHand.transfrm[0].rotate = deg2Rad(angHor);
	g_minHand.transfrm[0].rotate = deg2Rad(angMin);
	g_secHand.transfrm[0].rotate = deg2Rad(angSec);
#endif
#if 0
	g_almHand.transfrm[1].rotate = deg2Rad(gRun.angleAlm);
	// 2nd timezone hour hand angle = circle-in-degs / hours-per-circle * hours-offset-from-local
//	g_tznHand.transfrm[1].rotate = deg2Rad(gRun.angleHor+360.0/12.0*5.0);
	g_horHand.transfrm[1].rotate = deg2Rad(gRun.angleHor);
	g_minHand.transfrm[1].rotate = deg2Rad(gRun.angleMin);
	g_secHand.transfrm[1].rotate = deg2Rad(gRun.angleSec);
#else
	g_almHand.transfrm[1].rotate = g_almHand.transfrm[0].rotate;
	// 2nd timezone hour hand angle = circle-in-degs / hours-per-circle * hours-offset-from-local
//	g_tznHand.transfrm[1].rotate = g_tznHand.transfrm[0].rotate;
	g_horHand.transfrm[1].rotate = g_horHand.transfrm[0].rotate;
	g_minHand.transfrm[1].rotate = g_minHand.transfrm[0].rotate;
	g_secHand.transfrm[1].rotate = g_secHand.transfrm[0].rotate;
#endif
	// TODO: move this elsewhere
#if 0
	if( gRun.scrsaver )
	{
		cairo_reset_clip(pContext);
		cairo_set_source_rgba(pContext, gCfg.bkgndRed, gCfg.bkgndGrn, gCfg.bkgndBlu, 1);
		cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);
		cairo_paint(pContext);
		cairo_rectangle(pContext, 0, 0, gCfg.clockW, gCfg.clockH);
		cairo_clip(pContext);
	}
#endif
	// TODO: move this elsewhere

#if _USEGTK
	if( g_angChartDo && g_angChart )
	{
		// TODO: is mt-threaded gdk locking needed here or is LockContext enough?

		if( lock )
			g_sync_threads_gui_beg();

		if( GTK_IS_WIDGET(g_angChart) )
		{
//			gtk_widget_queue_draw(g_angChart);

			const double wh17 = g_angChartH*1.0/6.0;
			const double wh67 = g_angChartH*4.0/6.0;
			const int    x1   = g_angX1;
			const int    y1   = g_angY1;
			const int    x2   = g_angAdjX*g_angChartW;
			const int    y2   = g_angChartH-(int)(g_angAdjY*wh67)-wh17;
			const int    d    = 4;

			gtk_widget_queue_draw_area(g_angChart, x1-d, y1-d, 2*d, 2*d);
			gtk_widget_queue_draw_area(g_angChart, x2-d, y2-d, 2*d, 2*d);

			g_angX1    = x2;
			g_angY1    = y2;
		}
		else
			g_angChart = NULL;

		if( lock )
			g_sync_threads_gui_end();
	}
#endif // _USEGTK

	DEBUGLOGE;
	return tval.tv_usec;
}

// -----------------------------------------------------------------------------
void draw::render_set(PWidget* pWidget, bool full, bool lock, int clockW, int clockH)
{
	DEBUGLOGB;
	DEBUGLOGP("widget ptr: %p, lock: %s, clockW: %d, clockH: %d\n", pWidget, lock ? "on" : "off", clockW, clockH);

	XForm rxfB,  rxfH;
	const double anim_fract = 1.0 - gRun.animScale;
	const double hani_fract = 0.5 * anim_fract;

	rxfB.width  = clockW ? clockW : gCfg.clockW;
	rxfB.height = clockH ? clockH : gCfg.clockH;
	rxfB.scaleX = gRun.drawScaleX * gRun.animScale;
	rxfB.scaleY = gRun.drawScaleY * gRun.animScale;
	rxfB.xlateX = gRun.appStart   ? gCfg.clockW*hani_fract : 0;
	rxfB.xlateY = gRun.appStart   ? gCfg.clockH*hani_fract : 0;

	const double halfW = gCfg.clockW*0.5;
	const double halfH = gCfg.clockH*0.5;

	cairo_matrix_init_identity(&rxfB.matrix);
	cairo_matrix_translate    (&rxfB.matrix,  halfW,       halfH);
	cairo_matrix_rotate       (&rxfB.matrix,  deg2Rad(gCfg.clockR));
	cairo_matrix_translate    (&rxfB.matrix, -halfW,      -halfH);
	cairo_matrix_translate    (&rxfB.matrix,  rxfB.xlateX, rxfB.xlateY);
	cairo_matrix_scale        (&rxfB.matrix,  rxfB.scaleX, rxfB.scaleY);

	rxfH = rxfB;

	DEBUGLOGP("clock w/h(%d, %d), render x/y(%d, %d) w/h(%d, %d)\n",
		(int) rxfB.width,              (int) rxfB.height,
		(int) rxfB.xlateX,             (int) rxfB.xlateY,
		(int)(rxfB.scaleX*rxfB.width), (int)(rxfB.scaleY*rxfB.height));

	if( lock )
		draw::lock(true);

	g_rendrxfB = rxfB;
	g_rendrxfH = rxfH;

	render_hand_hl_chk_set(g_almHand);
//	render_hand_hl_chk_set(g_tznHand);
	render_hand_hl_chk_set(g_horHand);
	render_hand_hl_chk_set(g_minHand);
	render_hand_hl_chk_set(g_secHand);

	if( !lock && !full )
	{
		DEBUGLOGR(1);
		return;
	}

	if( pWidget )
	{
		DEBUGLOGS("creating/storing a new drawing context");

		if( g_pContext )
			cairo_destroy(g_pContext);

//		TODO: figure out if using this lock is okay here or if another one is needed

//		if( lock )
//			g_sync_threads_gui_beg();

#if _USEGTK
		g_pClockWindow = gtk_widget_get_window(pWidget);
//		g_pContext     = gdk_cairo_create(g_pClockWindow);
		g_pContext     = cairo_context(g_pClockWindow);
#else
		g_pContext     = cairo_context(pWidget);
#endif

//		if( g_pContext && cairo_status(g_pContext) == CAIRO_STATUS_SUCCESS )
		if( g_pContext )
		{
#if _USEGTK
#if !GTK_CHECK_VERSION(3,0,0)
//			cairo_reset_clip(g_pContext);
			gdk_cairo_reset_clip(g_pContext, gtk_widget_get_window(pWidget));
#endif
#endif
#ifdef _DEBUGLOG
			double x1, y1, x2, y2;
			cairo_clip_extents(g_pContext, &x1, &y1, &x2, &y2);
			DEBUGLOGP("clip=(%d, %d, %d, %d)\n", (int)x1, (int)y1, (int)x2, (int)y2);
#endif
		}

//		if( lock )
//			g_sync_threads_gui_end();

		DEBUGLOGP("new drawing context %s\n", g_pContext ? "created" : "not created");
	}

	if( gRun.renderIt )
	{
		if( gCfg.noDisplay )
		{
			DEBUGLOGS("'no display' rendering path");
			g_pRender = render3;
		}
		else
		{
			if( gCfg.textOnly )
			{
				DEBUGLOGS("'text only' rendering path");
				g_pRender = render4;
			}
			else
			{
				if( gCfg.handsOnly )
				{
					DEBUGLOGS("'hands only' rendering path");
					g_pRender = render5;
				}
				else
				{
					if( gRun.evalDraws )
					{
						DEBUGLOGS("'evaluate drawing' rendering path");
						g_pRender = render6;
					}
					else
					{
						if( gCfg.showAlarms  && gCfg.showHours && gCfg.showMinutes &&
							gCfg.showSeconds && g_rendrxfB.scaleX == 1.0 && g_rendrxfB.scaleY == 1.0 )
						{
							DEBUGLOGS("'fully optimized w/o skipping' rendering path");
							g_pRender = render1a;
						}
						else
						{
							DEBUGLOGS("'partially optimized' rendering path");
							g_pRender = render1b;
						}
					}
				}
			}
		}
	}
	else
	{
		DEBUGLOGS("'no rendering' rendering path");
		g_pRender = render2;
	}

	if( lock )
		draw::lock(false);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool draw::render_dsktp(cairo_t* pContext, int width, int height, bool yield, bool bright, bool compChk)
{
	DEBUGLOGB;

	// TODO: need to pass in a locking indicator here for mt-threaded use

//	bool okay    = false;
	bool compScr = gRun.composed;
	bool comp    = compChk ? compScr : true;

	DEBUGLOGP("clock dims: %d x %d\n", width, height);
	DEBUGLOGP("clock screen %s composited\n", compScr ? "is" : "is NOT");

	g_yield_thread(yield);

	if( comp )
	{
		DEBUGLOGS("treating clock screen as composited");

		cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
		cairo_paint(pContext);
	}
	else
	{
		DEBUGLOGS("treating clock screen as NOT composited");
#if 0
		SvgLockBeg(CLOCK_MASK);

		bool okay  = false;
		bool maskd = g_pSvgHandle[CLOCK_MASK] != NULL;

		if( maskd )
		{
			DEBUGLOGS("svg mask used for cbase bkgnd");

			cairo_save(pContext);
			cairo_scale(pContext, (double)width/(double)g_svgClockW, (double)height/(double)g_svgClockH);
//			cairo_scale(pContext, (double)width/(double)ds.svgClockW, (double)height/(double)ds.svgClockH);
			cairo_set_operator(pContext, bright ? CAIRO_OPERATOR_LIGHTEN : CAIRO_OPERATOR_OVER);

			okay = rsvg_handle_render_cairo(g_pSvgHandle[CLOCK_MASK], pContext) == TRUE;

			cairo_restore(pContext);
		}

		SvgLockEnd(CLOCK_MASK);
#endif
//		if( !okay )
		{
			if( g_pDesktop )
			{
				DEBUGLOGS("desktop pixbuf used for dsktp surf");
				cairo_set_source_surface(pContext, g_pDesktop, 0.0, 0.0);
			}
			else
			{
				DEBUGLOGS("bkgnd color used for dsktp surf");
				cairo_set_source_rgba(pContext, gCfg.bkgndRed, gCfg.bkgndGrn, gCfg.bkgndBlu, 1);
			}

			cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);
			cairo_paint(pContext);
		}
	}

	DEBUGLOGE;
//	return okay;
	return true;
}

// -----------------------------------------------------------------------------
bool draw::render_bbase(cairo_t* pContext, int width, int height, bool yield, bool bright)
//bool draw::render_bbase(cairo_t* pContext, const DrawSurf& ds, int width, int height, bool yield, bool bright)
{
	DEBUGLOGB;

	// TODO: need to pass in a locking indicator here for mt-threaded use

	bool okay = false;

	g_yield_thread(yield);

	cairo_save (pContext);
	cairo_scale(pContext, (double)width/(double)g_svgClockW,  (double)height/(double)g_svgClockH);
//	cairo_scale(pContext, (double)width/(double)ds.svgClockW, (double)height/(double)ds.svgClockH);
	cairo_set_operator(pContext, bright ? CAIRO_OPERATOR_LIGHTEN : CAIRO_OPERATOR_OVER);

	DEBUGLOGP("scaling width:  (%d, %d, %f)\n", (int)width,  (int)g_svgClockW,  (double)width /(double)g_svgClockW);
	DEBUGLOGP("scaling height: (%d, %d, %f)\n", (int)height, (int)g_svgClockH,  (double)height/(double)g_svgClockH);
//	DEBUGLOGP("scaling width:  (%d, %d, %f)\n", (int)width,  (int)ds.svgClockW, (double)width /(double)g_svgClockW);
//	DEBUGLOGP("scaling width:  (%d, %d, %f)\n", (int)height, (int)ds.svgClockH, (double)height/(double)g_svgClockH);

	// draw the lowest layer elements

	ClockElement      le;
//#if _USEGTK
	RsvgDimensionData ed;
//#endif
//	bool              compScr      =   gRun.composed;
	bool              use24        =   gCfg.show24Hrs && g_pSvgHandle[CLOCK_MARKS_24H] != NULL;
//	ClockElement      ceDropShadow =   compScr ? CLOCK_DROP_SHADOW : CLOCK_ELEMENTS;
	ClockElement      ceMarks      =   use24   ? CLOCK_MARKS_24H   : CLOCK_MARKS;
	ClockElement      ClockElems[] = { CLOCK_DROP_SHADOW, CLOCK_FACE, ceMarks };
//	ClockElement      ClockElems[] = { ceDropShadow, CLOCK_FACE, ceMarks };

	for( size_t e = 0; e < vectsz(ClockElems); e++ )
	{
//		if( (le = ClockElems[e]) == CLOCK_ELEMENTS )
//			continue;

		le = ClockElems[e];

		SvgLockBeg(le);

		if( g_pSvgHandle[le] )
		{
//#if _USEGTK
			rsvg_handle_get_dimensions(g_pSvgHandle[le], &ed);
			cairo_translate(pContext, (g_svgClockW -ed.width)*0.5, (g_svgClockH -ed.height)*0.5);
//			cairo_translate(pContext, (ds.svgClockW-ed.width)*0.5, (ds.svgClockH-ed.height)*0.5);
//#endif
			if( rsvg_handle_render_cairo(g_pSvgHandle[le], pContext) )
			{
				DEBUGLOGP("rendered element %d, xlate width (%d, %d)\n", (int)le, (int)g_svgClockW,  (int)ed.width);
//				DEBUGLOGP("rendered element %d, xlate width (%d, %d)\n", (int)le, (int)ds.svgClockW, (int)ed.width);
				okay = true;
			}
		}

		SvgLockEnd(le);

		g_yield_thread(yield);
	}

	cairo_restore(pContext);

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
bool draw::render_bkgnd(cairo_t* pContext, int width, int height, bool yield, bool bright, bool date, bool temp)
{
	DEBUGLOGB;

	cairo_save(pContext);

	SurfLockBeg(SURF_BBASE);

	DrawSurf& dsbb   = temp ? g_temps[SURF_BBASE] : g_surfs[SURF_BBASE];
	double    wratbb = temp ? 1 : (double)width /(double)dsbb.width;
	double    hratbb = temp ? 1 : (double)height/(double)dsbb.height;
	bool      dtOnly = gCfg.faceDate && gCfg.textOnly && date;
//	bool      okay   = dsbb.pCairoSurf != NULL && !(gCfg.faceDate && gCfg.textOnly && date);
	bool      okay   = dsbb.pCairoSurf != NULL && !dtOnly;
//	bool      okay   = dsbb.pCairoSurf != NULL;

	DEBUGLOGP("%s bbase widths (%d, %d, %2.2f)\n", temp ? "temp" : "surf", (int)width, (int)dsbb.width, (float)wratbb);

	cairo_scale(pContext, wratbb, hratbb);
	cairo_set_operator(pContext, okay ? CAIRO_OPERATOR_SOURCE : CAIRO_OPERATOR_CLEAR);
//	cairo_set_operator(pContext, okay ? CAIRO_OPERATOR_OVER : CAIRO_OPERATOR_CLEAR);

	if( okay )
	{
		DEBUGLOGS(" drawing bbase surf into bkgnd");
		cairo_set_source_surface(pContext, dsbb.pCairoSurf, 0.0, 0.0);
	}

	SurfLockEnd(SURF_BBASE);

	cairo_paint  (pContext);
	cairo_restore(pContext);

	g_yield_thread(yield);

	cairo_set_operator(pContext, bright ? CAIRO_OPERATOR_LIGHTEN : CAIRO_OPERATOR_OVER);

	if( gCfg.showDate && gCfg.faceDate && date )
	{
		DEBUGLOGS(" bef drawing date into bkgnd");
		render_date(pContext, width, height, yield, bright);
		DEBUGLOGS(" aft drawing date into bkgnd");
		g_yield_thread(yield);
		okay = true;
	}

	bool drawAlm = g_almHand.optBest && !g_almHand.useSurf;
	bool drawHor = g_horHand.optBest && !g_horHand.useSurf;
	bool drawMin = g_minHand.optBest && !g_minHand.useSurf;

	if( !gCfg.textOnly && (drawAlm || drawHor || drawMin) )
	{
		DEBUGLOGS("beg drawing req'd non-second hands into ctxt");
//		DEBUGLOGP("angleAlm/Hor/Min=%f, %f, %f\n", gRun.angleAlm, gRun.angleHor, gRun.angleMin);

		cairo_save(pContext);

		if( drawAlm )
		{
			DEBUGLOGS("drawing alarm hand into background surface");
			render_handalm(pContext, width, height, yield, false, true, true, true, true, true);
			okay = true;
		}

		if( drawHor )
		{
			DEBUGLOGS("drawing hour hand into background surface");
			render_handhor(pContext, width, height, yield, false, true, true, true, true, true);
			okay = true;
		}

		if( drawMin )
		{
			DEBUGLOGS("drawing minute hand into background surface");
			render_handmin(pContext, width, height, yield, false, true, true, true, true, true);
			okay = true;
		}

		cairo_restore(pContext);

		DEBUGLOGS("end drawing req'd non-second hands into ctxt");
	}

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
bool draw::render_chand(cairo_t* pContext, int handElem, int shadElem, int width, int height, bool bright, double hoff, bool center, double rotate)
{
	DEBUGLOGB;

	cairo_save(pContext);

	if( bright )
	{
		cairo_set_source_rgba(pContext, 1.0, 1.0, 1.0, 0.0); // transparent white
		cairo_set_operator(pContext, CAIRO_OPERATOR_LIGHTEN);
		cairo_paint(pContext);
	}

	double hoffX =  hoff;
	double hoffY = -hoff;
	double halfW =  center ? width *0.5 : width *0.5/g_xDiv;
	double halfH =  center ? height*0.5 : height*0.5/g_yDiv;

	cairo_translate(pContext, halfW, halfH); // rendered hand's origin is at the clock's center
	cairo_scale(pContext,  (double)width/(double)g_svgClockW, (double)height/(double)g_svgClockH);
	cairo_rotate(pContext, -M_PI*0.5);       // rendered hand's zero angle is at 12 o'clock

	if( rotate <  0.0 )     // indicates a clock hand shadow - TODO: change this to something more appropriate
	{
		hoffX  = -g_coffX; // zero sum the offsets since the offset has already been applied
		hoffY  = -g_coffY;
		rotate =  90.0;    // assumes rendering into a memory surface for later animation, so minimize needed area
	}

	bool okay  =  render_hand(pContext, handElem, shadElem, hoffX, hoffY, deg2Rad(rotate));

	cairo_restore(pContext);

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::render_date(cairo_t* pContext, int width, int height, bool yield, bool bright)
{
	DEBUGLOGB;

	if( gCfg.showDate )
	{
		double xb, yb, xe, ye;
		bool   evalDraws = !gRun.appStart && gRun.evalDraws;

		cairo_save(pContext);
		cairo_scale(pContext, (double)width/(double)g_svgClockW, (double)height/(double)g_svgClockH);
		cairo_translate(pContext, g_svgClockW*0.5, g_svgClockH*0.5);
#if 1
		if( !gCfg.faceDate ) // 'on top' text needs a shadow
		{
/*			double dateTxtRed = gCfg.dateTxtRed*gCfg.dateShdRed*gCfg.dateShdAlf;
			double dateTxtGrn = gCfg.dateTxtGrn*gCfg.dateShdGrn*gCfg.dateShdAlf;
			double dateTxtBlu = gCfg.dateTxtBlu*gCfg.dateShdBlu*gCfg.dateShdAlf;
			double dateTxtAlf = gCfg.dateTxtAlf*gCfg.dateShdAlf*gCfg.dateShdAlf*gCfg.dateShdAlf;
			double dateShdWid = gCfg.dateShdWid;*/

//			cairo_set_source_rgba(pContext, dateTxtRed, dateTxtGrn, dateTxtBlu, dateTxtAlf);
			cairo_set_source_rgba(pContext, 0.25, 0.25, 0.25, 0.25);

			for( size_t p = 0; p < vectsz(g_pTextPath); p+=2 )
			{
				if( g_pTextPath[p] )
				{
					cairo_new_path       (pContext);
					cairo_append_path    (pContext, g_pTextPath[p]);
//					cairo_set_source_rgba(pContext, dateTxtRed, dateTxtGrn, dateTxtBlu, dateTxtAlf);
//					cairo_fill_preserve  (pContext);
//					cairo_set_source_rgba(pContext, dateShdRed, dateShdGrn, dateShdBlu, dateShdAlf);
//					cairo_set_line_width (pContext, dateShdWid);
//					cairo_stroke         (pContext);
					cairo_fill           (pContext);
				}
			}
		}
#endif
		for( size_t p = 1; p < vectsz(g_pTextPath); p+=2 )
		{
			if( g_pTextPath[p] )
			{
				cairo_new_path       (pContext);
				cairo_append_path    (pContext, g_pTextPath[p]);

				if( evalDraws )
				cairo_path_extents   (pContext, &xb, &yb, &xe, &ye);

				cairo_set_source_rgba(pContext, gCfg.dateTxtRed, gCfg.dateTxtGrn, gCfg.dateTxtBlu, gCfg.dateTxtAlf);
				cairo_fill_preserve  (pContext);
				cairo_set_source_rgba(pContext, gCfg.dateShdRed, gCfg.dateShdGrn, gCfg.dateShdBlu, gCfg.dateShdAlf);
				cairo_set_line_width (pContext, gCfg.dateShdWid);
				cairo_stroke         (pContext);

				if( gRun.evalDraws )
					render_text_help(pContext, minv(xb, xe), maxv(yb, ye), abs(xe-xb), abs(ye-yb), 0, 0);
			}
		}

		cairo_restore(pContext);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::render_date_make(cairo_t* pContext, int width, int height, bool yield, bool bright)
{
	DEBUGLOGB;

	if( gCfg.showDate )
	{
		DEBUGLOGS("bef drawing date into the given context");

		cairo_save(pContext);
		cairo_scale(pContext, (double)width/(double)g_svgClockW, (double)height/(double)g_svgClockH);
		cairo_translate(pContext, g_svgClockW*0.5, g_svgClockH*0.5);

#if _USEPANGO
		if( g_pTextFontFace == NULL )
		{
			update_text_font(pContext);
			g_yield_thread(yield);
		}
#endif
		if( g_pTextOptions )
			cairo_set_font_options(pContext, g_pTextOptions);

		if( g_pTextFontFace )
			cairo_set_font_face   (pContext, g_pTextFontFace);

//		cairo_scale(pContext, gCfg.fontSize*0.1, gCfg.fontSize*0.1);
//		cairo_scale(pContext, gCfg.fontSize*0.1, 1);
#if 1
		DEBUGLOGP("date string  is *%s*\n", gRun.ttlDateTxt);
		DEBUGLOGP("time string  is *%s*\n", gRun.ttlTimeTxt);
		DEBUGLOGP("text string1 is *%s*\n", gRun.cfaceTxta3);
		DEBUGLOGP("text string2 is *%s*\n", gRun.cfaceTxta2);
		DEBUGLOGP("text string3 is *%s*\n", gRun.cfaceTxta1);
		DEBUGLOGP("text string4 is *%s*\n", gRun.cfaceTxtb1);
		DEBUGLOGP("text string5 is *%s*\n", gRun.cfaceTxtb2);
		DEBUGLOGP("text string6 is *%s*\n", gRun.cfaceTxtb3);
#endif
		static drawTextPath dtps[] = {
			{  0, gRun.cfaceTxta1, &gCfg.sfTxta1, &gCfg.hfTxta1, -1 },
			{  1, gRun.cfaceTxta1, &gCfg.sfTxta1, &gCfg.hfTxta1, -1 },
			{  2, gRun.cfaceTxta2, &gCfg.sfTxta2, &gCfg.hfTxta2, -1 },
			{  3, gRun.cfaceTxta2, &gCfg.sfTxta2, &gCfg.hfTxta2, -1 },
			{  4, gRun.cfaceTxta3, &gCfg.sfTxta3, &gCfg.hfTxta3, -1 },
			{  5, gRun.cfaceTxta3, &gCfg.sfTxta3, &gCfg.hfTxta3, -1 },
			{  6, gRun.cfaceTxtb1, &gCfg.sfTxtb1, &gCfg.hfTxtb1,  1 },
			{  7, gRun.cfaceTxtb1, &gCfg.sfTxtb1, &gCfg.hfTxtb1,  1 },
			{  8, gRun.cfaceTxtb2, &gCfg.sfTxtb2, &gCfg.hfTxtb2,  1 },
			{  9, gRun.cfaceTxtb2, &gCfg.sfTxtb2, &gCfg.hfTxtb2,  1 },
			{ 10, gRun.cfaceTxtb3, &gCfg.sfTxtb3, &gCfg.hfTxtb3,  1 },
			{ 11, gRun.cfaceTxtb3, &gCfg.sfTxtb3, &gCfg.hfTxtb3,  1 },
		};

		double ea1, ea2, ea3, eb1, eb2, eb3;

		if( !gCfg.faceDate ) // 'on top' text needs a shadow
		{
/*		ea1 = render_text_path(pContext, yield,  0, gRun.cfaceTxta1, gCfg.sfTxta1, gCfg.hfTxta1, 0,   -1, 1, 1);
		ea2 = render_text_path(pContext, yield,  2, gRun.cfaceTxta2, gCfg.sfTxta2, gCfg.hfTxta2, ea1, -1, 1, 1) + ea1;
		ea3 = render_text_path(pContext, yield,  4, gRun.cfaceTxta3, gCfg.sfTxta3, gCfg.hfTxta3, ea2, -1, 1, 1) + ea2;
		eb1 = render_text_path(pContext, yield,  6, gRun.cfaceTxtb1, gCfg.sfTxtb1, gCfg.hfTxtb1, 0,   +1, 1, 1);
		eb2 = render_text_path(pContext, yield,  8, gRun.cfaceTxtb2, gCfg.sfTxtb2, gCfg.hfTxtb2, eb1, +1, 1, 1) + eb1;
		eb3 = render_text_path(pContext, yield, 10, gRun.cfaceTxtb3, gCfg.sfTxtb3, gCfg.hfTxtb3, eb2, +1, 1, 1) + eb2;*/
		ea1 = render_text_path(pContext, yield, dtps[ 0], 0,   1, 1);
		ea2 = render_text_path(pContext, yield, dtps[ 2], ea1, 1, 1) + ea1;
		ea3 = render_text_path(pContext, yield, dtps[ 4], ea2, 1, 1) + ea2;
		eb1 = render_text_path(pContext, yield, dtps[ 6], 0,   1, 1);
		eb2 = render_text_path(pContext, yield, dtps[ 8], eb1, 1, 1) + eb1;
		eb3 = render_text_path(pContext, yield, dtps[10], eb2, 1, 1) + eb2;
		}
//		                                                                                         prev
//		                                                             size factor   hite factor   off  dir
/*		ea1 = render_text_path(pContext, yield,  1, gRun.cfaceTxta1, gCfg.sfTxta1, gCfg.hfTxta1, 0,   -1, 0, 0);
		ea2 = render_text_path(pContext, yield,  3, gRun.cfaceTxta2, gCfg.sfTxta2, gCfg.hfTxta2, ea1, -1, 0, 0) + ea1;
		ea3 = render_text_path(pContext, yield,  5, gRun.cfaceTxta3, gCfg.sfTxta3, gCfg.hfTxta3, ea2, -1, 0, 0) + ea2;
		eb1 = render_text_path(pContext, yield,  7, gRun.cfaceTxtb1, gCfg.sfTxtb1, gCfg.hfTxtb1, 0,   +1, 0, 0);
		eb2 = render_text_path(pContext, yield,  9, gRun.cfaceTxtb2, gCfg.sfTxtb2, gCfg.hfTxtb2, eb1, +1, 0, 0) + eb1;
		eb3 = render_text_path(pContext, yield, 11, gRun.cfaceTxtb3, gCfg.sfTxtb3, gCfg.hfTxtb3, eb2, +1, 0, 0) + eb2;*/
		ea1 = render_text_path(pContext, yield, dtps[ 1], 0,   0, 0);
		ea2 = render_text_path(pContext, yield, dtps[ 3], ea1, 0, 0) + ea1;
		ea3 = render_text_path(pContext, yield, dtps[ 5], ea2, 0, 0) + ea2;
		eb1 = render_text_path(pContext, yield, dtps[ 7], 0,   0, 0);
		eb2 = render_text_path(pContext, yield, dtps[ 9], eb1, 0, 0) + eb1;
		eb3 = render_text_path(pContext, yield, dtps[11], eb2, 0, 0) + eb2;

		cairo_restore(pContext);

		DEBUGLOGS("aft drawing date into the given context");
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
//double draw::render_text_path(cairo_t* pContext, bool yield, int index, const char* text, double sizeFact, double hiteFact, double hiteAddO, double hiteSign, double xoff, double yoff)
double draw::render_text_path(cairo_t* pContext, bool yield, const drawTextPath& dtp, double hiteAddO, double xoff, double yoff)
{
//	if( !text ) // *text is allowed to be null to support creating a new 'empty' path
	if( !dtp.text ) // *text is allowed to be null to support creating a new 'empty' path
		return 0;

	cairo_text_extents_t ext;
	double               extH = 0;
	double               sizeFact = *dtp.sizeFact;
	double               hiteFact = *dtp.hiteFact;

	cairo_save(pContext);
	cairo_set_font_size(pContext, gCfg.fontSize*sizeFact);
//	cairo_text_extents (pContext, text, &ext);
	cairo_text_extents (pContext, dtp.text, &ext);

	double xb =  ext.x_bearing;
	double yb =  ext.y_bearing;
	double w  =  ext.width;
	double h  =  ext.height;
	double x  = -w*0.5;         // center the text on the face
	double y  =  h*0.5;         //

/*	y   += hiteAddO*  hiteSign; // add in prev text's offset
	y   += h*         hiteSign; // add in this text's height
	y   += h*hiteFact*hiteSign; // add in this text's offset*/
	y   += hiteAddO*  dtp.hiteSign; // add in prev text's offset
	y   += h*         dtp.hiteSign; // add in this text's height
	y   += h*hiteFact*dtp.hiteSign; // add in this text's offset

	x   += xoff;
	y   += yoff;

	extH = h*0.5 + h + h*hiteFact;

	cairo_new_path (pContext);
	cairo_move_to  (pContext, x, y);
//	cairo_text_path(pContext, text);
	cairo_text_path(pContext, dtp.text);

/*	if( g_pTextPath[index] )
		cairo_path_destroy(g_pTextPath[index]);
	g_pTextPath[index] = cairo_copy_path(pContext);*/
	if( g_pTextPath[dtp.index] )
		cairo_path_destroy(g_pTextPath[dtp.index]);
	g_pTextPath[dtp.index] = cairo_copy_path(pContext);

	cairo_new_path (pContext);
	cairo_restore  (pContext);

	g_yield_thread (yield);

	return extH;
}

// -----------------------------------------------------------------------------
void draw::render_text_help(cairo_t* pContext, double xbl, double ybl, double w, double h, double xb, double yb)
{
	double x = xbl, y = ybl;

	cairo_save           (pContext);

	cairo_set_line_width (pContext, 0.2);
	cairo_set_source_rgba(pContext, 1.0,  0.0, 0.0, 1.00);
	cairo_arc            (pContext, x, y, 0.4, 0, 2*M_PI); // ll corner
	cairo_fill           (pContext);

	cairo_set_line_width (pContext, 0.2);
	cairo_set_source_rgba(pContext, 1.0,  0.2, 0.2, 0.75);
	cairo_move_to        (pContext, x, y); // to ll corner
	cairo_rel_line_to    (pContext, 0,-h); // to ul corner
	cairo_rel_line_to    (pContext, w, 0); // to ur corner
	cairo_rel_line_to    (pContext, 0, h); // to lr corner
	cairo_rel_line_to    (pContext,-w, 0); // to ll corner
	cairo_stroke         (pContext);

	cairo_restore        (pContext);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool draw::render_frgnd(cairo_t* pContext, int width, int height, bool yield, bool bright, bool date, bool temp)
{
	DEBUGLOGB;

	bool okay = false;

//	cairo_scale(pContext, (double)width/(double)g_svgClockW, (double)height/(double)g_svgClockH);
	cairo_set_source_rgba(pContext, 1.0, 1.0, 1.0, 0.0); // transparent white
	cairo_set_operator(pContext, bright ? CAIRO_OPERATOR_LIGHTEN : CAIRO_OPERATOR_OVER);
	cairo_paint(pContext);

//	DEBUGLOGP("scaling set (%d, %d)\n", (int)width, (int)g_svgClockW);

	// draw the upper layer elements

//	if( gCfg.showDate && !gCfg.faceDate )
	if( gCfg.showDate && !gCfg.faceDate && date )
	{
		DEBUGLOGS(" bef drawing date into frgnd");
		render_date(pContext, width, height, yield, bright);
		DEBUGLOGS(" aft drawing date into frgnd");
		g_yield_thread(yield);
		okay = true;
	}

	if( !gCfg.textOnly )
	{
/*		ClockElement      le;
		RsvgDimensionData ed;

		static const ClockElement ClockElems[] = { CLOCK_FACE_SHADOW, CLOCK_GLASS, CLOCK_FRAME };

		cairo_save (pContext);
		cairo_scale(pContext, (double)width/(double)g_svgClockW, (double)height/(double)g_svgClockH);

		for( size_t e = 0; e < vectsz(ClockElems); e++ )
		{
			le = ClockElems[e];

			SvgLockBeg(le);

			if( g_pSvgHandle[le] )
			{
				DEBUGLOGP(" bef drawing %d svg element\n", le);
				rsvg_handle_get_dimensions(g_pSvgHandle[le], &ed);
				cairo_translate(pContext, (g_svgClockW-ed.width)*0.5, (g_svgClockH-ed.height)*0.5);

				if( rsvg_handle_render_cairo(g_pSvgHandle[le], pContext) )
				{
//					DEBUGLOGP("successfully rendered clock element %d\n", (int)le);
					okay = true;
				}
				DEBUGLOGP(" aft drawing %d svg element\n", le);
			}

			SvgLockEnd(le);

			g_yield_thread(yield);
		}

		cairo_restore(pContext);*/

		cairo_save(pContext);

		SurfLockBeg(SURF_FCOVR);

		DrawSurf& dsfc   = temp ? g_temps[SURF_FCOVR] : g_surfs[SURF_FCOVR];
		double    wratfc = temp ? 1 : (double)width /(double)dsfc.width;
		double    hratfc = temp ? 1 : (double)height/(double)dsfc.height;
		bool      dtOnly = !gCfg.faceDate && gCfg.textOnly && date;
//		bool      okay   = dsfc.pCairoSurf != NULL && !(!gCfg.faceDate && gCfg.textOnly && date);
		bool      okay   = dsfc.pCairoSurf != NULL && !dtOnly;
//		bool      okay   = dsfc.pCairoSurf != NULL;
#if 0
	bool      dtOnly = gCfg.faceDate && gCfg.textOnly && date;
//	bool      okay   = dsbb.pCairoSurf != NULL && !(gCfg.faceDate && gCfg.textOnly && date);
	bool      okay   = dsbb.pCairoSurf != NULL && !dtOnly;
#endif
		DEBUGLOGP("%s fcovr widths (%d, %d, %2.2f)\n", temp ? "temp" : "surf", (int)width, (int)dsfc.width, (float)wratfc);

		cairo_scale(pContext, wratfc, hratfc);
//		cairo_scale(pContext, (double)width/(double)g_svgClockW, (double)height/(double)g_svgClockH);
		cairo_set_operator(pContext, okay ? CAIRO_OPERATOR_OVER : CAIRO_OPERATOR_CLEAR);
//		cairo_set_operator(pContext, CAIRO_OPERATOR_OVER);

		if( okay )
		{
			DEBUGLOGS(" drawing fcovr surf into frgnd");
			cairo_set_source_surface(pContext, dsfc.pCairoSurf, 0.0, 0.0);
//			cairo_set_source_surface(pContext, g_surfs[SURF_FCOVR].pCairoSurf, 0.0, 0.0);
		}

		SurfLockEnd(SURF_FCOVR);

		cairo_paint  (pContext);
		cairo_restore(pContext);

		g_yield_thread(yield);
	}

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
bool draw::render_fcovr(cairo_t* pContext, int width, int height, bool yield, bool bright)
{
	DEBUGLOGB;

	bool okay = false;

//	cairo_scale(pContext, (double)width/(double)g_svgClockW, (double)height/(double)g_svgClockH);
	cairo_set_source_rgba(pContext, 1.0, 1.0, 1.0, 0.0); // transparent white
	cairo_set_operator(pContext, bright ? CAIRO_OPERATOR_LIGHTEN : CAIRO_OPERATOR_OVER);
	cairo_paint(pContext);

//	DEBUGLOGP("scaling set (%d, %d)\n", (int)width, (int)g_svgClockW);

	// draw the upper layer elements

//	if( !gCfg.textOnly )
	{
		ClockElement      le;
		RsvgDimensionData ed;

		static const ClockElement ClockElems[] = { CLOCK_FACE_SHADOW, CLOCK_GLASS, CLOCK_FRAME };

		cairo_save (pContext);
		cairo_scale(pContext, (double)width/(double)g_svgClockW, (double)height/(double)g_svgClockH);

		for( size_t e = 0; e < vectsz(ClockElems); e++ )
		{
			le = ClockElems[e];

			SvgLockBeg(le);

			if( g_pSvgHandle[le] )
			{
				DEBUGLOGP(" bef drawing %d svg element\n", le);
				rsvg_handle_get_dimensions(g_pSvgHandle[le], &ed);
				cairo_translate(pContext, (g_svgClockW-ed.width)*0.5, (g_svgClockH-ed.height)*0.5);

				if( rsvg_handle_render_cairo(g_pSvgHandle[le], pContext) )
				{
//					DEBUGLOGP("successfully rendered clock element %d\n", (int)le);
					okay = true;
				}
				DEBUGLOGP(" aft drawing %d svg element\n", le);
			}

			SvgLockEnd(le);

			g_yield_thread(yield);
		}

		cairo_restore(pContext);
	}

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool draw::render_hand(cairo_t* pContext, int handElem, int shadElem, double hoffX, double hoffY, double angle)
{
	DEBUGLOGB;
	DEBUGLOGP("  for %d (%d)\n", handElem, shadElem);

	bool okay = false;

	SvgLockBeg(shadElem);

	if( shadElem != -1 && g_pSvgHandle[shadElem] )
	{
//		DEBUGLOGP("rendering clock hand shadow element %d\n", shadElem);

		cairo_save(pContext);
		cairo_translate(pContext, g_coffX+hoffX, g_coffY+hoffY);
		cairo_rotate(pContext, angle);
		cairo_set_operator(pContext, CAIRO_OPERATOR_OVER);

		if( rsvg_handle_render_cairo(g_pSvgHandle[shadElem], pContext) )
		{
//			DEBUGLOGP("successfully rendered clock element %d\n", shadElem);
			okay = true;
		}

		cairo_restore(pContext);
	}

	SvgLockEnd(shadElem);
	SvgLockBeg(handElem);

	if( handElem != -1 && g_pSvgHandle[handElem] )
	{
//		DEBUGLOGP("rendering clock hand element %d\n", handElem);

		cairo_rotate(pContext, angle);
//		cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);
		cairo_set_operator(pContext, CAIRO_OPERATOR_OVER);

		if( rsvg_handle_render_cairo(g_pSvgHandle[handElem], pContext) )
		{
//			DEBUGLOGP("successfully rendered clock element %d\n", handElem);
			okay = true;
		}
	}

	SvgLockEnd(handElem);

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
void draw::render_hand_hl(cairo_t* pContext, const Hand& hand, bool evalDraws)
{
	DEBUGLOGB;

	if( !hand.draw )
	{
		DEBUGLOGR(1);
		return;
	}

	if( hand.optBest )
	{
		if( hand.handOkay[0] && hand.surfOkay[0] )
		{
			cairo_set_matrix(pContext, &hand.transfrm[0].matrix);
			cairo_rotate    (pContext,  hand.transfrm[0].rotate);
			cairo_translate (pContext,  hand.ox, hand.oy);
			render_surf     (pContext,  hand.surfType[0], CAIRO_OPERATOR_OVER, -1, hand.alfa);
		}

		if( hand.handOkay[1] && hand.surfOkay[1] )
		{
			cairo_set_matrix(pContext, &hand.transfrm[1].matrix);
			cairo_rotate    (pContext,  hand.transfrm[1].rotate);
			cairo_translate (pContext,  hand.ox, hand.oy);
			render_surf     (pContext,  hand.surfType[1], CAIRO_OPERATOR_OVER, -1, hand.alfa);
		}
	}
	else
	{
		cairo_set_matrix(pContext, &g_rendrxfH.matrix);
		cairo_scale(pContext, hand.wratio, hand.hratio);
		cairo_translate(pContext, hand.halfW, hand.halfH); // rendered hand's origin is at the clock's center
		cairo_rotate(pContext, -M_PI*0.5);                 // rendered hand's zero angle is at 12 o'clock
		render_hand(pContext, hand.handElem, hand.shadElem, hand.hoff, -hand.hoff, hand.transfrm[0].rotate);
	}

	if( evalDraws && hand.bbox && hand.useSurf )
	{
		cairo_identity_matrix(pContext);
		cairo_scale(pContext, hand.wratio, hand.hratio);
		cairo_translate(pContext, hand.halfW, hand.halfH);         // rendered hand's origin is at the clock's center
		cairo_rotate(pContext, -M_PI*0.5+hand.transfrm[0].rotate); // rendered hand's zero angle is at 12 o'clock

		cairo_new_path(pContext);
		cairo_set_line_width(pContext, 0.25);
		cairo_set_operator(pContext, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba(pContext, 0.2, 1, 0.2, 0.5);
		cairo_rectangle(pContext, hand.bbox[0], hand.bbox[1], hand.bbox[2]-hand.bbox[0]+1, hand.bbox[3]-hand.bbox[1]+1);
		cairo_stroke (pContext);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool draw::render_hand_hl_chk(const Hand& hand, bool* handOkay, bool* surfOkay, SurfaceType* surfType)
{
	DEBUGLOGB;

	SurfaceType st;
	bool        _handOkay[] = { false,       false };
	bool        _surfOkay[] = { false,       false };
	SurfaceType _surfType[] = { hand.shadST, hand.handST };

	for( size_t s = 0; s < vectsz(_surfType); s++ )
	{
		handOkay[s] = _handOkay[s];
		surfOkay[s] = _surfOkay[s];
		surfType[s] = _surfType[s];

		SurfLockBeg(st=surfType[s]);
		surfOkay[s] =  hand.useSurf | g_surfs[st].pCairoSurf != NULL;
		SurfLockEnd(st);

		if( surfOkay[s] || !hand.optBest )
			handOkay[s] =  true;
	}

	DEBUGLOGE;
	return  handOkay[0] || handOkay[1];
}

// -----------------------------------------------------------------------------
void draw::render_hand_hl_chk_set(Hand& hand)
{
	DEBUGLOGB;

	hand.draw   =  render_hand_hl_chk(hand, hand.handOkay, hand.surfOkay, hand.surfType);
	hand.halfW  =  g_svgClockW*0.5;
	hand.halfH  =  g_svgClockH*0.5;
	hand.wratio = (double)g_rendrxfH.width /(double)g_svgClockW;
	hand.hratio = (double)g_rendrxfH.height/(double)g_svgClockH;
	hand.cx     =  g_rendrxfH.width *0.5;
	hand.cy     =  g_rendrxfH.height*0.5;
	hand.ox     = -hand.cx/g_xDiv;
	hand.oy     = -hand.cy/g_yDiv;

	hand.transfrm[0] = g_rendrxfH;
	hand.transfrm[1] = g_rendrxfH;

	// shadow
	cairo_matrix_translate(&hand.transfrm[0].matrix,  hand.cx, hand.cy);
	cairo_matrix_rotate   (&hand.transfrm[0].matrix, -M_PI*0.5);
	cairo_matrix_translate(&hand.transfrm[0].matrix,  hand.wratio*(g_coffX+hand.hoff), hand.hratio*(g_coffY-hand.hoff));

	// hand
	cairo_matrix_translate(&hand.transfrm[1].matrix,  hand.cx, hand.cy);
	cairo_matrix_rotate   (&hand.transfrm[1].matrix, -M_PI*0.5);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool draw::render_handalm(cairo_t* pContext, int width, int height, bool yield, bool bright, bool center, bool rotate, bool hand, bool shadow, bool offset)
{
	DEBUGLOGB;

	int    handElem = hand   ? CLOCK_ALARM_HAND         : -1;
	int    shadElem = shadow ? CLOCK_ALARM_HAND_SHADOW  : -1;
	double angle    = rotate ? gRun.angleAlm            : -1;
	double shadOffs = offset ? g_ahoff                  :  0;

	bool   ret      = render_chand(pContext, handElem, shadElem, width, height, bright, shadOffs, center, angle);

	g_yield_thread(yield);

	return ret;
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool draw::render_handhor(cairo_t* pContext, int width, int height, bool yield, bool bright, bool center, bool rotate, bool hand, bool shadow, bool offset)
{
	DEBUGLOGB;

	int    handElem = hand   ? CLOCK_HOUR_HAND          : -1;
	int    shadElem = shadow ? CLOCK_HOUR_HAND_SHADOW   : -1;
	double angle    = rotate ? gRun.angleHor            : -1;
	double shadOffs = offset ? g_hhoff                  :  0;

	bool   ret      = render_chand(pContext, handElem, shadElem, width, height, bright, shadOffs, center, angle);

	g_yield_thread(yield);

	return ret;
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool draw::render_handmin(cairo_t* pContext, int width, int height, bool yield, bool bright, bool center, bool rotate, bool hand, bool shadow, bool offset)
{
	DEBUGLOGB;

	int    handElem = hand   ? CLOCK_MINUTE_HAND        : -1;
	int    shadElem = shadow ? CLOCK_MINUTE_HAND_SHADOW : -1;
	double angle    = rotate ? gRun.angleMin            : -1;
	double shadOffs = offset ? g_mhoff                  :  0;

	bool   ret      = render_chand(pContext, handElem, shadElem, width, height, bright, shadOffs, center, angle);

	g_yield_thread(yield);

	return ret;
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool draw::render_handsec(cairo_t* pContext, int width, int height, bool yield, bool bright, bool center, bool rotate, bool hand, bool shadow, bool offset)
{
	DEBUGLOGB;

	int    handElem = hand   ? CLOCK_SECOND_HAND        : -1;
	int    shadElem = shadow ? CLOCK_SECOND_HAND_SHADOW : -1;
	double angle    = rotate ? gRun.angleSec            : -1;
	double shadOffs = offset ? g_shoff                  :  0;

	bool   ret      = render_chand(pContext, handElem, shadElem, width, height, bright, shadOffs, center, angle);

	g_yield_thread(yield);

	return ret;
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::render_surf(cairo_t* pContext, SurfaceType st, cairo_operator_t op1, int op2, double alf)
{
	DEBUGLOGB;
	DEBUGLOGP("  surface type is %d\n", (int)st);

	SurfLockBeg(st);

	if( g_surfs[st].pCairoPtrn )
	{
		DEBUGLOGS("  pattern available");

		cairo_set_operator(pContext, op1);
		cairo_set_source  (pContext, g_surfs[st].pCairoPtrn);
	}
	else
	if( g_surfs[st].pCairoSurf )
	{
		DEBUGLOGS("  surface available (pattern not)");

		cairo_set_operator      (pContext, op1);
		cairo_set_source_surface(pContext, g_surfs[st].pCairoSurf, 0.0, 0.0);
	}
	else
	{
		DEBUGLOGS("  no pattern or surface available");

		if( op2 != -1 )
			cairo_set_operator(pContext, (cairo_operator_t)op2);
	}

//	cairo_paint_with_alpha(pContext, alf); // TODO: make use of this as intended
	cairo_paint(pContext);

	SurfLockEnd(st);
	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::reset_ani()
{
	static double time31X[] =
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
	static double flickrY[] =
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
	static double time03X[] = { 0.0, 0.5, 1.0 }; // min of 3 pts for gsl::interp funcs use
	static double customY[] = { 0.0, 0.5, 1.0 };
	static int    ntimes    =   0;

	DEBUGLOGB;

	hand_anim_beg();

	double* timeX = NULL;
	double* rotaY = NULL;

	switch( gCfg.shandType )
	{
	default:
	case ANIM_FLICK:  timeX = time31X; rotaY = flickrY; break; // curve from watching an analog clock
	case ANIM_ORIG:   timeX = time31X; rotaY = bounceY; break; // MacSlow's clock animation curve
	case ANIM_SWEEP:  timeX = time31X; rotaY = smoothY; break; // continuous (smooth)
	case ANIM_CUSTOM: timeX = time03X; rotaY = customY; break; // user specified
	}

	if( timeX == time31X )
		ntimes = sizeof(time31X)/sizeof(time31X[0]);
	else
	if( timeX == time03X )
		ntimes = sizeof(time03X)/sizeof(time03X[0]);

	if( timeX && rotaY && ntimes )
		hand_anim_set(timeX, rotaY, ntimes);

	update_ani();

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::reset_hands()
{
	g_almHand.optBest = gCfg.optHand[0] == TRUE;
	g_almHand.useSurf = gCfg.useSurf[0] == TRUE;
	g_horHand.optBest = gCfg.optHand[1] == TRUE;
	g_horHand.useSurf = gCfg.useSurf[1] == TRUE;
	g_minHand.optBest = gCfg.optHand[2] == TRUE;
	g_minHand.useSurf = gCfg.useSurf[2] == TRUE;
	g_secHand.optBest = gCfg.optHand[3] == TRUE;
	g_secHand.useSurf = gCfg.useSurf[3] == TRUE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::update_ani()
{
#ifdef _USEREFRESHER
	DEBUGLOGB;

	const double* timeX;
	const double* angrY;
	int           ntimes = hand_anim_get(&timeX, &angrY);

	refreshCount = 0;
#if 0
	int  xindx[vectsz(refreshTime)];
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
	}
#endif
#if 0
	int      b = 0;
	for( int i = 0; i < ntimes; i++ )
//	for( int i = 0; i < refreshCount; i++ )
	{
		if( fabs(angrY[i]) > 0.01 )
//		if( fabs(refreshTime[i]) > 0.01 )
		{
			b  = i - (i == 0 ? 0 : 1);
			break;
		}
	}
#endif
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
	int    nt =  gCfg.renderRate + 2;
//	double td = (1.0 - timeX[b])/double(nt-1);
	double td = (1.0 - timeX[b])/double(nt-2);

	refreshTime[0] = 0;
	refreshFrac[0] = 0;
	refreshCumm[0] = 0;

	DEBUGLOGP("nt=%d, b=%d, td=%f\n", nt, b, td);
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

	// make sure time increases so gsl doesn't barf on app later on

	double tb4 = refreshTime[0];
	for( int i = 1; i < nt; i++ )
	{
		if(   refreshTime[i] <= tb4 )
			  refreshTime[i] += 0.0001;
		tb4 = refreshTime[i];
	}

	for( int i = 0; i < nt; i++ )
	{
//		refreshFrac[i] /= tf;
		refreshCumm[i] /= tf;
		DEBUGLOGP("%2.2d: %4.4f, %4.4f, %4.4f\n", i, refreshTime[i], refreshFrac[i], refreshCumm[i]);
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
#endif
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::update_bkgnd(bool yield)
{
	DEBUGLOGB;

	bool bkgndHor = g_horHand.optBest && !g_horHand.useSurf;
	bool bkgndMin = g_minHand.optBest && !g_minHand.useSurf;

	if( bkgndHor || bkgndMin )
	{
		DEBUGLOGS("updating background drawing surface");

		SurfLockBeg(SURF_BKGND);

		cairo_t* pContext = g_surfs[SURF_BKGND].pCairoSurf ? cairo_create(g_surfs[SURF_BKGND].pCairoSurf) : NULL;

		if( pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS )
		{
//			render_bkgnd(pContext, gCfg.clockW, gCfg.clockH, false);
			render_bkgnd(pContext, g_surfs[SURF_BKGND].width, g_surfs[SURF_BKGND].height, yield);
			cairo_destroy(pContext);
			pContext = NULL;
		}

		SurfLockEnd(SURF_BKGND);
	}
	else
	if( gCfg.handsOnly )
	{
		bool surfHor = g_horHand.optBest && g_horHand.useSurf;
		bool surfMin = g_minHand.optBest && g_minHand.useSurf;

		if( surfHor || surfMin )
		{
#if _USEGTK
//			cairo_t* pContext = gdk_cairo_create(g_pClockWindow);
			cairo_t* pContext = cairo_context(g_pClockWindow);
#else
			cairo_t* pContext = cairo_context(gRun.pMainWindow);
#endif
//			if( pContext || cairo_status(pContext) == CAIRO_STATUS_SUCCESS )
			if( pContext )
			{
				if( surfHor && !g_surfs[SURF_HHAND].pCairoSurf )
				{
					DEBUGLOGS("updating hour hand surface");
					update_surf(pContext, SURF_HHAND, g_surfaceW, g_surfaceH, true, yield);
					update_surf(pContext, SURF_HHSHD, g_surfaceW, g_surfaceH, true, yield);
					update_surf_swap(SURF_HHAND);
					update_surf_swap(SURF_HHSHD);
				}

				if( surfMin && !g_surfs[SURF_MHAND].pCairoSurf )
				{
					DEBUGLOGS("updating minute hand surface");
					update_surf(pContext, SURF_MHAND, g_surfaceW, g_surfaceH, true, yield);
					update_surf(pContext, SURF_MHSHD, g_surfaceW, g_surfaceH, true, yield);
					update_surf_swap(SURF_MHAND);
					update_surf_swap(SURF_MHSHD);
				}

				cairo_destroy(pContext);
				pContext = NULL;
			}
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::update_date_surf(bool toggle, bool yield, bool paths, bool txtchg)
{
	DEBUGLOGB;

	SurfaceType st1   =   gCfg.faceDate ? SURF_BKGND : SURF_FRGND;
	SurfaceType st2   =  !gCfg.faceDate ? SURF_BKGND : SURF_FRGND;
	SurfaceType sts[] = { st1, st2 };

	for( size_t s = 0; s < vectsz(sts); s++ )
	{
		SurfaceType st = sts[s];

		SurfLockBeg(st);

		cairo_surface_t* pSurface = g_surfs[st].pCairoSurf;
		cairo_t*         pContext = pSurface ? cairo_create(pSurface) : NULL;

		if( pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS )
		{
			cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
			cairo_paint       (pContext);

			if( paths && s == 0 )
			{
				DEBUGLOGS("bef updating date paths");
				render_date_make(pContext, g_surfs[st].width, g_surfs[st].height, yield, false);
				DEBUGLOGS("aft updating date paths");
			}

			if( st == SURF_BKGND )
			{
				DEBUGLOGS("bef updating bkgnd surface");
//				render_bkgnd(pContext, gCfg.clockW, gCfg.clockH, false);
				render_bkgnd(pContext, g_surfs[st].width, g_surfs[st].height, yield, false, true);
				DEBUGLOGS("aft updating bkgnd surface");
			}
			else
			{
				DEBUGLOGS("bef updating frgnd surface");
//				render_frgnd(pContext, gCfg.clockW, gCfg.clockH, false);
				render_frgnd(pContext, g_surfs[st].width, g_surfs[st].height, yield, false, true);
				DEBUGLOGS("aft updating frgnd surface");
			}

			cairo_destroy(pContext);
			pContext = NULL;
		}

		SurfLockEnd(st);

//		if( !toggle )
		if( !toggle && !txtchg )
			break;
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::update_marks(bool yield)
{
	DEBUGLOGB;

//	update_cbase();

	SurfLockBeg(SURF_BBASE);

	cairo_t* pContext = g_surfs[SURF_BBASE].pCairoSurf ? cairo_create(g_surfs[SURF_BBASE].pCairoSurf) : NULL;

	if( pContext && cairo_status(pContext) == CAIRO_STATUS_SUCCESS )
	{
		cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
		cairo_paint       (pContext);

//		render_bbase (pContext, gCfg.clockW, gCfg.clockH, yield);
		render_bbase (pContext, g_surfs[SURF_BBASE].width, g_surfs[SURF_BBASE].height, yield);
//		render_bbase (pContext, g_surfs[SURF_BBASE], g_surfs[SURF_BBASE].width, g_surfs[SURF_BBASE].height, yield);

		cairo_destroy(pContext);
		pContext = NULL;
	}

	SurfLockEnd(SURF_BBASE);

	update_bkgnd();

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool draw::update_surface(cairo_t* pSourceContext, SurfaceType st, int width, int height, bool hand, bool yield)
{
	DEBUGLOGB;

	g_temps[st].pCairoSurf = NULL;
	g_temps[st].pCairoPtrn = NULL;
	g_temps[st].width      = width;
	g_temps[st].height     = height;
	g_temps[st].svgClockW  = g_svgClockW; // TODO: these need to come from the 'temp' svg clock's width
	g_temps[st].svgClockH  = g_svgClockH; //       and height

	DEBUGLOGS("destroying old and creating a new surface");
	DEBUGLOGP("creating a new surface of type %d\n", st);

//	int handW = width;
//	if( hand )  width  /= (int)g_xDiv;

	int handH = height;
	if( hand )  height /= (int)g_yDiv;

	g_temps[st].pCairoSurf = cairo_surface_create_similar(cairo_get_target(pSourceContext), CAIRO_CONTENT_COLOR_ALPHA, width, height);

//	if( hand )  width  = handW;
	if( hand )  height = handH;

	if( g_temps[st].pCairoSurf == NULL || cairo_surface_status(g_temps[st].pCairoSurf) != CAIRO_STATUS_SUCCESS )
	{
		g_temps[st].pCairoSurf =  NULL;

		DEBUGLOGR(1);
		return false;
	}

//	DEBUGLOGP("created new surface of type %d\n", st);

	cairo_t* pContext = cairo_create(g_temps[st].pCairoSurf);

	if( !pContext || cairo_status(pContext) != CAIRO_STATUS_SUCCESS )
	{
//		DEBUGLOGP("failed to create a context using the new surface of type %d\n", st);
//		DEBUGLOGP("new surface of type %d was valid - so what went wrong?\n", st);

		cairo_surface_destroy(g_temps[st].pCairoSurf);
		g_temps[st].pCairoSurf = NULL;

		DEBUGLOGR(2);
		return false;
	}

//	DEBUGLOGP("clearing the context using the new surface of type %d\n", st);

	cairo_set_operator(pContext, CAIRO_OPERATOR_CLEAR);
	cairo_paint(pContext);

	g_yield_thread(yield);

	bool  okay = false;
	const DrawSurf& ds = g_temps[st];

	switch( st )
	{
	case SURF_BBASE: okay = render_bbase  (pContext,     width, height, yield); break;
//	case SURF_BBASE: okay = render_bbase  (pContext, ds, width, height, yield); break;
	case SURF_FCOVR: okay = render_fcovr  (pContext,     width, height, yield); break;
	case SURF_BKGND: okay = render_bkgnd  (pContext,     width, height, yield,  false, true, true); break;
	case SURF_FRGND: okay = render_frgnd  (pContext,     width, height, yield,  false, true, true); break;

	case SURF_AHAND: okay = render_handalm(pContext,     width, height, yield, false, false, false, true,  false, false); break;
	case SURF_HHAND: okay = render_handhor(pContext,     width, height, yield, false, false, false, true,  false, false); break;
	case SURF_MHAND: okay = render_handmin(pContext,     width, height, yield, false, false, false, true,  false, false); break;
	case SURF_SHAND: okay = render_handsec(pContext,     width, height, yield, false, false, false, true,  false, false); break;
	case SURF_AHSHD: okay = render_handalm(pContext,     width, height, yield, false, false, false, false, true,  false); break;
	case SURF_HHSHD: okay = render_handhor(pContext,     width, height, yield, false, false, false, false, true,  false); break;
	case SURF_MHSHD: okay = render_handmin(pContext,     width, height, yield, false, false, false, false, true,  false); break;
	case SURF_SHSHD: okay = render_handsec(pContext,     width, height, yield, false, false, false, false, true,  false); break;
	}

//	DEBUGLOGP("%s render to new surface of type %d\n", okay ? "good" : "new", st);

	cairo_destroy(pContext);
	pContext = NULL;

	DEBUGLOGE;
	return true;
}

// -----------------------------------------------------------------------------
bool draw::update_surf(cairo_t* pContext, SurfaceType st, int width, int height, bool hand, bool yield)
{
	DEBUGLOGB;

//	TODO: need 'asyncing' bool passed in (and pass it to update_surface as well?)
//        use this to determine whether to do any surface use locking

	g_yield_thread(yield);

	bool okay = update_surface(pContext, st, width, height, hand, yield);

	DEBUGLOGE;

//	return g_temps[st].pCairoSurf != NULL;
//	return okay;
	return true;
}

#if _USEGTK
// -----------------------------------------------------------------------------
void draw::update_surfs(PWidget* pWidget, int width, int height, bool yield)
{
	DEBUGLOGB;

	// TODO: need to pass in a locking indicator here for mt-threaded use

//	cairo_t* pContext = gdk_cairo_create(gtk_widget_get_window(pWidget));
	cairo_t* pContext = cairo_context(pWidget);

//	if( !pContext || cairo_status(pContext) != CAIRO_STATUS_SUCCESS )
	if( !pContext )
	{
		DEBUGLOGR(1);
		return;
	}

#ifdef _DEBUGLOG
	double wratio = (double)width/ (double)g_svgClockW;
	double hratio = (double)height/(double)g_svgClockH;

	DEBUGLOGP("surfaces=%dx%d, svgs=%dx%d, ratios=%4.4f & %4.4f\n",
		width, height, g_svgClockW, g_svgClockH, (float)wratio, (float)hratio);
#endif

	DEBUGLOGS("bef updating date paths");
	render_date_make(pContext, width, height, yield, false);
	DEBUGLOGS("aft updating date paths");

	bool lokay = update_surf(pContext, SURF_BBASE, width, height, false, yield);
	bool hokay = update_surf(pContext, SURF_FCOVR, width, height, false, yield);
	bool bokay = update_surf(pContext, SURF_BKGND, width, height, false, yield);
	bool fokay = update_surf(pContext, SURF_FRGND, width, height, false, yield);

	static SurfaceType handST[] = {  SURF_AHAND,         SURF_HHAND,         SURF_MHAND,         SURF_SHAND };
	static SurfaceType shadST[] = {  SURF_AHSHD,         SURF_HHSHD,         SURF_MHSHD,         SURF_SHSHD };
	static const bool* uzSurf[] = { &g_almHand.useSurf, &g_horHand.useSurf, &g_minHand.useSurf, &g_secHand.useSurf };

	for( size_t s = 0; s < vectsz(handST); s++ )
	{
		if( *uzSurf[s] )
		{
			update_surf(pContext, handST[s], width, height, true, yield);
			update_surf(pContext, shadST[s], width, height, true, yield);
		}
	}

	if( bokay && fokay )
	{
		DEBUGLOGP("setting surface dims to (%d, %d)\n", width, height);

		g_surfaceW = width;
		g_surfaceH = height;
	}

	cairo_destroy(pContext);
	pContext = NULL;

	DEBUGLOGE;
}
#endif // _USEGTK

// -----------------------------------------------------------------------------
void draw::update_surfs_swap()
{
	DEBUGLOGB;

	for( size_t st  = 0; st < SURF_COUNT; st++ )
	{
		g_temps[st].pCairoPtrn = g_temps[st].pCairoSurf ? cairo_pattern_create_for_surface(g_temps[st].pCairoSurf) : NULL;
	}

	DrawSurf backs[SURF_COUNT];

	for( size_t st  = 0; st < SURF_COUNT; st++ )
		backs[st]   = g_surfs[st];

	LockAllSurfBeg();

	for( size_t st  = 0; st < SURF_COUNT; st++ )
	{
		SurfLockBeg(st);
		g_surfs[st] = g_temps[st];
		SurfLockEnd(st);
	}

	render_hand_hl_chk_set(g_almHand);
//	render_hand_hl_chk_set(g_tznHand);
	render_hand_hl_chk_set(g_horHand);
	render_hand_hl_chk_set(g_minHand);
	render_hand_hl_chk_set(g_secHand);

	LockAllSurfEnd();

	for( size_t st = 0; st < SURF_COUNT; st++ )
	{
		if( backs[st].pCairoPtrn && (g_temps[st].pCairoPtrn != backs[st].pCairoPtrn) )
			cairo_pattern_destroy(backs[st].pCairoPtrn);

		if( backs[st].pCairoSurf && (g_temps[st].pCairoSurf != backs[st].pCairoSurf) )
			cairo_surface_destroy(backs[st].pCairoSurf);
	}

	backs[0].pCairoSurf = NULL;
	backs[0].pCairoPtrn = NULL;
	backs[0].width      = 0;
	backs[0].height     = 0;
	backs[0].svgClockW  = 0;
	backs[0].svgClockH  = 0;

	for( size_t st  = 0; st < SURF_COUNT; st++ )
		g_temps[st] = backs[0];

	gRun.updateSurfs = false;

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void draw::update_surf_swap(SurfaceType st)
{
	DEBUGLOGB;

	g_temps[st].pCairoPtrn = g_temps[st].pCairoSurf ? cairo_pattern_create_for_surface(g_temps[st].pCairoSurf) : NULL;

	DrawSurf backs = g_surfs[st];

	SurfLockBeg(st);
	g_surfs[st] = g_temps[st];
	SurfLockEnd(st);

	switch( st )
	{
	case SURF_AHAND:
	case SURF_AHSHD: render_hand_hl_chk_set(g_almHand); break;
//	case SURF_ZHAND:
//	case SURF_ZHSHD: render_hand_hl_chk_set(g_tznHand); break;
	case SURF_HHAND:
	case SURF_HHSHD: render_hand_hl_chk_set(g_horHand); break;
	case SURF_MHAND:
	case SURF_MHSHD: render_hand_hl_chk_set(g_minHand); break;
	case SURF_SHAND:
	case SURF_SHSHD: render_hand_hl_chk_set(g_secHand); break;
	}

	if( backs.pCairoPtrn && (g_temps[st].pCairoPtrn != backs.pCairoPtrn) )
		cairo_pattern_destroy(backs.pCairoPtrn);

	if( backs.pCairoSurf && (g_temps[st].pCairoSurf != backs.pCairoSurf) )
		cairo_surface_destroy(backs.pCairoSurf);

	backs.pCairoSurf = NULL;
	backs.pCairoPtrn = NULL;
	backs.width      = 0;
	backs.height     = 0;
	backs.svgClockW  = 0;
	backs.svgClockH  = 0;

	g_temps[st] = backs;

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void draw::update_text_font(cairo_t* pContext, bool force)
{
	DEBUGLOGB;

	bool gotMW = gRun.pMainWindow != NULL;
	DEBUGLOGS(gotMW ? "do have a main window widget pointer" : "do NOT have a main window widget pointer");

	if( *gCfg.fontName && (!g_pTextFontFace || force) )
	{
		DEBUGLOGP("creating a cairo font face for:\n\t%s\n", gCfg.fontName);

#if _USEPANGO
		DEBUGLOGS("bef getting a pango context");
#if _USEGTK
		PangoContext* pPContext = gotMW ? gtk_widget_get_pango_context(gRun.pMainWindow) : (pContext ? pango_cairo_create_context(pContext) : NULL);
#else
		PangoContext* pPContext = gotMW ? NULL                                           : (pContext ? pango_cairo_create_context(pContext) : NULL);
#endif
		DEBUGLOGS("aft getting a pango context");

		DEBUGLOGS(pPContext ? "do have a pango context" : "do NOT have a pango context");

		if( pPContext )
		{
			DEBUGLOGS("bef getting a pango-based cairo font");
			PangoFontDescription* pPFDesc = pango_font_description_from_string(gCfg.fontName);
			PangoFont*            pPFont  = pango_context_load_font           (pPContext, pPFDesc);
			PangoCairoFont*       pPCFont = PANGO_CAIRO_FONT                  (pPFont);
			cairo_scaled_font_t*  pCSFont = pango_cairo_font_get_scaled_font  (pPCFont);

			if( pCSFont )
			{
				cairo_font_face_t* pTextFontFace = cairo_scaled_font_get_font_face(pCSFont);

				if( pTextFontFace )
				{
					if( g_pTextFontFace )
					{
						DEBUGLOGS("destroying old cairo font face");
						cairo_font_face_destroy(g_pTextFontFace);
					}

					DEBUGLOGS("setting old cairo font face to new one");
					g_pTextFontFace = pTextFontFace;
				}
			}

			DEBUGLOGS("aft getting a pango-based cairo font");

			DEBUGLOGS("bef unrefing font loading related objects");

			if( pPFont )
			{
/*				PangoFontDescription* pf = pango_font_describe(pPFont);

				if( pf )
				{
					const char* ds = pango_font_description_to_string(pf);
					DEBUGLOGP("pPFont description:\n\t%s\n", ds ? ds : "<null>");
					pango_font_description_free(pf);
				}*/
#if _USEGTK
				g_object_unref(pPFont);
#endif
			}

			if( pPFDesc )
				pango_font_description_free(pPFDesc);
#if _USEGTK
			if( !gotMW )
				g_object_unref(pPContext);
#endif
			DEBUGLOGS("aft unrefing font loading related objects");
		}

		DEBUGLOGP("new cairo font face is \"%s\"\n", g_pTextFontFace ? "valid" : "NOT valid");

#else  // _USEPANGO
#endif // _USEPANGO
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include "loadTheme.h" // for svg clock element bounding box retrieval
#include "svgData.h"

// -----------------------------------------------------------------------------
bool draw::update_theme(const ThemeEntry& te, UpdateThemeDone callBack, ppointer cbData)
{
	DEBUGLOGB;

	if( callBack )
	{
		DEBUGLOGS("attempting to async update theme");
		DEBUGLOGP("path is %s\n", te.pPath && te.pPath->str ? te.pPath->str : "");
		DEBUGLOGP("file is %s\n", te.pFile && te.pFile->str ? te.pFile->str : "");
		DEBUGLOGP("name is %s\n", te.pName && te.pName->str ? te.pName->str : "");

		UpdateTheme* pUT  = new UpdateTheme;

		if( pUT )
		{
			pUT->callBack = callBack;
			pUT->data     = cbData;
			pUT->okay     = false;

			theme_ntry_cpy(pUT->te, te);

			DEBUGLOGS("before theme thread creation");
			DEBUGLOGP("utpath is %s\n", pUT->te.pPath->str);
			DEBUGLOGP("utfile is %s\n", pUT->te.pFile->str);

			GThread* pThread = g_thread_try_new(__func__, update_theme_func, pUT, NULL);

			if( pThread )
			{
				DEBUGLOGS("update theme thread created");
				g_thread_unref(pThread);
				DEBUGLOGR(1);
				return true;
			}
		}
	}

	DEBUGLOGS("attempting to sync update theme");

	bool ret = update_theme_internal(te);

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
ppointer draw::update_theme_func(ppointer data)
{
	DEBUGLOGB;

	if( !data )
	{
		DEBUGLOGR(1);
		return 0;
	}

	g_expri_thread(+10);

	UpdateTheme* pUT = (UpdateTheme*)data;

	DEBUGLOGS("before internal update_theme call");
	DEBUGLOGP("  path is %s\n", pUT->te.pPath->str);
	DEBUGLOGP("  file is %s\n", pUT->te.pFile->str);

	g_sync_threads_gui_beg();
	pUT->okay = update_theme_internal(pUT->te);
	g_sync_threads_gui_end();

	DEBUGLOGP("after internal update_theme call (%s)\n", pUT->okay ? "succeeded" : "failed");
	DEBUGLOGS("before callBack call");

	if( pUT->callBack )
		pUT->callBack(pUT->data, pUT->te, pUT->okay);

	DEBUGLOGS("after callBack call");

	theme_ntry_del(pUT->te);
	delete pUT;
	pUT = NULL;

	DEBUGLOGE;
	return 0;
}

// -----------------------------------------------------------------------------
bool draw::update_theme_internal(const ThemeEntry& te)
{
	struct svgEle
	{
		char              path[PATH_MAX];
		const char*       name;
		int               type;
		RsvgHandle*       pSvg;
		RsvgDimensionData cdim;
		bool              goodh;
		bool              blank;
	};

	static svgEle svgs[] = // TODO: must be ordered as in global.h enum ClockElement?
	{
		{ "", "drop-shadow",        CLOCK_DROP_SHADOW },
		{ "", "face",               CLOCK_FACE },
		{ "", "marks",              CLOCK_MARKS },
		{ "", "marks-24h",          CLOCK_MARKS_24H },
		{ "", "alarm-hand-shadow",  CLOCK_ALARM_HAND_SHADOW },
		{ "", "alarm-hand",         CLOCK_ALARM_HAND },
		{ "", "hour-hand-shadow",   CLOCK_HOUR_HAND_SHADOW },
		{ "", "hour-hand",          CLOCK_HOUR_HAND },
		{ "", "minute-hand-shadow", CLOCK_MINUTE_HAND_SHADOW },
		{ "", "minute-hand",        CLOCK_MINUTE_HAND },
		{ "", "second-hand-shadow", CLOCK_SECOND_HAND_SHADOW },
		{ "", "second-hand",        CLOCK_SECOND_HAND },
		{ "", "face-shadow",        CLOCK_FACE_SHADOW },
		{ "", "glass",              CLOCK_GLASS },
		{ "", "frame",              CLOCK_FRAME },
		{ "", "mask",               CLOCK_MASK },
	};

	DEBUGLOGB;

	bool yield = true;

	if( !te.pPath || !te.pFile )
	{
		DEBUGLOGR(1);
		return false;
	}

	DEBUGLOGP("  name=%s\n", te.pFile->str);

	RsvgDimensionData mdim;
	gchar             fpath[PATH_MAX];

	bool ntern = strcmp(te.pPath->str, INTERNAL_THEME) == 0;
	bool valid = ntern;
	bool maskd = false;

	mdim.width = mdim.height = ntern ? 100 : 0;

	DEBUGLOGP("loading %s theme's elements\n", te.pFile->str);

	for( size_t s = 0; s < vectsz(svgs); s++ )
	{
		g_yield_thread(yield);

		int         e       =   svgs[s].type;
		const char* ename   =   svgs[s].name;

		svgs[s].path[0]     = '\0';
		svgs[s].pSvg        =   NULL;
		svgs[s].cdim.width  =   0;
		svgs[s].cdim.height =   0;
		svgs[s].goodh       =   false;
		svgs[s].blank       =   true;

		if( ntern )
		{
			for( size_t t = 0; t < CLOCK_ELEMENTS; t++ )
			{
				if( g_svgDefDTyp[t]  == -1 )
					break;

				if( g_svgDefDTyp[t]  ==  e )
				{
					guint8* pRSvgData = (guint8*)g_svgDefData[t];
					gsize   pRSvgDLen = (gsize)  g_svgDefDLen[t];
					svgs[s].pSvg      =  rsvg_handle_new_from_data(pRSvgData, pRSvgDLen, NULL);
					break;
				}
			}
		}
		else
		{
			static const char* pfmt = "%s/%s/clock-%s.svg%c";

			snprintf(fpath, vectsz(fpath), pfmt, te.pPath->str, te.pFile->str, ename, 'z');

			if( !g_isa_file(fpath) )
			snprintf(fpath, vectsz(fpath), pfmt, te.pPath->str, te.pFile->str, ename, '\0');

			strvcpy(svgs[s].path, fpath);

			svgs[s].pSvg = rsvg_handle_new_from_file(fpath, NULL);
		}

		if( svgs[s].pSvg )
		{
			svgs[s].goodh = true;

			if( (ClockElement)e == CLOCK_MASK )
			{
				DEBUGLOGP("%s element loaded for the theme\n", svgs[s].name);
				maskd = true;
			}

			rsvg_handle_get_dimensions(svgs[s].pSvg, &svgs[s].cdim);

//			svgs[s].blank   = svgs[s].cdim.width == 0 || svgs[s].cdim.height == 0;

			if( mdim.width  < svgs[s].cdim.width )
				mdim.width  = svgs[s].cdim.width;

			if( mdim.height < svgs[s].cdim.height )
				mdim.height = svgs[s].cdim.height;
		}
	}

	if( g_pMaskRgn )
		cairo_region_destroy(g_pMaskRgn);
	g_pMaskRgn = NULL;
#if 0
	if( g_pMaskBeg )
		delete[] g_pMaskBeg;

	if( g_pMaskEnd )
		delete[] g_pMaskEnd;
#endif
	g_pMaskRgn = maskd ? NULL : cairo_region_create();

	if( g_pMaskRgn && cairo_region_status(g_pMaskRgn) != CAIRO_STATUS_SUCCESS )
	{
		cairo_region_destroy(g_pMaskRgn);
		g_pMaskRgn = NULL;
	}
#if 0
	g_nMaskCnt = 0;
	g_pMaskBeg = NULL;
	g_pMaskEnd = NULL;

//	if( g_pMaskRgn )
	if( !maskd )
	{
		g_nMaskCnt = mdim.height;
		g_pMaskBeg = new int[g_nMaskCnt];
		g_pMaskEnd = new int[g_nMaskCnt];

		for( int y = 0; y < g_nMaskCnt; y++ )
		{
			g_pMaskBeg[y] = MASKBEGBAD;
			g_pMaskEnd[y] = MASKENDBAD;
		}
	}
#endif
#if 0
	for( size_t s = 0; s < vectsz(svgs); s++ )
	{
		g_yield_thread(yield);

		int         e     = svgs[s].type;
		const char* ename = svgs[s].name;

//		strvcpy(fpath, svgs[s].path);

		if( svgs[s].goodh )
		{
			float iw, ih, bb[4];
//			bool  blank = false;

			DEBUGLOGP("svg %s was loaded %sternal\n", ename, ntern ? "in" : "ex");

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
				cdim.width = cdim.height = 0;
				rsvg_handle_get_dimensions(svgs[s].pSvg, &cdim);
//				blank      = cdim.width == 0 || cdim.height == 0;

				if( mdim.width  < cdim.width )
					mdim.width  = cdim.width;

				if( mdim.height < cdim.height )
					mdim.height = cdim.height;
			}
		}
	}
#endif
	for( size_t s = 0; s < vectsz(svgs); s++ )
	{
		g_yield_thread(yield);

		int         e     = svgs[s].type;
		const char* ename = svgs[s].name;

		if( svgs[s].goodh )
		{
//			float iw, ih, bb[4];

//			if( svgs[s].blank )
			{
				// fallback to using a pixel buffer since nanosvg seems to miss
				// getting some types of objects (like pngs and other embeddeds)

				GdkPixbuf* pBuffer = rsvg_handle_get_pixbuf(svgs[s].pSvg);

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
								svgs[s].blank = false;
//								bb[0] = bb[1] = bb[2] = bb[3] = 0; // indicate a full window redraw
								break;
							}
						}

						if( !svgs[s].blank && g_pMaskRgn )
//						if( !svgs[s].blank && g_nMaskCnt > 0 && g_pMaskBeg != NULL && g_pMaskEnd != NULL )
						{
							ClockElement ce = (ClockElement)e;
//							bool         m1 = !maskd && !blank && gdk_pixbuf_get_n_channels(pBuffer) == 4 && gdk_pixbuf_get_bits_per_sample(pBuffer) == 8;
							bool         m1 = !maskd && !svgs[s].blank && gdk_pixbuf_get_n_channels(pBuffer) == 4 && gdk_pixbuf_get_bits_per_sample(pBuffer) == 8;
							bool         m2 =         ce == CLOCK_DROP_SHADOW ||
											          ce == CLOCK_FACE        || ce == CLOCK_FACE_SHADOW ||
//							bool         m2 =         ce == CLOCK_FACE        || ce == CLOCK_FACE_SHADOW ||
//							bool         m2 =         ce == CLOCK_FACE        ||
											          ce == CLOCK_GLASS       || ce == CLOCK_FRAME;
							bool         m3 = !m2 && (ce == CLOCK_MARKS       || ce == CLOCK_MARKS_24H);

							if( m1 && (m2 || m3) )
							{
								// rescan to get outline (min/max of each scanline)
								// and 'add' to the current mask region

								int    xbeg, xend;
//								int    amin =   1; // mininum % of alpha to be considered visible
								int    amin =  24; // mininum % of alpha to be considered visible
//								int    amin =  50; // mininum % of alpha to be considered visible
//								int    amin = 100; // mininum % of alpha to be considered visible
								int    xoff = (mdim.width -svgs[s].cdim.width) /2;
								int    yoff = (mdim.height-svgs[s].cdim.height)/2;
								int    xlen =  gdk_pixbuf_get_width    (pBuffer);
								int    ylen =  gdk_pixbuf_get_height   (pBuffer);
								int    rlen =  gdk_pixbuf_get_rowstride(pBuffer);
								guint* pixs = (guint*)pBytes;

								DEBUGLOGP("  outline svg %s parms (%d, %d, %d)\n", ename, xlen, ylen, rlen);
								DEBUGLOGP("  svg dims are (%d, %d)\n", svgs[s].cdim.width, svgs[s].cdim.height);
								DEBUGLOGP("  clock svg dims are (%d, %d)\n", mdim.width, mdim.height);
								DEBUGLOGP("  offsets are (%d, %d)\n", xoff, yoff);

//								int xbb = xlen, xbe = 0, ybb = ylen, ybe = 0, nrs = 0;

								// TODO: need to make this more robust to somehow handle concave
								//       outlined clocks while not throwing out the inner pixels
								//       of the clock

								for( int y = 0; y < ylen; y++ )
								{
									xbeg = xend = -1;

									for( int x = 0; x < xlen; x++ ) // find scanline's first non-zero pixel
									{
										if( ((guchar*)(&pixs[x]))[3] >= amin )
										{
											xbeg = x;
											break;
										}
									}

									if( xbeg != -1 )
									{
										xend  = xbeg + 1; // at least one non-zero pixel found

										for( int x = xlen-1; x >= 0; x-- ) // find scanline's last non-zero pixel
										{
											if( ((guchar*)(&pixs[x]))[3] >= amin )
											{
												xend = x + 1;
												break;
											}
										}
									}

									if( xbeg != -1 && xend != -1 )
									{
										cairo_rectangle_int_t r = { xbeg+xoff, y+yoff, xend-xbeg, 1 };
										cairo_region_union_rectangle(g_pMaskRgn, &r);
#if 0
										g_pMaskBeg[y] = xbeg < g_pMaskBeg[y] ? xbeg : g_pMaskBeg[y];
										g_pMaskEnd[y] = xend > g_pMaskEnd[y] ? xend : g_pMaskEnd[y];
#endif
//										DEBUGLOGP("  y: %3.3d, beg: %3.3d, %3.3d, end: %3.3d, %3.3d\n", y, xbeg, xend, g_pMaskBeg[y], g_pMaskEnd[y]);

//										nrs++;
//										xbb = xbeg < xbb ? xbeg : xbb;
//										xbe = xend > xbe ? xend : xbe;
//										ybb = y    < ybb ? y    : ybb;
//										ybe = y    > ybe ? y    : ybe;
									}
#if 0
									else
									if( y )
									{
										if( g_pMaskBeg[y] == MASKBEGBAD && g_pMaskBeg[y-1] != MASKBEGBAD &&
											g_pMaskEnd[y] == MASKENDBAD && g_pMaskEnd[y-1] != MASKENDBAD )
										{
											g_pMaskBeg[y] =  g_pMaskBeg[y-1];
											g_pMaskEnd[y] =  g_pMaskEnd[y-1];
										}
									}
#endif
									pixs = (guint*)((guchar*)pixs + rlen); // to next scanline
								}

//								DEBUGLOGP("    found %d rects within box of (%d, %d)->(%d, %d)\n",
//									nrs, xbb, ybb, xbe, ybe);

								if( ce == CLOCK_FACE )
								{
									for( int x = 0; x < xlen; x++ )
									{
										pixs = (guint*)((guchar*)pBytes + rlen); // to 2nd scanline

										for( int y = 1; y < ylen; y++ ) // find scanrow's first non-zero pixel
										{
											if( ((guchar*)(&pixs[x]))[3] >= amin )
											{
												cairo_rectangle_int_t r = { x+xoff, 0, 1, y+yoff };
												cairo_region_subtract_rectangle(g_pMaskRgn, &r);
												break;
											}

											pixs = (guint*)((guchar*)pixs + rlen); // to next scanline
										}
									}

									for( int x = 0; x < xlen; x++ )
									{
										pixs = (guint*)((guchar*)pBytes + rlen*(ylen-1)); // to next-to-last scanline

										for( int y = 1; y < ylen; y++ ) // find scanrow's first non-zero pixel
										{
											if( ((guchar*)(&pixs[x]))[3] >= amin )
											{
												cairo_rectangle_int_t r = { x+xoff, ylen-y+yoff, 1, y };
												cairo_region_subtract_rectangle(g_pMaskRgn, &r);
												break;
											}

											pixs = (guint*)((guchar*)pixs - rlen); // to previous scanline
										}
									}
								}
							}
						}
					}

#if _USEGTK
//					gdk_pixbuf_unref(pBuffer); // deprecated
					g_object_unref(pBuffer);
#endif
					pBuffer = NULL;
				}
			}

			if( svgs[s].blank )
			{
//				DEBUGLOGP("svg element %s was loaded\n", ename);
//				DEBUGLOGP("cw=%f, ch=%f, xmin=%f, ymin=%f, xmax=%f, ymax=%f, xext=%f, yext=%f\n",
//					iw, ih, bb[0], bb[1], bb[2], bb[3], bb[2]-bb[0], bb[3]-bb[1]);
//				DEBUGLOGP("discarding %s's %s element (zero area or no objects)\n", name, ename);
				DEBUGLOGP("discarding %s's %s element (zero area or no objects)\n", te.pName->str, ename);

#if _USEGTK
//				rsvg_handle_free(svgs[s].pSvg); // deprecated
				g_object_unref(svgs[s].pSvg);
#endif
				svgs[s].pSvg = NULL;
			}
			else
			{
				valid = true;

				DEBUGLOGP("svg element %s was loaded\n", ename);
#if 0
				if( e == CLOCK_ALARM_HAND || e == CLOCK_HOUR_HAND || e == CLOCK_MINUTE_HAND || e == CLOCK_SECOND_HAND )
				{
					for( size_t b = 0; b < vectsz(bb); b++ )
					{
						switch( e )
						{
						case CLOCK_ALARM_HAND:  gRun.bboxAlmHand[b] = bb[b]; break;
						case CLOCK_HOUR_HAND:   gRun.bboxHorHand[b] = bb[b]; break;
						case CLOCK_MINUTE_HAND: gRun.bboxMinHand[b] = bb[b]; break;
						case CLOCK_SECOND_HAND: gRun.bboxSecHand[b] = bb[b]; break;
						}
					}

					DEBUGLOGP("svg element %s was loaded\n", ename);
					DEBUGLOGP("cw=%f, ch=%f, xmin=%f, ymin=%f, xmax=%f, ymax=%f, xext=%f, yext=%f\n",
						iw, ih, bb[0], bb[1], bb[2], bb[3], bb[2]-bb[0], bb[3]-bb[1]);
				}
#endif
			}
		}
		else
		{
			DEBUGLOGP("(1) svg element %s was NOT loaded\n", ename);
		}
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

//	TODO: fix this new mod so that the 'e' is the 's' for CLOCK_MASK
//	if( g_pSvgHandle[CLOCK_MASK] != NULL && !g_pMaskRgn )
/*	if( svgs[e].pSvg != NULL && !g_pMaskRgn )
	{
		DEBUGLOGS("  mask svg supplied so all's well");
	}*/

	if( mdim.width && mdim.height )
	{
		g_svgClockW = mdim.width;
		g_svgClockH = mdim.height;
	}
	else
	{
		g_svgClockW = 100;
		g_svgClockH = 100;
	}

	DEBUGLOGP("  global clock svg dims are (%d, %d)\n", g_svgClockW, g_svgClockH);
#if 0
	if( valid )
	{
		g_bboxZero = gRun.bboxSecHand[0] == 0 && gRun.bboxSecHand[1] == 0 &&
		             gRun.bboxSecHand[2] == 0 && gRun.bboxSecHand[3] == 0;

//		cairo_matrix_init_scale(&g_mat, gCfg.clockW/g_svgClockW, gCfg.clockH/g_svgClockH);
//		cairo_matrix_translate (&g_mat, g_svgClockW/2,           g_svgClockH/2);

		const double cww = gCfg.clockW; // clock window width
		const double cwh = gCfg.clockH; // clock window height
		const double fcw = g_svgClockW; // svg hand file width (total)
		const double fch = g_svgClockH; // svg hand file height (total)
//
		cairo_matrix_init_scale(&g_mat, cww/fcw, cwh/fch);
		cairo_matrix_translate (&g_mat, fcw/2,   fch/2);
	}
#endif
	DEBUGLOGP(" theme is %s\n\n", valid ? "'valid'" : "not 'valid' (no drawable elements)");
	DEBUGLOGE;

	if( valid )
	{
		int  e;
		for( size_t s = 0; s < vectsz(svgs); s++ )
		{
			SvgLockBeg(e = svgs[s].type);

			if( g_pSvgHandle[e] )
			{
#if _USEGTK
//				rsvg_handle_free(g_pSvgHandle[e]); // deprecated
				g_object_unref(g_pSvgHandle[e]);
#endif
				g_pSvgHandle[e] = NULL;
			}

			g_pSvgHandle[e] = svgs[s].pSvg;
			svgs[s].pSvg = NULL;

			SvgLockEnd(e);
		}
	}
	else
	{
		for( size_t s = 0; s < vectsz(svgs); s++ )
		{
			if( svgs[s].pSvg )
			{
#if _USEGTK
//				rsvg_handle_free(svs[s].pSvg); // deprecated
				g_object_unref(svgs[s].pSvg);
#endif
				svgs[s].pSvg = NULL;
			}
		}
	}

	return valid;
}

#if 0
#if _USEGTK
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void prt_main_surf_type(PWidget* pWidget)
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

	cairo_t*         pContext = pWidget  ?  gdk_cairo_create(gtk_widget_get_window(pWidget)) :  NULL;
	cairo_surface_t* pSurface = pContext ?  cairo_get_target(pContext)                       :  NULL;
	int              st       = pSurface ? (int)cairo_surface_get_type(pSurface)             : -1;

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
#endif // _USEGTK
#endif
#if 0
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// funcs pulled from cairo-dock to eventually be used in getting an X Window's
// screen's background put into a cairo surface, as a better alternative for
// 'below all' clocks to what draw::grab currently does via its call to
// gdk_cairo_set_source_window, which is more appropriate for 'above all' clocks
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
Pixmap cairo_dock_get_window_background_pixmap(Window Xid)
{
	Atom          type   = 0;
	unsigned long nels   = 0;
	int           format = 0;
	unsigned long left   = 0;
	Pixmap*       pBuffr = NULL;
	Pixmap        retID  = None;

	XGetWindowProperty(s_XDisplay, Xid, s_aRootMapID, 0, G_MAXULONG, False, XA_PIXMAP, &type, &format, &nels, &left, (guchar**)&pBuffr);

	if( nels != 0 )
	{
		retID = *pBuffr;
		XFree(pBuffr);
	}

	return retID;
}

// -----------------------------------------------------------------------------
GdkPixbuf* cairo_dock_get_pixbuf_from_pixmap(int XPixmapID, gboolean bAddAlpha)  // cette fonction est inspiree par celle de libwnck.
{
	Window root;
	int    x, y;
	guint  w, h, bw, d;

	if( !XGetGeometry(s_XDisplay, XPixmapID, &root, &x, &y, &w, &h, &bw, &d) )
		return NULL;

	cairo_surface_t* surface     = cairo_xlib_surface_create(s_XDisplay, XPixmapID, DefaultVisual(s_XDisplay, 0), w, h);
	GdkPixbuf*       pIconPixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, w, h);

	cairo_surface_destroy(surface);

	if( !gdk_pixbuf_get_has_alpha(pIconPixbuf) && bAddAlpha )
	{
		GdkPixbuf* tmp_pixbuf = gdk_pixbuf_add_alpha(pIconPixbuf, FALSE, 255, 255, 255);
		g_object_unref(pIconPixbuf);
		pIconPixbuf = tmp_pixbuf;
	}

	return pIconPixbuf;
}
#endif

