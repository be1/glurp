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
#include <glib-object.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <libmpd/libmpd-strfsong.h>

#include "structs.h"
#include "support.h"
#include "comm.h"
#include "conf.h"
#include "player.h"
#include "gui.h"
#include "trayicon.h"
#include "traymenu.h"

#define SONG_SCROLL 500

extern GtkBuilder* builder;
extern GlurpState *glurp;

void gui_build_icon_factory(){
  int i;
  static char* image_files[] =
  { "player-prev.png",
    "player-play.png",
    "player-pause.png",
    "player-stop.png",
    "player-next.png",
    "player-repeat.png",
    "player-random.png",
    "player-tabswitch.png",
    "playlist.png",
    "online.png",
    "offline.png",
    "remove.png",
    "outputs.png",
    "condense.png",
    "media-audiofile.png",
    NULL
  };
  static char* icon_names[] = 
  { "glurp-prev",
    "glurp-play",
    "glurp-pause",
    "glurp-stop",
    "glurp-next",
    "glurp-repeat",
    "glurp-random",
    "glurp-tabswitch",
    "glurp-playlist",
    "glurp-online",
    "glurp-offline",
    "glurp-remove",
    "glurp-outputs",
    "glurp-condense",
    "glurp-media-audiofile",
    NULL
  };
  
  GtkIconSet* newSet;
  
  gtk_icon_size_register("glurp_condensed", 10, 10); 
  glurp->icon_factory = gtk_icon_factory_new();

  for (i=0;image_files[i]!=NULL; i++) {
    newSet = gtk_icon_set_new_from_pixbuf(get_pixbuf_from_name(image_files[i]));
    gtk_icon_factory_add(glurp->icon_factory, icon_names[i], newSet);
  }
  gtk_icon_factory_add_default(glurp->icon_factory);
}

void gui_show_song_name() {
  gchar title[1024];
  
  /* check if there is a current song */
  if (!glurp->current_song) return;

  mpd_song_markup(title, 1023, "[%artist% - %title%]|%name%|%file%", glurp->current_song);
  gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(builder, "entry_trackname")), title);
  trayicon_set_text(glurp->trayicon, title);
  
  debug("Moving playlist focus to #%d", glurp->current_song->pos);
  g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc)gui_playlist_scroll_cb, GINT_TO_POINTER(glurp->current_song->pos), NULL);
  g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc)gui_playlist_hilight_cb, GINT_TO_POINTER(glurp->current_song->pos), NULL);
  //g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc)gui_playlist_hilight, GINT_TO_POINTER(status->song), NULL);
  gui_check_scroll();
}

void gui_check_scroll() {
  GtkWidget *wi = GTK_WIDGET(gtk_builder_get_object(builder, "entry_trackname"));
  int width;
  pango_layout_get_pixel_size(gtk_entry_get_layout(GTK_ENTRY(wi)), &width, NULL);
  if( width > glurp->trackname_width ) {
    debug("Starting song scroll");
    if (glurp->scroll == 0) g_timeout_add(SONG_SCROLL, gui_trackname_scroll, NULL);
    glurp->scroll=1;
  } else {
    debug("Song changed, stopping titlebar scrolling");
    glurp->scroll = 0;
  }
}

void gui_init_notebooks() {
  static gchar* tabs[][2] = {
    {"move_playlist_tab","glurp_playlist_label"},
    {"move_database_tab","glurp_database_label"},
    {"move_streams_tab","glurp_streams_label"},
    {"move_playlists_tab","glurp_playlists_label"},
    {"move_config_tab","glurp_config_label"}};
  GtkNotebook *notebook = GTK_NOTEBOOK(gtk_builder_get_object(builder, "glurp_functional_notebook1"));
  GtkWidget* wi;
  GtkRcStyle* style;
  gint tab;

  for (tab=gtk_notebook_get_n_pages(notebook)-1; tab>=0; tab--) {
    wi = gtk_notebook_get_nth_page(notebook, tab);
    g_object_set(G_OBJECT(wi), "user_data", GINT_TO_POINTER(tab), NULL);
    wi = GTK_WIDGET(gtk_builder_get_object(builder, tabs[tab][0]));
    gtk_label_set_mnemonic_widget(GTK_LABEL(gtk_builder_get_object(builder,tabs[tab][1])), gtk_notebook_get_tab_label(notebook, wi));
    gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-tabswitch", gtk_icon_size_from_name ("glurp_condensed") ) );
    style = gtk_widget_get_modifier_style(wi);
    style->xthickness = 0;
    style->ythickness = 0;
    gtk_widget_modify_style(wi, style);
    gtk_widget_show_all(wi);
  }
}

