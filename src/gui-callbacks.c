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
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>

#include <libmpd/libmpd.h>
#include <libmpd/libmpd-playlist.h>
#include <libmpd/libmpd-database.h>

#include "structs.h"
#include "support.h"
#include "comm.h"
#include "gui.h"
#include "player.h"
#include "conf.h"

extern GtkBuilder *builder;
extern GlurpState *glurp;



void on_move_tab(GtkWidget* tab) {
  GtkWidget *label;
  GtkWidget *pane;
  GtkNotebook *notebook[2] = {
    GTK_NOTEBOOK(gtk_builder_get_object(builder, "glurp_functional_notebook1")),
    GTK_NOTEBOOK(gtk_builder_get_object(builder, "glurp_functional_notebook2"))
  };
  gpointer tab_indexp;
  gint page_num,notebook_num,tab_index, target_pos;
  if ( ( page_num = gtk_notebook_page_num(notebook[0], tab) ) >= 0) {
    notebook_num = 0;
  } else {
    if ( ( page_num = gtk_notebook_page_num(notebook[1], tab) ) >= 0) {
      notebook_num = 1;
    } else {
      return;
    }
  }
  /* get the position of this tab */
  g_object_get(G_OBJECT(tab), "user_data", &tab_indexp, NULL);
  tab_index = GPOINTER_TO_INT(tab_indexp);
  
  label = gtk_notebook_get_tab_label(notebook[notebook_num], tab);
  g_object_ref(tab);
  g_object_ref(label);
  gtk_notebook_remove_page(notebook[notebook_num], page_num);
  for (target_pos = gtk_notebook_get_n_pages(notebook[1-notebook_num]) - 1 ; target_pos >= 0; target_pos--) {
    g_object_get(G_OBJECT(gtk_notebook_get_nth_page(notebook[1-notebook_num], target_pos)), "user_data", &tab_indexp, NULL);
    if (tab_index > GPOINTER_TO_INT(tab_indexp)) {
      gtk_notebook_insert_page(notebook[1-notebook_num], tab, label, target_pos+1);
      break;
    }
  }
  if (target_pos < 0)
    gtk_notebook_prepend_page(notebook[1-notebook_num], tab, label);

  if (gtk_notebook_get_n_pages(notebook[notebook_num])==0) {
    pane = GTK_WIDGET(gtk_builder_get_object(builder, "glurp_functional_area"));
    gtk_paned_set_position(GTK_PANED(pane), (notebook_num==1)?0:pane->allocation.width);
  }
  if ( gtk_notebook_get_n_pages( notebook[1-notebook_num] ) == 1) {
    pane = GTK_WIDGET(gtk_builder_get_object(builder, "glurp_functional_area"));
    gtk_paned_set_position(GTK_PANED(pane), pane->allocation.width/2);
  }
  g_object_unref(tab);
  g_object_unref(label);
}

void on_move_playlist_tab_clicked(GtkWidget *widget, gpointer user_data) {
  on_move_tab(GTK_WIDGET(gtk_builder_get_object(builder, "playlist_page")));
}
void on_move_database_tab_clicked(GtkWidget *widget, gpointer user_data) {
  on_move_tab(GTK_WIDGET(gtk_builder_get_object(builder, "database_page")));
}
void on_move_streams_tab_clicked(GtkWidget *widget, gpointer user_data) {
  on_move_tab(GTK_WIDGET(gtk_builder_get_object(builder, "streams_page")));
}
void on_move_playlists_tab_clicked(GtkWidget *widget, gpointer user_data) {
  on_move_tab(GTK_WIDGET(gtk_builder_get_object(builder, "playlists_page")));
}
void on_move_config_tab_clicked(GtkWidget *widget, gpointer user_data) {
  on_move_tab(GTK_WIDGET(gtk_builder_get_object(builder, "config_page")));
}


void on_ui_quit(GtkWidget *widget, gpointer user_data) {

  debug("Quitting Glurp...");

  if (glurp->mpd) {
    if( mpd_check_connected(glurp->mpd) ) glurp_disconnect();
  
    mpd_free(glurp->mpd);
  }
  config_save();

  gtk_main_quit();
}

gboolean on_window_moved(GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
  debug("Storing window position and size (%dx%d+%d+%d)", event->width, event->height, event->x, event->y);

  glurp->config->pos_x = event->x;
  glurp->config->pos_y = event->y;

  glurp->config->width = event->width;
  glurp->config->height = event->height;

  return FALSE;
}

static void set_condensed_modifier_style(GtkWidget* widget, gboolean condensed) {
  GtkRcStyle* style;
  if (condensed) {
    style = gtk_widget_get_modifier_style(widget);
    style->xthickness = 0;
    style->ythickness = 0;
    gtk_widget_modify_style(widget, style);
  } else 
    gtk_widget_modify_style(widget, gtk_rc_style_new());
}

