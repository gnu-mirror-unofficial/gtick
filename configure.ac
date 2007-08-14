#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_INIT(gtick)
AC_PREREQ(2.57)
AM_INIT_AUTOMAKE(gtick, "0.4.0", gtick-devel@gnu.org)
AC_CONFIG_SRCDIR([config.h.in])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

AM_PROG_LEX
if test "$LEX" != flex; then
  LEX="$SHELL $missing_dir/missing flex"
  AC_SUBST(LEX_OUTPUT_ROOT, lex.yy)
  AC_SUBST(LEXLIB, '')
fi
AC_PROG_YACC

# Checks for libraries.
AC_CHECK_LIB([m], [floor])
AC_CHECK_LIB([sndfile], [sf_open])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h libintl.h stdlib.h sys/ioctl.h unistd.h sys/time.h math.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_CHECK_FUNCS([floor strdup setlocale strtol])

PKG_CHECK_MODULES(DEPS, gtk+-2.0 gthread-2.0)
# samplerate

#AC_ARG_WITH([alsa],
#	    AS_HELP_STRING([--with-alsa],
#	                   [Use ALSA (in addition to OSS)]),
#	    [if test "$withval" = "yes" ; then
#	       PKG_CHECK_MODULES(ALSA, alsa,
#			         AC_DEFINE(WITH_ALSA, 1,
#					   [Alsa library selection]))
#	     fi])

AC_ARG_WITH([sndfile],
	    AS_HELP_STRING([--with-sndfile],
			   [Use libsndfile]),
	    [if test "$withval" = "yes" ; then
	       PKG_CHECK_MODULES(SNDFILE, sndfile >= 1,
			         AC_DEFINE(WITH_SNDFILE, 1,
					   [libsndfile selection]))
	     fi])


AC_MSG_CHECKING(to see if we can add '-Wall -W' to CFLAGS)
if test x$GCC != x ; then
  CFLAGS="$CFLAGS -D_U_=\"__attribute__((unused))\" -Wall -W -D_GNU_SOURCE -O3"
  AC_MSG_RESULT(yes)
else
  CFLAGS="-D_U_=\"\" $CFLAGS"
  AC_MSG_RESULT(no)
fi

# GNU gettext
AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION(0.16.1)

# directory containing configuration support files
# defaults to package root
#AC_CONFIG_AUX_DIR

AC_CONFIG_FILES([Makefile
		 doc/Makefile
		 intl/Makefile
		 po/Makefile.in
		 src/Makefile])
AC_OUTPUT
