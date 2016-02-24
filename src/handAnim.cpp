/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "hani"

#undef   _USEGSL
#undef   _USEGSL_STATIC
#define  _USESPLINE

#include <memory.h>         // for memcpy
#include "handAnim.h"
#include "debug.h"          // for debugging prints

#ifdef   _USEGSL
#include <gsl/gsl_interp.h> // for spline interpolation
#endif

#ifdef   _USESPLINE
#include "cspline.h"        // for static implementation
#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static const size_t g_curvePtsM = 64;
static double       g_curvePtsX[g_curvePtsM];
static double       g_curvePtsY[g_curvePtsM];
static int          g_curvePtsN = 0;

#ifdef _USEGSL

static       gsl_interp*       g_curveTerp = NULL;
static       gsl_interp_accel* g_curveAccl = NULL;
static const gsl_interp_type*  g_curveType = NULL;

#ifndef _USEGSL_STATIC

#include "loadlib.h" // for runtime loading

static lib::symb g_symbs[] =
{
	{ "gsl_interp_init",        NULL },
	{ "gsl_interp_alloc",       NULL },
	{ "gsl_interp_eval",        NULL },
	{ "gsl_interp_free",        NULL },
	{ "gsl_interp_accel_alloc", NULL },
	{ "gsl_interp_accel_reset", NULL },
	{ "gsl_interp_accel_free",  NULL },
	{ "gsl_interp_cspline",     NULL },
	{ "gsl_interp_linear",      NULL }
};

typedef int               (*II) (gsl_interp*, const double [], const double [], size_t);
typedef gsl_interp*       (*IA) (const gsl_interp_type*, size_t);
typedef double            (*IE) (const gsl_interp*, const double [], const double [], double, gsl_interp_accel*);
typedef void              (*IF) (gsl_interp*);
typedef gsl_interp_accel* (*IAA)();
typedef int               (*IAR)(gsl_interp_accel*);
typedef void              (*IAF)(gsl_interp_accel*);

typedef const gsl_interp_type** IT;

static lib::LoadLib* g_pLLib;
static bool          g_pLLibok = false;
static II            g_gsl_interp_init;
static IA            g_gsl_interp_alloc;
static IE            g_gsl_interp_eval;
static IF            g_gsl_interp_free;
static IAA           g_gsl_interp_accel_alloc;
static IAR           g_gsl_interp_accel_reset;
static IAF           g_gsl_interp_accel_free;
static IT            g_gsl_interp_cspline;
static IT            g_gsl_interp_linear;

static double g_curvePtsS[g_curvePtsM]; // if can't load gsl at runtime, fallback to own curve

#endif // !_USEGSL_STATIC
#else  //  _USEGSL
#ifdef _USESPLINE

static cspline g_curve;

#else

static double g_curvePtsS[g_curvePtsM];