/* this should only be called at startup */
gboolean glurp_init_gui() {
  gchar *builder_file;
  GtkWidget *window_main = NULL, *wi = NULL;
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;
  GtkTreeView *tv;
  /*  PangoFontDescription *time_font;

  time_font = pango_font_description_from_string("Sans 14");*/

  glurp->progress_dragging = FALSE;
  glurp->gui_playlist = NULL;

  debug("Trying to load glurp.glade from installed directory first");
  
  builder_file = builder_path();
  debug("Using %s.", builder_file);
  builder = gtk_builder_new ();
  if(!gtk_builder_add_from_file(builder, builder_file, NULL)) {
    debug("Could not load builder file, giving up. Goodnight.");
  }
  g_free(builder_file);
  
	gtk_builder_connect_signals (builder, NULL);

  create_playlist_liststore();

  gtk_tree_view_set_reorderable(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist")), TRUE);

  gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "combobox_qsearch_type")), GLURP_QSEARCH_ALL);
  
  gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(builder, "glurp_functional_notebook1")), 0);

  /* put correct icons to player control buttons */
  gui_build_icon_factory();
  /* prev */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "button_prev"));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-prev", GTK_ICON_SIZE_BUTTON) );
  gtk_widget_show_all(wi);
  /* play */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "button_play"));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-play", GTK_ICON_SIZE_BUTTON) );
  gtk_widget_show_all(wi);
  /* pause */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "button_pause"));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-pause", GTK_ICON_SIZE_BUTTON) );
  gtk_widget_show_all(wi);
  /* stop */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "button_stop"));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-stop", GTK_ICON_SIZE_BUTTON) );
  gtk_widget_show_all(wi);
  /* next */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "button_next"));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-next", GTK_ICON_SIZE_BUTTON) );
  gtk_widget_show_all(wi);

  /* connect/disconnect */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "button_server_disconnect"));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-offline", GTK_ICON_SIZE_BUTTON) );
  gtk_widget_show_all(wi);
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "button_server_connect"));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-online", GTK_ICON_SIZE_BUTTON) );
  gtk_widget_show_all(wi);

  /* playlist show/hide */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_playlist"));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-playlist", GTK_ICON_SIZE_BUTTON) );
  gtk_widget_show_all(wi);

  /* repeat */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_repeat"));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-repeat", GTK_ICON_SIZE_BUTTON) );
  gtk_widget_show_all(wi);

  /* random */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_random"));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-random", GTK_ICON_SIZE_BUTTON) );
  gtk_widget_show_all(wi);

  /* remove */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_pl_remove"));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-remove", GTK_ICON_SIZE_BUTTON) );
  gtk_widget_show_all(wi);

  /* outputs */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_outputs"));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-outputs", GTK_ICON_SIZE_BUTTON) );
  gtk_widget_show_all(wi);
  gtk_widget_set_sensitive(wi, FALSE);
  
  /* optionally show playlist */
  show_gui_playlist();
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_playlist"));
  if(glurp->config->playlist_vis_on_start) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wi), TRUE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wi), FALSE);
    hide_gui_playlist();
  }

  gui_init_notebooks();
  /* condense */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_condense"));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-condense", GTK_ICON_SIZE_BUTTON) );
  gtk_widget_show_all(wi);
 
  gui_set_disconnected();

  window_main = GTK_WIDGET(gtk_builder_get_object(builder, "glurp_window_main" ));
  glurp->window = window_main;

  title_print(window_main, NULL);

  /* restore window size if requested */
  if( glurp->config->width >= 0 && glurp->config->height >= 0 && glurp->config->save_size )
    gtk_window_resize(GTK_WINDOW(window_main), glurp->config->width, glurp->config->height);

  gtk_window_set_icon(GTK_WINDOW(window_main), get_pixbuf_from_name("media-audiofile.png"));

  /* optionally show trayicon */
  glurp->trayicon = trayicon_new(glurp);
  glurp->traymenu = traymenu_new();
  traymenu_append_image_item(glurp->traymenu,
                          GTK_STOCK_MEDIA_PLAY,
                          G_CALLBACK(traymenu_on_play),
                          NULL);
  traymenu_append_image_item(glurp->traymenu, 
                          GTK_STOCK_MEDIA_PAUSE,
                          G_CALLBACK(traymenu_on_pause),
                          NULL);
  traymenu_append_image_item(glurp->traymenu,
                          GTK_STOCK_MEDIA_NEXT,
                          G_CALLBACK(traymenu_on_next),
                          NULL);
  traymenu_append_image_item(glurp->traymenu,
                          GTK_STOCK_MEDIA_PREVIOUS,
                          G_CALLBACK(traymenu_on_prev),
                          NULL);
  traymenu_append_image_item(glurp->traymenu,
                          GTK_STOCK_MEDIA_STOP,
                          G_CALLBACK(traymenu_on_stop),
                          NULL);
  traymenu_append_image_item(glurp->traymenu,
                          GTK_STOCK_QUIT,
                          G_CALLBACK(traymenu_on_quit),
                          glurp);

  wi = GTK_WIDGET(gtk_builder_get_object(builder, "checkbutton_display_trayicon"));
  if(glurp->config->trayicon) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wi), TRUE);
    trayicon_show(glurp->trayicon);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wi), FALSE);
    trayicon_hide(glurp->trayicon);
  }

  statusbar_print("Initializing...");
  debug("Showing main window...");
  
  gtk_widget_show(window_main);
  
  /* now make sure the main window is actually shown before 
   * continuing to calculate stuff
   */ 
  while (gtk_events_pending ())
    gtk_main_iteration ();

  debug("stored window position: %dx%d", glurp->config->pos_x, glurp->config->pos_y);
  if( glurp->config->pos_x != -11000 && glurp->config->pos_y != -11000 )
    gtk_window_move(GTK_WINDOW(window_main), glurp->config->pos_x, glurp->config->pos_y);


  /* populate the config tab */
  populate_config();
  /* create the database tab */
  create_gui_add_tree();
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "combo_add_find_type"));
  gtk_combo_box_set_active(GTK_COMBO_BOX(wi), 0);

  create_stream_liststore();
  
  /* create playlists tab */
  create_playlist_list_liststore();
  
  tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist"));

  rend = gtk_cell_renderer_text_new();

  col = gtk_tree_view_column_new_with_attributes("#", rend,
                  "text", PL_POS,
                  "weight", PL_BOLD,
                  "weight-set", PL_BOLD_SET,
                  NULL);
  gtk_tree_view_column_set_resizable(col, TRUE);
  gtk_tree_view_append_column(tv, col);
    
  col = gtk_tree_view_column_new_with_attributes("Title", rend,
                  "text", PL_TITLE,
                  "weight", PL_BOLD,
                  "weight-set", PL_BOLD_SET,
                  NULL);
  gtk_tree_view_column_set_resizable(col, TRUE);
  gtk_tree_view_append_column(tv, col);


  /* workaround to stop a GTK_CRITICAL, 
   * TODO: need to properly debug that one 
   */
  gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(builder, "button_stop")));
  
  return TRUE;
}

