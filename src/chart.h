/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __chart_h__
#define __chart_h__

#include "platform.h" // platform specific

namespace chart
{

#if _USEGTK
void init(GtkWidget* pDrawingArea);
#endif

}

#endif // __chart_h__

