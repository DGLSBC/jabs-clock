/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __copts_h__
#define __copts_h__

#include "platform.h" // platform specific

// -----------------------------------------------------------------------------
namespace copts
{

#define NOVALPARM 876543210

bool keep_on_bot  (PWidget* window=NULL, int val=NOVALPARM);
bool keep_on_top  (PWidget* window=NULL, int val=NOVALPARM);
bool pagebar_shown(PWidget* window=NULL, int val=NOVALPARM);
bool sticky       (PWidget* window=NULL, int val=NOVALPARM);
bool taskbar_shown(PWidget* window=NULL, int val=NOVALPARM);

} // copts

#endif // __copts_h__

