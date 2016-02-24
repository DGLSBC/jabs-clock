/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __infoGUI_h__
#define __infoGUI_h__

#include "platform.h" // platform specific

namespace igui
{

#if _USEGTK
void init(GtkWidget* pDialog, char key);
#endif

} // igui

#endif // __infoGUI_h__

