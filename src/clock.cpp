/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "clok"

#include "clock.h"
#include "config.h"
#include "debug.h"   // for debugging prints

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool cclock::keep_on_bot(GtkWidget* window, int val)
{
	if( val != NOVALPARM )
	{
		gCfg.keepOnBot = (bool)val;

		if( window )
		{
			gtk_window_set_keep_below(GTK_WINDOW(window), gCfg.keepOnBot);
			gtk_window_set_keep_above(GTK_WINDOW(window), gCfg.keepOnBot ? false : gCfg.keepOnTop);
		}
	}

	return gCfg.keepOnBot;
}

// -----------------------------------------------------------------------------
bool cclock::keep_on_top(GtkWidget* window, int val)
{
	if( val != NOVALPARM )
	{
		gCfg.keepOnTop = (bool)val;

		if( window )
		{
			gtk_window_set_keep_above(GTK_WINDOW(window), gCfg.keepOnTop);
			gtk_window_set_keep_below(GTK_WINDOW(window), gCfg.keepOnTop ? false : gCfg.keepOnBot);
		}
	}

	return gCfg.keepOnTop;
}

// -----------------------------------------------------------------------------
bool cclock::pagebar_shown(GtkWidget* window, int val)
{
	if( val != NOVALPARM )
	{
		gCfg.showInPager = (bool)val;

		if( window )
		{
			gtk_window_set_skip_pager_hint(GTK_WINDOW(window), !gCfg.showInPager);
		}
	}

	return gCfg.showInPager;
}

// -----------------------------------------------------------------------------
bool cclock::sticky(GtkWidget* window, int val)
{
	if( val != NOVALPARM )
	{
		gCfg.sticky = (bool)val;

		if( window )
		{
			if( gCfg.sticky )
				gtk_window_stick  (GTK_WINDOW(window));
			else
				gtk_window_unstick(GTK_WINDOW(window));
		}
	}

	return gCfg.sticky;
}

// -----------------------------------------------------------------------------
bool cclock::taskbar_shown(GtkWidget* window, int val)
{
	if( val != NOVALPARM )
	{
		gCfg.showInTasks = (bool)val;

		if( window )
		{
			gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), !gCfg.showInTasks);
		}
	}

	return gCfg.showInTasks;
}

