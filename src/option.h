/*
 * option.h: Options handling functions
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

#ifndef OPTION_H
#define OPTION_H

/* constructor: initialize object with default value
   returns 0 on success, -1 otherwise */
typedef int (*option_new_t) (void* object);

/* destructor */
typedef void (*option_delete_t) (void* object);

/* setter: returns 0 on success, -1 otherwise */
typedef int (*option_set_t) (void* object, const char* name, const char* value);

/* getter_n: returns number of options */
typedef int (*option_get_n_t) (void* object);

/* getter: returns associated value (and name) for specified option
 * (i.e., for the prefix represented by the option)
 *
 * name must be NULL, or be initialized with prefix when calling (then, it will
 * be changed by getter function, if necessary) */
typedef const char* (*option_get_t) (void* object, int n, const char** name);

typedef struct option_list_t option_list_t;
struct option_list_t {
  const char* prefix;
  option_delete_t destructor;
  option_set_t setter;
  option_get_n_t getter_n;
  option_get_t getter;
  void* object;
  option_list_t* next;
  option_list_t* previous;
};

option_list_t* option_list_new(void);
void option_list_delete(option_list_t* list);
int option_register(option_list_t** list,
                    const char* prefix,
		    option_new_t constructor,
		    option_delete_t destructor,
		    option_set_t setter,
		    option_get_n_t getter_n,
		    option_get_t getter,
		    void* object);
int option_set(option_list_t* list, const char* name, const char* value);
int option_get_n(option_list_t* list, const char* prefix);
const char* option_get(option_list_t* list, const char* prefix, int n, const char** name);

int option_restore_all(option_list_t* list);
int option_save_all(option_list_t* list);

/* can be used with for every registered option that has 1 value
   for getter_n */
int option_return_one(void* object);

#endif /* OPTION_H */
