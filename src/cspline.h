/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __cspline_h__
#define __cspline_h__

#include <stddef.h>
#include "tkspline.h"

typedef tk::spline cspline;

namespace csp
{

void set_points(cspline& tki, const double* x, const double* y, size_t n);

}

#endif // __cspline_h__

