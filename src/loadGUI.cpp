/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "lgui"

#include "loadGUI.h" //
#include "debug.h"   // for debugging prints

#if      _USEGTK
#if GTK_CHECK_VERSION(3,0,0)
#undef   _USEGLADE
#define  _USEBUILDER
#else
#include "glade.h"   //
#define  _USEGLADE
#undef   _USEBUILDER
#endif
#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool lgui::init()
{
#ifdef  _USEGLADE
	return glade::init();
#else
	return false;
#endif
}

// -----------------------------------------------------------------------------
void lgui::dnit(bool unload)
{
#ifdef _USEGLADE
	glade::dnit(unload);
#else
	// nothing to do yet
#endif
}

// -----------------------------------------------------------------------------
bool lgui::okay()
{
#ifdef _USEGLADE
	return glade::okay();
#else
	return false;
#endif
}

// -----------------------------------------------------------------------------
bool lgui::okayGUI()
{
#ifdef _USEGLADE
	return glade::okayGUI();
#else
	return false;
#endif
}

#if    _USEGTK
// -----------------------------------------------------------------------------
GtkWidget* lgui::pWidget(const char* name)
{
#ifdef _USEGLADE
	return glade::pWidget(name);
#else
	return NULL;
#endif
}
#endif

#if    _USEGTK
// -----------------------------------------------------------------------------
bool lgui::getXml(const char* name, const char* root, GCallback callBack)
{
#ifdef _USEGLADE
	return glade::getXml(name, root, callBack);
#else
	return false;
#endif
}
#endif