void populate_config() {
  GtkWidget *w;

  w = GTK_WIDGET(gtk_builder_get_object(builder, "entry_hostname"));
  if(glurp->config->server_host) gtk_entry_set_text(GTK_ENTRY(w), glurp->config->server_host);

  w = GTK_WIDGET(gtk_builder_get_object(builder, "entry_port"));
  if(glurp->config->server_port) gtk_entry_set_text(GTK_ENTRY(w), g_strdup_printf("%d", glurp->config->server_port));

  w = GTK_WIDGET(gtk_builder_get_object(builder, "entry_password"));
  if(glurp->config->server_pass) gtk_entry_set_text(GTK_ENTRY(w), glurp->config->server_pass);

  w = GTK_WIDGET(gtk_builder_get_object(builder, "checkbutton_autoconnect"));
  if(glurp->config->autoconnect) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), 1);
  else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), 0);

  w = GTK_WIDGET(gtk_builder_get_object(builder, "checkbutton_save_size"));
  if(glurp->config->autoconnect) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), 1);
  else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), 0);

  w = GTK_WIDGET(gtk_builder_get_object(builder, "checkbutton_display_trayicon"));
  if(glurp->config->trayicon) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), 1);
  else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), 0);

  w = GTK_WIDGET(gtk_builder_get_object(builder, "spinbutton_refresh_rate"));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), glurp->config->refresh_rate);

} 

void store_config() {
  GtkWidget *w;
  gdouble nr;

  g_free(glurp->config->server_host);
  w = GTK_WIDGET(gtk_builder_get_object(builder, "entry_hostname"));
  glurp->config->server_host = g_strdup(gtk_entry_get_text(GTK_ENTRY(w)));

  w = GTK_WIDGET(gtk_builder_get_object(builder, "entry_port"));
  glurp->config->server_port = atoi(gtk_entry_get_text(GTK_ENTRY(w)));

  g_free(glurp->config->server_pass);
  w = GTK_WIDGET(gtk_builder_get_object(builder, "entry_password"));
  glurp->config->server_pass = g_strdup(gtk_entry_get_text(GTK_ENTRY(w)));

  w = GTK_WIDGET(gtk_builder_get_object(builder, "checkbutton_autoconnect"));
  glurp->config->autoconnect = yesno(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));

  w = GTK_WIDGET(gtk_builder_get_object(builder, "spinbutton_refresh_rate"));
  nr = gtk_spin_button_get_value(GTK_SPIN_BUTTON(w));

  if( nr != glurp->config->refresh_rate ) {
    debug("Refresh rate changed, setting timeout to %fms", nr);
    glurp->refresh_rate_status++;
    /* a safety check to keep the counter in loop */
    if( glurp->refresh_rate_status > 30000 ) glurp->refresh_rate_status = 0;
    g_timeout_add((guint)nr, gui_update_cb, GINT_TO_POINTER((guint)glurp->refresh_rate_status));
    glurp->config->refresh_rate = nr;
  }

  w = GTK_WIDGET(gtk_builder_get_object(builder, "checkbutton_save_size"));
  glurp->config->save_size = yesno(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));

  w = GTK_WIDGET(gtk_builder_get_object(builder, "checkbutton_display_trayicon"));
  glurp->config->trayicon = yesno(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
  if (yesno(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))))
    trayicon_show(glurp->trayicon);
  else
    trayicon_hide(glurp->trayicon);
} 

void create_playlist_liststore() {
  GtkListStore *liststore;
  GtkTreeViewColumn *column;
  GtkCellRenderer *rend;
  GtkWidget *tv;

  if(glurp->gui_playlist) return;

  liststore = gtk_list_store_new(
        NUM_PL,
        G_TYPE_INT,
        G_TYPE_STRING,  /* title */
        G_TYPE_STRING,  /* time */
        G_TYPE_INT,  /* id */

        G_TYPE_INT,  /* bold */
        G_TYPE_BOOLEAN  /* bold-set */
        );

  debug("playlist liststore created");

  tv = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_playlist"));
  gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(liststore));

  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(tv)), GTK_SELECTION_MULTIPLE);

  rend = gtk_cell_renderer_text_new();

  column = gtk_tree_view_column_new_with_attributes("#", rend, "text", PL_POS, "weight", PL_BOLD, "weight-set", PL_BOLD_SET, NULL);
  gtk_tree_view_column_set_resizable(column, TRUE);

  column = gtk_tree_view_column_new_with_attributes("Title", rend, "text", PL_TITLE, "weight", PL_BOLD, "weight-set", PL_BOLD_SET, NULL);
  gtk_tree_view_column_set_resizable(column, TRUE);

  column = gtk_tree_view_column_new_with_attributes("Time", rend, "text", PL_TIME, "weight", PL_BOLD, "weight-set", PL_BOLD_SET, NULL);
  gtk_tree_view_column_set_resizable(column, TRUE);

  column = gtk_tree_view_column_new_with_attributes("Id", rend, "text", PL_ID, "weight", PL_BOLD, "weight-set", PL_BOLD_SET, NULL);
  gtk_tree_view_column_set_resizable(column, TRUE);

  gtk_widget_show(tv);

  glurp->gui_playlist = liststore;
}

void add_song_to_gui_playlist(mpd_Song *song, gint pos) {
  GtkTreeIter iter;
  gchar title[1024];

  if( !glurp->gui_playlist ) {
    debug("No playlist liststore!");
    return;
  }

  if( !song ) {
    debug("song == NULL, ignoring");
    return;
  }

  gtk_list_store_append(glurp->gui_playlist, &iter);
  mpd_song_markup(title, 1023, "[%artist% - %title%]|%name%|%file%", song);

  gtk_list_store_set(glurp->gui_playlist, &iter,
    PL_POS, song->pos + 1,
    PL_TITLE, title,
    PL_ID, song->id,
    -1);
}

void detach_gui_playlist_liststore() {
  g_object_ref(glurp->gui_playlist);
  gtk_tree_view_set_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist")), NULL);
}

void attach_gui_playlist_liststore() {
  gtk_tree_view_set_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist")), GTK_TREE_MODEL(glurp->gui_playlist));
  g_object_unref(glurp->gui_playlist);
}


void hide_gui_playlist() {
  gint width, height;

  gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "glurp_functional_area")));
  gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "glurp_notebook_separator")));
  gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(builder, "glurp_window_main")), &width, &height);
  gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(builder, "glurp_window_main")), width, 1);
}

void show_gui_playlist() {
  gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "glurp_notebook_separator")));
  gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "glurp_functional_area")));
}

