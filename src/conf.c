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
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#include "trayicon.h"
#include "structs.h"
#include "support.h"
#include "conf.h"

extern GlurpState *glurp;
extern GtkBuilder *builder;

const gboolean config_column_defaults[] = {
    FALSE,  /* filename */
    TRUE,  /* artist */
    TRUE,  /* title */
    FALSE,  /* album */
    FALSE,  /* trackno */
    FALSE,  /* name */
    TRUE,  /* pl. pos. */
    FALSE,  /* id */
};

gboolean config_column_ready[] = {
    FALSE,  /* filename */
    FALSE,  /* artist */
    FALSE,  /* title */
    FALSE,  /* album */
    FALSE,  /* trackno */
    FALSE,  /* name */
    FALSE,  /* pl. pos. */
    FALSE,  /* id */
};

GlurpConfig *config_init() {
  GlurpConfig *c = NULL;
  guint i;

  c = malloc(sizeof(GlurpConfig));

  c->server_host = g_strdup("localhost");
  c->server_port = MPD_PORT;
  c->server_pass = g_strdup("");
  c->autoconnect = FALSE;
  c->playlist_vis_on_start = FALSE;
  c->time_display_left = FALSE;
  c->refresh_rate = 500.0;
  c->pos_x = -11000;
  c->pos_y = -11000;
  c->width = -1;
  c->height = -1;
  c->save_size = FALSE;
  c->trayicon = FALSE;

  for( i = 0; i < PL_BOLD; i++ ) c->playlist_columns[i] = FALSE;

  return c;
}

gboolean config_load() {
  gchar *conf_str, *conf_path, **conf_items, **conf_item;
  GError *error = NULL;
  gint i = 0;

  glurp->config = config_init();
  if (glurp->alternate_config_file)
    conf_path = g_strdup(glurp->alternate_config_file);
  else
    conf_path = g_strdup_printf("%s/%s", g_strdup(g_get_home_dir()), GLURP_CONFIG_FILE);

  if( g_file_test(conf_path, G_FILE_TEST_EXISTS) ) {
    if( !g_file_test(conf_path, G_FILE_TEST_IS_REGULAR) ) {
      debug("Cannot open config file, it is not a regular file!");
      return FALSE;
    }
  }

  g_file_get_contents(conf_path, &conf_str, NULL, &error);

  if( conf_str ) {
    conf_items = g_strsplit(conf_str, "\n", NUM_SETTINGS);
  } else {
    debug("Config file empty, continuing.");
    config_set_defaults();

    return TRUE;
  }

  while(conf_items[i] && strlen(conf_items[i])) {
    conf_item = g_strsplit(conf_items[i], " = ", 2);
    if(conf_item[0] && conf_item[1] &&
       strlen(conf_item[0]) ) {
      config_load_item(g_strstrip(conf_item[0]), g_strstrip(conf_item[1]));
    } else debug("Invalid config item, ignoring.");

    i++;
  }

  return TRUE;
}

void config_load_item(const gchar *key, const gchar *value) {
  debug("%s = %s", key, value);

  if(!strcmp(key, "server")) {
    g_free(glurp->config->server_host);
    glurp->config->server_host = g_strdup(value);
    return;
  }
  if(!strcmp(key, "port")) {
    glurp->config->server_port = atoi(value);
    if( glurp->config->server_port < 1 || glurp->config->server_port > 65534 ) glurp->config->server_port = MPD_PORT;
    return;
  }
  if(!strcmp(key, "password")) {
    g_free(glurp->config->server_pass);
    glurp->config->server_pass = g_strdup(value);
    return;
  }
  if(!strcmp(key, "autoconnect")) {
    glurp->config->autoconnect = yesno(atoi(value));
    return;
  }

  if(!strcmp(key, "playlist-visible-on-start")) {
    glurp->config->playlist_vis_on_start = yesno(atoi(value));
    return;
  }

  if(!strcmp(key, "time-display-left")) {
    glurp->config->time_display_left = yesno(atoi(value));
    return;
  }

  if(!strcmp(key, "refresh-rate")) {
    glurp->config->refresh_rate = atof(value);
    if( glurp->config->refresh_rate < MIN_REFRESH_RATE ) glurp->config->refresh_rate = MIN_REFRESH_RATE;
    if( glurp->config->refresh_rate > MAX_REFRESH_RATE ) glurp->config->refresh_rate = MAX_REFRESH_RATE;
    return;
  }

  if(!strcmp(key, "stream-history")) {
    glurp->stream_history = get_stream_history((gchar *)value);
    return;
  }

  if(!strcmp(key, "vis-id")) {
    glurp->config->playlist_columns[PL_POS] = yesno(atoi(value));
    config_column_ready[PL_ID] = TRUE;
    return;
  }

  if(!strcmp(key, "window-x")) {
    glurp->config->pos_x = atoi(value);
    return;
  }

  if(!strcmp(key, "window-y")) {
    glurp->config->pos_y = atoi(value);
    return;
  }

  if(!strcmp(key, "width")) {
    glurp->config->width = atoi(value);
    return;
  }

  if(!strcmp(key, "height")) {
    glurp->config->height = atoi(value);
    return;
  }

  if(!strcmp(key, "save-size")) {
    glurp->config->save_size = yesno(atoi(value));
    return;
  }

  if(!strcmp(key, "trayicon")) {
    glurp->config->trayicon = yesno(atoi(value));
    return;
  }

  debug("Invalid config item, ignoring.");
  return;
}