gboolean on_condense_clicked(GtkWidget *widget, gpointer user_data) {
  GtkWidget *wi, *wi2;
  guint size;
  gint x;
  gint y;

  wi = GTK_WIDGET(gtk_builder_get_object(builder, "glurp_window_main"));
  gtk_window_get_position(GTK_WINDOW(wi), &x, &y);
  gtk_widget_hide(wi);

  gboolean condensed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  size = condensed?gtk_icon_size_from_name ("glurp_condensed"):GTK_ICON_SIZE_BUTTON;
  if (condensed) {
    debug("switching to condensed style");
    gtk_widget_hide_all(GTK_WIDGET(gtk_builder_get_object(builder, "frame_bitrate")));
    gtk_widget_hide_all(GTK_WIDGET(gtk_builder_get_object(builder, "frame_mode")));
    gtk_widget_hide_all(GTK_WIDGET(gtk_builder_get_object(builder, "volumebar")));
    gtk_widget_hide_all(GTK_WIDGET(gtk_builder_get_object(builder, "statusbar_main")));
    gtk_widget_hide_all(GTK_WIDGET(gtk_builder_get_object(builder, "progressbar")));
    gtk_widget_hide_all(GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_outputs")));
    if( mpd_check_connected(glurp->mpd) ) 
    	gtk_widget_hide_all(GTK_WIDGET(gtk_builder_get_object(builder, "button_server_connect")));
		else
	    gtk_widget_hide_all(GTK_WIDGET(gtk_builder_get_object(builder, "button_server_disconnect")));
    
    /* remove decoration, but add a border */
    wi = GTK_WIDGET(gtk_builder_get_object(builder, "glurp_window_main"));
    gtk_window_set_decorated(GTK_WINDOW(wi), FALSE);
    wi2 = gtk_bin_get_child(GTK_BIN(wi));
    g_object_ref(G_OBJECT(wi2));
    gtk_container_remove(GTK_CONTAINER(wi), wi2);
    gtk_container_add(GTK_CONTAINER(wi), gtk_frame_new(NULL));
    wi = gtk_bin_get_child(GTK_BIN(wi));
    gtk_frame_set_shadow_type(GTK_FRAME(wi), GTK_SHADOW_ETCHED_OUT);
    gtk_widget_show(wi);
    gtk_container_add(GTK_CONTAINER(wi), wi2);
    g_object_unref(wi2);
    gtk_container_set_border_width(GTK_CONTAINER(wi2), 3);
    /* remove frames */
    glurp->frame_time  = GTK_WIDGET(gtk_builder_get_object(builder, "frame_time"));
    wi = GTK_WIDGET(gtk_builder_get_object(builder, "hbox_player_info"));
    wi2 = gtk_bin_get_child(GTK_BIN(glurp->frame_time));
    g_object_ref(G_OBJECT(glurp->frame_time));
    g_object_ref(G_OBJECT(wi2));
    gtk_container_remove(GTK_CONTAINER(wi), glurp->frame_time);
    gtk_container_remove(GTK_CONTAINER(glurp->frame_time), wi2);
    gtk_box_pack_start(GTK_BOX(wi), wi2, FALSE, FALSE, 0);
    g_object_unref(G_OBJECT(wi2));
    
    glurp->frame_trackname  = GTK_WIDGET(gtk_builder_get_object(builder, "frame_trackname"));
    wi2 = gtk_bin_get_child(GTK_BIN(glurp->frame_trackname));
    g_object_ref(G_OBJECT(glurp->frame_trackname));
    g_object_ref(G_OBJECT(wi2));
    gtk_container_remove(GTK_CONTAINER(wi), glurp->frame_trackname);
    gtk_container_remove(GTK_CONTAINER(glurp->frame_trackname), wi2);
    gtk_box_pack_start(GTK_BOX(wi), wi2, TRUE, TRUE, 0);
    g_object_unref(G_OBJECT(wi2));
  } else {
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "frame_bitrate")));
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "frame_mode")));
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "volumebar")));
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "statusbar_main")));
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "progressbar")));
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_outputs")));
    if( mpd_check_connected(glurp->mpd) ) 
    	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "button_server_connect")));
		else
    	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "button_server_disconnect")));
    /* replace decoration and remove the border */
    wi = GTK_WIDGET(gtk_builder_get_object(builder, "glurp_window_main"));
    gtk_window_set_decorated(GTK_WINDOW(wi), TRUE);
    wi = gtk_bin_get_child(GTK_BIN(wi));
    wi2 = gtk_bin_get_child(GTK_BIN(wi));
    g_object_ref(G_OBJECT(wi2));
    gtk_container_remove(GTK_CONTAINER(wi), wi2);
    gtk_widget_destroy(wi);
    wi = GTK_WIDGET(gtk_builder_get_object(builder, "glurp_window_main"));
    gtk_container_add(GTK_CONTAINER(wi), wi2);
    g_object_unref(wi2);
    gtk_container_set_border_width(GTK_CONTAINER(wi2), 0);
    /* replace frames */
    wi = GTK_WIDGET(gtk_builder_get_object(builder, "hbox_player_info"));
//    wi2 = GTK_WIDGET(gtk_builder_get_object(builder, "button_time"));
    wi2 = GTK_WIDGET(gtk_builder_get_object(builder, "label_time"));
    g_object_ref(G_OBJECT(wi2));
    gtk_container_remove(GTK_CONTAINER(wi), wi2);
    gtk_box_pack_start(GTK_BOX(wi), glurp->frame_time, FALSE, FALSE, 5);
    gtk_container_add(GTK_CONTAINER(glurp->frame_time), wi2);
    g_object_unref(G_OBJECT(glurp->frame_time));
    g_object_unref(G_OBJECT(wi2));
    
    wi2 = GTK_WIDGET(gtk_builder_get_object(builder, "entry_trackname"));
    g_object_ref(G_OBJECT(wi2));
    gtk_container_remove(GTK_CONTAINER(wi), wi2);
    gtk_box_pack_start(GTK_BOX(wi), glurp->frame_trackname, TRUE, TRUE, 5);
    gtk_container_add(GTK_CONTAINER(glurp->frame_trackname), wi2);
    g_object_unref(G_OBJECT(glurp->frame_trackname));
    g_object_unref(G_OBJECT(wi2));
  }
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "button_prev"));
  set_condensed_modifier_style(wi, condensed);
  gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(wi)));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-prev", size));
  gtk_widget_show_all(wi);
  /* prev */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "button_prev"));
  set_condensed_modifier_style(wi, condensed);
  gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(wi)));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-prev", size) );
  gtk_widget_show_all(wi);
  /* play */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "button_play"));
  set_condensed_modifier_style(wi, condensed);
  gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(wi)));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-play", size) );
  gtk_widget_show_all(wi);
  /* pause */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "button_pause"));
  set_condensed_modifier_style(wi, condensed);
  gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(wi)));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-pause", size) );
  gtk_widget_show_all(wi);
  /* stop */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "button_stop"));
  set_condensed_modifier_style(wi, condensed);
  gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(wi)));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-stop", size) );
  gtk_widget_show_all(wi);
  /* next */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "button_next"));
  set_condensed_modifier_style(wi, condensed);
  gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(wi)));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-next", size) );
  gtk_widget_show_all(wi);

  /* playlist show/hide */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_playlist"));
  set_condensed_modifier_style(wi, condensed);
  gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(wi)));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-playlist", size) );
  gtk_widget_show_all(wi);

  /* repeat */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_repeat"));
  set_condensed_modifier_style(wi, condensed);
  gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(wi)));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-repeat", size) );
  gtk_widget_show_all(wi);

  /* random */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_random"));
  set_condensed_modifier_style(wi, condensed);
  gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(wi)));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-random", size) );
  gtk_widget_show_all(wi);

  /* condense */
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_condense"));
  set_condensed_modifier_style(wi, condensed);
  gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(wi)));
  gtk_container_add(GTK_CONTAINER(wi), gtk_image_new_from_stock("glurp-condense", size) );
  gtk_widget_show_all(wi);

  if (condensed)
    gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(builder, "glurp_window_main")), 1,1);
  
  wi = GTK_WIDGET(gtk_builder_get_object(builder, "glurp_window_main"));
  gtk_window_move(GTK_WINDOW(wi), x, y);
  gtk_widget_show(wi);
  return FALSE;
};

