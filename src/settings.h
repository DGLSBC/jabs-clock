/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __settings_h__
#define __settings_h__

#include "platform.h" // platform specific
#include "basecpp.h"  // some useful macros and functions
#include "themes.h"   // for ThemeEntry

// -----------------------------------------------------------------------------
namespace prefs
{

#if _USEGTK
typedef void (*TOGBTNCB)(GtkToggleButton*, gpointer);
#endif // _USEGTK

void begin();
void end  ();
void open (bool opening);
bool opend();

#if _USEGTK
void init(GtkWidget* pPropsDialog);

GtkWidget* initControl  (const char* name, const char* event,            GCallback callback, GtkWidget* pWindow =NULL);
void       initToggleBtn(const char* name, const char* event, int value, TOGBTNCB  callback, gpointer   userData=NULL);

void on_24_hour_toggled        (GtkToggleButton* pToggButton, gpointer window);
void on_click_thru_toggled     (GtkToggleButton* pToggButton, gpointer window);
void on_face_date_toggled      (GtkToggleButton* pToggButton, gpointer window);
void on_hands_only_toggled     (GtkToggleButton* pToggButton, gpointer window);
void on_keep_on_bot_toggled    (GtkToggleButton* pToggButton, gpointer window);
void on_keep_on_top_toggled    (GtkToggleButton* pToggButton, gpointer window);
void on_no_display_toggled     (GtkToggleButton* pToggButton, gpointer window);
void on_seconds_toggled        (GtkToggleButton* pToggButton, gpointer window);
void on_show_date_toggled      (GtkToggleButton* pToggButton, gpointer window);
void on_show_in_pager_toggled  (GtkToggleButton* pToggButton, gpointer window);
void on_show_in_taskbar_toggled(GtkToggleButton* pToggButton, gpointer window);
void on_sticky_toggled         (GtkToggleButton* pToggButton, gpointer window);
void on_take_focus_toggled     (GtkToggleButton* pToggButton, gpointer window);
void on_text_only_toggled      (GtkToggleButton* pToggButton, gpointer window);
#endif // _USEGTK

void set_ani_rate(int renderRate);
void set_theme_info(const ThemeEntry& te);

#if _USEGTK
GtkToggleButton* ToggleBtnSet(const char* name, int value);
#endif // _USEGTK

extern const char* PREFSTR_24HOUR;
extern const char* PREFSTR_FACEDATE;
extern const char* PREFSTR_HEIGHT;
extern const char* PREFSTR_KEEPONBOT;
extern const char* PREFSTR_KEEPONTOP;
extern const char* PREFSTR_SECONDS;
extern const char* PREFSTR_SHOWDATE;
extern const char* PREFSTR_SHOWINPAGER;
extern const char* PREFSTR_SHOWINTASKS;
extern const char* PREFSTR_STICKY;
extern const char* PREFSTR_TEXTONLY;
extern const char* PREFSTR_WIDTH;
extern const char* PREFSTR_X;
extern const char* PREFSTR_Y;

}

#endif // __settings_h__

