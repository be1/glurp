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

#ifndef __SUPPORT_H
#define __SUPPORT_H

#include "structs.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef GLURP_DEBUG
#define debug(str, ARGS...)  debug_real(__FILE__, __LINE__, __FUNCTION__, str, ##ARGS)
#else
#define debug(str, ARGS...)
#endif

void debug_real(const char *file, const int line, const char *function, const char *msg, ...);

void statusbar_print(const char *string, ...);
gboolean statusbar_reset();
gchar *status_string();

void title_print(GtkWidget *window, const char *string, ...);

mpd_Song *get_nth_song_from_playlist(gint pos);
gchar *strip_dirs(gchar *path);
gchar *builder_path();
GtkWidget* get_image_from_name(gchar* name);
GdkPixbuf* get_pixbuf_from_name(gchar* name);

gchar *glurp_filename(gchar *path);

mpd_Song *get_song_by_pos(gint pos);
mpd_Song *get_song_by_id(gint id);
gint get_song_pos(mpd_Song *song);
gint get_num_songs();

void update_song(mpd_Song *song);
void add_song(mpd_Song *song);

void debug_print_playlist();

GlurpStream *get_stream_history(gchar *urls);
void print_stream_history();
gchar *dump_stream_history();
void push_stream(gchar *url);
void pull_stream(gchar *url);

void sigint_handler(int s);

char *remove_extension_and_basepath(const char *filename);

/* convenience macros */
#define NONBLOCKING_UPDATE_CAPABLE_MPD(mi)  (mpd_server_check_version(mi, 0, 11, 0))

#define STREAM_CAPABLE_MPD(mi)  (mpd_server_check_version(mi, 0, 11, 0))

#define OUTPUTS_CAPABLE_MPD(mi)  (mpd_server_check_version(mi, 0, 12, 0))

#define GLURP_TITLE_PREFIX  "Glurp v"GLURP_VERSION

#define MAX_STREAM_HISTORY  30

#endif /* __SUPPORT_H */
// vim: et sw=2 smarttab
