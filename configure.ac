#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_INIT([gtick], [0.5.3])
AC_PREREQ(2.57)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([config.h.in])
AM_CONFIG_HEADER([config.h])

AC_GNU_SOURCE

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
AC_CHECK_LIB([check], [fail_if])
AC_CHECK_LIB([dmalloc], [dmalloc_debug])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h libintl.h stdlib.h sys/ioctl.h unistd.h sys/time.h math.h sys/types.h stdarg.h assert.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_CHECK_FUNCS([floor strdup setlocale strtol])

PKG_CHECK_MODULES(DEPS, gtk+-2.0 gthread-2.0 libpulse-simple)
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
  CFLAGS="$CFLAGS -D_U_=\"__attribute__((unused))\" -Wall -W -D_GNU_SOURCE"
  AC_MSG_RESULT(yes)
else
  CFLAGS="-D_U_=\"\" $CFLAGS"
  AC_MSG_RESULT(no)
fi

# GNU gettext
AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION(0.18.1)

# directory containing configuration support files
# defaults to package root
#AC_CONFIG_AUX_DIR

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4],
		  AC_DEFINE(WITH_CHECK, 1, [Check detection]),
		  AC_DEFINE(WITH_CHECK, 0, [Check detection]))

AC_CHECK_HEADERS([dmalloc.h],
		 AC_DEFINE(DMALLOC_LIBS, "-ldmalloc", "Dmalloc LDFLAGS"),
                 [],
		 [
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
		])
AC_SUBST(DMALLOC_CFLAGS)
AC_SUBST(DMALLOC_LIBS)

AC_CONFIG_FILES([Makefile
		 doc/Makefile
		 intl/Makefile
		 po/Makefile.in
		 src/Makefile
		 testsuite/Makefile])
AC_OUTPUT
