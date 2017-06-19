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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libmpd/libmpd-strfsong.h>

#include "structs.h"
#include "support.h"
#include "comm.h"
#include "gui.h"
#include "gui-callbacks.h"

#include <libmpd/libmpd-player.h>
extern GtkBuilder *builder;
extern GlurpState *glurp;

void debug_real (const char *file, const int line, const char *function, const char *msg, ...) {

  char str[4097];
  va_list va;

  va_start(va, msg);
  g_vsnprintf(str, sizeof(str)-1, msg, va);
  fprintf(stderr, "%s:%d, %s(): %s\n", file, line, function, str);
  va_end(va);
  fflush(stderr);

}

void statusbar_print(const char *string, ...) {
  char str[129];
  va_list va;
  GtkWidget *sb = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar_main"));

  va_start(va, string);
  g_vsnprintf(str, sizeof(str)-1, string, va);

  gtk_statusbar_push(GTK_STATUSBAR(sb), 0, str);

  glurp->statusbar_status++;

  g_timeout_add(2000, statusbar_reset, (gpointer)&glurp->statusbar_status);
}

gboolean statusbar_reset(gpointer msg) {
  GtkWidget *sb;

  if( glurp->progress_dragging ) return FALSE;

  if( *(gint*)msg == glurp->statusbar_status ) {
    sb = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar_main"));
    gtk_statusbar_push(GTK_STATUSBAR(sb), 0, status_string());
    glurp->statusbar_status = 0;
  }

  return FALSE;
}

void title_print(GtkWidget *window, const char *string, ...) {
  char str[129-strlen(GLURP_TITLE_PREFIX)], *str2;
  va_list va;

  if( !string ) string = "";

  va_start(va, string);
  g_vsnprintf(str, sizeof(str)-1, string, va);

  if(!string || !strlen(str)) str2 = g_strdup_printf("%s", GLURP_TITLE_PREFIX);
  else str2 = g_strdup_printf("%s :: %s", GLURP_TITLE_PREFIX, str);

  gtk_window_set_title(GTK_WINDOW(window), str2);
  g_free(str2);
}

gchar *status_string() {
  MpdState state = mpd_player_get_state(glurp->mpd);

  if(!mpd_check_connected(glurp->mpd)) return "Not connected to server";

  if (glurp->updating_db) {
    if(state == MPD_PLAYER_PLAY)  return "Connected to server, Updating DB, Playing";
    if(state == MPD_PLAYER_PAUSE) return "Connected to server, Updating DB, Paused";
    if(state == MPD_PLAYER_STOP)  return "Connected to server, Updating DB, Stopped";
  } else {
    if(state == MPD_PLAYER_PLAY)  return "Connected to server, Playing";
    if(state == MPD_PLAYER_PAUSE) return "Connected to server, Paused";
    if(state == MPD_PLAYER_STOP)  return "Connected to server, Stopped";
  }
  return "Connected to server, unknown state";
}

/* this will return a pointer to the first character after last '/' in path */
gchar *strip_dirs(gchar *path) {
  gchar *c = path, *s = path;

  while( c && *c != '\0' ) {
    if( *c == '/' ) s = c;
    c++;
  }
  return s+1;
}

GtkWidget* get_image_from_name(gchar* name) {
  gchar* path;
  GtkWidget* retval;
#ifdef WIN32
  gchar *packagedir;
  packagedir = g_win32_get_package_installation_directory("glurp",NULL);
  debug("Got %s as package installation dir", packagedir);
  path = g_build_filename(packagedir,"data",name,NULL);
  g_free(packagedir);
#else
  path = g_strdup_printf(DATA_DIR"pixmaps/%s", name);
#endif
  debug("Using %s for image %s",path, name);
  retval = gtk_image_new_from_file(path);
  g_free(path);
  return retval;
}

GdkPixbuf *get_pixbuf_from_name(gchar* name) {
  gchar *path;
  GdkPixbuf *retval;
#ifdef WIN32
  gchar *packagedir;
  packagedir = g_win32_get_package_installation_directory("glurp",NULL);
  debug("Got %s as package installation dir", packagedir);
  path = g_build_filename(packagedir,"data",name,NULL);
  g_free(packagedir);
#else
  path = g_strdup_printf(DATA_DIR"pixmaps/%s", name);
#endif
  debug("Using %s for image %s",path, name);
  retval = gdk_pixbuf_new_from_file(path,NULL);
  g_free(path);
  return retval;
}

 
gchar *builder_path() {
  gchar *path;
#ifdef WIN32
  gchar *packagedir;
  packagedir = g_win32_get_package_installation_directory_utf8("glurp",NULL);
  debug("Got %s as package installation dir", packagedir);
  
  path = g_build_filename(packagedir,"data", "glurp.glade",NULL);
#else
  if (g_file_test("./glurp.glade", G_FILE_TEST_EXISTS)) return g_strdup("./glurp.glade");
  path = g_strdup_printf(DATA_DIR"glurp.glade");
#endif
  
  if(!g_file_test(path, G_FILE_TEST_EXISTS)) {
    debug("file '%s' does not exist", path);
    g_free(path);
    path = g_strdup("glurp.glade");
    if(!g_file_test(path, G_FILE_TEST_EXISTS)) {
      g_free(path);
      return NULL;
    } else {
      debug("found file '%s'", path);
      return path;
    }
  } else {
    debug("found file '%s'", path);
    return path;
  }
}


