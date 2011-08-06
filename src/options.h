/*
 * Metronome options object interface
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "option.h"

/*
 * actually a part of a metro(nome)
 */
typedef struct options_t {
  option_list_t* option_list; /* list of available rc options */

  char* sample_name;
  char* soundsystem;
  char* sound_device_name;
  char* command_on_start;
  char* command_on_stop;
} options_t;

options_t* options_new(void);
void options_delete(options_t* options);

#endif /* OPTIONS_H */
