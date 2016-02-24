/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP    "hgui"

#include "helpGUI.h"  //
#include "loadGUI.h"  // for lgui::pWidget
#include "basecpp.h"  // some useful macros and functions
#include "clockGUI.h" // for cgui::setPopup
#include "debug.h"    // for debugging prints

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
namespace hgui
{

#if _USEGTK
static void on_btn_clicked(GtkButton* pButton, gpointer window);
#endif

static const char* bfmt = "buttonTopic%d";
static const char* sfmt = "scrolledwindowHelp%d";

#define MAXTOPICS   8
#define MINBTNNAME 12
#define MAXBTNNAME 13
#define MAXWGTNAME 32

} // namespace hgui

#if _USEGTK
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void hgui::init(GtkWidget* pDialog, char key)
{
	DEBUGLOGB;

	if( pDialog )
	{
		GtkWidget* pWidget;

		for( int b = 1; b <= MAXTOPICS; b++ )
		{
			gchar    wnm[MAXWGTNAME];
			snprintf(wnm, vectsz(wnm), bfmt, b);
			if( !(pWidget = lgui::pWidget(wnm)) ) break; // done
			g_signal_connect(G_OBJECT(pWidget), "clicked", G_CALLBACK(on_btn_clicked), NULL);
		}

		cgui::setPopup(pDialog, true, key);
	}

	DEBUGLOGE;
}
#endif // _USEGTK

#if _USEGTK
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void hgui::on_btn_clicked(GtkButton* pButton, gpointer window)
{
	DEBUGLOGB;

	const gchar* bnm  = gtk_widget_get_name(GTK_WIDGET(pButton));
	int          nsz  = bnm ? strlen(bnm) : 0;

	if( bnm &&   nsz >= MINBTNNAME && nsz <= MAXBTNNAME )
	{
		GtkWidget* pWidget;
		gchar      wnm[MAXWGTNAME];
		int        btn = 0;

		sscanf  (bnm,              bfmt, &btn);
		snprintf(wnm, vectsz(wnm), sfmt,  btn);

		if( pWidget = lgui::pWidget(wnm) )
		{
			for( int b = 1; b <= MAXTOPICS; b++ )
			{
				gchar    wnm[MAXWGTNAME];
				snprintf(wnm, vectsz(wnm), sfmt, b);
				GtkWidget* pWidget = lgui::pWidget(wnm);
				if(       !pWidget ) break; // done
				gtk_widget_set_visible(pWidget, FALSE);
			}

			gtk_widget_set_visible(pWidget, TRUE);
		}

		if( pWidget = lgui::pWidget("labelHelp") )
		{
			const gchar* blb = gtk_button_get_label(pButton);
			int          lsz = blb ? strlen(blb) : 0;

			if( blb && lsz )
			{
				gchar    lbl[MAXWGTNAME];
				snprintf(lbl, vectsz(lbl), "<b>%s</b>",  blb);
				gtk_label_set_markup(GTK_LABEL(pWidget), lbl);
			}
		}
	}

	DEBUGLOGE;
}
#endif // _USEGTK

