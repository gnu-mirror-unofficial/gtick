## Process this file with automake to produce Makefile.in

check_PROGRAMS = testdsp \
		 testg711 \
		 testmetro \
		 testmetro-static

TESTS=$(check_PROGRAMS)

testdsp_SOURCES = testdsp.c \
		  ../src/dsp.c \
		  ../src/g711.c \
		  ../src/util.c \
		  ../src/threadtalk.c \
		  common.c

testg711_SOURCES = testg711.c \
		  ../src/g711.c \
		  common.c

testmetro_SOURCES = testmetro.c \
		  ../src/metro.c \
		  ../src/g711.c \
		  ../src/visualtick.c \
		  ../src/threadtalk.c \
		  ../src/util.c \
		  ../src/option.c \
		  ../src/options.c \
		  ../src/gtkoptions.c \
		  ../src/dsp.c \
		  ../src/help.c \
		  ../src/gtkutil.c \
		  ../src/profiles.c \
		  ../src/optionlexer.l \
		  ../src/optionparser.y \
		  common.c

testmetro_static_SOURCES = testmetro-static.c \
		  ../src/g711.c \
		  ../src/visualtick.c \
		  ../src/threadtalk.c \
		  ../src/util.c \
		  ../src/option.c \
		  ../src/options.c \
		  ../src/gtkoptions.c \
		  ../src/dsp.c \
		  ../src/help.c \
		  ../src/gtkutil.c \
		  ../src/profiles.c \
		  ../src/optionlexer.l \
		  ../src/optionparser.y \
		  common.c

#testdsp_
LDADD = @DEPS_LIBS@ @SNDFILE_LIBS@ @CHECK_LIBS@ @DMALLOC_LIBS@

noinst_HEADERS = common.h

top_srcdir = @top_srcdir@
datadir = @datadir@
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
LIBS = @LIBINTL@ @LIBS@

INCLUDES = -I../src -I../intl -I$(top_srcdir)/intl @DEPS_CFLAGS@ @SNDFILE_CFLAGS@ @CHECK_CFLAGS@ @DMALLOC_CFLAGS@
AM_CFLAGS = -DVERSION='"@VERSION@"' -DPACKAGE='"@PACKAGE@"' -DUSE_DMALLOC @CFLAGS@

AM_YFLAGS = -d
AM_LFLAGS=-olex.yy.c
BUILT_SOURCES = ../src/optionparser.h

