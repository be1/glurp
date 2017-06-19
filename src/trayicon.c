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
#include "structs.h"
#include "support.h"
#include "traymenu.h"

void trayicon_on_left_click(GtkStatusIcon* instance, gpointer app_data) {
  static gint x = 0;
  static gint y = 0;
  static gboolean hidden = FALSE;
  GlurpState* glurp = (GlurpState*) app_data;

  if (hidden) {
    gtk_window_move(GTK_WINDOW(glurp->window), x, y);
    gtk_widget_show(glurp->window);
    hidden = FALSE;
  } else {
    gtk_window_get_position(GTK_WINDOW(glurp->window), &x, &y);
    gtk_widget_hide(glurp->window);
    hidden = TRUE;
  }
}

void trayicon_on_right_click(GtkStatusIcon* instance, guint button, guint activate_time, gpointer app_data) {
    GlurpState* glurp = (GlurpState*) app_data;
    traymenu_show (GTK_MENU(glurp->traymenu), button, activate_time);
}

GtkStatusIcon* trayicon_new(GlurpState* glurp) {
  GtkStatusIcon* trayicon = NULL;

  trayicon = gtk_status_icon_new();
  gtk_status_icon_set_tooltip(trayicon, "Glurp");
  gtk_status_icon_set_from_pixbuf(GTK_STATUS_ICON(trayicon),
                                  get_pixbuf_from_name("media-audiofile.png"));

  g_signal_connect(G_OBJECT(trayicon), "activate",
                            G_CALLBACK(trayicon_on_left_click), glurp);
  g_signal_connect(G_OBJECT(trayicon), "popup-menu",
                            G_CALLBACK(trayicon_on_right_click), glurp);
  return trayicon;
}

void trayicon_show(GtkStatusIcon* trayicon) {
  gtk_status_icon_set_visible(trayicon, TRUE);
}

void trayicon_hide(GtkStatusIcon* trayicon) {
  gtk_status_icon_set_visible(trayicon, FALSE);
}

void trayicon_set_text(GtkStatusIcon* trayicon, const gchar* text) {
  gtk_status_icon_set_tooltip(trayicon, text);
}

void trayicon_set_markup(GtkStatusIcon* trayicon, const gchar* text) {
  gtk_status_icon_set_tooltip_markup(trayicon, text);
}

gchar* trayicon_get_text(GtkStatusIcon* trayicon) {
  return gtk_status_icon_get_tooltip_text(trayicon);
}

gchar* trayicon_get_markup(GtkStatusIcon* trayicon) {
  return gtk_status_icon_get_tooltip_markup(trayicon);
}

// vim: et sw=2 smarttab