void on_entry_trackname_size_allocate(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data) {
  glurp->trackname_width = allocation->width;
  gui_check_scroll();
}

gboolean on_button_config_revert_clicked(GtkWidget *widget, gpointer user_data) {
  
  debug("Revert, refilling config window...");
  populate_config();

  return FALSE;
}

gboolean on_config_destroy(GtkWidget *widget, gpointer user_data) {
  return FALSE;
}

gboolean on_button_config_apply_clicked(GtkWidget *widget, gpointer user_data) {
 
  debug("Apply pressed, hiding config window...");

  debug("Storing variables into GlurpConfig");
  store_config();

  config_save();

  statusbar_print("Config saved...");

  return FALSE;
}

gboolean on_ui_press_connect(GtkWidget *widget, gpointer user_data) {
  glurp_connect();
  return FALSE;
}

gboolean on_ui_press_disconnect(GtkWidget *widget, gpointer user_data) {
  glurp_disconnect();
  return FALSE;
}

gboolean on_ui_volume_changed(GtkWidget *widget, gpointer user_data) {
  gdouble i = gtk_scale_button_get_value(GTK_SCALE_BUTTON(widget));

  if( mpd_check_connected(glurp->mpd) ) {
    mpd_status_set_volume(glurp->mpd, (int)(i*100));
    
    statusbar_print("Volume: %d%%", (int)(i*100));
  }

  return FALSE;
}

gboolean on_ui_progress_change(GtkWidget *widget, gpointer user_data) {
  double pos, tt;

  if(!glurp->progress_dragging) return FALSE;

  tt = mpd_status_get_total_song_time(glurp->mpd);

  pos = gtk_range_get_value(GTK_RANGE(widget));

  debug("Seeking to %d seconds", (gint)pos);
  statusbar_print("Seek to %02d:%02d/%02d:%02d", (gint)pos/60, (gint)pos%60, (gint)tt/60, (gint)tt%60);
  mpd_player_seek(glurp->mpd, pos);
  debug("setting FALSE");
  glurp->progress_dragging = FALSE;

  return FALSE;
}

gboolean on_ui_progress_change_start_kb(GtkWidget *widget, GdkEventKey *event,
                gpointer user_data) {
  guint k = event->keyval;

  if( !glurp->progress_dragging &&
      !(event->state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD1_MASK)) &&
      ((k == GDK_KP_Left) || 
       (k == GDK_KP_Right) || 
       (k == GDK_Left) || 
       (k == GDK_Right) ||
       (k == GDK_Page_Up) ||
       (k == GDK_Page_Down) )) {
    debug("setting TRUE");
    glurp->progress_dragging = TRUE;
  } else if( glurp->progress_dragging )
    return FALSE;
  return TRUE;
}

gboolean on_ui_progress_change_start(GtkWidget *widget, gpointer user_data) {
  if( !glurp->progress_dragging ) {
    debug("setting TRUE");
    glurp->progress_dragging = TRUE;
  }
  return FALSE;
}

gboolean on_ui_playlist_clicked(GtkWidget *widget, gpointer user_data) {
  if( !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) )
    hide_gui_playlist();
  else
    show_gui_playlist();

  return FALSE;
}

gboolean on_ui_player_play(GtkWidget *widget, gpointer user_data) {
  GtkTreeView *tv;
  GtkTreeModel *tm;
  GtkTreeSelection *sel;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint id, pos, num;
  GList *selected_rows;

  tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist"));
  tm = gtk_tree_view_get_model(tv);

  if(!gtk_tree_model_iter_n_children(tm, NULL)) {
    debug("Nothing loaded in playlist, cannot play");
    statusbar_print("Playlist empty");
    return FALSE;
  }

  if( PAUSED ) 
    /* prone to race condition... should be made better. */
    player_pause(); 
  else {

    sel = gtk_tree_view_get_selection(tv);

    if( !(num = gtk_tree_selection_count_selected_rows(sel)) ) {
      debug("No song selected, letting MPD decide which song to play");
      if( !STOPPED ) player_stop();
      player_play_song(-1);
      return FALSE;
    }

    if( !(selected_rows = gtk_tree_selection_get_selected_rows(sel, &tm)) ) {
      debug("Couldn't get selected rows");
      return FALSE;
    }

    path = (GtkTreePath *)g_list_nth_data(selected_rows, 0);

    if ( !gtk_tree_model_get_iter(tm, &iter, path) ) {
      debug("Couldn't get GtkTreeIter, what now?");
      return FALSE;
    }

    debug("Getting trackno. of selected song");
    gtk_tree_model_get(tm, &iter, PL_ID, &id, PL_POS, &pos, -1);
    debug("Song number is %d, id %d", pos, id);

    if( num > 1 ) gui_playlist_set_cursor(pos - 1);

    if( !STOPPED ) player_stop();
    player_play_song(id);
  }

  return FALSE;
}

gboolean on_ui_player_pause(GtkWidget *widget, gpointer user_data) {
  player_pause();

  return FALSE;
}

gboolean on_ui_player_stop(GtkWidget *widget, gpointer user_data) {
  player_stop();

  return FALSE;
}

gboolean on_ui_player_prev(GtkWidget *widget, gpointer user_data) {
  player_prev();

  return FALSE;
}

