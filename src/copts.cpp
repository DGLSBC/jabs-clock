/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP  "copts"

#include "copts.h"  //
#include "config.h" // for gCfg fields
#include "debug.h"  // for debugging prints

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool copts::keep_on_bot(PWidget* window, int val)
{
	if( val != NOVALPARM )
	{
		DEBUGLOGB;

//		gCfg.keepOnBot = (bool)val;
		gCfg.keepOnBot = val;

		if( gCfg.keepOnBot )
		{
//			gCfg.keepOnTop = false;
			gCfg.keepOnTop = FALSE;
		}

		if( window )
		{
#if _USEGTK
			DEBUGLOGP("making clock %s other windows\n",  gCfg.keepOnTop ? "above" : (gCfg.keepOnBot ? "below" : "just like"));
			gtk_window_set_keep_below(GTK_WINDOW(window), gCfg.keepOnBot);
//			gtk_window_set_keep_above(GTK_WINDOW(window), gCfg.keepOnBot ? false : gCfg.keepOnTop);
			gtk_window_set_keep_above(GTK_WINDOW(window), gCfg.keepOnBot ? FALSE : gCfg.keepOnTop);
#endif
		}

		DEBUGLOGE;
	}

//	return gCfg.keepOnBot;
	return gCfg.keepOnBot ? true : false;
}

// -----------------------------------------------------------------------------
bool copts::keep_on_top(PWidget* window, int val)
{
	if( val != NOVALPARM )
	{
		DEBUGLOGB;

//		gCfg.keepOnTop = (bool)val;
		gCfg.keepOnTop = val;

		if( gCfg.keepOnTop )
		{
//			gCfg.keepOnBot = false;
			gCfg.keepOnBot = FALSE;
		}

		if( window )
		{
#if _USEGTK
			DEBUGLOGP("making clock %s other windows\n",  gCfg.keepOnTop ? "above" : (gCfg.keepOnBot ? "below" : "just like"));
			gtk_window_set_keep_above(GTK_WINDOW(window), gCfg.keepOnTop);
//			gtk_window_set_keep_below(GTK_WINDOW(window), gCfg.keepOnTop ? false : gCfg.keepOnBot);
			gtk_window_set_keep_below(GTK_WINDOW(window), gCfg.keepOnTop ? FALSE : gCfg.keepOnBot);
#endif
		}

		DEBUGLOGE;
	}

//	return gCfg.keepOnTop;
	return gCfg.keepOnTop ? true : false;
}

// -----------------------------------------------------------------------------
bool copts::pagebar_shown(PWidget* window, int val)
{
	if( val != NOVALPARM )
	{
		DEBUGLOGB;

//		gCfg.showInPager = (bool)val;
		gCfg.showInPager = val;

		if( window )
		{
#if _USEGTK
			DEBUGLOGP("making clock %s in pager\n", gCfg.showInPager ? "show" : "NOT show");
			gtk_window_set_skip_pager_hint(GTK_WINDOW(window), !gCfg.showInPager);
#endif
		}

		DEBUGLOGE;
	}

//	return gCfg.showInPager;
	return gCfg.showInPager ? true : false;
}

// -----------------------------------------------------------------------------
bool copts::sticky(PWidget* window, int val)
{
	if( val != NOVALPARM )
	{
		DEBUGLOGB;

//		gCfg.sticky = (bool)val;
		gCfg.sticky = val;

		if( window )
		{
#if _USEGTK
			DEBUGLOGP("making clock %s\n", gCfg.sticky ? "sticky" : "unsticky");
			if( gCfg.sticky )
				gtk_window_stick  (GTK_WINDOW(window));
			else
				gtk_window_unstick(GTK_WINDOW(window));
#endif
		}

		DEBUGLOGE;
	}

//	return gCfg.sticky;
	return gCfg.sticky ? true : false;
}

// -----------------------------------------------------------------------------
bool copts::taskbar_shown(PWidget* window, int val)
{
	if( val != NOVALPARM )
	{
		DEBUGLOGB;

//		gCfg.showInTasks = (bool)val;
		gCfg.showInTasks = val;

		if( window )
		{
#if _USEGTK
			DEBUGLOGP("making clock %s in taskbar\n", gCfg.showInTasks ? "show" : "NOT show");
			gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), !gCfg.showInTasks);
#endif
		}

		DEBUGLOGE;
	}

//	return gCfg.showInTasks;
	return gCfg.showInTasks ? true : false;
}

