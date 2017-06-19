## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DDATA_DIR=\""$(datadir)/glurp/"\" \
	@PACKAGE_CFLAGS@ @EXTRA_CFLAGS@ -Wall

bin_PROGRAMS = glurp

glurp_SOURCES = \
	conf.h conf.c \
	mpd-callbacks.h mpd-callbacks.c \
	gui.h gui.c \
	comm.h comm.c \
	gui-callbacks.h gui-callbacks.c \
	player.h player.c \
	keyboard.c \
	main.c \
	structs.h \
	support.h support.c \
  trayicon.h trayicon.c \
  traymenu.h traymenu.c

glurp_LDFLAGS = -export-dynamic @EXTRA_LDFLAGS@

glurp_LDADD = @PACKAGE_LIBS@
