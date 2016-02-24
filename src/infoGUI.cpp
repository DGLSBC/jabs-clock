/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP    "igui"

#include "infoGUI.h"  //
#include "loadGUI.h"  // for lgui::pWidget
#include "basecpp.h"  // some useful macros and functions
#include "clockGUI.h" // for cgui::setPopup
#include "global.h"   // for gRun.appVersion and get_theme_icon_filename
#include "debug.h"    // for debugging prints

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
namespace igui
{

static const char* get_logo_filename();

} // namespace igui

#if _USEGTK
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void igui::init(GtkWidget* pDialog, char key)
{
	DEBUGLOGB;

	if( pDialog )
	{
		gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(pDialog), gRun.appVersion);

		if( get_logo_filename() )
			gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(pDialog), gdk_pixbuf_new_from_file(get_logo_filename(), NULL));

		cgui::setPopup(pDialog, true, key);
	}

	DEBUGLOGE;
}
#endif // _USEGTK

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
const char* igui::get_logo_filename()
{
#ifdef _RELEASE
	return PKGDATA_DIR "/pixmaps/" APP_NAME_OLD "-logo.png";
#else
	return get_theme_icon_filename();
#endif
}

