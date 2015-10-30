/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __settings_h__
#define __settings_h__

#include <gtk/gtk.h>
#include "basecpp.h" // some useful macros and functions

// -----------------------------------------------------------------------------
namespace prefs
{

void begin();
void end  ();
void open (bool opening=true);
void init (GtkWidget* pPropsDialog, GtkWindow*& g_pPopupDlg, bool& g_inPopup, char& g_nmPopup);

void on_24h_toggled            (GtkToggleButton* pToggButton, gpointer window);
void on_face_date_toggled      (GtkToggleButton* pToggButton, gpointer window);
void on_keep_on_bot_toggled    (GtkToggleButton* pToggButton, gpointer window);
void on_keep_on_top_toggled    (GtkToggleButton* pToggButton, gpointer window);
void on_seconds_toggled        (GtkToggleButton* pToggButton, gpointer window);
void on_show_date_toggled      (GtkToggleButton* pToggButton, gpointer window);
void on_show_in_pager_toggled  (GtkToggleButton* pToggButton, gpointer window);
void on_show_in_taskbar_toggled(GtkToggleButton* pToggButton, gpointer window);
void on_sticky_toggled         (GtkToggleButton* pToggButton, gpointer window);

void set_ani_rate(int refreshRate);

}

#endif // __settings_h__

