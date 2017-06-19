/*
    Glurp - A GTK+ client for Music Player Daemon
    Copyright (C) 2004, 2005 Andrej Kacian

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    http://musicpd.org/glurp.shtml

*/

#ifndef TRAYICON_H
#define TRAY_ICON_H

#include "structs.h"

GtkStatusIcon* trayicon_new(GlurpState* glurp);

void trayicon_on_left_click(GtkStatusIcon* instance, gpointer app_data);
void trayicon_on_right_click(GtkStatusIcon* instance, guint button, guint activate_time, gpointer app_data);

void trayicon_show(GtkStatusIcon* trayicon);

void trayicon_hide(GtkStatusIcon* trayicon);

void trayicon_set_text(GtkStatusIcon* trayicon, const gchar* text);
#if GTK_CHECK_VERSION(2,16,0)
void trayicon_set_markup(GtkStatusIcon* trayicon, const gchar* text);
#endif
gchar* trayicon_get_text(GtkStatusIcon* trayicon);
#if GTK_CHECK_VERSION(2,16,0)
gchar* trayicon_get_markup(GtkStatusIcon* trayicon);
#endif

#endif
// vim: et sw=2 smarttab
