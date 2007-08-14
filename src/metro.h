/* 
 * Metronome session interface
 *
 * This file is part of GTick
 * 
 *
 * Copyright (c) 1999, Alex Roberts
 * Copyright (c) 2005 Marco Tulio Gontijo e Silva <mtgontijo@yahoo.com.br>
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
 
#ifndef METRO_H
#define METRO_H

/* GNU headers */
#ifdef HAVE_SYS_TIME_H
  #include <sys/time.h>
#endif
#include <time.h>

/* GTK+ headers */
#include <gtk/gtk.h>
#include <glib.h>

/* own headers */
#include "options.h"
#include "threadtalk.h"

typedef enum state_t {
  STATE_IDLE,     /* Metronome off */
  STATE_RUNNING,  /* Metronome on  */
  
  STATE_NUMBER
} state_t;

typedef struct state_data_t {
  char* state;
  char* toggle_label;
} state_data_t;

/*
 * the main metro object
 */
typedef struct metro_t {
  GtkWidget* window;

  GtkWidget* menu;
  GtkAccelGroup* accel_group;

  GtkWidget* visualtick;
  GtkWidget* visualtick_count;         /* subwidget */
  GdkPixmap* visualtick_count_pixmap;  /* double buffering cache */
  GtkWidget* visualtick_slider;        /* subwidget */
  GdkPixmap* visualtick_slider_pixmap; /* double buffering cache */
  GtkToggleAction* visualtick_action;
  guint visualtick_timeout_handler_id;
  struct timeval visualtick_sync_time; /* time of last sync */
  unsigned int visualtick_sync_pos;    /* position of last sync */
  unsigned int visualtick_sync_pos_old;
  double visualtick_old_pos;           /* last drawed position */
  unsigned int visualtick_ticks;       /* number of ticks for this vt session */

  GtkWidget* meter_button_1;
  GtkWidget* meter_button_2;
  GtkWidget* meter_button_3;
  GtkWidget* meter_button_4;
  GtkWidget* meter_button_more;
  GtkWidget* meter_spin_button;
  	
  GtkWidget* togglebutton_label;
  GtkWidget* speed_name;               /* ComboBox */

  struct timeval lasttap;              /* Last activation of Tap button */

  GtkObject* speed_adjustment;
  GtkObject* volume_adjustment;
  int volume_initialized; /* do we have an initial volume from server yet? */

  GtkWidget* accentframe;              /* show / hide */
  GtkWidget** accentbuttons;           /* to switch on / off */
  GtkToggleAction* accenttable_action;
  
  GtkWidget* profileframe;             /* show / hide */
  GtkToggleAction* profiles_action;
  GtkTreeView* profiles_tree;

  GtkWidget* statusbar;
  guint status_context;

  GtkWidget* start_error; /* message to be shown on metronome start error */
  GtkAction* start_action;

  options_t* options;         /* structure with all the handled rc options */

  int state; /* metronome active flag */

  GThread* audio_thread;
  comm_t* inter_thread_comm;
} metro_t;

metro_t* metro_new(void);
void metro_delete(metro_t* metro);
int gui_get_meter(metro_t* metro);

int set_meter(metro_t* metro, const char* option_name _U_, const char* meter);
const char* get_meter(metro_t* metro, int n _U_, char** option_name _U_);
int set_speed(metro_t* metro, const char* option_name _U_, const char* speed);
const char* get_speed(metro_t* metro, int n _U_, char** option_name _U_);
int set_accents(metro_t* metro,
                const char* option_name _U_, const char* accents);
const char* get_accents(metro_t* metro,
                        int n _U_, char** option_name _U_);

#endif /* METRO_H */

