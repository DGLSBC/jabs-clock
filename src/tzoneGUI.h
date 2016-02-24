/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __tzoneGUI_h__
#define __tzoneGUI_h__

#include "platform.h" // for platform specific

// -----------------------------------------------------------------------------
namespace tzng
{

#if _USEGTK
void init(GtkWidget* pDialog, char key);
#endif

} // tzng

#endif // __tzoneGUI_h__

