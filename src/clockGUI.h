/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __clockGUI_h__
#define __clockGUI_h__

#include "platform.h" // platform specific

namespace cgui
{

bool init();
void dnit(bool clrPopup=true, bool unloadGUI=false);

void initDragDrop();

void initWorkspace();
void dnitWorkspace();

#if _USEGTK
void setPopup(GtkWidget* pPopupDlg, bool inPopup, char key);
#endif

} // cgui

#endif // __clockGUI_h__

