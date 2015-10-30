/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __clock_h__
#define __clock_h__

#include "global.h"
#define   NOVALPARM 876543210

// -----------------------------------------------------------------------------
namespace cclock
{

bool keep_on_bot  (GtkWidget* window=NULL, int val=NOVALPARM);
bool keep_on_top  (GtkWidget* window=NULL, int val=NOVALPARM);
bool pagebar_shown(GtkWidget* window=NULL, int val=NOVALPARM);
bool sticky       (GtkWidget* window=NULL, int val=NOVALPARM);
bool taskbar_shown(GtkWidget* window=NULL, int val=NOVALPARM);

} // cclock

#endif // __clock_h__

