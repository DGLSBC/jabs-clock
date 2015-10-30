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
/*
 * Taken from libwnck code and ever so slightly modified for this app's use.
 */

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#undef    DEBUGLOG
#define   DEBUGNSP   "x"

#include "x.h"

#include <glib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#define  WNCK_CLIENT_TYPE_APPLICATION 1

#define _wnck_atom_get(atom_name) gdk_x11_get_xatom_by_name(atom_name)
#define _wnck_get_client_type()   WNCK_CLIENT_TYPE_APPLICATION

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static void _wnck_change_workspace(Screen*  screen, Window xwindow, int new_space);
static void _wnck_error_trap_push (Display* display);
static int  _wnck_error_trap_pop  (Display* display);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void g_init_threads()
{
//	printf("%s: entry\n", __func__);
	XInitThreads();
//	printf("%s: exit\n", __func__);
}

// -----------------------------------------------------------------------------
void g_window_space(GtkWidget* pWidget, int space)
{
	Screen* screen  = GDK_SCREEN_XSCREEN(gdk_screen_get_default());
	Window  xwindow = GDK_WINDOW_XWINDOW(pWidget->window);

	_wnck_change_workspace(screen, xwindow, space);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void _wnck_change_workspace(Screen* screen, Window xwindow, int new_space)
{
	XEvent   xev;
	Display* display = DisplayOfScreen   (screen);
	Window   root    = RootWindowOfScreen(screen);

	xev.xclient.type         =  ClientMessage;
	xev.xclient.serial       =  0;
	xev.xclient.send_event   =  True;
	xev.xclient.display      =  display;
	xev.xclient.window       =  xwindow;
	xev.xclient.message_type = _wnck_atom_get("_NET_WM_DESKTOP");
	xev.xclient.format       =  32;
	xev.xclient.data.l[0]    =  new_space;
	xev.xclient.data.l[1]    = _wnck_get_client_type();
	xev.xclient.data.l[2]    =  0;
	xev.xclient.data.l[3]    =  0;
	xev.xclient.data.l[4]    =  0;

	_wnck_error_trap_push(display);
	XSendEvent(display, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
	_wnck_error_trap_pop (display);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void _wnck_error_trap_push(Display* display)
{
	gdk_error_trap_push();
}

// -----------------------------------------------------------------------------
int _wnck_error_trap_pop(Display* display)
{
	XSync(display, False);
	return gdk_error_trap_pop();
}