gboolean on_ui_player_next(GtkWidget *widget, gpointer user_data) {
  player_next();

  return FALSE;
}
#if 0
gboolean on_ui_time_clicked(GtkWidget *widget, gpointer user_data) {
  if(glurp->config->time_display_left) glurp->config->time_display_left = FALSE;
  else glurp->config->time_display_left = TRUE;

  gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(builder, "glurp_functional_notebook1")));

  return FALSE;
}
#endif
gboolean on_ui_playlist_row_activated(GtkTreeView *treeview, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer user_data) {
  GtkTreeIter act;
  GtkTreeModel *model;
  gint id;

  debug("Playlist item activated.");

  if(!mpd_check_connected(glurp->mpd)) {
    debug("We're not connected, cannot start playing anything.");
    return FALSE;
  }

  model = gtk_tree_view_get_model(treeview);
  gtk_tree_model_get_iter(model, &act, tp);
  gtk_tree_model_get(model, &act, PL_ID, &id, -1);
  
  player_stop();
  player_play_song(id);

  return FALSE;
}

gboolean on_ui_playlist_list_row_activated(GtkTreeView *treeview, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer user_data) {
  GtkTreeIter act;
  GtkTreeModel *model;
  gchar *name;

  debug("playlist activated");

  if( !mpd_check_connected(glurp->mpd) )  {
    debug("we're not connected, how can this be?");
    return FALSE;
  }

  model = gtk_tree_view_get_model(treeview);
  gtk_tree_model_get_iter(model, &act, tp);
  gtk_tree_model_get(model, &act, 0, &name, -1);

  load_playlist(name);
  g_free(name);

  return FALSE;
}

gboolean on_ui_playlist_load(GtkWidget *widget, gpointer user_data) {
  GtkTreeView *tv;

  GtkTreeModel *tm;
  GtkTreeSelection *sel;
  GtkTreeIter iter;
  gchar *name;

  if( !mpd_check_connected(glurp->mpd) ) {
    debug("We're not connected, how can this be?");
    return FALSE;
  }

  tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist_list"));
  tm = gtk_tree_view_get_model(tv);

  sel = gtk_tree_view_get_selection(tv);
  if( !gtk_tree_selection_get_selected(sel, &tm, &iter)) {
    debug("No playlist selected");
    return FALSE;
  }

  debug("getting name of selected playlist");
  gtk_tree_model_get(tm, &iter, 0, &name, -1);

  load_playlist(name);
  g_free(name);

  return FALSE;
}

gboolean on_ui_playlist_delete(GtkWidget *widget, gpointer user_data) {
  GtkTreeView *tv;

  GtkTreeModel *tm;
  GtkTreeSelection *sel;
  GtkTreeIter iter;
  gchar *name;

  if( !mpd_check_connected(glurp->mpd) ) {
    debug("We're not connected, how can this be?");
    return FALSE;
  }

  tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist_list"));
  tm = gtk_tree_view_get_model(tv);

  sel = gtk_tree_view_get_selection(tv);
  if( !gtk_tree_selection_get_selected(sel, &tm, &iter)) {
    debug("No playlist selected");
    return FALSE;
  }

  gtk_tree_model_get(tm, &iter, 0, &name, -1);

  mpd_database_delete_playlist(glurp->mpd, name);

  debug("Playlist '%s' deleted", name);
  g_free(name);
  get_playlist_list();
  populate_gui_playlist_list();

  return FALSE;
}

gboolean on_ui_playlist_save(GtkWidget *widget, gpointer user_data) {
  GtkEntry *entry = GTK_ENTRY(gtk_builder_get_object(builder, "entry_playlist_name"));
  gchar *name = (gchar *)gtk_entry_get_text(entry);
  GlurpPl *pl;

  if( !mpd_check_connected(glurp->mpd) ) {
    debug("We're not connected, how can this be? Ignoring");
    return FALSE;
  }

  if( !name || !g_utf8_strlen(name, -1) ) {
    debug("Empty playlist name, ignoring");
    return FALSE;
  }

  for( pl = glurp->playlists; pl; pl = pl->next ) {
    if( !g_ascii_strcasecmp(pl->name, name) ) {
      debug("Not overwriting existing playlist");
      return FALSE;
    }
  }

  mpd_database_save_playlist(glurp->mpd, name);

  debug("Playlist '%s' saved", name);
  get_playlist_list();
  populate_gui_playlist_list();

  return FALSE;
}

gboolean on_ui_progress_drag(GtkWidget *widget, gpointer user_data) {
  double pos = gtk_range_get_value(GTK_RANGE(widget));
  double sec, tt;
  gint t_min, t_sec, s_min, s_sec;

  //if( !glurp->conn || !glurp->current_song ) return FALSE;

  if( !glurp->progress_dragging ) return FALSE;

  tt = glurp->current_song->time;

  t_min = (int)tt/60;
  t_sec = (int)tt%60;

  sec = pos;
  s_min = (int)sec/60;
  s_sec = (int)sec%60;

  statusbar_print("Seek to %02d:%02d/%02d:%02d", s_min, s_sec, t_min, t_sec);

  return FALSE;
}

gboolean on_ui_playlist_list_cursor_changed(GtkWidget *widget, gpointer user_data) {
  GtkTreeView *tv;
  GtkTreeModel *tm;
  GtkTreeSelection *sel;
  GtkTreeIter iter;
  gchar *name;

  if( !mpd_check_connected(glurp->mpd) ) {
    debug("We're not connected, how can this be?");
    return FALSE;
  }

  tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist_list"));
  tm = gtk_tree_view_get_model(tv);

  sel = gtk_tree_view_get_selection(tv);
  if( !gtk_tree_selection_get_selected(sel, &tm, &iter)) {
    debug("No playlist selected");
    return FALSE;
  }

  gtk_tree_model_get(tm, &iter, 0, &name, -1);

  gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(builder, "entry_playlist_name")), name);
  g_free(name);

  return FALSE;
}

