/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "dbug"

#include <glib.h>    // for time related
#include <stdio.h>   // for fflush, printf, stdout, vprintf
#include "debug.h"   //
#include "global.h"  // for gRun.appDebug

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void DEBUGLOGL(bool force, const char* nsp, const char* fnc, const char* fmt, ...)
{
	if( force || gRun.appDebug )
	{
		GTimeVal            ctv;
		g_get_current_time(&ctv);
		long     sec =     (ctv.tv_sec %       (60));
		long     min =     (ctv.tv_sec %    (60*60))/(60);
		long     hor =     (ctv.tv_sec % (24*60*60))/(60*60);

		va_list  lst;
		va_start(lst, fmt);
		printf  ("%2.2d:%2.2d:%2.2d.%3.3d: %s::%s: ", (int)hor, (int)min, (int)sec, (int)(ctv.tv_usec/1000), nsp, fnc);
		vprintf (fmt, lst); 
		va_end  (lst);
		fflush  (stdout);
	}
}

// -----------------------------------------------------------------------------
void DEBUGPAUZ(bool force, int msecs, const char* nsp, const char* fnc, const char* log)
{
	if( force || gRun.appDebug )
	{
		if( log )
			DEBUGLOGL(force, nsp, fnc, "bef sleep %s\n", log);

		g_usleep((unsigned int)((double)G_USEC_PER_SEC*(double)msecs/1000.0));

		if( log )
			DEBUGLOGL(force, nsp, fnc, "aft sleep %s\n", log);
	}
}

