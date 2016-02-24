/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __x_h__
#define __x_h__

#include "platform.h" // platform specific

#if      _USEGTK
#if      !GTK_CHECK_VERSION(3,0,0)
#define  _USEWKSPACE 0  // partially working via own code, or fully via wnck (but wnck may hang the system?)
#endif
#endif

#if _USEWKSPACE
#define   WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#endif

void g_init_threads();

#if _USEGTK
void g_window_workspace(GtkWidget* pWidget, int space);
#endif

int  g_workspace_count();

#endif // __x_h__