void create_playlist_list_liststore() {
  GtkListStore *ls;
  GtkTreeViewColumn *col;
  GtkCellRenderer *rend;
  GtkWidget *tv;

  if(glurp->gui_playlist_list) return;

  debug("Creating playlist list liststore...");

  ls = gtk_list_store_new(1, G_TYPE_STRING);

  tv = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_playlist_list"));
  gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(ls));

  rend = gtk_cell_renderer_text_new();

  col = gtk_tree_view_column_new_with_attributes("Filename", rend, "text", 0, NULL);
  gtk_tree_view_column_set_resizable(col, FALSE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

  gtk_widget_show(tv);

  glurp->gui_playlist_list = ls;

  debug("Playlist list liststore created");
}

void gui_set_connecting() {
  gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "button_server_disconnect")));
  gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "button_server_connect")));
  statusbar_print("Connecting");
  gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(builder, "entry_trackname")), "Connecting...");
}

void gui_set_connected() {
  debug("updating gui");
  gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "button_server_connect")));
  gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "button_server_disconnect")));

  gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "volumebar")), TRUE);

  if( OUTPUTS_CAPABLE_MPD(glurp->mpd) )
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_outputs")), TRUE);
  
  populate_gui_add_tree();
  get_playlist_list();
  populate_gui_playlist_list();
  populate_stream_liststore();
}    

void gui_set_disconnected() {
  debug("updating gui");
  gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(builder, "entry_trackname")), "Not connected...");

  gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "progressbar")), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "volumebar")), FALSE);

  gtk_range_set_value(GTK_RANGE(gtk_builder_get_object(builder, "progressbar")), 0);
  gtk_scale_button_set_value(GTK_SCALE_BUTTON(gtk_builder_get_object(builder, "volumebar")), 0);

//  gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(builder, "button_time")), "--:--");
  gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "label_time")), "--:--");
  gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "label_bitrate")), "---");

  gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "button_server_connect")));
  gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "button_server_disconnect")));

//  gtk_widget_hide(gtk_builder_get_object(builder, "button_pause"));
//  gtk_widget_show(gtk_builder_get_object(builder, "button_play"));

  gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_outputs")), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "togglebutton_repeat")), FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "togglebutton_random")), FALSE);
  gtk_list_store_clear(glurp->gui_playlist);
  glurp->playlist_version = -1;
  glurp->scroll = 0;

  statusbar_print("Disconnected");
}

gboolean gui_update_cb(gpointer val) {
  return gui_update(GPOINTER_TO_INT(val));
}

gboolean gui_update(gint refresh_rate_status) {

  if (!mpd_check_connected(glurp->mpd)) return FALSE;
  mpd_status_update(glurp->mpd); // this will call our mpd-callbacks if anything interesting happens
  
  /* detect first run */
/*  if( !glurp->playlist_version || (glurp->playlist_version == -1) ) {
    firstrun = TRUE;
    debug("first playlist update run");
    update_playlist();
    populate_gui_playlist();
  }
*/  
  if( refresh_rate_status != glurp->refresh_rate_status ) {
    return FALSE;
  }

  
/* PERIODIC: unhilight song */
  if( !PLAYING && !PAUSED && glurp->prev_song_num != -1 ) {
    gui_playlist_hilight(-1);
    glurp->scroll = 0;
  }

  return TRUE;
}

void add_playlist_to_gui_list(GlurpPl *pl) {
  GtkTreeIter iter;

  if( !glurp->gui_playlist_list ) {
    debug("No playlist list liststore!");
    return;
  }

  if( !pl ) {
    debug("pl == NULL, ignoring");
    return;
  }

  gtk_list_store_append(glurp->gui_playlist_list, &iter);

  gtk_list_store_set(glurp->gui_playlist_list, &iter, 0, pl->name, -1);
}

void populate_gui_playlist_list() {
  GlurpPl *pl;
  GtkTreeView *tv;
  GtkTreeModel *tm = GTK_TREE_MODEL(glurp->gui_playlist_list);
  GtkTreePath *path;
  GtkTreeIter iter;

  gtk_list_store_clear(glurp->gui_playlist_list);

  if( !glurp->playlists ) return;

  for( pl = glurp->playlists; pl; pl = pl->next ) {
    debug("Adding '%s' to gui list", pl->name);
    add_playlist_to_gui_list(pl);
  }

  tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist_list"));

  gtk_tree_model_get_iter_first(tm, &iter);
  path = gtk_tree_model_get_path(tm, &iter);

  gtk_tree_view_set_cursor(tv, path, gtk_tree_view_get_column(tv, 0), FALSE);

  gtk_tree_view_columns_autosize(tv);

}

void playlists_window_destroyed() {
  if(!glurp->gui_playlist_list) return;
  debug("playlists window destroyed, cleaning up");
  if( G_IS_OBJECT(glurp->gui_playlist_list) ) g_object_unref(G_OBJECT(glurp->gui_playlist_list));
  glurp->gui_playlist_list = NULL;
}

void gui_playlist_set_cursor(gint pos) {
  GtkTreeView *tv;
  GtkTreeModel *tm;
  GtkTreePath *path;
  GtkTreeIter iter;

  debug("Scrolling to row #%d", pos + 1);

  tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist"));
  tm = gtk_tree_view_get_model(tv);

  gtk_tree_model_iter_nth_child(tm, &iter, NULL, pos);

  path = gtk_tree_model_get_path(tm, &iter);

  gtk_tree_view_set_cursor(tv, path, gtk_tree_view_get_column(tv, PL_POS), FALSE);

  gtk_tree_path_free(path);
}

gboolean gui_playlist_scroll_cb(gpointer val) {
  return gui_playlist_scroll(GPOINTER_TO_INT(val));
}

gboolean gui_playlist_scroll(gint pos) {
  GtkTreeView *tv;
  GtkTreeModel *tm;
  GtkTreePath *path;
  GtkTreeIter iter;

  debug("Scrolling to row #%d", pos + 1);

  tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist"));
  tm = gtk_tree_view_get_model(tv);

  gtk_tree_model_iter_nth_child(tm, &iter, NULL, pos);

  path = gtk_tree_model_get_path(tm, &iter);

  gtk_tree_view_scroll_to_cell(tv, path, NULL, TRUE, 0.5, 0);
  gtk_tree_view_set_cursor(tv, path, gtk_tree_view_get_column(tv, PL_POS), FALSE);

  gtk_tree_path_free(path);

  return FALSE;
}

