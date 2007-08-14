/* 
 * Miscellaneous utility functions
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

#ifndef UTIL_H
#define UTIL_H

#include <config.h>

#ifdef HAVE_SYS_TIME_H
  #include <sys/time.h>
#endif

int timeval_subtract(struct timeval* result,
                     struct timeval* x, struct timeval* y);
char* stripchr(char* s, char c);
char* get_rc_filename(void);
void execute(char *command);

#endif /* UTIL_H */
