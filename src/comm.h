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

#ifndef __COMM_H
#define __COMM_H

gint glurp_connect();
void glurp_disconnect();
void update_playlist(long long oldid);
void clear_playlist();
void get_playlist_list();
void clear_playlist_list();
void load_playlist(gchar *name);
mpd_Song *glurp_get_nth_song(gint n);
gboolean check_mpd_error();
void glurp_add_add_dir(gchar *path, GtkTreePath *gpath);
void glurp_add_search_result_dir( const gchar *what, gint type, GtkTreePath *gpath);
gboolean glurp_process_plchanges(mpd_Status *status);
void glurp_update_song(mpd_Song *song);
void fix_playlist_length();

#endif /* __COMM_H */
// vim: et sw=2 smarttab