gboolean gui_playlist_hilight_cb(gpointer val) {
  return gui_playlist_hilight(GPOINTER_TO_INT(val));
}

gboolean gui_playlist_hilight(gint pos) {
  GtkTreeView *tv;
  GtkTreeModel *tm;
  GtkTreeIter iter;

  if( pos < -1 ) return FALSE;

  tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist"));
  tm = gtk_tree_view_get_model(tv);

  if(glurp->prev_song_num != -1) {
    gtk_tree_model_iter_nth_child(tm, &iter, NULL, glurp->prev_song_num);
    gtk_list_store_set(GTK_LIST_STORE(tm), &iter, PL_BOLD, PANGO_WEIGHT_NORMAL, PL_BOLD_SET, TRUE, -1);
  }

  glurp->prev_song_num = pos;

  if( pos >= 0 ) {
    gtk_tree_model_iter_nth_child(tm, &iter, NULL, pos);
    gtk_list_store_set(GTK_LIST_STORE(tm), &iter, PL_BOLD, PANGO_WEIGHT_ULTRABOLD, PL_BOLD_SET, TRUE, -1);
  }

  return FALSE;
}

gboolean gui_trackname_scroll() {
  GtkEntry *entry = GTK_ENTRY(gtk_builder_get_object(builder, "entry_trackname"));
  gint length;
  gint width;
  if (!GTK_IS_ENTRY(entry)) return TRUE;
  length = g_utf8_strlen(gtk_entry_get_text(entry), -1);
  pango_layout_xy_to_index(gtk_entry_get_layout(entry), glurp->trackname_width*PANGO_SCALE, 0, &width, NULL);
  width = gtk_entry_layout_index_to_text_index(entry, width);


  if( glurp->scroll == 1 ) {
    debug("Scrolling started, glurp->scroll = %d", glurp->scroll);
    glurp->scroll = width - 1;
  }

  if( glurp->scroll > 0 ) {
    if( glurp->scroll < length ) glurp->scroll++;
    else glurp->scroll = - width; 
  } else {
    if( -(glurp->scroll) < length ) glurp->scroll--;
    else glurp->scroll = width - 1;
  }

  if( glurp->scroll > 0 )
    gtk_editable_set_position(GTK_EDITABLE(entry), glurp->scroll);
  else
    gtk_editable_set_position(GTK_EDITABLE(entry), length + glurp->scroll);

  if( gtk_widget_is_focus(GTK_WIDGET(entry)) )
    gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(builder, "treeview_playlist")));

  return TRUE;
}

void add_window_destroyed() {
  debug("'add' window destroyed, cleaning up");
	/* ? */
}

void create_gui_add_tree() {
  GtkWidget *tv;
  GtkTreeViewColumn *column;
  GtkCellRenderer *rend, *prend;
  GValue val={0};

  debug("Creating 'add' treeview...");

  if( !glurp->gui_addtree ) {
    glurp->gui_addtree = gtk_tree_store_new(4,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_INT,
      G_TYPE_STRING
             );
    debug("'add' treestore created");
  }

  tv = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_add"));

  rend = gtk_cell_renderer_text_new();
  prend = gtk_cell_renderer_pixbuf_new();
  g_value_init(&val, G_TYPE_UINT);
  g_value_set_uint(&val, GTK_ICON_SIZE_SMALL_TOOLBAR);
  g_object_set_property(G_OBJECT(prend), "stock-size", &val);

  column = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_end(column, rend, TRUE);
  gtk_tree_view_column_set_attributes(column, rend, "text", ADD_NAME, NULL);
  gtk_tree_view_column_pack_start(column, prend, FALSE);
  gtk_tree_view_column_set_title(column, "Database");
  gtk_tree_view_column_add_attribute(column, prend, "stock_id", ADD_ICON);
  gtk_tree_view_column_set_resizable(column, FALSE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tv), column);

  column = gtk_tree_view_column_new_with_attributes("", rend, NULL);
  gtk_tree_view_column_set_resizable(column, FALSE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tv), column);
  gtk_tree_view_column_set_visible(column, FALSE);

  column = gtk_tree_view_column_new_with_attributes("", rend, NULL);
  gtk_tree_view_column_set_resizable(column, FALSE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tv), column);
  gtk_tree_view_column_set_visible(column, FALSE);

  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(tv)), GTK_SELECTION_MULTIPLE);

  gtk_widget_show(tv);

  debug("'add' treeview created");
}

void populate_gui_add_tree() {
  if( !glurp->gui_addtree ) return;

  gtk_tree_store_clear(GTK_TREE_STORE(glurp->gui_addtree));

  glurp->num_add_dirs = 0;

  glurp_add_add_dir("", NULL);

  if( !glurp->updating_db ) {
    debug("Setting 'Add' treeview model");
    gtk_tree_view_set_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add")), GTK_TREE_MODEL(glurp->gui_addtree));
  }
}

void populate_gui_add_search_tree() {
  GtkComboBox* combo_type;
  GtkEntry* find_entry;
  GtkTreeIter iter;
  GtkTreePath *path;
  gint n, nd;
  const gchar *what;

  if (!glurp->gui_addtree) return;

  /* remove last toplevel item if it's search results */
  n = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(glurp->gui_addtree), NULL);
  if ( n > 0 ) {
    gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(glurp->gui_addtree), &iter, NULL, n - 1);
    gtk_tree_model_get(GTK_TREE_MODEL(glurp->gui_addtree), &iter, ADD_NUM_DIRS, &nd, -1);
    if( nd == -2 ) {  /* last item is search results, let's ixnay it */
      debug("Removing previous search results treeview item");
      gtk_tree_store_remove(glurp->gui_addtree, &iter);
    }
  }

  combo_type = GTK_COMBO_BOX(gtk_builder_get_object(builder, "combo_add_find_type"));

  debug("Adding a folder for find results");
  find_entry = GTK_ENTRY(gtk_builder_get_object(builder, "entry_add_find_what"));
  what = gtk_entry_get_text(find_entry);
  if( what && g_utf8_strlen(what, -1) ) {
    gtk_tree_store_append(glurp->gui_addtree, &iter, NULL);
    gtk_tree_store_set(glurp->gui_addtree, &iter, ADD_ICON, GTK_STOCK_FIND, ADD_NAME, "Search results", ADD_NUM_DIRS, -2, -1);

    path = gtk_tree_model_get_path(GTK_TREE_MODEL(glurp->gui_addtree), &iter);
    glurp_add_search_result_dir( gtk_entry_get_text(find_entry), gtk_combo_box_get_active(combo_type), path);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add")), path, NULL, FALSE);
  }
  
  if( !glurp->updating_db ) {
    debug("Setting 'Add' treeview model");
    gtk_tree_view_set_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add")), GTK_TREE_MODEL(glurp->gui_addtree));
  }

  gtk_editable_select_region(GTK_EDITABLE(find_entry), 0, -1);

}
  