void config_save() {
  gchar *conf_path = NULL, *stream_history = NULL;
  FILE *f;
  GtkWidget *w;
  gboolean vis;

  if (glurp->alternate_config_file)
    conf_path = g_strdup(glurp->alternate_config_file);
  else
    conf_path = g_strdup_printf("%s/%s", g_strdup(g_get_home_dir()), GLURP_CONFIG_FILE);

  if( g_file_test(conf_path, G_FILE_TEST_EXISTS) ) {
    if( !g_file_test(conf_path, G_FILE_TEST_IS_REGULAR) ) {
      debug("Cannot open config file!");
      return;
    }
  }

  if( !(f = fopen(conf_path, "w")) ) {
    debug("Could not write into config: %s", conf_path);
    statusbar_print("Could not write into config: %s", conf_path);
    return;
  }

  debug("Writing config file: %s", conf_path);

  if( glurp->config->server_host && strlen(glurp->config->server_host) ) {
    g_fprintf(f, "server = %s\n", glurp->config->server_host);
    debug("server = %s", glurp->config->server_host);
  }

  g_fprintf(f, "port = %d\n", glurp->config->server_port);
  debug("port = %d", glurp->config->server_port);

  if( glurp->config->server_pass && strlen(glurp->config->server_pass) ) {
    g_fprintf(f, "password = %s\n", glurp->config->server_pass);
    debug("password = %s", glurp->config->server_pass);
  }

  if( glurp->config->autoconnect ) {
    g_fprintf(f, "autoconnect = %d\n", yesno(glurp->config->autoconnect));
    debug("autoconnect = %d", yesno(glurp->config->autoconnect));
  }

  w = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton_playlist"));

  if( (vis = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) ) {
    debug("playlist visible");
    g_fprintf(f, "playlist-visible-on-start = %d\n", yesno(vis));
    debug("playlist-visible-on-start = %d", yesno(vis));
  } else
    debug("playlist not visible");

  if( glurp->config->time_display_left ) {
    g_fprintf(f, "time-display-left = %d\n", yesno(glurp->config->time_display_left));
    debug("time-display-left = %d", yesno(glurp->config->time_display_left));
  }

  g_fprintf(f, "refresh-rate = %f\n", glurp->config->refresh_rate);
  debug("refresh-rate = %f", glurp->config->refresh_rate);

  if( glurp->stream_history ) {
    stream_history = dump_stream_history();
    g_fprintf(f, "stream-history = %s\n", stream_history);
    debug("stream-history = %s", stream_history);
    g_free(stream_history);
  }

  if( glurp->config->pos_x != -11000 && glurp->config->pos_y != -11000 ) {
    g_fprintf(f, "window-x = %d\nwindow-y = %d\n", glurp->config->pos_x, glurp->config->pos_y);
    debug("window-x = %d", glurp->config->pos_x);
    debug("window-y = %d", glurp->config->pos_y);
  }


  if( glurp->config->save_size ) {
    g_fprintf(f, "save-size = %d\n", yesno(glurp->config->save_size));
    debug("save-size = %d", yesno(glurp->config->save_size));
  }

  if( glurp->config->width >= 0 && glurp->config->height >= 0 ) {
    g_fprintf(f, "width = %d\nheight = %d\n", glurp->config->width, glurp->config->height);
    debug("width = %d", glurp->config->width);
    debug("height = %d", glurp->config->height);
  }

  if( glurp->config->trayicon ) {
    g_fprintf(f, "trayicon = %d\n", yesno(glurp->config->trayicon));
    debug("trayicon = %d", yesno(glurp->config->trayicon));
  }

  debug("Config file saved");

  fclose(f);
}

void config_set_defaults() {
  if( glurp->config->server_host ) g_free(glurp->config->server_host);
  glurp->config->server_host = g_strdup("localhost");
}
// vim: et sw=2 smarttab
