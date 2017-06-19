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
#include <stdio.h>
#include <getopt.h>
#include <signal.h>

#include "structs.h"
#include "support.h"
#include "gui.h"
#include "comm.h"
#include "conf.h"
#include "gui-callbacks.h"
#include "mpd-callbacks.h"
#include "trayicon.h"

GtkBuilder* builder = NULL;

GlurpState *glurp = NULL;

int main (int argc, char *argv[])
{
  gint option_index, c;

  static struct option long_options[] = {
    { "version", 0, NULL, 0 },
    { "help", 0, NULL, 0 },
    { "config", 1, NULL, 0 },
    { 0, 0, NULL, 0 }
  };
  
  /* NLS not yet implemented */
#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  gtk_set_locale ();
  gtk_init (&argc, &argv);
  
  while (1) {
    option_index = 0;
    if(  (c = getopt_long(argc, argv, "vh", long_options, &option_index)) == -1)
      break;

    if( option_index == 1 || c == 'h' ) { /* help */
      printf("Usage: %s [-v|--version] [-h|--help] [--config=...]\n", argv[0]);
      return 0;
    }
    if( option_index == 0 || c == 'v' ) { /* version */
      printf("%s version %s\n", argv[0], GLURP_VERSION);
      return 0;
    }

    if( option_index == 2 ) { /* config */
      if (optarg) {
        glurp = g_new0(GlurpState,1);
        glurp->alternate_config_file = g_strdup(optarg);
      }
      continue;
    }
    printf("Usage: %s [-v|--version] [-h|--help] [--config=...]\n", argv[0]);
    return -1;
  }
  


  if (!glurp) glurp = g_new0(GlurpState, 1);
     
  glurp->progress_dragging = FALSE;
  glurp->refresh_rate_status = 0;

  glurp->mpd = NULL;
  glurp->playlist_version = 0;

  glurp->current_song = NULL;

  glurp->gui_playlist = NULL;
  glurp->gui_playlist_list = NULL;
  glurp->gui_stream_list = NULL;
  glurp->gui_addtree = NULL;

  glurp->playlists = NULL;
  
  glurp->statusbar_status = 0;
  glurp->play_state = MPD_STATUS_STATE_STOP;
  glurp->prev_song_num = -1;
  glurp->scroll = 0;

  glurp->num_add_dirs = 0;

  glurp->updating_db = FALSE;

  glurp->stream_history = NULL;
  glurp->trayicon = NULL;
  glurp->traymenu = NULL;
  glurp->window = NULL;

  if( !config_load()) {
    debug("Couldn't read config file, aborting. Goodnight!");
    return 0;
  }

  if( !(glurp_init_gui()) ) g_error("Can't initialize GUI, exiting...\n");
  
  glurp->mpd = mpd_new(glurp->config->server_host, glurp->config->server_port, glurp->config->server_pass);
  /* add the callbacks now; we'll need them when we connect...
   */
  mpd_signal_connect_connection_changed( glurp->mpd, mpd_connection_changed_callback, glurp);
  mpd_signal_connect_status_changed( glurp->mpd, mpd_status_changed_callback, glurp);
  mpd_signal_connect_error( glurp->mpd, mpd_error_callback, glurp);
  
  if(glurp->config->autoconnect) {
    debug("autoconnecting...");
    glurp_connect();
  } else {
    statusbar_print("Glurp v%s loaded...", GLURP_VERSION);
  }
  signal(SIGINT, sigint_handler);

  gtk_main();

  return 0;
}
// vim: et sw=2 smarttab
