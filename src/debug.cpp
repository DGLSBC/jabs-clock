/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "dbg"

#include "global.h"
#include "debug.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void DEBUGLOGL(bool force, const char* nsp, const char* fnc, const char* fmt, ...)
{
	if( force || gRun.appDebug )
	{
		va_list  lst;
		va_start(lst, fmt);
		 printf ("%s::%s: ", nsp, fnc);
		vprintf (fmt, lst); 
		va_end  (lst);
		fflush  (stdout);
	}
}

