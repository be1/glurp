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

#ifndef __GUI_H
#define __GUI_H

gboolean glurp_init_gui();
void populate_config();
void store_config();

void gui_show_song_name();
void gui_check_scroll();

void create_playlist_liststore();
void attach_gui_playlist_liststore();
void detach_gui_playlist_liststore();
void add_song_to_gui_playlist(mpd_Song *s, gint pos);
void populate_gui_playlist();

void hide_gui_playlist();
void show_gui_playlist();

void gui_set_connecting();
void gui_set_connected();
void gui_set_disconnected();

gboolean gui_update(gint rrs);
gboolean gui_update_cb(gpointer val);

void create_playlist_list_liststore();
void add_playlist_to_gui_playlist(GlurpPl *pl);
void populate_gui_playlist_list();
void playlists_window_destroyed();

void gui_playlist_set_cursor(gint pos);

gboolean gui_playlist_scroll(gint pos);
gboolean gui_playlist_scroll_cb(gpointer pos);
gboolean gui_playlist_hilight(gint pos);
gboolean gui_playlist_hilight_cb(gpointer pos);

gboolean gui_trackname_scroll();
void gui_trackname_scroll_start();

void gui_set_search_model();

void add_window_destroyed();
void create_gui_add_tree();
void populate_gui_add_tree();
void populate_gui_add_search_tree();

void gui_load_selected();
void gui_load_song(GtkTreeModel *tm, GtkTreeIter iter);

void gui_updating_disable_add_controls();
void gui_updating_enable_add_controls();

void gui_add_fill_dir(GtkTreeIter *iter, gboolean silent);

gboolean gui_get_iter_by_id(GtkTreeIter *iter, gint id);
gboolean gui_get_iter_by_pos(GtkTreeIter *iter, gint pos);

void create_stream_liststore();
void populate_stream_liststore();
void stream_window_destroyed();

gchar *get_selected_stream();

GtkMenu *populate_outputs_menu();

gboolean on_menu_outputs_deactivate(GtkWidget *widget, gpointer data);
gboolean on_menu_output_activate(GtkWidget *widget, gpointer data);

void functional_notebook_select_playlist();
void functional_notebook_select_database();
void functional_notebook_select_config();
void functional_notebook_select_playlists();

gboolean functional_notebook_playlist_selected();
gboolean functional_notebook_database_selected();
gboolean functional_notebook_config_selected();
gboolean functional_notebook_playlists_selected();

/* this one is in tooltips.c */
void gui_create_tooltips();

void gui_add_append(gchar *path, GtkTreePath *iter, gboolean song);

enum {
    ADD_NAME,
    ADD_FILENAME,
    ADD_NUM_DIRS,
    ADD_ICON,

    NUM_ADD
};

enum {
  GLURP_QSEARCH_ALL,
  GLURP_QSEARCH_TITLE,
  GLURP_QSEARCH_ARTIST,
  GLURP_QSEARCH_ALBUM,
  GLURP_QSEARCH_FILENAME,

  NUM_QSEARCH
};

void glurp_switch_functional_page(GtkWidget* focus, gboolean next);
#define TRACKNAME_SCROLL_START    50

#endif /* __GUI_H */
// vim: et sw=2 smarttab
