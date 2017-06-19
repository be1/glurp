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
#include <gtk/gtktreemodel.h>
#include <string.h>

#include "structs.h"
#include "support.h"
#include "comm.h"
#include "gui.h"

extern GlurpState *glurp;
extern GtkBuilder *builder;

gint glurp_connect() {
  if(mpd_check_connected(glurp->mpd)) {
    debug("Already connected");
    statusbar_print("Already connected to server");
    return 0;
  }

  if(!glurp->config->server_host || !strlen(glurp->config->server_host)) {
    debug("Can not connect, no server set in config");
    statusbar_print("Can not connect, no server set in config");
    return 0;
  }

  if(!glurp->config->server_port || 
      glurp->config->server_port > 65535 || glurp->config->server_port < 1 ) {
    debug("Invalid port specified in config");
    return 0;
  }

  gui_set_connecting();

  mpd_set_hostname(glurp->mpd, glurp->config->server_host);
  mpd_set_password(glurp->mpd, glurp->config->server_pass);
  debug("Using password: %s",glurp->config->server_pass);
  mpd_set_port(glurp->mpd, glurp->config->server_port);
  if (mpd_connect(glurp->mpd)!=MPD_OK) {
    gui_set_disconnected();
    debug("Unable to connect");
    statusbar_print("Unable to connect to server: %s:%d", glurp->config->server_host, glurp->config->server_port);
  }
  if (mpd_send_password(glurp->mpd)!=MPD_OK) {
    debug("Invalid password");
    statusbar_print("Invalid password for %s:%d", glurp->config->server_host, glurp->config->server_port);
  }
  return 1;
}

void glurp_disconnect() {
  if (mpd_check_connected(glurp->mpd)) {
    mpd_disconnect(glurp->mpd);   
    gui_set_disconnected();
    return;
  }

  glurp->prev_song_num = -1;
  glurp->playlist_version = 0;

  glurp->play_state = MPD_STATUS_STATE_STOP;
  debug("Disconnected");

  gui_set_disconnected();
}


void update_playlist(long long oldid) {
  MpdData *entity;

  if (!mpd_check_connected(glurp->mpd)) {
    debug("Not connected to server, returning");
    return;
  }

  statusbar_print("Updating playlist...");
  debug("Updating playlist...");

  for (entity = mpd_playlist_get_changes(glurp->mpd, oldid);entity;entity = mpd_data_get_next(entity)) {
    if( entity->type == MPD_DATA_TYPE_SONG ) {
      add_song(entity->song);
    }
  }
  debug("playlist succesfully updated");
  
  fix_playlist_length();
}

void fix_playlist_length() {
  gint real_length = mpd_playlist_get_playlist_length(glurp->mpd);
  gint our_length = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(glurp->gui_playlist), NULL);
  GtkTreeIter iter;
  while (real_length < our_length ) {
    our_length--;
    gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(glurp->gui_playlist), &iter, NULL, our_length);
    gtk_list_store_remove(glurp->gui_playlist, &iter);
  }
}
    
void get_playlist_list() {
  MpdData *entity;
  GlurpPl *npl, *pl=NULL;
  

  if(! mpd_check_connected(glurp->mpd)) {
    debug("Not connected to server, returning");
    return;
  }

  

  for (entity = mpd_database_get_directory(glurp->mpd, "");entity;entity = mpd_data_get_next(entity)) {
    if ( entity->type == MPD_DATA_TYPE_PLAYLIST ) {
      npl = g_new(GlurpPl,1);
      npl->name = g_strdup(entity->playlist->path);
      npl->next = NULL;

      debug("Received playlist '%s'", npl->name);

      if(pl) {
        pl->next = npl;
      } else {
        glurp->playlists = npl;
      }
      
      pl = npl;
      
    }
  }

}

void load_playlist(gchar *name) {
  if( !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "checkbutton_append_playlist"))) ) {
    debug("clearing mpd playlist");
    mpd_playlist_clear(glurp->mpd);
  }

  debug("loading playlist '%s'", name);
  mpd_playlist_queue_load(glurp->mpd, name);
  mpd_playlist_queue_commit(glurp->mpd);
}



void glurp_add_add_dir(gchar *path, GtkTreePath *gpath) {
  MpdData *entity;

  if( !gpath && ( gtk_tree_model_iter_n_children(GTK_TREE_MODEL(glurp->gui_addtree), NULL) ) ) return;

  debug("Starting to list '%s'", path);

  if (mpd_check_connected(glurp->mpd)) {
    
    for (entity = mpd_database_get_directory(glurp->mpd, path);entity;entity = mpd_data_get_next(entity)) {
      switch(entity->type) {
      case MPD_DATA_TYPE_DIRECTORY:
        gui_add_append(g_strdup(entity->directory), gpath, FALSE);
        break;
      case MPD_DATA_TYPE_SONG:
        gui_add_append(g_strdup(entity->song->file), gpath, TRUE);
        break;
      default:
        break;
      }
    } 
  }
}

void glurp_add_search_result_dir(const gchar *what, gint type, GtkTreePath *gpath) {
  MpdData *entity;

  debug("Starting to search for '%s' with type %d", what, type);
  if( what == NULL || !strlen(what) ) return;

  
  for (entity = mpd_database_find(glurp->mpd, type, (gchar*)what, FALSE);entity;entity = mpd_data_get_next(entity)) {
    switch(entity->type) {
      case MPD_DATA_TYPE_DIRECTORY:
        gui_add_append(g_strdup(entity->directory), gpath, FALSE);
        break;
      case MPD_DATA_TYPE_SONG:
        gui_add_append(g_strdup(entity->song->file), gpath, TRUE);
        break;
//      case MPD_DATA_TYPE_TAG:
//        gui_add_Append(g_strdup(entity->tag), gpath, TRUE);
      default:
        break;
    }
  }
}

// vim: et sw=2 smarttab
