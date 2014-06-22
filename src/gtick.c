/*
 * gnu gtick  -  a gtk+ metronome
 *
 * gtick.c: main app code
 * 
 *
 * copyright (c) 1999 alex roberts
 * Copyright (c) 2003, 2004, 2005, 2006 Roland Stigge <stigge@antcom.de>
 *
 * GTick is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GTick is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GTick; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <config.h>

/* GNU headers */
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

/* GTK+ headers */
#include <gtk/gtk.h>
#include <glib.h>

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* own headers */
#include "globals.h"
#include "metro.h"
#include "dsp.h"
#include "util.h"

metro_t* metro;

int main(int argc, char *argv[])
{
  struct option long_options[] = {
    {"help",     no_argument,       0, 'h'},
    {"usage",    no_argument,       0, 'h'},
    {"version",  no_argument,       0, 'v'},
    {"debug",    optional_argument, 0, 'd'},
    {0, 0, 0, 0}
  };
  char *short_options = "hvd::";
  int option_index = 0;
  int c;

  /* prepare for i18n */
#ifdef ENABLE_NLS
  setlocale(LC_ALL, "");
  if (!bindtextdomain(PACKAGE, LOCALEDIR)) {
    fprintf(stderr, "Error setting directory for textdomain (i18n).\n");
  }
  if (!textdomain(PACKAGE)) {
    fprintf(stderr, "Error setting domainname for gettext() "
                    "(internationalization).\n");
  }
#endif

  /* Initialise GTK+ */
  gtk_init (&argc, &argv);

  while ((c = getopt_long(argc, argv, short_options, long_options,
			  &option_index)) != -1) {
    switch(c) {
    case 'h': /* help */
      printf(_("\
Usage: %s [OPTION...]\n\
\n\
Options:\n\
  -h, --help              Show this help message\n\
  -v, --version           Print version information\n\
  -d, --debug[=level]     Print additional runtime debugging data to stdout\n\
\n"),
      argv[0]);
      exit(0);
    case 'v': /* version */
      printf(PACKAGE " " VERSION "\n");
      exit(0);
    case 'd': /* debug mode */
      if (optarg) {
	debug = strtol(optarg, NULL, 0);
      } else {
	debug = 1;
      }
      break;
    case '?':
      exit(1);
    }
  }
  /* no further arguments expected, so not handled */

#ifdef ENABLE_NLS
  if (!bind_textdomain_codeset(PACKAGE, "UTF-8")) { /* needed for GTK */
    fprintf(stderr, "Error setting gettext output codeset to UTF-8.\n");
  }
#endif

  /* Initialise UI */
  metro = metro_new();
  
  /* GTK+ internal main loop */
  gtk_main();

  metro_delete(metro);
  return 0;
}

