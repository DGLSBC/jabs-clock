/* Xlib utils */
/* vim: set sw=2 et: */

/*
 * Copyright (C) 2001 Havoc Pennington
 * Copyright (C) 2005-2007 Vincent Untz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
//
// Taken from libwnck code and modified slightly for this app's use.
//

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#undef    DEBUGLOG
#define   DEBUGNSP     "x"

#include "x.h"         //
#include "debug.h"     // for debugging prints
#include <X11/Xlib.h>  // for ?
#include <X11/Xatom.h> // for ?

#if _USEGTK
#if !_USEWKSPACE
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#define   WNCK_CLIENT_TYPE_APPLICATION 1
#define  _wncki_atom_get(atom_name)    gdk_x11_get_xatom_by_name(atom_name)
#define  _wncki_get_client_type()      WNCK_CLIENT_TYPE_APPLICATION

// -----------------------------------------------------------------------------
static void _wncki_change_workspace(Screen*  xscreen, Window xwindow, int  new_space);
static void _wncki_error_trap_push (Display* xdisplay);
static int  _wncki_error_trap_pop  (Display* xdisplay);
static bool _wncki_get_cardinal    (Screen*  xscreen, Window xwindow, Atom xatom, int* val);
static int  _wncki_screen_get_workspace_count(Screen* xscreen);
#endif
#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void g_init_threads()
{
	DEBUGLOGB;

	XInitThreads();

	DEBUGLOGE;
}

#if _USEGTK
// -----------------------------------------------------------------------------
void g_window_workspace(GtkWidget* pWidget, int space)
{
	DEBUGLOGB;

#if !_USEWKSPACE

#if GTK_CHECK_VERSION(3,0,0)
	Screen* xscreen = gdk_x11_screen_get_xscreen(gdk_screen_get_default());
	Window  xwindow = gdk_x11_window_get_xid(gtk_widget_get_window(pWidget));
#else
	Screen* xscreen = GDK_SCREEN_XSCREEN(gdk_screen_get_default());
	Window  xwindow = GDK_WINDOW_XWINDOW(gtk_widget_get_window(pWidget));
#endif

	_wncki_change_workspace(xscreen, xwindow, space);

#else  // !_USEWKSPACE

	WnckScreen* screen = wnck_screen_get_default();

	if( screen )
	{
		DEBUGLOGS("got default screen");
		wnck_screen_force_update(screen);

		DEBUGLOGP("sticky is off and workspace is %d\n", space);
		WnckWindow* window = wnck_window_get(gdk_x11_drawable_get_xid(gtk_widget_get_window(pWidget)));

		if( window && !wnck_window_is_pinned(window) )
		{
			DEBUGLOGS("got non-pinned window");
			WnckWorkspace* trgtWrk = wnck_screen_get_workspace       (screen, space);
			WnckWorkspace* actvWrk = wnck_screen_get_active_workspace(screen);

			if( trgtWrk && actvWrk && trgtWrk != actvWrk )
			{
				DEBUGLOGS("got target workspace is diff from current so moving window to target");
				wnck_window_move_to_workspace(window, trgtWrk);
			}
		}
#ifdef _DEBUGLOG
		else
		{
			DEBUGLOGP("WnckWindow for clock window is%svalid\n", window ? " " : " NOT ");
			guint xw1 = GDK_WINDOW_XWINDOW      (gtk_widget_get_window(pWidget));
			guint xw2 = gdk_x11_drawable_get_xid(gtk_widget_get_window(pWidget));
			DEBUGLOGP("X11 XID1 for clock window is %d\n", (int)xw1);
			DEBUGLOGP("X11 XID2 for clock window is %d\n", (int)xw2);
		}
#endif
	}

#endif // !_USEWKSPACE

	DEBUGLOGE;
}
#endif // _USEGTK

// -----------------------------------------------------------------------------
int g_workspace_count()
{
	DEBUGLOGB;

#if _USEGTK
#if _USEWKSPACE

	int     ret     =  wnck_screen_get_workspace_count(wnck_screen_get_default());

#else  // _USEWKSPACE

#if GTK_CHECK_VERSION(3,0,0)
	Screen* xscreen =  gdk_x11_screen_get_xscreen(gdk_screen_get_default());
#else
	Screen* xscreen =  GDK_SCREEN_XSCREEN(gdk_screen_get_default());
#endif
	int     ret     = _wncki_screen_get_workspace_count(xscreen);

#endif // _USEWKSPACE
#else  // _USEGTK

	int     ret     = 1;

#endif // _USEGTK

	DEBUGLOGE;
	return ret;
}

#if _USEGTK
#if !_USEWKSPACE
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void _wncki_change_workspace(Screen* xscreen, Window xwindow, int new_space)
{
	DEBUGLOGB;

	XEvent   xev;
	Display* display = DisplayOfScreen   (xscreen);
	Window   root    = RootWindowOfScreen(xscreen);

	xev.xclient.type         =  ClientMessage;
	xev.xclient.serial       =  0;
	xev.xclient.send_event   =  True;
	xev.xclient.display      =  display;
	xev.xclient.window       =  xwindow;
	xev.xclient.message_type = _wncki_atom_get("_NET_WM_DESKTOP");
	xev.xclient.format       =  32;
	xev.xclient.data.l[0]    =  new_space;
	xev.xclient.data.l[1]    = _wncki_get_client_type();
	xev.xclient.data.l[2]    =  0;
	xev.xclient.data.l[3]    =  0;
	xev.xclient.data.l[4]    =  0;

	_wncki_error_trap_push(display);
	XSendEvent(display, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
	_wncki_error_trap_pop (display);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void _wncki_error_trap_push(Display* xdisplay)
{
	gdk_error_trap_push();
}

// -----------------------------------------------------------------------------
int _wncki_error_trap_pop(Display* xdisplay)
{
	XSync(xdisplay, False);
	return gdk_error_trap_pop();
}

// -----------------------------------------------------------------------------
bool _wncki_get_cardinal(Screen* xscreen, Window xwindow, Atom xatom, int* val)
{
	*val = 0;

	Display* xdisplay = DisplayOfScreen(xscreen);

	_wncki_error_trap_push(xdisplay);

	int           format;
	unsigned long nitems, bytes_after, *num;

	Atom type =  None;
	int  res  =  XGetWindowProperty(xdisplay, xwindow, xatom, 0, LONG_MAX, False,
						XA_CARDINAL, &type, &format, &nitems, &bytes_after, (unsigned char**)&num);
	bool okay = _wncki_error_trap_pop(xdisplay) == Success && res == Success;

	if( okay )
	{
		if( okay = type == XA_CARDINAL )
			*val = *num;
		XFree(num);
	}

	return okay;
}

// -----------------------------------------------------------------------------
int _wncki_screen_get_workspace_count(Screen* xscreen)
{
	int    ns    = 0;
	Window xroot = RootWindowOfScreen(xscreen);

	if( !_wncki_get_cardinal(xscreen, xroot, _wncki_atom_get("_NET_NUMBER_OF_DESKTOPS"), &ns) )
		ns  = 1;

	if( ns <= 0 )
		ns  = 1;

	return ns;
}

#endif // !_USEWKSPACE
#endif // _USEGTK

