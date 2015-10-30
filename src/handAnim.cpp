/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "hani"

#include <memory.h>         // for memcpy
#include <gsl/gsl_interp.h> // for spline interpolation
#include "handAnim.h"
#include "debug.h"          // for debugging prints

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static double            g_handAnimPtsX[1024];
static double            g_handAnimPtsY[1024];
static const size_t      g_handAnimPtsM = sizeof(g_handAnimPtsX)/sizeof(g_handAnimPtsX[0]);
static int               g_handAnimPtsN = 0;
static gsl_interp*       g_handAnimTerp = NULL;
static gsl_interp_accel* g_handAnimAccl = NULL;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void hand_anim_beg(const double* xs, const double* ys, int n)
{
	DEBUGLOGB;

	hand_anim_end();

//	g_handAnimTerp = gsl_interp_alloc      (gsl_interp_cspline, g_handAnimPtsM);
	g_handAnimAccl = gsl_interp_accel_alloc();

	hand_anim_set(xs, ys, n);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void hand_anim_end()
{
	DEBUGLOGB;

	gsl_interp_free(g_handAnimTerp);
	gsl_interp_accel_free(g_handAnimAccl);

	g_handAnimTerp = NULL;
	g_handAnimAccl = NULL;

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void hand_anim_set(const double* xs, const double* ys, int n)
{
	DEBUGLOGB;

	if( n > g_handAnimPtsM )
		n = g_handAnimPtsM;

	g_handAnimPtsN = n;

//	if( xs && ys &&  n )
	if( n && (xs || ys) )
	{
		for( int i = 0; i < n; i++ )
		{
			if( xs ) g_handAnimPtsX[i] = xs[i];
			if( ys ) g_handAnimPtsY[i] = ys[i];
		}

		if( g_handAnimTerp )
		{
			gsl_interp_free(g_handAnimTerp);
			g_handAnimTerp = NULL;
		}

		g_handAnimTerp = gsl_interp_alloc(gsl_interp_cspline, g_handAnimPtsN);

		if( g_handAnimTerp )
			gsl_interp_init(g_handAnimTerp, g_handAnimPtsX, g_handAnimPtsY, g_handAnimPtsN);

		if( g_handAnimAccl )
			gsl_interp_accel_reset(g_handAnimAccl);
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
int hand_anim_get(const double** xs, const double** ys)
{
	DEBUGLOGB;

	if( xs ) *xs = g_handAnimPtsX;
	if( ys ) *ys = g_handAnimPtsY;
	int ret      = g_handAnimPtsN;

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
int hand_anim_get(double* xs, double* ys)
{
	DEBUGLOGB;

	if( xs )
		memcpy(xs, g_handAnimPtsX, g_handAnimPtsN*sizeof(double));

	if( ys )
		memcpy(ys, g_handAnimPtsY, g_handAnimPtsN*sizeof(double));

	int ret = g_handAnimPtsN;

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
double hand_anim_get(double x)
{
	DEBUGLOGB;

	double interpY = gsl_interp_eval(g_handAnimTerp, g_handAnimPtsX, g_handAnimPtsY, x, g_handAnimAccl);

	DEBUGLOGE;
	return interpY;
}

// -----------------------------------------------------------------------------
void hand_anim_chg(int p, double x, double y)
{
	DEBUGLOGB;

	if( p >= 0 && p < g_handAnimPtsN )
	{
		g_handAnimPtsX[p] = x;
		g_handAnimPtsY[p] = y;

/*		if( g_handAnimTerp )
		{
			gsl_interp_free(g_handAnimTerp);
			g_handAnimTerp = NULL;
		}

		g_handAnimTerp = gsl_interp_alloc(gsl_interp_cspline, g_handAnimPtsN);*/

		if( g_handAnimTerp )
			gsl_interp_init(g_handAnimTerp, g_handAnimPtsX, g_handAnimPtsY, g_handAnimPtsN);

		if( g_handAnimAccl )
			gsl_interp_accel_reset(g_handAnimAccl);
	}

	DEBUGLOGE;
}

