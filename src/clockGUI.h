/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __clockGUI_h__
#define __clockGUI_h__

#include <glade/glade.h>
#include "basecpp.h" // some useful macros and functions

namespace gui
{

bool   init();
void   dnit(bool clrPopup=true);

const  gchar* getGladeFilename();

void   initDragDrop();

void   initWorkspace();
void   dnitWorkspace();

extern GladeXML* pGladeXml;

} // gui

#endif // __clockGUI_h__

