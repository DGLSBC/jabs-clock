/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __helpGUI_h__
#define __helpGUI_h__

#include "platform.h" // platform specific

namespace hgui
{

#if _USEGTK
void init(GtkWidget* pDialog, char key);
#endif

} // hgui

#endif // __helpGUI_h__

