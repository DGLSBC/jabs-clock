/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "llib"

#include <gmodule.h> // for runtime module loading types & funcs
#include <limits.h>  // for PATH_MAX

#include "loadlib.h" //
#include "basecpp.h" // for strv... funcs
#include "debug.h"   // for debugging prints

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
namespace lib
{

struct LoadLib
{
	GModule* pShared;
	symb*    pSymbs;
	size_t   nSymbs;
	size_t   iSymb;
};

static bool load(const char* name, const char* sufx, symb* symbs, size_t nsymbs);
static bool func(gpointer* sfunc);
static bool free();

extern "C"
{
static gboolean load_symbs(GModule* pShared, symb*& symbs, size_t nsymbs);
}

static GModule* g_pShared = NULL;
static symb*    g_pSymbs  = NULL;
static size_t   g_nSymbs  = 0;
static size_t   g_iSymb   = 0;

}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
lib::LoadLib* lib::create()
{
	DEBUGLOGB;

	LoadLib* pLLib = new LoadLib;

	if( pLLib )
	{
		pLLib->pShared = NULL;
		pLLib->pSymbs  = NULL;
		pLLib->nSymbs  = 0;
		pLLib->iSymb   = 0;
	}

	DEBUGLOGE;
	return pLLib;
}

// -----------------------------------------------------------------------------
bool lib::load(LoadLib* pLLib, const char* name, const char* sufx, symb* symbs, size_t nsymbs)
{
	DEBUGLOGB;

	bool ret = false;

	if( pLLib )
	{
		ret = load(name, sufx, symbs, nsymbs);

		pLLib->pShared = g_pShared;
		pLLib->pSymbs  = g_pSymbs;
		pLLib->nSymbs  = g_nSymbs;
		pLLib->iSymb   = g_iSymb;

		DEBUGLOGP("  module is %s\n", g_pShared ? g_module_name(g_pShared) : "null");
	}

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
bool lib::func(LoadLib* pLLib, gpointer* sfunc)
{
	DEBUGLOGB;

	bool ret = false;

	if( pLLib )
	{
		g_pShared = pLLib->pShared;
		g_pSymbs  = pLLib->pSymbs;
		g_nSymbs  = pLLib->nSymbs;
		g_iSymb   = pLLib->iSymb;

		DEBUGLOGP("  module is %s\n", g_pShared ? g_module_name(g_pShared) : "null");

		ret = func(sfunc);

		pLLib->pShared = g_pShared;
		pLLib->pSymbs  = g_pSymbs;
		pLLib->nSymbs  = g_nSymbs;
		pLLib->iSymb   = g_iSymb;
	}

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
bool lib::destroy(LoadLib* pLLib)
{
	DEBUGLOGB;

	bool ret = false;

	if( pLLib )
	{
		g_pShared = pLLib->pShared;
		g_pSymbs  = pLLib->pSymbs;
		g_nSymbs  = pLLib->nSymbs;
		g_iSymb   = pLLib->iSymb;

		DEBUGLOGP("  module is %s\n", g_pShared ? g_module_name(g_pShared) : "null");

		ret = free();

		pLLib->pShared = g_pShared;
		pLLib->pSymbs  = g_pSymbs;
		pLLib->nSymbs  = g_nSymbs;
		pLLib->iSymb   = g_iSymb;

		delete pLLib;
	}

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool lib::load(const char* name, const char* sufx, symb* symbs, size_t nsymbs)
{
	DEBUGLOGB;

	bool okay = false;

	if( !g_module_supported() )
	{
		DEBUGLOGS("runtime module loading NOT supported");
		DEBUGLOGE;
		return okay;
	}

	DEBUGLOGS("runtime module loading supported");

	char* path = g_module_build_path(NULL, name);

	if( !path )
	{
		DEBUGLOGP("%s shared library path NOT built\n", name);
		DEBUGLOGP("%s\n", g_module_error());
		DEBUGLOGE;
		return okay;
	}

	DEBUGLOGP("%s shared library path built - path is:\n\t%s\n", name, path);

	g_pShared = g_module_open(path, GModuleFlags(G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL));
	g_pSymbs  = NULL;
	g_nSymbs  = 0;
	g_iSymb   = 0;

	if( !g_pShared && sufx )
	{
		DEBUGLOGP("%s shared library module was NOT opened - trying secondary\n", name);

		char    pths[PATH_MAX];
		strvcpy(pths, path);
		strvcat(pths, sufx);

		g_pShared = g_module_open(pths, GModuleFlags(G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL));
	}

	g_free(path);

	if( !g_pShared )
	{
		DEBUGLOGP("%s shared library module was NOT opened\n", name);
		DEBUGLOGP("%s\n", g_module_error());
		DEBUGLOGE;
		return okay;
	}

	DEBUGLOGP("%s shared library module was opened\n", name);

	okay = load_symbs(g_pShared, symbs, nsymbs);

	if( okay )
	{
		DEBUGLOGP("%s shared library module requested symbols were loaded\n", name);
		g_pSymbs =  symbs;
		g_nSymbs = nsymbs;
	}
	else
	{
		DEBUGLOGP("%s shared library module requested symbols were NOT loaded\n", name);
	}

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
bool lib::func(gpointer* sfunc)
{
	DEBUGLOGB;
	bool   okay =  sfunc && g_iSymb >= 0 && g_iSymb < g_nSymbs;
	if(    okay ) *sfunc =  g_pSymbs[g_iSymb++].symb;
	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
bool lib::free()
{
	DEBUGLOGB;

	bool okay = false;

	if( g_pShared )
	{
		DEBUGLOGS("module closed");
		okay = g_module_close(g_pShared) == TRUE;
//		g_object_unref(g_pShared);
	}

	g_pShared = NULL;
	g_pSymbs  = NULL;
	g_nSymbs  = 0;
	g_iSymb   = 0;

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
extern "C"
{

// -----------------------------------------------------------------------------
gboolean lib::load_symbs(GModule* pShared, symb*& symbs, size_t nsymbs)
{
	DEBUGLOGB;

	gboolean okay = TRUE;

	for( size_t s = 0; s < nsymbs; s++ )
		symbs[s].symb = NULL;

	for( size_t s = 0; s < nsymbs; s++ )
	{
		g_module_symbol(pShared, symbs[s].name,  &symbs[s].symb);

		okay = symbs[s].symb != NULL;

		DEBUGLOGP("  symbol %s address was %s\n", symbs[s].name, okay ? "set" : "NOT set");

		if( !okay )
			break;
	}

	DEBUGLOGE;
	return okay;
}

} // extern "C"

