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

#include <glib.h>
#include <gtk/gtk.h>
#include "player.h"
#include "gui-callbacks.h"

GtkMenu* traymenu_new(void)
{
	return GTK_MENU(gtk_menu_new ());
}

/* append an item to the menu, and connect its callback on "activate" event */
GtkMenuItem* traymenu_append_item(GtkMenu* menu, gchar* label, GCallback callback, gpointer cb_data)
{
	GtkWidget* item;

	item = gtk_menu_item_new_with_label (label);
	gtk_menu_shell_append ((GtkMenuShell*) (menu), item);
	if (callback)
		g_signal_connect (G_OBJECT(item), "activate", G_CALLBACK(callback), cb_data);
	gtk_widget_show (item);

	return GTK_MENU_ITEM(item);
}

/* show the menu */
void traymenu_show(GtkMenu* menu, guint button, guint activate_time)
{
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, button, activate_time);
	return;
}

/* hide the menu */
void tarymenu_hide(GtkMenu* menu)
{
	gtk_menu_popdown(GTK_MENU(menu));
	return;
}

/* append a stock imageitem to the menu, and connect its callback on "activate" event */
GtkMenuItem* traymenu_append_image_item(GtkMenu* menu, const gchar* stock_id, GCallback callback, gpointer cb_data)
{
	GtkWidget* item;

	item = gtk_image_menu_item_new_from_stock(stock_id, NULL);
	gtk_menu_shell_append ((GtkMenuShell*) (menu), item);
	if (callback)
		g_signal_connect (G_OBJECT(item), "activate", G_CALLBACK(callback), cb_data);
	gtk_widget_show (item);

	return GTK_MENU_ITEM(item);
}

gboolean traymenu_on_play(GtkWidget *widget, gpointer user_data) {
  player_play_song(-1);

  return FALSE;
}

gboolean traymenu_on_pause(GtkWidget *widget, gpointer user_data) {
  player_pause();

  return FALSE;
}

gboolean traymenu_on_stop(GtkWidget *widget, gpointer user_data) {
  player_stop();

  return FALSE;
}

gboolean traymenu_on_next(GtkWidget *widget, gpointer user_data) {
  player_next();

  return FALSE;
}

gboolean traymenu_on_prev(GtkWidget *widget, gpointer user_data) {
  player_prev();

  return FALSE;
}

void traymenu_on_quit(GtkWidget *widget, gpointer user_data) {
  on_ui_quit(NULL, NULL);
}
// vim: et sw=2 smarttab