#endif //  _USESPLINE
#endif //  _USEGSL

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void hand_anim_beg(const double* xs, const double* ys, int n)
{
	DEBUGLOGB;
	DEBUGLOGP("xs is %s, ys is %s, n is %d\n", xs ? "valid" : "NOT valid", ys ? "valid" : "NOT valid", n);

	hand_anim_end();

#ifdef _USEGSL
#ifdef _USEGSL_STATIC

	g_curveType = gsl_interp_cspline;
//	g_curveType = gsl_interp_linear;
	g_curveAccl = gsl_interp_accel_alloc();

#else // _USEGSL_STATIC

	g_pLLib     = lib::create();
	g_pLLibok   = g_pLLib && lib::load(g_pLLib, "gsl", ".0", g_symbs, vectsz(g_symbs));

	DEBUGLOGP("g_pLLib is %s\n", g_pLLibok ? "okay" : "NOT okay");

	if( g_pLLibok )
	{
		lib::func(g_pLLib, LIB_FUNC_ADDR(g_gsl_interp_init));
		lib::func(g_pLLib, LIB_FUNC_ADDR(g_gsl_interp_alloc));
		lib::func(g_pLLib, LIB_FUNC_ADDR(g_gsl_interp_eval));
		lib::func(g_pLLib, LIB_FUNC_ADDR(g_gsl_interp_free));
		lib::func(g_pLLib, LIB_FUNC_ADDR(g_gsl_interp_accel_alloc));
		lib::func(g_pLLib, LIB_FUNC_ADDR(g_gsl_interp_accel_reset));
		lib::func(g_pLLib, LIB_FUNC_ADDR(g_gsl_interp_accel_free));
		lib::func(g_pLLib, LIB_FUNC_ADDR(g_gsl_interp_cspline));
		lib::func(g_pLLib, LIB_FUNC_ADDR(g_gsl_interp_linear));

		g_curveType = *g_gsl_interp_cspline;
//		g_curveType = *g_gsl_interp_linear;
		g_curveAccl =  g_gsl_interp_accel_alloc();
	}

	DEBUGLOGP("g_curveType is %s (%p)\n", g_curveType ? "okay" : "NOT okay", g_curveType);

#ifdef DEBUGLOG

	if( g_curveType )
	{
		typedef struct
		{
			const char* name;
			unsigned int min_size;
			void* (*alloc)      (size_t);
			int   (*init)       (void*,       const double xa[], const double ya[], size_t size);
			int   (*eval)       (const void*, const double xa[], const double ya[], size_t size, double x, gsl_interp_accel*, double* y);
			int   (*eval_deriv) (const void*, const double xa[], const double ya[], size_t size, double x, gsl_interp_accel*, double* y_p);
			int   (*eval_deriv2)(const void*, const double xa[], const double ya[], size_t size, double x, gsl_interp_accel*, double* y_pp);
			int   (*eval_integ) (const void*, const double xa[], const double ya[], size_t size, gsl_interp_accel*, double a, double b, double* result);
			void  (*free)       (void*);
		} g_gsl_interp_type;

		g_gsl_interp_type* pCT = (g_gsl_interp_type*)g_curveType;
		DEBUGLOGP("g_curveType name is %s, size is %d)\n", pCT->name, pCT->min_size);
	}

#endif //  DEBUGLOG
#endif // _USEGSL_STATIC
#endif // _USEGSL

	hand_anim_set(xs, ys, n);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void hand_anim_end()
{
	DEBUGLOGB;

#ifdef _USEGSL
#ifdef _USEGSL_STATIC

	gsl_interp_free(g_curveTerp);
	gsl_interp_accel_free(g_curveAccl);

#else

	if( g_pLLibok )
	{
		g_gsl_interp_free(g_curveTerp);
		g_gsl_interp_accel_free(g_curveAccl);

		lib::destroy(g_pLLib);
		g_pLLib   =  NULL;
		g_pLLibok =  false;
	}

#endif

	g_curveTerp = NULL;
	g_curveAccl = NULL;

#endif

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void hand_anim_set(const double* xs, const double* ys, int n)
{
	DEBUGLOGB;
	DEBUGLOGP("xs is %s, ys is %s, n is %d\n", xs ? "valid" : "NOT valid", ys ? "valid" : "NOT valid", n);

	if( n > g_curvePtsM )
		n = g_curvePtsM;

	g_curvePtsN = n;

	if( n && (xs || ys) )
	{
		for( int i = 0; i < n; i++ )
		{
			if( xs ) g_curvePtsX[i] = xs[i];
			if( ys ) g_curvePtsY[i] = ys[i];
		}

#ifdef _USEGSL
#ifdef _USEGSL_STATIC

		if( g_curveTerp )
		{
			gsl_interp_free(g_curveTerp);
			g_curveTerp = NULL;
		}

		g_curveTerp = gsl_interp_alloc(g_curveType, g_curvePtsN);

		if( g_curveTerp )
			gsl_interp_init(g_curveTerp, g_curvePtsX, g_curvePtsY, g_curvePtsN);

		if( g_curveAccl )
			gsl_interp_accel_reset(g_curveAccl);

#else // _USEGSL_STATIC

		if( g_pLLibok )
		{
			if( g_curveTerp )
			{
				g_gsl_interp_free(g_curveTerp);
				g_curveTerp = NULL;
			}

			g_curveTerp = g_gsl_interp_alloc(g_curveType, g_curvePtsN);

			if( g_curveTerp )
				g_gsl_interp_init(g_curveTerp, g_curvePtsX, g_curvePtsY, g_curvePtsN);

			if( g_curveAccl )
				g_gsl_interp_accel_reset(g_curveAccl);
		}
		else
		{
			g_curvePtsS[0] = 0;
			for( int i = 1; i < g_curvePtsN; i++ )
			{
				g_curvePtsS[i] =
					(g_curvePtsY[i] - g_curvePtsY[i-1])/(g_curvePtsX[i] - g_curvePtsX[i-1]);
			}
		}

#endif // _USEGSL_STATIC
#else  // _USEGSL
#ifdef _USESPLINE

		csp::set_points(g_curve, g_curvePtsX, g_curvePtsY, g_curvePtsN);

#else

		g_curvePtsS[0] = 0;
		for( int i = 1; i < g_curvePtsN; i++ )
		{
			g_curvePtsS[i] =
				(g_curvePtsY[i] - g_curvePtsY[i-1])/(g_curvePtsX[i] - g_curvePtsX[i-1]);
		}

#endif // _USESPLINE
#endif // _USEGSL
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
int hand_anim_get(const double** xs, const double** ys)
{
	DEBUGLOGB;

	if( xs ) *xs = g_curvePtsX;
	if( ys ) *ys = g_curvePtsY;
	int ret      = g_curvePtsN;

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
int hand_anim_get(double* xs, double* ys)
{
	DEBUGLOGB;

	if( xs )
		memcpy(xs, g_curvePtsX, g_curvePtsN*sizeof(double));

	if( ys )
		memcpy(ys, g_curvePtsY, g_curvePtsN*sizeof(double));

	int ret = g_curvePtsN;

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
double hand_anim_get(double x)
{
	DEBUGLOGB;

	double interpY = 0;

#ifdef _USEGSL
#ifdef _USEGSL_STATIC

	interpY = gsl_interp_eval(g_curveTerp, g_curvePtsX, g_curvePtsY, x, g_curveAccl);

#else // _USEGSL_STATIC

	if( g_pLLibok )
	{
		interpY = g_gsl_interp_eval(g_curveTerp, g_curvePtsX, g_curvePtsY, x, g_curveAccl);
	}
	else
	{
		interpY = g_curvePtsY[0];
		for( size_t p = 1; p < g_curvePtsN; p++ )
		{
			if( g_curvePtsX[p] >= x )
			{
				double a =    g_curvePtsS[p];
				double X = (x-g_curvePtsX[p-1]);
				double b =    g_curvePtsY[p-1];
				interpY  =  a*X+b;
				break;
			}
		}
	}

#endif // _USEGSL_STATIC
#else  // _USEGSL
#ifdef _USESPLINE

	interpY = g_curve(x);

#else

	interpY = g_curvePtsY[0];
	for( size_t p  = 1; p < g_curvePtsN; p++ )
	{
		if( g_curvePtsX[p] >= x )
		{
			double a =    g_curvePtsS[p];
			double X = (x-g_curvePtsX[p-1]);
			double b =    g_curvePtsY[p-1];
			interpY  =  a*X+b;
			break;
		}
	}

#endif // _USESPLINE
#endif // _USEGSL

	DEBUGLOGE;
	return interpY;
}

// -----------------------------------------------------------------------------
void hand_anim_add(double x, double y)
{
	DEBUGLOGB;

	if( g_curvePtsN < g_curvePtsM )
	{
		for( int p  = 1; p < g_curvePtsN; p++ )
		{
			if(  x > g_curvePtsX[p-1] && x < g_curvePtsX[p] )
			{
				int b = p;
				int e = g_curvePtsN;

				while(  e >= b )
				{
					g_curvePtsX[e] = g_curvePtsX[e-1];
					g_curvePtsY[e] = g_curvePtsY[e-1];
					e--;
				}

				g_curvePtsX[b] = x;
				g_curvePtsY[b] = y;

				DEBUGLOGP("added point (%f, %f) into the %d slot\n", x, y, b);
				DEBUGLOGP("lower x is %f, upper x is %f, # points is %d\n", g_curvePtsX[b-1], g_curvePtsX[b+1], g_curvePtsN+1);

				hand_anim_set(g_curvePtsX, g_curvePtsY, g_curvePtsN+1);
				break;
			}
		}
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void hand_anim_chg(int p, double x, double y)
{
	DEBUGLOGB;

	if( p >= 0 && p < g_curvePtsN )
	{
		g_curvePtsX[p] = x;
		g_curvePtsY[p] = y;

#ifdef _USEGSL
#ifdef _USEGSL_STATIC

		if( g_curveTerp )
			gsl_interp_init(g_curveTerp, g_curvePtsX, g_curvePtsY, g_curvePtsN);

		if( g_curveAccl )
			gsl_interp_accel_reset(g_curveAccl);

#else // _USEGSL_STATIC

		if( g_pLLibok )
		{
			if( g_curveTerp )
				g_gsl_interp_init(g_curveTerp, g_curvePtsX, g_curvePtsY, g_curvePtsN);

			if( g_curveAccl )
				g_gsl_interp_accel_reset(g_curveAccl);
		}
		else
		{
			g_curvePtsS[0] = 0;
			for( int i = 1; i < g_curvePtsN; i++ )
			{
				g_curvePtsS[i] =
					(g_curvePtsY[i] - g_curvePtsY[i-1])/(g_curvePtsX[i] - g_curvePtsX[i-1]);
			}
		}

#endif // _USEGSL_STATIC
#else //  _USEGSL
#ifdef _USESPLINE

		csp::set_points(g_curve, g_curvePtsX, g_curvePtsY, g_curvePtsN);

#else

		g_curvePtsS[0] = 0;
		for( int i = 1; i < g_curvePtsN; i++ )
		{
			g_curvePtsS[i] =
				(g_curvePtsY[i] - g_curvePtsY[i-1])/(g_curvePtsX[i] - g_curvePtsX[i-1]);
		}

#endif // _USESPLINE
#endif // _USEGSL
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void hand_anim_del(int p)
{
	DEBUGLOGB;

	if( p >= 0 && p < g_curvePtsN && g_curvePtsN > 3 )
	{
		for( ;    p < g_curvePtsN-1; p++ )
		{
			g_curvePtsX[p] = g_curvePtsX[p+1];
			g_curvePtsY[p] = g_curvePtsY[p+1];
		}

		hand_anim_set(g_curvePtsX, g_curvePtsY, g_curvePtsN-1);
	}

	DEBUGLOGE;
}

