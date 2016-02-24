/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __loadlib_h__
#define __loadlib_h__

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include "platform.h" // platform specific

namespace lib
{

struct LoadLib;

struct symb
{
	const char* name;
	gpointer    symb;
};

LoadLib* create ();
bool     load   (LoadLib* pLLib, const char* name, const char* sufx, symb* symbs, size_t nsymbs);
bool     func   (LoadLib* pLLib, gpointer* sfunc);
bool     destroy(LoadLib* pLLib);

}

// convenience macro for passing function addresses into lib::func
#define LIB_FUNC_ADDR(LFA) (gpointer*)&LFA

#endif // __loadlib_h__

