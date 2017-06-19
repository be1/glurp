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
#include "support.h"
#include "gui.h"
#include "comm.h"
#include "player.h"
#include "mpd-callbacks.h"

extern GtkBuilder *builder;

void mpd_status_changed_callback (MpdObj *mi, ChangedStatusType what, void *userdata) {
  struct glurp_t* glurp = userdata;
  gchar* temp;
  int tel;

  if (what != MPD_CST_ELAPSED_TIME) /* we don't want to have a line each second... */
    debug ("Status change callback... what: %x", what);
  if (what & MPD_CST_PLAYLIST) {
    update_playlist(mpd_playlist_get_old_playlist_id(glurp->mpd));
    /* update song name display for streams that change it on the fly */
    glurp->current_song = mpd_playlist_get_current_song(glurp->mpd);
    gui_show_song_name();
  }

  if (what & MPD_CST_VOLUME ) {
    /* update volumebar */
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(gtk_builder_get_object(builder, "volumebar")), (gdouble)(mpd_status_get_volume(glurp->mpd)/100.0));
  }
  if (what & MPD_CST_SONGID ) {
    /* save current song */
    glurp->current_song = mpd_playlist_get_current_song(glurp->mpd);
    gui_show_song_name();
  }
   
  if (what & MPD_CST_ELAPSED_TIME ) {
    if(!glurp->progress_dragging) {
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "progressbar")), TRUE);
    
      /* +0.1 to make sure min<max is true... */
      gtk_range_set_range(GTK_RANGE(gtk_builder_get_object(builder, "progressbar")), 0, (gdouble)mpd_status_get_total_song_time(glurp->mpd) + 0.1); 
    }
  }
  
  if ( what & MPD_CST_ELAPSED_TIME ) {
    /* update progressbar unless the user is dragging it...*/
    if(!glurp->progress_dragging) {
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "progressbar")), TRUE);
      
      gtk_range_set_value(GTK_RANGE(gtk_builder_get_object(builder, "progressbar")), 
          mpd_status_get_elapsed_song_time(glurp->mpd));
    }
    
    if(glurp->config->time_display_left) 
      tel = mpd_status_get_total_song_time(glurp->mpd) - mpd_status_get_elapsed_song_time(glurp->mpd);
    else 
      tel = mpd_status_get_elapsed_song_time(glurp->mpd);
    
   temp = g_strdup_printf("%s%02d:%02d", (glurp->config->time_display_left ? "-" : ""), tel/60, tel%60); 
#if 0 /* strange: this makes tooltips disappear! */
    gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(builder, "button_time")), temp);
/*    gtk_tooltip_trigger_tooltip_query (gdk_display_get_default()); */
#else
    gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "label_time")), temp);
#endif
    g_free(temp);
  }

  if ( what & MPD_CST_STATE ) {
    switch (mpd_player_get_state(glurp->mpd)) {
      case MPD_PLAYER_STOP:
        gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(builder, "entry_trackname")), "Stopped...");
    
        /* change appropriate gui parts */
        gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "label_bitrate")), g_strdup("---kbps"));
        gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "label_mode")), g_strdup("---"));
        break;
      case MPD_PLAYER_PLAY:
        glurp->play_state = MPD_PLAYER_PLAY;
        gui_show_song_name();
        break;
      case MPD_PLAYER_PAUSE:
        glurp->play_state = MPD_PLAYER_PAUSE;
        break;
      case MPD_PLAYER_UNKNOWN:
        /* wazige sh*t */
        break;
    }
  }
  /* PERIODIC: update bitrate and mode labels */
  if ( what & MPD_CST_AUDIOFORMAT ) {
    char *text;
    switch (mpd_status_get_channels(glurp->mpd)) {
      case 0: 
        text = g_strdup("---");
        break;
      case 1:
        text = g_strdup("Mono");
        break;
      case 2:
        text = g_strdup("Stereo");
        break;
      default:
        text = g_strdup("More");
        break;
    }
    gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "label_mode")), text); 
  }
  if ( what & MPD_CST_BITRATE ) {
    char *text;
    int bitrate = mpd_status_get_bitrate(glurp->mpd);
    if (bitrate == 0) 
      text = g_strdup("---kbps");
    else 
      text = g_strdup_printf("%dkbps", bitrate);
    gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "label_bitrate")), text);
  }
  
  if ( what & MPD_CST_REPEAT ) {
    /* PERIODIC: update repeat button */
    if( mpd_player_get_repeat(glurp->mpd) ) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "togglebutton_repeat")), TRUE);
    } else {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "togglebutton_repeat")), FALSE);
    }
  }

  if ( what & MPD_CST_RANDOM ) {
    /* PERIODIC: update random button */
    if( mpd_player_get_random(glurp->mpd) ) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "togglebutton_random")), TRUE);
    } else {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "togglebutton_random")), FALSE);
    }
  }

  if ( what & MPD_CST_DATABASE ) {
  /* PERIODIC: reenable Add window controls if we just finished updating db (>=0.11.0) */
    statusbar_print("Database updated");
    gui_updating_enable_add_controls();
    glurp->updating_db = FALSE;
    gtk_tree_view_set_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add")), GTK_TREE_MODEL(glurp->gui_addtree));
    populate_gui_add_tree();
  }

  // TODO: hmm misunderstood something; this should be done anywhere an error could occur (as it was... D'oh)
  mpd_check_error(glurp->mpd);
}
  
int mpd_error_callback (MpdObj *mi, int id, char *msg, void *userdata) {
  debug("Damn, mpd returned an error, %s", msg);
  return 0;
}

void mpd_connection_changed_callback(MpdObj *mi, int connect, void *userdata) {
  struct glurp_t* glurp = userdata;

  if (connect) {
    debug("We're connected!");
    /* we're just connected */
    debug("Connected to %s:%d", glurp->config->server_host, glurp->config->server_port);
    statusbar_print("Connected to server %s:%d", glurp->config->server_host, glurp->config->server_port);

    title_print(GTK_WIDGET(gtk_builder_get_object(builder, "glurp_window_main")), "%s:%d", glurp->config->server_host, glurp->config->server_port);

    if( !STREAM_CAPABLE_MPD(glurp->mpd) ) {
      debug("MPD VERSION TOO OLD, DISCONNECTING");
      glurp_disconnect();
      statusbar_print("MPD version too old, disconnecting");
    } else {

      gui_set_connected();

      debug("Adding a timeout call to gui_update().");
      /* force immediate schedule beacuse of time dependency */
      gui_update_cb(GINT_TO_POINTER((guint)glurp->refresh_rate_status));
      g_timeout_add((guint)glurp->config->refresh_rate, gui_update_cb, NULL);
    }
  } else {
    debug("We're disconnected!");
    gui_set_disconnected();
    /* we're just disconnected */
  }
}

// vim: et sw=2 smarttab
