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

#include <config.h>

/* regular GNU system includes */
#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif
#ifdef HAVE_SYS_TIME_H
  #include <sys/time.h>
#endif
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

/* GTK+ headers */
#include <glib.h>

/* own headers */
#include "globals.h"
#include "util.h"

/*
 * Subtract the `struct timeval' values X and Y,
 * storing the result in RESULT.
 * Return 1 if the difference is negative, otherwise 0.
 *
 * This function comes from the GNU C Library documentation.
 */
int timeval_subtract(struct timeval* result,
                     struct timeval* x, struct timeval* y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

/*
 * returns a string where all occurences of the character c are removed
 * from the input string s
 * NOTE: caller has to free() the result
 */
char* stripchr(char* s, char c) {
  int count = 1; /* trailing space */
  char* temp;
  char* result;

  for (temp = s; *temp; temp++) {
    if (*temp != c)
      count++;
  }
  result = (char*) g_malloc (count);
  temp = result;
  while (*s) {
    if (*s != c) {
      *temp = *s;
      temp ++;
    }
    s++;
  }
  *temp = '\0';

  return result;
}

/*
 * returns name of rc file for this program, NULL on error
 *
 * NOTE: caller has to free the returned buffer himself
 */
char* get_rc_filename(void) {
  const char* homedir = g_get_home_dir();

  if (homedir == NULL) {
    return NULL;
  } else {
    return g_strdup_printf("%s/.%src", homedir, PACKAGE);
  }
}

/*
 * forks and executes given command (with arguments) via sh
 */
void execute(char *command) {
  pid_t result = fork();
  char *argv[] = {"sh", "-c", command, NULL};

  if (result == -1) { /* error */
    fprintf(stderr, "Fork error.\n");
  } else if (result == 0) { /* we are the child */
    if (execvp("sh", argv)) {
      fprintf(stderr, "Exec error.\n");
      exit(1);
    }
  } else { /* we are the parent */
    waitpid(result, NULL, 0);
  }
}

