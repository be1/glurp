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

#ifndef __STRUCTS_H
#define __STRUCTS_H

#include <gtk/gtk.h>
#include <libmpd/libmpd.h>

enum {
  PL_POS,
  PL_TITLE,
  PL_TIME,
  PL_ID,

  PL_BOLD,
  PL_BOLD_SET,

  NUM_PL
};

struct conf_t {
    gchar *server_host;
    gint  server_port;
    gchar *server_pass;
    gboolean autoconnect;
    gboolean playlist_vis_on_start;
    gboolean time_display_left;
    gboolean save_size;
    gdouble refresh_rate;
    gint pos_x;
    gint pos_y;
    gint width;
    gint height;
    gboolean trayicon;

    gboolean playlist_columns[PL_BOLD];
};

typedef struct conf_t GlurpConfig;

struct pl_t {
    gchar *name;
    struct pl_t *next;
};

typedef struct pl_t GlurpPl;

struct stream_t {
    gchar *url;
    struct stream_t *next;
};

typedef struct stream_t GlurpStream;

struct glurp_t {
    GlurpConfig *config;
    char* alternate_config_file;
    MpdObj* mpd;
    mpd_Song *current_song;
    GtkListStore *gui_playlist;
    GtkListStore *gui_playlist_list;
    GtkListStore *gui_stream_list;
    GtkTreeStore *gui_addtree;
    gboolean progress_dragging;
    guint refresh_rate_status;
    long long playlist_version;
    GlurpPl *playlists;
    gint statusbar_status;
    gint play_state;
    gint prev_song_num;
    gint scroll;
    gint num_add_dirs;
    gboolean updating_db;
    GlurpStream *stream_history;
    
    GtkIconFactory *icon_factory;
    /* below is used for parking those frames in condensed mode */
    GtkWidget* frame_time;
    GtkWidget* frame_trackname;
    /* current pixel width of the trackname entry */
    gint trackname_width;
    /* the main window */
    GtkWidget* window;
    /* the sytem-tray icon */
    GtkStatusIcon* trayicon;
    /* the sytem-tray menu */
    GtkMenu* traymenu;
};

typedef struct glurp_t GlurpState;

#endif /* __STRUCTS_H */
// vim: et sw=2 smarttab