gboolean on_ui_qsearch_activate(GtkWidget *widget, gpointer user_data)
{
  gchar *srch = (gchar *)gtk_entry_get_text(GTK_ENTRY(widget));
  gchar *sample = NULL;
  gint type = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "combobox_qsearch_type")));
  GtkTreePath *path;
  GtkTreeIter iter;
  GtkTreeView *view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist"));
  gint cpos, spos;
  gboolean found = FALSE;

  if( !srch || !g_utf8_strlen(srch, -1) )
    return FALSE;

  debug("QSearch string: '%s'", srch);

  gtk_tree_view_get_cursor(view, &path, NULL);
  if( path ) {
    /* Remember starting position */
    spos = cpos = atoi(gtk_tree_path_to_string(path));
    gtk_tree_path_free(path);
  } else {
    spos = cpos = 0;
  }

  debug("Cursor position: %d", cpos + 1);

  cpos++;

  while( !found && cpos != spos ) {
    if( !gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(glurp->gui_playlist), &iter, NULL, cpos) ) {
      cpos = 0;
      continue;
    }

    switch(type) {
      case GLURP_QSEARCH_TITLE:
        gtk_tree_model_get(GTK_TREE_MODEL(glurp->gui_playlist), &iter, PL_TITLE, &sample, -1);
        if( sample && srch && g_strrstr(g_ascii_strdown(sample, -1), g_ascii_strdown(srch, -1)) ) {
          debug("Found suitable haystack: '%s'", sample);
          gui_playlist_scroll(cpos);
          found = TRUE;
        }

        break;

      case GLURP_QSEARCH_ARTIST:
        gtk_tree_model_get(GTK_TREE_MODEL(glurp->gui_playlist), &iter, PL_TITLE, &sample, -1);
        if( sample && srch && g_strrstr(g_ascii_strdown(sample, -1), g_ascii_strdown(srch, -1)) ) {
          debug("Found suitable haystack: '%s'", sample);
          gui_playlist_scroll(cpos);
          found = TRUE;
        }

        break;

      case GLURP_QSEARCH_ALBUM:
        gtk_tree_model_get(GTK_TREE_MODEL(glurp->gui_playlist), &iter, PL_TITLE, &sample, -1);
        if( sample && srch && g_strrstr(g_ascii_strdown(sample, -1), g_ascii_strdown(srch, -1)) ) {
          debug("Found suitable haystack: '%s'", sample);
          gui_playlist_scroll(cpos);
          found = TRUE;
        }

        break;

      case GLURP_QSEARCH_FILENAME:
        gtk_tree_model_get(GTK_TREE_MODEL(glurp->gui_playlist), &iter, PL_TITLE, &sample, -1);
        if( sample && srch && g_strrstr(g_ascii_strdown(sample, -1), g_ascii_strdown(srch, -1)) ) {
          debug("Found suitable haystack: '%s'", sample);
          gui_playlist_scroll(cpos);
          found = TRUE;
        }

        break;

      case GLURP_QSEARCH_ALL:
        gtk_tree_model_get(GTK_TREE_MODEL(glurp->gui_playlist), &iter, PL_TITLE, &sample, -1);
        if( sample && srch && g_strrstr(g_ascii_strdown(sample, -1), g_ascii_strdown(srch, -1)) ) {
          debug("Found suitable haystack: '%s'", sample);
          gui_playlist_scroll(cpos);
          found = TRUE;
          break;
        }

        gtk_tree_model_get(GTK_TREE_MODEL(glurp->gui_playlist), &iter, PL_TITLE, &sample, -1);
        if( sample && srch && g_strrstr(g_ascii_strdown(sample, -1), g_ascii_strdown(srch, -1)) ) {
          debug("Found suitable haystack: '%s'", sample);
          gui_playlist_scroll(cpos);
          found = TRUE;
          break;
        }

        gtk_tree_model_get(GTK_TREE_MODEL(glurp->gui_playlist), &iter, PL_TITLE, &sample, -1);
        if( sample && srch && g_strrstr(g_ascii_strdown(sample, -1), g_ascii_strdown(srch, -1)) ) {
          debug("Found suitable haystack: '%s'", sample);
          gui_playlist_scroll(cpos);
          found = TRUE;
          break;
        }

        gtk_tree_model_get(GTK_TREE_MODEL(glurp->gui_playlist), &iter, PL_TITLE, &sample, -1);
        if( sample && srch && g_strrstr(g_ascii_strdown(sample, -1), g_ascii_strdown(srch, -1)) ) {
          debug("Found suitable haystack: '%s'", sample);
          gui_playlist_scroll(cpos);
          found = TRUE;
          break;
        }

        break;
        
    }

    cpos++;

  }

  if( !found ) debug("Nothing found...");

  return FALSE;
}

gboolean on_menu_add_activate(GtkWidget *widget, gpointer data) {
  return FALSE;
}

gboolean on_menu_pl_remove_all_activate(GtkWidget *widget, gpointer data) {
  if(!mpd_check_connected(glurp->mpd)) return FALSE;

  mpd_playlist_clear(glurp->mpd);
  return FALSE;
}

gboolean on_menu_pl_remove_selected_activate(GtkWidget *widget, gpointer data) {
  GtkWidget *tv;
  GtkTreeModel *tm;
  GtkTreeSelection *ts;
  gint num_sel, i, id;
  GtkTreeIter iter;
  GList *selected_rows;

  if(!mpd_check_connected(glurp->mpd)) return FALSE;

  tv = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_playlist"));
  tm = gtk_tree_view_get_model(GTK_TREE_VIEW(tv));
  ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));

  num_sel = gtk_tree_selection_count_selected_rows(ts);
  selected_rows = gtk_tree_selection_get_selected_rows(ts, NULL);

  debug("Selected %d rows", num_sel);

  if( num_sel ) {
    for( i=0; i<num_sel; i++ ) {
      gtk_tree_model_get_iter(tm, &iter, (GtkTreePath *)g_list_nth_data(selected_rows, i));
      gtk_tree_model_get(tm, &iter, PL_ID, &id, -1);
      mpd_playlist_queue_delete_id(glurp->mpd, id);
    }
    mpd_playlist_queue_commit(glurp->mpd);
  }
  return FALSE;
}

