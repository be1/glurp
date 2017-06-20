# Glurp
An _old_ gtk+ 2.x MPD client

## Prerequisites
You need autotools, pkg-config, and headers and libs for gtk+-2.0 (>= 2.4) glib-2.0 (>= 2.4) gmodule-2.0 (>= 2.6) libmpd (>= 0.0.9.8)

## Build on *nix
Use __autoreconf -i__ to generate configure script and install build helpers.
Use __./configure && make__ to build glurp.
As root, use __make install__ to properly install gluid.

## Uninstall
In the source tree, as root, use __make uninstall__.
