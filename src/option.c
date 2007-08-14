/*
 * option.c: Options handling functions
 *
 * This file is part of GTick
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

/* own headers */
#include "globals.h"
#include "option.h"
#include "optionlexer.h"
#include "optionparser.h"
#include "util.h"

/*
 * returns new, empty options list
 */
option_list_t* option_list_new(void) {
  return NULL;
}

/*
 * destroys list, releasing all memory associated with it and destructing
 * options
 */
void option_list_delete(option_list_t* list) {
  option_list_t* temp;

  while (list) {
    temp = list;
    list = list->next;
    if (temp->destructor)
      temp->destructor(temp->object);
    free(temp);
  }
}

/*
 * inserts new option entry to specified list* with:
 * 
 * name: name of the option
 * constructor: will be called once on registration
 *              (actually, by this function)
 *              can be NULL for no constructor
 * destructor: will be called on options list destruction
 *             can be NULL for nu destructor
 * getter, setter: getter and setter functions for this option
 * object: argument to deliver to setter and getter
 *
 * returns 0 on success, -1 otherwise
 */
int option_register(option_list_t** list,
                    const char* prefix,
		    option_new_t constructor,
		    option_delete_t destructor,
		    option_set_t setter,
		    option_get_n_t getter_n,
		    option_get_t getter,
		    void* object)
{
  option_list_t* temp;

  temp = g_malloc(sizeof(option_list_t));

  temp->prefix = prefix;
  temp->destructor = destructor;
  temp->setter = setter;
  temp->getter_n = getter_n;
  temp->getter = getter;
  temp->object = object;
  temp->next = *list;
  temp->previous = NULL;
  if (*list)
    (*list)->previous = temp;
  *list = temp;

  if (constructor)
    constructor(object);
  return 0;
}

/*
 * sets specified value to specified option (by name) in option list
 * <list> by calling setter
 *
 * returns 0 on success, -1 otherwise
 */
int option_set(option_list_t* list, const char* name, const char* value) {
  if (debug)
    fprintf(stderr, "Setting %s to \"%s\" ...", name, value);
  while (list && strncmp(list->prefix, name, strlen(list->prefix))) {
    list = list->next;
  }

  if (list) {
    if (debug)
      fprintf(stderr, " OK.\n");
    return list->setter ? list->setter(list->object, name, value) : 0;
  } else {
    if (debug)
      fprintf(stderr, " WARNING: Couldn't find option.\n");
    return -1;
  }
}

/*
 * gets number of options for specified option prefix
 *
 * returns -1 if prefix doesn't exist
 */
int option_get_n(option_list_t* list, const char* prefix) {
  while (list && strcmp(list->prefix, prefix)) {
    list = list->next;
  }

  if (list) {
    return list->getter_n ? list->getter_n(list->object) : 0;
  } else {
    return -1;
  }
}

/*
 * gets specified option (value and name, by prefix and number)
 * from <list> by calling getter
 *
 * returns associated value, NULL on error
 */
const char* option_get(option_list_t* list, const char* prefix, int n,
                       const char** name)
{
  while (list && strcmp(list->prefix, prefix)) {
    list = list->next;
  }

  if (list) {
    if (name)
      *name = prefix;
    return list->getter ? list->getter(list->object, n, name) : 0;
  } else {
    return NULL;
  }
}

/*
 * loads options from rc file
 *
 * returns 0 on success, -1 otherwise
 */
int option_restore_all(option_list_t* list) {
  char* filename = get_rc_filename();

  if (!filename) {
    if (debug)
      fprintf(stderr, "Warning: Couldn't determine rc file name.\n");
    return -1;
  } else {
    if (debug)
      fprintf(stderr, "Reading rc file %s ...\n", filename);
    option_lexer_init(filename);
    if (option_in) {
      option_parse(list);
      option_lexer_deinit();
      free(filename);
      return 0;
    } else {
      if (debug)
	fprintf(stderr, "Warning: Couldn't open %s. Proceeding without it.\n",
	        filename);
      free(filename);
      return -1;
    }
  }
}

/*
 * saves options to rc file
 *
 * returns 0 on success, -1 otherwise
 */
int option_save_all(option_list_t* list) {
  char* filename = get_rc_filename();
  
  if (!filename) {
    if (debug)
      fprintf(stderr, "Warning: Couldn't determine rc file name.\n");
    return -1;
  } else {
    FILE* fp = fopen(filename, "w");

    if (debug)
      fprintf(stderr, "Writing all options to rc file %s... \n", filename);
    if (fp) {
      fprintf(fp, "#\n"
	          "# This is a personal GTick run control file and "
		    "was generated by GTick.\n"
		  "# (Feel free to edit it while GTick isn't running to "
		    "customize it for the\n"
		  "#  next GTick startup)\n"
		  "#\n"
		  "\n");
      while (list && list->next)
	list = list->next;
      while (list) {
        int i;
	int num = option_get_n(list, list->prefix);
	const char* name;
	const char* value;

	for (i = 0; i < num; i++) {
	  name = list->prefix;
	  value = option_get(list, list->prefix, i, &name);
	  if (!value)
	    fprintf(stderr, "Couldn't get value #%d from prefix \"%s\".\n", i, list->prefix);
	  else
	    fprintf(fp, "%s = %s\n", name, value);
	}
	list = list->previous;
      }
      fclose(fp);
      if (debug)
	fprintf(stderr, "OK.\n");
      return 0;
    } else {
      if (debug)
	fprintf(stderr, "Warning: Couldn't open rc file for writing.\n");
      return -1;
    }
  }
}

/* can be used with for every registered option that has 1 value
   for getter_n */
int option_return_one(void* object _U_) {
  return 1;
}