gboolean on_menu_pl_remove_crop_activate(GtkWidget *widget, gpointer data) {
  GtkWidget *tv;
  GtkTreeModel *tm;
  GtkTreeSelection *ts;
  GtkTreeIter iter;
  gint i = 0, id;

  if(!mpd_check_connected(glurp->mpd)) return FALSE;

  tv = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_playlist"));
  tm = gtk_tree_view_get_model(GTK_TREE_VIEW(tv));
  ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));

  if( !gtk_tree_model_get_iter_first(tm, &iter) ) {
    debug("Couldn't get first iter, playlist empty?");
    return FALSE;
  }

  do {
    if( !gtk_tree_selection_iter_is_selected(ts, &iter) ) {
      gtk_tree_model_get(tm, &iter, PL_ID, &id, -1);
      mpd_playlist_queue_delete_id( glurp->mpd, id);
    }
    else i++;
  } while( gtk_tree_model_iter_next(tm, &iter) );

  mpd_playlist_queue_commit(glurp->mpd);

  return FALSE;
}


gboolean on_ui_add_update_clicked(GtkWidget *widget, gpointer data) {

  gtk_tree_store_clear(glurp->gui_addtree);

  debug("Removing 'Add' treeview model");
//gtk_tree_view_set_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add")), NULL);

  gui_updating_disable_add_controls();
  glurp->updating_db = TRUE;
  statusbar_print("Updating MPD database, please wait...");
  debug("Starting to update MPD db...");

  
  mpd_database_update_dir(glurp->mpd, "/");
  mpd_check_error(glurp->mpd);
  
  return FALSE;
}

gboolean on_ui_add_add_clicked(GtkWidget *widget, gpointer data) {
  GtkTreeView *tv;
  GtkTreeModel *tm;
  GtkTreeSelection *ts;

  if(!mpd_check_connected(glurp->mpd)) return FALSE;

  tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add"));
  tm = gtk_tree_view_get_model(tv);
  if( !(ts = gtk_tree_view_get_selection(tv)) ) {
    debug("No selection, ignoring");
    return FALSE;
  }

  debug("Starting to add songs...");

  gui_load_selected();

  debug("Finished adding songs");

  return FALSE;
}

gboolean on_ui_repeat_clicked(GtkWidget *widget, gpointer user_data) {
  if( !mpd_check_connected(glurp->mpd) ) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
    return FALSE;
  }

  if( !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) {
    statusbar_print("Repeat Off");
    debug("Repeat Off");
    mpd_player_set_repeat(glurp->mpd, FALSE);

  } else {
    statusbar_print("Repeat On");
    debug("Repeat On");
    mpd_player_set_repeat(glurp->mpd, TRUE);
  }
  return FALSE;
}

gboolean on_ui_random_clicked(GtkWidget *widget, gpointer user_data) {
  if( !mpd_check_connected(glurp->mpd) ) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
    return FALSE;
  }

  if( !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) {
    statusbar_print("Random Off");
    debug("Random Off");
    mpd_player_set_random(glurp->mpd, FALSE);
  } else {
    statusbar_print("Random On");
    debug("Random On");
    mpd_player_set_random(glurp->mpd, TRUE);
  }
  return FALSE;
}


gboolean on_ui_add_row_expanded(GtkTreeView *tv, GtkTreeIter *iter, GtkTreePath *path, gpointer user_data) {
  gchar *the_path;
  gtk_tree_model_get(GTK_TREE_MODEL(glurp->gui_addtree), iter, ADD_NAME, &the_path,-1);
  debug("expanded path: %s", the_path);
  gui_add_fill_dir(iter, FALSE);

  return FALSE;
}

void on_ui_add_row_activated(GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data) {
  GtkTreeModel *tm = gtk_tree_view_get_model( tv );
  GtkTreeIter iter;

  if (!tm) {
    debug("Couldn't get the tree model for add tree");
    return;
  }
  
  if (!gtk_tree_model_get_iter(tm, &iter, path)) {
    debug("couldn't get a valid iter for the activated row (!)");
    return;
  }
  
  if( gtk_tree_model_iter_n_children(tm, &iter) > 0 ) {
    /* this is a directory, so we expand it. */
    debug("A directory is activated in the add window");
    if ( gtk_tree_view_row_expanded(tv, path) ) {
      gtk_tree_view_collapse_row(tv, path);
    } else {
      gtk_tree_view_expand_row(tv, path, FALSE);
    }
  } else {
    /* this is a song, add it to the playlist
     * fixme: should we add the activated song, or all selected songs? */
    debug("A song is activated in the add window");
    gui_load_song(tm, iter);
  }
}


gboolean on_ui_add_find_clicked( GtkWidget *widget, gpointer data) {
  
  if( !mpd_check_connected(glurp->mpd)) {
    debug("Not connected, ignoring.");
    statusbar_print("Not connected, cannot add");
    return FALSE;
  }
  debug("Search started");
  
  populate_gui_add_search_tree();
  gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(builder, "treeview_add")));
  return TRUE;
  
}

void on_entry_add_find_what_activate( GtkWidget *widget, gpointer data) {
  debug("relaying activation of the entry to the find button");
  g_signal_emit_by_name( (gpointer) gtk_builder_get_object(builder, "button_add_find"), "clicked");
}

gboolean on_ui_stream_add_clicked(GtkWidget *widget, gpointer user_data) {
  gchar *url = get_selected_stream();

  if( !url || !g_utf8_strlen(url, -1) ) {
    debug("No stream URL to add, ignoring");

    statusbar_print("No URL to add");

    return FALSE;
  }

  debug("Adding URL: '%s'", url);
  mpd_playlist_add(glurp->mpd, url);
  statusbar_print("URL '%s' added", url);
  push_stream(url);
  populate_stream_liststore();

  return FALSE;
}

gboolean on_ui_playlist_drag_begin(GtkWidget *widget, GdkDragContext *cont, gpointer user_data) {
  debug("DRAG BEGIN");
  return FALSE;
}