void gui_load_selected() {
  GtkTreeIter iter;
  GtkTreeView *tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add"));
  GtkTreeModel *tm;
  GtkTreeSelection *ts;
  GList *selected_rows;
  gint i, num;

  tm = gtk_tree_view_get_model(tv);

  if( !(ts = gtk_tree_view_get_selection(tv)) || !(num = gtk_tree_selection_count_selected_rows(ts)) ) {
    debug("No selection, ignoring");
    return;
  }

  selected_rows = gtk_tree_selection_get_selected_rows(ts, NULL);

  for( i = 0; i < num; i++ ) {
    gtk_tree_model_get_iter(tm, &iter, (GtkTreePath *)g_list_nth_data(selected_rows, i));
    gui_load_song(tm, iter);
  }
  mpd_playlist_queue_commit(glurp->mpd);
  return;
}

void gui_load_song(GtkTreeModel *tm, GtkTreeIter iter) {
  gchar *fname = NULL;
  GtkTreeIter child;
  gint i;

  gui_add_fill_dir(&iter, TRUE);

  
  if( gtk_tree_model_iter_has_child(tm, &iter) ) {
    for( i = 0; i < gtk_tree_model_iter_n_children(tm, &iter); i++ ) {
      gtk_tree_model_iter_nth_child(tm, &child, &iter, i);
      gui_load_song(tm, child);
    }
    return;
  }
      
  gtk_tree_model_get(tm, &iter, 1, &fname, -1);

  if( fname && g_utf8_strlen(fname, -1) ) {
    debug("Adding song '%s'", fname);

    mpd_playlist_queue_add(glurp->mpd, fname);
    g_free(fname);

  } else debug("Invalid song filename");

  return;
}

void gui_updating_disable_add_controls() {
  debug("Disabling 'add' controls");
  gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "scrolledwindow_add")), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "button_add_update")), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "button_add_add")), FALSE);

  debug("Removing 'Add' treeview model");
  gtk_tree_view_set_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add")), NULL);
}

void gui_updating_enable_add_controls() {
  debug("Enabling 'add' controls");
  gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "scrolledwindow_add")), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "button_add_update")), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "button_add_add")), TRUE);

  debug("Setting 'Add' treeview model");
  gtk_tree_view_set_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add")), GTK_TREE_MODEL(glurp->gui_addtree));
}

void gui_add_fill_dir(GtkTreeIter *iter, gboolean silent) {
  GtkTreeView *tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add"));
  GtkTreeModel *tm = gtk_tree_view_get_model(tv);
  GtkTreeIter child;
  gint nd;
  gchar *filepath;

  gtk_tree_model_get(tm, iter, ADD_NUM_DIRS, &nd, ADD_FILENAME, &filepath, -1);

  if( gtk_tree_model_iter_n_children(tm, iter) == 1 && nd == -1 ) {
    if( !gtk_tree_model_iter_children(tm, &child, iter) ) {
      debug("Couldn't get stub child");
      return;
    }

    debug("Filling dir '%s'", filepath);

    gtk_tree_store_set(GTK_TREE_STORE(tm), iter, ADD_NUM_DIRS, 0, -1);

    glurp_add_add_dir(filepath, gtk_tree_model_get_path(tm, iter));
    gtk_tree_store_remove(GTK_TREE_STORE(tm), &child);
    if( silent ) gtk_tree_view_collapse_row(tv, gtk_tree_model_get_path(tm, iter));

    debug("Filling complete");
  }
}

gboolean gui_get_iter_by_id(GtkTreeIter *iter, gint id) {
  GtkTreeModel *tm = GTK_TREE_MODEL(glurp->gui_playlist);
  gint cid;

  if( !gtk_tree_model_get_iter_first(tm, iter) ) {
    debug("Couldn't get first iter");
    return FALSE;
  }

  do {
    gtk_tree_model_get(tm, iter, PL_ID, &cid, -1);
    if( cid == id ) return TRUE;
  } while( gtk_tree_model_iter_next(tm, iter) );

  return FALSE;
}

gboolean gui_get_iter_by_pos(GtkTreeIter *iter, gint pos) {
  GtkTreeModel *tm = GTK_TREE_MODEL(glurp->gui_playlist);
  return gtk_tree_model_iter_nth_child(tm, iter, NULL, pos);
/*   gint cpos = 0;
 * 
 *   if( !gtk_tree_model_get_iter_first(tm, iter) ) {
 *     debug("Couldn't get first iter");
 *     return FALSE;
 *   }
 * 
 *   while( cpos < pos ) {
 *     if( !gtk_tree_model_iter_next(tm, iter) ) return FALSE;;
 *     cpos++;
 *   }
 *  return TRUE;
 */

}


void create_stream_liststore() {
  GtkListStore *ls;
  GtkWidget *tv;
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;

  if(glurp->gui_stream_list) {
    debug("Stream liststore exists, returning");
    return;
  }

  debug("Creating stream liststore...");

  ls = gtk_list_store_new(1, G_TYPE_STRING);
  tv = GTK_WIDGET(gtk_builder_get_object(builder, "streams_treeview"));

  gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(ls));

  rend = gtk_cell_renderer_text_new();

  col = gtk_tree_view_column_new_with_attributes("#", rend,
                  "text", 0,
                  NULL);
  gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tv),col);

  glurp->gui_stream_list = ls;

  debug("Stream combobox model created");
}

