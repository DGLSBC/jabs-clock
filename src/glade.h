/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __glade_h__
#define __glade_h__

#include "platform.h" // platform specific

namespace glade
{

bool init();
void dnit(bool unload=false);
bool okay();

bool okayGUI();

#if _USEGTK
GtkWidget* pWidget(const char* name);

bool getXml(const char* name, const char* root=NULL, GCallback callBack=NULL);
#endif // _USEGTK

} // glade

#endif // __glade_h__

