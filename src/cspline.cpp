/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "csp"

#include <vector>    //
#include "cspline.h" //
#include "debug.h"   // for debugging prints

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
//void csp::set_points(tk::spline& tki, const double* x, const double* y, size_t n)
void csp::set_points(cspline& tki, const double* x, const double* y, size_t n)
{
	std::vector<double> tkx(n);
	std::vector<double> tky(n);

	for( int i = 0; i < n; i++ )
	{
		tkx[i] = x[i];
		tky[i] = y[i];
	}

	tki.set_points(tkx, tky);
}