void populate_stream_liststore() {
  GlurpStream *s = NULL;
  GtkTreeIter iter;

  gtk_list_store_clear(glurp->gui_stream_list);

  for( s = glurp->stream_history; s; s = s->next ) {
    debug("Adding row '%s'", s->url);
    gtk_list_store_append(glurp->gui_stream_list, &iter);
    gtk_list_store_set(glurp->gui_stream_list, &iter, 0, g_strdup(s->url), -1);
  }

  statusbar_print("Stream combobox model populated");

}

void stream_window_destroyed() {
  if(!glurp->gui_stream_list) return;
  debug("playlists window destroyed, cleaning up");
  if( G_IS_OBJECT(glurp->gui_stream_list) ) g_object_unref(G_OBJECT(glurp->gui_stream_list));
  glurp->gui_stream_list = NULL;
}

gchar *get_selected_stream() {
  gchar *text = NULL;
  GtkWidget *cb = NULL;

  if(!glurp->gui_stream_list ) {
    debug("No combobox");
    return NULL;
  }

  cb = GTK_WIDGET(gtk_builder_get_object(builder, "streams_entry"));
  text = (gchar *)gtk_entry_get_text(GTK_ENTRY(cb));
  return text;
}

GtkMenu *populate_outputs_menu() {
  GtkWidget *menu = gtk_menu_new();
  GtkWidget *hbox, *label, *icon;
  GtkMenuItem *item = NULL;
  mpd_OutputEntity *output;
  MpdData* entity;
  glong i;

  for (entity = mpd_server_get_output_devices(glurp->mpd);entity;entity = mpd_data_get_next(entity)  )  {
    output = entity->output_dev;
    debug("%d: %s", output->id, output->name);
    hbox = GTK_WIDGET(gtk_hbox_new(FALSE, 0));
    gtk_widget_show(hbox);

    /* "enabled" icon */
    if( output->enabled ) {
      icon = gtk_image_new_from_stock(GTK_STOCK_APPLY, GTK_ICON_SIZE_MENU);
      i = output->id + 1;
    } else {
      icon = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
      i = -(output->id + 1);
    }
    gtk_widget_show(icon);
    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);

    /* label */
    label = GTK_WIDGET(gtk_label_new(g_strdup_printf("%d: %s", output->id, output->name)));
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  
    item = GTK_MENU_ITEM(gtk_menu_item_new());
    gtk_container_add(GTK_CONTAINER(item), hbox);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
    gtk_widget_show(GTK_WIDGET(item));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_menu_output_activate), (gpointer)i);
  }
  g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(on_menu_outputs_deactivate), NULL);

  return GTK_MENU(menu);
}

