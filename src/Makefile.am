## Process this file with automake to produce Makefile.in

bin_PROGRAMS = gtick

gtick_SOURCES = gtick.c \
		metro.c \
		dsp.c \
		help.c \
		g711.c \
		gtkutil.c \
		util.c \
		option.c \
		options.c \
		gtkoptions.c \
		optionlexer.l \
		optionparser.y \
		profiles.c \
		threadtalk.c \
		visualtick.c
gtick_LDADD = @DEPS_LIBS@ @SNDFILE_LIBS@

noinst_HEADERS = metro.h \
		 dsp.h \
		 help.h \
		 tickdata.c \
		 globals.h \
		 gettext.h \
		 g711.h \
		 gtkutil.h \
		 util.h \
		 option.h \
		 options.h \
		 gtkoptions.h \
		 optionlexer.h \
		 profiles.h \
		 threadtalk.h \
		 visualtick.h

EXTRA_DIST = aboutlogo.xpm \
	     icon32x32.xpm \
	     icon48x48.xpm \
	     icon64x64.xpm \
	     converter.c

top_srcdir = @top_srcdir@
datadir = @datadir@
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
LIBS = @LIBINTL@ @LIBS@

INCLUDES = -I../intl -I$(top_srcdir)/intl @DEPS_CFLAGS@ @SNDFILE_CFLAGS@
AM_CFLAGS = -DVERSION='"@VERSION@"' -DPACKAGE='"@PACKAGE@"'

AM_YFLAGS = -d
AM_LFLAGS=-olex.yy.c
BUILT_SOURCES = optionparser.h
