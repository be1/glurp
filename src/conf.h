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

#ifndef __CONF_H
#define __CONF_H

#define GLURP_CONFIG_FILE  ".glurp"

#define NUM_SETTINGS    50
#define MPD_PORT    6600
#define MIN_REFRESH_RATE  100
#define MAX_REFRESH_RATE  10000

GlurpConfig *config_init();
gboolean config_load();
void config_load_item( const gchar *key, const gchar *value);
void config_save();
void config_set_defaults();
void config_columns_set_remaining();

#define yesno(i)  (i ? TRUE : FALSE)
#define zeroone(expr)  (expr ? 1 : 0)

#endif /* __CONF_H */
// vim: et sw=2 smarttab
