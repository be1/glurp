# glurp
An _old_ gtk+ 2.x MPD client

## prerequisites
You need autotools, pkg-config, and headers and libs for gtk+-2.0 (>= 2.4) glib-2.0 (>= 2.4) gmodule-2.0 (>= 2.6) libmpd (>= 0.0.9.8)

## build on *nix
Use autoreconf -i to generate configure script and install build helpers.
Use ./configure && make to build glurp.
As root, use make install to properly install gluid.

## uninstall
In the source tree, as root, use make uninstall.
