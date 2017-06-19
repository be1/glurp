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

#include <gtk/gtk.h>

#include "structs.h"
#include "support.h"
#include "player.h"
#include "comm.h"

extern GlurpState *glurp;

void player_play_song(gint id) {

  debug("PLAY requested");
  mpd_player_play_id(glurp->mpd, id);
  statusbar_print("Starting playback");

  return;
}

void player_pause() {

  if (mpd_player_get_state(glurp->mpd) == MPD_PLAYER_PAUSE)
    mpd_player_play(glurp->mpd);
  else
    mpd_player_pause(glurp->mpd);
  return;
}

void player_stop() {

  debug("STOP requested");
  mpd_player_stop(glurp->mpd);
  return;
}

void player_prev() {
  mpd_player_prev(glurp->mpd);
  return;
}

void player_next() {
  mpd_player_next(glurp->mpd);
  return;
}
// vim: et sw=2 smarttab
