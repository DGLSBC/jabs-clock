/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __loadGUI_h__
#define __loadGUI_h__

#include "platform.h" // platform specific

namespace lgui
{

bool init();
void dnit(bool unload=false);
bool okay();

bool okayGUI();

#if _USEGTK
GtkWidget* pWidget(const char* name);

bool getXml(const char* name, const char* root=NULL, GCallback callBack=NULL);
#endif

} // lgui

#endif // __loadGUI_h__

