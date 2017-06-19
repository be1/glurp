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

#ifndef _MENU_H
#define _MENU_H
#include <glib.h>
#include <gtk/gtk.h>

/* create a new (empty) menu */
GtkMenu* traymenu_new(void);

/* append an item to the menu, and connect its callback on "activate" event */
GtkMenuItem* traymenu_append_item(GtkMenu* menu, gchar* label, GCallback callback, gpointer cb_data);

/* append a stock image item to the menu, and connect its callback on "activate" event */
GtkMenuItem* traymenu_append_image_item(GtkMenu* menu, const gchar* stock_id, GCallback callback, gpointer cb_data);

/* show the menu */
void traymenu_show(GtkMenu* menu, guint button, guint activate_time);

/* hide the menu */
void traymenu_hide(GtkMenu* menu);

gboolean traymenu_on_play(GtkWidget *widget, gpointer user_data);

gboolean traymenu_on_pause(GtkWidget *widget, gpointer user_data);

gboolean traymenu_on_stop(GtkWidget *widget, gpointer user_data);

gboolean traymenu_on_next(GtkWidget *widget, gpointer user_data);

gboolean traymenu_on_prev(GtkWidget *widget, gpointer user_data);

void traymenu_on_quit(GtkWidget *widget, gpointer user_data);

#endif /* _MENU_H */
// vim: et sw=2 smarttab