gboolean on_ui_playlist_drag_drop(GtkTreeView *tree, GdkDragContext *con, gint x, gint y, guint time, gpointer data) {
  GtkTreePath *path = NULL;
  GtkTreeViewDropPosition pos;
  GtkTreeModel *tm = GTK_TREE_MODEL(glurp->gui_playlist);
  GtkTreeIter iter;
  gint did, dpos, spos;
  gint num_selected;
  gint* path_indices;
  GtkTreeSelection *sel;
  GList *selected_rows, *sel_rows_iterator;

  debug("DRAG DROP");

  if( !gtk_tree_view_get_dest_row_at_pos(tree, x, y, &path, &pos) ) {
    debug("Can't determine destination");
    return TRUE;
  }

  if( !gtk_tree_model_get_iter(tm, &iter, path) ) {
    debug("Can't get iter");
    return TRUE;
  }

  gtk_tree_model_get(tm, &iter, PL_ID, &did, -1);
  path_indices = gtk_tree_path_get_indices(path);
  dpos = path_indices[gtk_tree_path_get_depth(path) - 1];

  if( pos == GTK_TREE_VIEW_DROP_AFTER ) debug("AFTER id:%d", did);
  if( pos == GTK_TREE_VIEW_DROP_BEFORE ) debug("BEFORE id:%d", did);
  if( pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE ) debug("INTO OR BEFORE id:%d", did);
  if( pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER ) debug("INTO OR AFTER id:%d", did);

  if( pos == GTK_TREE_VIEW_DROP_AFTER ) debug("AFTER pos:%d", dpos);
  if( pos == GTK_TREE_VIEW_DROP_BEFORE ) debug("BEFORE pos:%d", dpos);
  if( pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE ) debug("INTO OR BEFORE pos:%d", dpos);
  if( pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER ) debug("INTO OR AFTER pos:%d", dpos);
  if( ( pos == GTK_TREE_VIEW_DROP_BEFORE ) || ( pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE ) ) dpos -= 1;
//  if (dpos < 0) dpos = 0;

  sel = gtk_tree_view_get_selection(tree);
  sel_rows_iterator = selected_rows = gtk_tree_selection_get_selected_rows(sel, &tm);
  num_selected = gtk_tree_selection_count_selected_rows(sel);
  do {
    path = (GtkTreePath*)sel_rows_iterator->data;
    path_indices = gtk_tree_path_get_indices(path);
    spos = path_indices[gtk_tree_path_get_depth(path) - 1];
    if( spos >= dpos ) {
      dpos++;
      debug("dropping backwards");
    }
    debug("movig from pos %d to pos %d", spos, dpos);
    mpd_playlist_move_pos(glurp->mpd, spos, dpos);
  } while ( ( sel_rows_iterator = g_list_next(sel_rows_iterator) ) );
  
  g_list_foreach( selected_rows, (GFunc)gtk_tree_path_free, NULL);
  g_list_free( selected_rows );
  
  gtk_tree_selection_unselect_all(sel);
  
  return FALSE;
}

gboolean on_ui_playlist_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
  GtkTreePath *path = NULL;
  GtkTreeSelection *sel = NULL;
  GtkTreeView *tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_playlist"));

  if( event->button != 3 ) return FALSE;

  if( gtk_tree_view_get_path_at_pos(tv, (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL) ) {
    debug("[%d,%d]", (gint)event->x, (gint)event->y);
    sel = gtk_tree_view_get_selection(tv);
    if( !gtk_tree_selection_path_is_selected(sel, path) ) {
      debug("User clicked on unselected row, selecting ");
      gtk_tree_selection_unselect_all(sel);
      gtk_tree_selection_select_path(sel, path);
    }
  }

  debug("Displaying playlist popup menu");
  gtk_menu_popup(GTK_MENU(gtk_builder_get_object(builder, "glurp_menu_playlist")), NULL, NULL, NULL, NULL, event->button, event->time);

  return TRUE;
}

gboolean on_pmenu_playlist_play(GtkWidget *widget, gpointer data) {
  debug("POPUP: Playlist -> Play");
  on_ui_player_play(GTK_WIDGET(gtk_builder_get_object(builder, "button_play")), NULL);
  return FALSE;
}

gboolean on_pmenu_playlist_remove_selected(GtkWidget *widget, gpointer data) {
  debug("POPUP: Playlist -> Remove selected");
  on_menu_pl_remove_selected_activate(GTK_WIDGET(gtk_builder_get_object(builder, "button_play")), NULL);
  return FALSE;
}

gboolean on_pmenu_playlist_remove_crop(GtkWidget *widget, gpointer data) {
  debug("POPUP: Playlist -> Remove crop");
  on_menu_pl_remove_crop_activate(GTK_WIDGET(gtk_builder_get_object(builder, "button_play")), NULL);
  return FALSE;
}

gboolean on_pmenu_playlist_remove_all(GtkWidget *widget, gpointer data) {
  debug("POPUP: Playlist -> Remove all");
  on_menu_pl_remove_all_activate(GTK_WIDGET(gtk_builder_get_object(builder, "button_play")), NULL);
  return FALSE;
}

gboolean on_ui_add_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
  GtkTreePath *path = NULL;
  GtkTreeSelection *sel = NULL;
  GtkTreeView *tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add"));

  if( event->button != 3 ) return FALSE;

  if( gtk_tree_view_get_path_at_pos(tv, (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL) ) {
    debug("[%d,%d]", (gint)event->x, (gint)event->y);
    sel = gtk_tree_view_get_selection(tv);
    if( !gtk_tree_selection_path_is_selected(sel, path) ) {
      debug("User clicked on unselected row, selecting ");
      gtk_tree_selection_unselect_all(sel);
      gtk_tree_selection_select_path(sel, path);
    }
  }

  debug("Displaying add popup menu");
  gtk_menu_popup(GTK_MENU(gtk_builder_get_object(builder, "glurp_menu_db")), NULL, NULL, NULL, NULL, event->button, event->time);
  return TRUE;
}

