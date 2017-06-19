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
#include <string.h>

#include "structs.h"
#include "support.h"
#include "player.h"
#include "gui.h"

extern GtkBuilder *builder;
extern GlurpState *glurp;

static void descend_ascend_helper(gboolean descend);

/* MAIN WINDOW SHORTCUTS */
gboolean on_main_key_pressed(GtkWidget *widget, GdkEventKey *key, gpointer data) {
  guint k = key->keyval;
  GtkWidget *w;
  const char *wname=NULL;

  if (!(key->state & (GDK_CONTROL_MASK|GDK_MOD1_MASK))) {
    w = gtk_window_get_focus(GTK_WINDOW(widget));
    if (w) {
      wname = gtk_widget_get_name(w);
      if( (GTK_IS_ENTRY(w)) && ( (wname==NULL) || (strcmp(wname,"entry_trackname")!=0) ) ) {
        debug("Passing keypress on towards the entry \"%s\" with focus", wname);
        return FALSE;
      }
    } 
  }

  if ( functional_notebook_database_selected() &&
      GTK_WIDGET_HAS_FOCUS(gtk_builder_get_object(builder, "treeview_add") ) ) {
    /* expand/move to first child */
    if( k == GDK_Right || k == GDK_KP_Right ) {
      descend_ascend_helper(TRUE);
      return TRUE;
    }
    /* collapse/move to parent */
    if( k == GDK_Left || k == GDK_KP_Left ) {
      descend_ascend_helper(FALSE);
      return TRUE;
    }
  }
  w = GTK_WIDGET(gtk_builder_get_object(builder, "progressbar"));
  if( !GTK_WIDGET_HAS_FOCUS(w) &&
      ( functional_notebook_playlist_selected() ) &&
      (k == GDK_Left || k == GDK_Right || k == GDK_KP_Left || k == GDK_KP_Right) &&
      !(key->state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD1_MASK)) )
    gtk_widget_grab_focus(w);
  
  if ( ( ( k == GDK_Page_Up ) || ( k == GDK_Page_Down ) ) && (key->state & GDK_CONTROL_MASK) && !(key->state & (GDK_SHIFT_MASK|GDK_MOD1_MASK))) {
    debug("moving to %s page in the functional notebook", (k == GDK_Page_Down)?"next":"previous");
    w = gtk_window_get_focus(GTK_WINDOW(widget));
    glurp_switch_functional_page(w, k == GDK_Page_Down);
    return TRUE;
  }

  /* previous */
  if(( k == GDK_Z || k == GDK_z )){
    g_signal_emit_by_name((gpointer)gtk_builder_get_object(builder, "button_prev"), "clicked");
    return TRUE;
  }

  /* play */
  if(( k == GDK_X || k == GDK_x )){
    g_signal_emit_by_name((gpointer)gtk_builder_get_object(builder, "button_play"), "clicked");
    return TRUE;
  }

  /* pause */
  if(( k == GDK_C || k == GDK_c )){
    g_signal_emit_by_name((gpointer)gtk_builder_get_object(builder, "button_pause"), "clicked");
    return TRUE;
  }

  /* stop */
  if(( k == GDK_V || k == GDK_v )){
    g_signal_emit_by_name((gpointer)gtk_builder_get_object(builder, "button_stop"), "clicked");
    return TRUE;
  }

  /* next */
  if(( k == GDK_B || k == GDK_b )){
    g_signal_emit_by_name((gpointer)gtk_builder_get_object(builder, "button_next"), "clicked");
    return TRUE;
  }

  /* playlist show/hide */
  if(( k == GDK_p )){
    w = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_playlist"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)) );

    // don't emit this signal, it's emitted by gtk_toggle_button_set_active
    //g_signal_emit_by_name((gpointer)gtk_builder_get_object(builder, "togglebutton_playlist"), "clicked");
    return TRUE;
  }

/* unimplemented idea??  
  if(( k == GDK_P ) && ( key->state & GDK_MOD1_MASK ) ){
    g_signal_emit_by_name((gpointer)gtk_builder_get_object(builder, "button_pl_shuffle"), "clicked");
    return TRUE;
  } */
    
  /* toggle repeat */
  if ( ( k == GDK_T || k == GDK_t ) ) {
    w = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_repeat"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)) );

    // don't emit this signal, it's emitted by gtk_toggle_button_set_active
    //g_signal_emit_by_name((gpointer)gtk_builder_get_object(builder, "togglebutton_repeat"), "clicked");
    return TRUE;
  }

  /* toggle random  */
  if(( k == GDK_M || k == GDK_m ) ){
    w = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_random"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)) );

    // don't emit this signal, it's emitted by gtk_toggle_button_set_active
    //g_signal_emit_by_name((gpointer)gtk_builder_get_object(builder, "togglebutton_random"), "clicked");
    return TRUE;
  }

  /* add file */
  if( (k == GDK_Insert || k == GDK_KP_Insert) && !(key->state & ( GDK_SHIFT_MASK || GDK_CONTROL_MASK || GDK_MOD1_MASK ) ) ) {
    functional_notebook_select_database();
    return TRUE;
  }
  
  if (functional_notebook_playlist_selected()) {
    /* remove selected */
    if( (k == GDK_Delete || k == GDK_KP_Delete) && !(key->state & GDK_SHIFT_MASK) && !(key->state & GDK_CONTROL_MASK) ) {
      g_signal_emit_by_name((gpointer)gtk_builder_get_object(builder, "menu_remove_selected"), "activate");
      return TRUE;
    }
  
    /* remove crop */
    if( (k == GDK_Delete || k == GDK_KP_Delete) && (key->state & GDK_SHIFT_MASK) && !(key->state & GDK_CONTROL_MASK) ) {
      g_signal_emit_by_name((gpointer)gtk_builder_get_object(builder, "menu_remove_crop"), "activate");
      return TRUE;
    }
  
    /* remove all */
    if( (k == GDK_Delete || k == GDK_KP_Delete) && !(key->state & GDK_SHIFT_MASK) && (key->state & GDK_CONTROL_MASK) ) {
      g_signal_emit_by_name((gpointer)gtk_builder_get_object(builder, "menu_remove_all"), "activate");
      return TRUE;
    }
  }


/*  debug("MAIN WINDOW KEYPRESS: %d", k); */

  return FALSE;
}

static void descend_ascend_helper(gboolean descend) {
  GtkTreeView *tv;
  GtkTreeModel *tm;
  GtkTreeSelection* sel;
  GtkTreePath *path;
  gint num;
  GList *selected_rows;
  tv = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_add"));
  tm = gtk_tree_view_get_model(tv);

  sel = gtk_tree_view_get_selection(tv);

  if( !(num = gtk_tree_selection_count_selected_rows(sel)) ) return;

  if( !(selected_rows = gtk_tree_selection_get_selected_rows(sel, &tm)) ) {
    debug("Couldn't get selected rows");
    return;
  }

  path = (GtkTreePath *)g_list_nth_data(selected_rows, 0);

  if( gtk_tree_view_row_expanded(tv, path) ) {
    if (descend) 
      gtk_tree_path_down(path);
    else
      gtk_tree_view_collapse_row(tv, path);
  } else {
    if (descend)
      gtk_tree_view_expand_row(tv, path, FALSE);
    else
      gtk_tree_path_up(path);
  }
  gtk_tree_view_set_cursor(tv, path, NULL, FALSE);
}
// vim: et sw=2 smarttab
