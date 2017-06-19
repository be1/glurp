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

#ifndef __PLAYER_H
#define __PLAYER_H

void player_play_song(gint id);
void player_pause();
void player_stop();
void player_prev();
void player_next();

#define STOPPED    (mpd_player_get_state(glurp->mpd) == MPD_PLAYER_STOP)
#define PLAYING    (mpd_player_get_state(glurp->mpd) == MPD_PLAYER_PLAY)
#define PAUSED     (mpd_player_get_state(glurp->mpd) == MPD_PLAYER_PAUSE)

#endif /* __PLAYER_H */
// vim: et sw=2 smarttab