gboolean on_pmenu_db_update_selected(GtkWidget *widget, gpointer data) {
  GtkTreeView *tv;
  GtkTreeModel *tm;
  GtkTreeSelection *ts;
  GList *selected_rows;
  GtkTreeIter iter;
  gint i, num = 0;
  gchar *path = NULL;

  debug("POPUP: DB -> Update selected");

  if(!mpd_check_connected(glurp->mpd)) return FALSE;
  tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add"));
  tm = gtk_tree_view_get_model(tv);
  if( !((ts = gtk_tree_view_get_selection(tv)) && (num = gtk_tree_selection_count_selected_rows(ts))) ) {
    debug("No selection, ignoring");
    return FALSE;
  }

  selected_rows = gtk_tree_selection_get_selected_rows(ts, NULL);

  debug("Removing 'Add' treeview model");
  gtk_tree_view_set_model(tv, NULL);

  gui_updating_disable_add_controls();
  glurp->updating_db = TRUE;

  for( i = 0; i < num; i++ ) {
    gtk_tree_model_get_iter(tm, &iter, (GtkTreePath *)g_list_nth_data(selected_rows, i));
    gtk_tree_model_get(tm, &iter, 1, &path, -1);
    mpd_database_update_dir(glurp->mpd, path);
    debug("**** Updating '%s'", path);
    g_free(path);
    if (mpd_check_error(glurp->mpd)) break;
  }

  return FALSE;
}

gboolean on_pmenu_db_add_selected(GtkWidget *widget, gpointer data) {
  debug("POPUP: DB -> Add selected");
  on_ui_add_add_clicked(GTK_WIDGET(gtk_builder_get_object(builder, "button_add_add")), NULL);
  return FALSE;
}

gboolean on_pmenu_db_info(GtkWidget *widget, gpointer data) {
  debug("POPUP: DB -> Database information (STUB)");
  return FALSE;
}

gboolean on_pmenu_playlist_shuffle_activate(GtkWidget *widget, gpointer data) {
  if( !mpd_check_connected(glurp->mpd) ) {
    debug("Not connected");
    return FALSE;
  }

  mpd_playlist_shuffle(glurp->mpd);

  statusbar_print("Playlist shuffled.");

  return FALSE;
}

gboolean on_togglebutton_pl_remove_toggled(GtkWidget *widget, gpointer data) {
  if( !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) return FALSE;

  debug("Remove button clicked");
  gtk_menu_popup(GTK_MENU(gtk_builder_get_object(builder, "glurp_menu_pl_remove")), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
  return FALSE;
}

gboolean on_menu_pl_remove_deactivate(GtkWidget *widget, gpointer data) {
  debug("Remove menu hidden, deactivating togglebutton");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "togglebutton_pl_remove")), FALSE);
  return FALSE;
}

gboolean on_outputs_toggled(GtkWidget *widget, gpointer data) {
  GtkMenu *menu = populate_outputs_menu();

  if( !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) return FALSE;

  debug("Outputs button clicked");

  if( !menu ) return FALSE;

  gtk_menu_popup(populate_outputs_menu(), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
  return FALSE;
}

gboolean on_menu_outputs_deactivate(GtkWidget *widget, gpointer data) {
  debug("Outputs menu hidden, deactivating togglebutton");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "togglebutton_outputs")), FALSE);
  return FALSE;
}

gboolean on_menu_output_activate(GtkWidget *widget, gpointer data) {
  gboolean enable = FALSE;
  gint i, d = (gint)data;

  if( d < 0 ) enable = TRUE;
  i = abs(d) - 1;

  debug("%s output %d", (enable ? "Enable" : "Disable"), i);

  statusbar_print("%s output %d", (enable ? "Enabling" : "Disabling"), i);
  mpd_server_set_output_device(glurp->mpd, i, enable);

  return FALSE;
}

gboolean on_ui_streams_row_activated(GtkTreeView *treeview, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer user_data) {
  GtkTreeIter act;
  GtkTreeModel *model;
  gchar* url;
  GtkWidget* entry;

  model = gtk_tree_view_get_model(treeview);
  gtk_tree_model_get_iter(model, &act, tp);
  gtk_tree_model_get(model, &act, 0, &url, -1);
  entry = GTK_WIDGET(gtk_builder_get_object(builder, "streams_entry"));
  gtk_entry_set_text(GTK_ENTRY(entry), url);
  g_free(url);
  return FALSE;
}

gboolean on_ui_streams_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
  GtkTreePath *path = NULL;
  GtkTreeSelection *sel = NULL;
  GtkTreeView *tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "streams_treeview"));

  if( event->button != 3 ) return FALSE;

  if( gtk_tree_view_get_path_at_pos(tv, (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL) ) {
    debug("[%d,%d]", (gint)event->x, (gint)event->y);
    sel = gtk_tree_view_get_selection(tv);
    if( !gtk_tree_selection_path_is_selected(sel, path) ) {
      debug("User clicked on unselected row, selecting ");
      gtk_tree_selection_unselect_all(sel);
      gtk_tree_selection_select_path(sel, path);
    }
  }

  debug("Displaying stream popup menu");
  gtk_menu_popup(GTK_MENU(gtk_builder_get_object(builder, "glurp_menu_streams")), NULL, NULL, NULL, NULL, event->button, event->time);

  return TRUE;
}


gboolean on_menu_streams_remove_selected_activate(GtkWidget *widget, gpointer user_data) {
  GtkWidget *tv;
  GtkTreeModel *tm;
  GtkTreeSelection *ts;
  gint num_sel, i;
  gchar* url;
  GtkTreeIter iter;
  GList *selected_rows;

  if(!mpd_check_connected(glurp->mpd)) return FALSE;

  tv = GTK_WIDGET(gtk_builder_get_object(builder, "streams_treeview"));
  tm = gtk_tree_view_get_model(GTK_TREE_VIEW(tv));
  ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));

  num_sel = gtk_tree_selection_count_selected_rows(ts);
  selected_rows = gtk_tree_selection_get_selected_rows(ts, NULL);

  debug("Selected %d rows", num_sel);

  if( num_sel ) {
    for( i=0; i<num_sel; i++ ) {
      gtk_tree_model_get_iter(tm, &iter, (GtkTreePath *)g_list_nth_data(selected_rows, i));
      gtk_tree_model_get(tm, &iter, 0, &url, -1);
      pull_stream(url);
    }
  }
  populate_stream_liststore();
  return FALSE;
}

// vim: et sw=2 smarttab