void gui_add_append(gchar *path, GtkTreePath *gpath, gboolean song) {
  GtkTreeModel *tm = GTK_TREE_MODEL(glurp->gui_addtree);
  GtkTreeIter iparent, iter, newchild;
  gint nd;

  debug("Adding '%s'", path);

  /* get stored number of dirs in parent dir */
  if( gpath ) {
    gtk_tree_model_get_iter(tm, &iparent, gpath);
    gtk_tree_model_get(tm, &iparent, ADD_NUM_DIRS, &nd, -1);
  }

  if( song ) {
    gtk_tree_store_append(glurp->gui_addtree, &iter, (gpath ? &iparent : NULL));
    gtk_tree_store_set(glurp->gui_addtree, &iter, ADD_ICON, "glurp-media-audiofile", -1);
  } else {
    gtk_tree_store_insert(glurp->gui_addtree, &iter, (gpath ? &iparent : NULL), (gpath ? nd : glurp->num_add_dirs));
    if( gpath ) gtk_tree_store_set(glurp->gui_addtree, &iparent, ADD_NUM_DIRS, nd+1, -1);
    else glurp->num_add_dirs++;
    gtk_tree_store_set(glurp->gui_addtree, &iter, ADD_ICON, GTK_STOCK_OPEN, ADD_NUM_DIRS, -1, -1);
    gtk_tree_store_insert(glurp->gui_addtree, &newchild, &iter, (gpath ? nd : glurp->num_add_dirs));
  }

  gtk_tree_store_set(glurp->gui_addtree, &iter, ADD_NAME, glurp_filename(path), ADD_FILENAME, path, -1);

  if( gpath )
    gtk_tree_view_expand_row(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add")), gpath, FALSE);
}

void find_notebook_page(GtkWidget* child, GtkNotebook** notebook, gint* page_num) {
	GtkNotebook *notebooks[2] = {
		GTK_NOTEBOOK(gtk_builder_get_object(builder, "glurp_functional_notebook1")),
		GTK_NOTEBOOK(gtk_builder_get_object(builder, "glurp_functional_notebook2"))
	};
	if ( ( *page_num = gtk_notebook_page_num(notebooks[0], child) ) >= 0) {
    *notebook = notebooks[0];
	} else {
		if ( ( *page_num = gtk_notebook_page_num(notebooks[1], child) ) >= 0) {
      *notebook = notebooks[1];
		} else {
			return;
		}
	}
}

void glurp_switch_functional_page(GtkWidget* focus, gboolean next) {
  GtkNotebook* notebooks[] =	{
    GTK_NOTEBOOK(gtk_builder_get_object(builder, "glurp_functional_notebook1")),
    GTK_NOTEBOOK(gtk_builder_get_object(builder, "glurp_functional_notebook2"))
  };
  gint n,nb,tab,tab2;
  GtkWidget* w;
  gpointer tab_indexp;
  if (focus) {
    if(GTK_IS_NOTEBOOK(focus)) {
      tab = gtk_notebook_get_current_page(GTK_NOTEBOOK(focus));
      w = gtk_notebook_get_nth_page(GTK_NOTEBOOK(focus), tab);
    } else {
      w = focus;
      while(gtk_widget_get_parent(w)) {
        if (GTK_IS_NOTEBOOK(gtk_widget_get_parent(w))) {
          break;
        } else {
          w = gtk_widget_get_parent(w);
        }
      }
      if (!gtk_widget_get_parent(w)) w = NULL;
    }
  } else 
    w = NULL;
  if (w) {
    /* w is now the page which had focus while CTRL PG* was pressed */
    g_object_get(G_OBJECT(w), "user_data", &tab_indexp, NULL);
    tab = GPOINTER_TO_INT(tab_indexp)+(next?1:-1);

    for (nb = 0; nb < 2; nb++) {
      for (n = 0; n < gtk_notebook_get_n_pages(notebooks[nb]);n++) {
        g_object_get(G_OBJECT(gtk_notebook_get_nth_page(notebooks[nb], n)), "user_data", &tab_indexp, NULL);
        if (tab == GPOINTER_TO_INT(tab_indexp)) {
          gtk_notebook_set_current_page(notebooks[nb], n);
          gtk_widget_grab_focus(GTK_WIDGET(notebooks[nb]));
          return;
        }
      }
    }
    return;
  } else {
    debug("PG{UP,DOWN} was pressed outside the functional area");
    /* something outside the functional notebooks had focus */
    w = GTK_WIDGET(gtk_builder_get_object(builder, "glurp_functional_notebook1"));
    tab = gtk_notebook_get_current_page(GTK_NOTEBOOK(w));
    if (tab >= 0) {
      g_object_get(G_OBJECT(gtk_notebook_get_nth_page(GTK_NOTEBOOK(w), tab)), "user_data", &tab_indexp, NULL);
      tab = GPOINTER_TO_INT(tab_indexp);
    } else {
      gtk_widget_grab_focus(GTK_WIDGET(notebooks[1]));
      return;
    }
    w = GTK_WIDGET(gtk_builder_get_object(builder, "glurp_functional_notebook2"));
    tab2 = gtk_notebook_get_current_page(GTK_NOTEBOOK(w));
    if (tab2 >= 0) {
      g_object_get(G_OBJECT(gtk_notebook_get_nth_page(GTK_NOTEBOOK(w), tab2)), "user_data", &tab_indexp, NULL);
      tab2 = GPOINTER_TO_INT(tab_indexp);
    } else {
      gtk_widget_grab_focus(GTK_WIDGET(notebooks[0]));
      return;
    }
    if (tab < tab2) 
      gtk_widget_grab_focus(GTK_WIDGET(notebooks[next?1:0]));
    else
      gtk_widget_grab_focus(GTK_WIDGET(notebooks[next?0:1]));
      
    return;
  }
}
        
        


/*void glurp_switch_functional_page(gint tab, gboolean next) {
  if ((tab < 0) || (tab > 4)) return;
  tab += next?1:-1;
  for (nb = 0; nb < 2; nb++) {
    for (n = 0; n < gtk_notebook_get_n_pages(notebooks[nb]);n++) {
      g_object_get(G_OBJECT(gtk_notebook_get_nth_page(notebooks[nb], n)), "user_data", &tab_indexp, NULL);
      if (tab == GPOINTER_TO_INT(tab_indexp)) {
        if (n == gtk_notebook_get_current_page(notebooks[nb])) 
          glurp_switch_functional_page(tab, next);
        else
          gtk_notebook_set_current_page(notebooks[nb], n);
        return;
      }
    }
  }
}*/

void functional_notebook_select_playlist() {
  GtkNotebook* notebook;
  gint page_num;
  
  find_notebook_page(GTK_WIDGET(gtk_builder_get_object(builder, "playlist_page")), &notebook, &page_num);
	gtk_notebook_set_current_page(notebook, page_num);
}

void functional_notebook_select_database() {
  GtkNotebook* notebook;
  gint page_num;
  
  find_notebook_page(GTK_WIDGET(gtk_builder_get_object(builder, "database_page")), &notebook, &page_num);
	gtk_notebook_set_current_page( notebook, page_num );
}

void functional_notebook_select_playlists() {
  GtkNotebook* notebook;
  gint page_num;
  
  find_notebook_page(GTK_WIDGET(gtk_builder_get_object(builder, "playlists_page")), &notebook, &page_num);
	gtk_notebook_set_current_page( notebook, page_num );
}

void functional_notebook_select_streams() {
  GtkNotebook* notebook;
  gint page_num;
  
  find_notebook_page(GTK_WIDGET(gtk_builder_get_object(builder, "streams_page")), &notebook, &page_num);
	gtk_notebook_set_current_page( notebook, page_num );
}

void functional_notebook_select_config() {
  GtkNotebook* notebook;
  gint page_num;
  
  find_notebook_page(GTK_WIDGET(gtk_builder_get_object(builder, "config_page")), &notebook, &page_num);
	gtk_notebook_set_current_page( notebook, page_num );
}

gboolean functional_notebook_playlist_selected() {
  GtkNotebook* notebook;
  gint page_num;

  find_notebook_page(GTK_WIDGET(gtk_builder_get_object(builder, "playlist_page")), &notebook, &page_num);
  return ( gtk_notebook_get_current_page(notebook) == page_num );
}

gboolean functional_notebook_database_selected() {
  GtkNotebook* notebook;
  gint page_num;

  find_notebook_page(GTK_WIDGET(gtk_builder_get_object(builder, "database_page")), &notebook, &page_num);
  return ( gtk_notebook_get_current_page(notebook) == page_num );
}

gboolean functional_notebook_config_selected() {
  GtkNotebook* notebook;
  gint page_num;

  find_notebook_page(GTK_WIDGET(gtk_builder_get_object(builder, "config_page")), &notebook, &page_num);
  return ( gtk_notebook_get_current_page(notebook) == page_num );
}

gboolean functional_notebook_streams_selected() {
  GtkNotebook* notebook;
  gint page_num;

  find_notebook_page(GTK_WIDGET(gtk_builder_get_object(builder, "streams_page")), &notebook, &page_num);
  return ( gtk_notebook_get_current_page(notebook) == page_num );
}

gboolean functional_notebook_playlists_selected() {
  GtkNotebook* notebook;
  gint page_num;

  find_notebook_page(GTK_WIDGET(gtk_builder_get_object(builder, "playlists_page")), &notebook, &page_num);
  return ( gtk_notebook_get_current_page(notebook) == page_num );
}
// vim: et sw=2 smarttab
