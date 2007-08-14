/*
 * Interface for Visual Tick
 *
 * This file is part of GTick
 *
 *
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

#ifndef VISUALTICK_H
#define VISUALTICK_H

/* GNU headers */
#ifdef HAVE_SYS_TIME_H
  #include <sys/time.h>
#endif
#include <time.h>

/* GTK+ headers */
#include <gtk/gtk.h>
#include <glib.h>

/* own headers */
#include "metro.h"

GtkWidget* visualtick_init(metro_t* metro);
void visualtick_enable(metro_t* metro);
void visualtick_disable(metro_t* metro);
void visualtick_sync(metro_t* metro, unsigned int pos);
void visualtick_new_meter(metro_t* metro, int meter);
gboolean visualtick_update(metro_t* metro);

#endif /* VISUALTICK_H */
