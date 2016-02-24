/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __platform_h__
#define __platform_h__

// NOTE: one of these specify the build type along with the associated src/makefile lib dep

#define  _USEGTK 1 // only one usable for now
#define  _USEKDE 0 // a lot of code has not been converted yet (not usable yet)

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#if      _USEGTK
#include <gtk/gtk.h>        // base
#include <gdk/gdk.h>        // for ?
#include <gdk/gdkx.h>       // for ?
#include <gdk/gdkkeysyms.h> // for GDK_... key defines

typedef GtkWidget  PWidget;
#endif //_USEGTK

#if      _USEKDE
#define   QT3_SUPPORT
#include <QApplication>     // base
#include <QWidget>

typedef QWidget    PWidget;
#endif //_USEKDE

#endif // __platform_h__