gchar *glurp_filename(gchar *path) {
  gchar **dirs = g_strsplit(path, "/", 0);
  gint i = 0;

  while( dirs[i+1] ) i++;
  return dirs[i];
}

void add_song(mpd_Song *song) {
  gint curlast = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(glurp->gui_playlist), NULL) - 1;
  gint needed = song->pos - curlast ;
  GtkTreeIter iter;
  gchar title[1024];
    
  
  if (needed>0) {
    for (;needed > 0; needed--) {
      /* need to add some positions in the liststore */
      gtk_list_store_append(glurp->gui_playlist, &iter);
    }
  } else {
    gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(glurp->gui_playlist), &iter, NULL, song->pos);
  }

  mpd_song_markup(title, 1023, "[%artist% - %title%]|%name%|%file%", song);

  gtk_list_store_set(glurp->gui_playlist, &iter,
    PL_POS, song->pos + 1,
    PL_TITLE, title,
    PL_ID, song->id,
    -1);
}

void debug_print_playlist() {
  GtkTreeIter iter;
  gchar* title;
  gint id;
  gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(glurp->gui_playlist), &iter, NULL, 0);
  debug("-----------------------------------------------");
  do {
    gtk_tree_model_get(GTK_TREE_MODEL(glurp->gui_playlist), &iter, PL_ID, &id, PL_TITLE, &title, -1);
    debug("(%d) %s", id, title);
  } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(glurp->gui_playlist), &iter));
  debug("-----------------------------------------------");
}

GlurpStream *get_stream_history(gchar *urls) {
  gchar **streamh = NULL;
  gint i;
  GlurpStream *s = NULL, *os = NULL, *ss = NULL;

  if( !urls || !strlen(urls) ) {
    debug("No stream history");
    return NULL;
  }

  streamh = g_strsplit(urls, " ", 0);

  for( i = 0; streamh[i] && i < MAX_STREAM_HISTORY; i++ ) {
    s = malloc(sizeof(GlurpStream));
    s->url = g_strdup(streamh[i]);
    s->next = NULL;
    if( !i ) ss = s;
    if( os ) os->next = s;
    os = s;
  }

  g_strfreev(streamh);

  return ss;
}

void print_stream_history() {
  GlurpStream *s;

  for( s = glurp->stream_history; s; s = s->next ) debug("<%s>", s->url);
}

GlurpStream *get_stream_by_url(gchar *url) {
  GlurpStream *s = NULL;

  for( s = glurp->stream_history; s; s = s->next )
    if( !strcmp(url, s->url) ) return s;

  return NULL;
}

gchar *dump_stream_history() {
  GlurpStream *s;
  gchar *h = g_strdup("");

  for( s = glurp->stream_history; s; s = s->next ) h = g_strconcat(h, (strlen(h) ? " " : ""), s->url, NULL);
  return h;
}

void prepend_stream(gchar *url) {
  GlurpStream *s = NULL;
  gint i = 0;

  debug("Prepending '%s'", url);
  if( !(s = malloc(sizeof(GlurpStream))) ) {
    debug("Couldn't allocate memory for stream history item");
    return;
  }
  s->url = g_strdup(url);
  s->next = NULL;

  if( !glurp->stream_history ) {
    debug("First stream history item");
    glurp->stream_history = s;
    return;
  }

  debug("Not a first item");
  s->next = glurp->stream_history;
  glurp->stream_history = s;

  while( i < (MAX_STREAM_HISTORY - 1) && s ) {
    s = s->next;
    i++;
  }

  if( s && s->next ) {
    g_free(s->next);
    s->next = NULL;
  }
}

void push_stream(gchar *url) {
  GlurpStream *s = NULL;

  if( !(s = get_stream_by_url(url)) )
    prepend_stream(url);
//  else lift_stream(s);
}

void pull_stream(gchar* url) {
  GlurpStream *s = NULL;
  GlurpStream *p = NULL;

  if ( !strcmp(url, glurp->stream_history->url) ) {
    s = glurp->stream_history;
    glurp->stream_history = s->next;
  } else {
    for( p = glurp->stream_history, s = glurp->stream_history->next; s; p = p->next, s = s->next ) {
      if( !strcmp(url, s->url) )
        break;
    }
    p->next = s->next;
  }
  if (s) {
    g_free(s->url);
    g_free(s);
  }
}

void sigint_handler(int s) {
  debug("SIGINT received, exiting gracefully");
  on_ui_quit(NULL, NULL);
}

char *remove_extension_and_basepath(const char *filename)
{
  int i = 0;
  char *buf = NULL;
  if(filename == NULL)
  {
    return NULL;
  }
  buf  = g_path_get_basename(filename);

  if(buf != NULL)
  {
    /* replace _ with spaces */
    for(i=0; i< strlen(buf);i++)
    {
      if(buf[i] == '_') buf[i] = ' ';
    }
    for(i=strlen(buf);buf[i] != '.';i--);
    /* cut of the extention */
    if(i > 0)
    {
      gchar *buf2 = g_strndup(buf, i);
      g_free(buf);
      return buf2;
    }
  }
  return buf;
}
// vim: et sw=2 smarttab
