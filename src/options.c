/* 
 * Metronome options object functions
 *
 * This file is part of GTick
 * 
 *
 * Copyright (c) 1999, Alex Roberts
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GTK+ headers */
#include <glib.h>

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* own headers */
#include "globals.h"
#include "options.h"
#include "option.h"

/*
 * returns a new options_t object
 */
options_t* options_new(void) {
  options_t* result;
    
  result = g_malloc0(sizeof(options_t));
  result->option_list = option_list_new();

  return result;
}

/*
 * destroys an options_t object
 */
void options_delete(options_t* options) {
  option_list_delete(options->option_list);
  free(options);
}

