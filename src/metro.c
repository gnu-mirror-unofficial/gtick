/*
 * Metronome session
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

#include <config.h>

/* GNU headers */
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <assert.h>

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
#include "help.h"
#include "gtkutil.h"
#include "util.h"
#include "option.h"
#include "gtkoptions.h"
#include "threadtalk.h"
#include "visualtick.h"
#include "profiles.h"

/* icons */
#include "icon32x32.xpm"
#include "icon48x48.xpm"
#include "icon64x64.xpm"

/* prototypes */
static void metro_toggle_cb(GtkToggleAction *action, metro_t* metro);
static void preferences_cb(GtkAction *action, metro_t *metro);
static void quit_cb(GtkAction *action, metro_t *metro);
static void toggle_accenttable_cb(GtkToggleAction *action, metro_t *metro);
static void toggle_visualtick_cb(GtkToggleAction *action, metro_t *metro);

#define TIMER_DELAY 300
volatile int interrupted = 0; /* the caught signal will be stored here */

static state_data_t state_data[] = {
  { N_("Idle"),    N_("Start") },  /* STATE_IDLE    */
  { N_("Running"), N_("Stop") }    /* STATE_RUNNING */
};

static const GtkActionEntry actions[] = {
  { "HelpMenu", NULL, N_("_Help"), NULL, NULL, NULL },
  { "MetronomeMenu", NULL, N_("_Metronome"), NULL, NULL, NULL },
  { "Quit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q", NULL, G_CALLBACK(quit_cb) },
  { "EditMenu", NULL, N_("_Edit"), NULL, NULL, NULL },
  { "Preferences", GTK_STOCK_PREFERENCES, N_("_Preferences"), "<control>P", NULL, G_CALLBACK(preferences_cb) },
  { "ViewMenu", NULL, N_("_View"), NULL, NULL, NULL },
  { "About", GTK_STOCK_ABOUT, N_("_About"), NULL, NULL, G_CALLBACK(about_box) },
  { "Shortcuts", NULL, N_("_Shortcuts"), NULL, NULL, G_CALLBACK(help_shortcuts) }
};

static const GtkToggleActionEntry toggle_actions[] = {
  { "Start", NULL, N_("Start"), "<control>S", N_("Start the metronome"), G_CALLBACK(metro_toggle_cb), FALSE },
  { "VisualTick", NULL, N_("_Visual Tick"), "<control>V", NULL, G_CALLBACK(toggle_visualtick_cb), FALSE },
  { "AccentTable", NULL, N_("_Accent Table"), "<control>A", NULL, G_CALLBACK(toggle_accenttable_cb), FALSE },
  { "Profiles", NULL, N_("Pr_ofiles"), "<control>O", NULL, G_CALLBACK(toggle_profiles_cb), FALSE },
};

static const char *ui_description =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='MetronomeMenu'>"
"      <menuitem action='Start'/>"
"      <separator/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='EditMenu'>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <menuitem action='VisualTick'/>"
"      <menuitem action='AccentTable'/>"
"      <menuitem action='Profiles'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='About'/>"
"      <menuitem action='Shortcuts'/>"
"    </menu>"
"  </menubar>"
"</ui>";

/* Stores speed names with their common range, non overlapping */
typedef struct speed_name_t {
  char* name;
  int min_bpm;                 /* min_bpm <= speed ("name") */
  int max_bpm;                 /* speed ("name") < max_bpm */
} speed_name_t;

/*
 * Numbers collected from wikipedia, with some adjustments.
 * http://en.wikipedia.org/wiki/Tempo
 */
static speed_name_t speed_names[] = {
  /* TRANSLATORS: This is the Italian name of a musical tempo. Normally, it's
     not not translated, at least not in languages with latin alphabets. */
  { N_("Larghissimo"),   0,  40 },
  /* TRANSLATORS: This is the Italian name of a musical tempo. Normally, it's
     not not translated, at least not in languages with latin alphabets. */
  { N_("Largo"),        40,  60 },
  /* TRANSLATORS: This is the Italian name of a musical tempo. Normally, it's
     not not translated, at least not in languages with latin alphabets. */
  { N_("Larghetto"),    60,  66 },
  /* TRANSLATORS: This is the Italian name of a musical tempo. Normally, it's
     not not translated, at least not in languages with latin alphabets. */
  { N_("Adagio"),       66,  76 },
  /* TRANSLATORS: This is the Italian name of a musical tempo. Normally, it's
     not not translated, at least not in languages with latin alphabets. */
  { N_("Andante"),      76, 108 },
  /* TRANSLATORS: This is the Italian name of a musical tempo. Normally, it's
     not not translated, at least not in languages with latin alphabets. */
  { N_("Moderato"),    108, 120 },
  /* TRANSLATORS: This is the Italian name of a musical tempo. Normally, it's
     not not translated, at least not in languages with latin alphabets. */
  { N_("Allegro"),     120, 168 },
  /* TRANSLATORS: This is the Italian name of a musical tempo. Normally, it's
     not not translated, at least not in languages with latin alphabets. */
  { N_("Presto"),      168, 208 },
  /* TRANSLATORS: This is the Italian name of a musical tempo. Normally, it's
     not not translated, at least not in languages with latin alphabets. */
  { N_("Prestissimo"), 208, MAX_BPM },
};

/* handler for SIGTERM and SIGINT */
static void terminate_signal_callback(int sig) {
  interrupted = sig;
}

static guint get_name_index_from_speed(int speed) {
  guint result = 0;

  while (result < sizeof(speed_names) / sizeof(speed_name_t) &&
         (speed_names[result].min_bpm > speed ||
	  speed_names[result].max_bpm <= speed)) {

    result ++;
  }

  return result;
}

/*
 * Returns center speed from indexed named speed from speed_names
 */
static int get_speed_from_name_index(int index) {
  return (speed_names[index].min_bpm + speed_names[index].max_bpm) / 2;
}

/*
 * Return 1 if speed is in range of indexed index name
 */
static int is_in_range_of_index_name(int speed, int index) {
  if (speed_names[index].min_bpm <= speed && speed < speed_names[index].max_bpm)
    return 1;
  else
    return 0;
}

/*
 * Called whenever tap button is clicked
 */
static void tap_cb(metro_t* metro)
{
  struct timeval thistime;
  struct timeval diffval;
  double timediff;

  assert(metro != NULL);

  gettimeofday(&thistime, NULL);

  timeval_subtract(&diffval, &thistime, &metro->lasttap);
  timediff = diffval.tv_sec + (double)diffval.tv_usec / 1000000.0;

  if (timediff < 10.0) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->speed_adjustment),
                             round(60.0 / timediff));
  } else {
    if (debug)
      g_print("tap_cb(): No new speed yet: Tap again.\n");
  }

  metro->lasttap = thistime;
}

/*
 * read meter from GUI (widgets)
 */
int gui_get_meter(metro_t* metro) {
  int meter;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(metro->meter_button_1)))
    meter = 1;
  else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
	  metro->meter_button_2)))
    meter = 2;
  else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
	  metro->meter_button_3)))
    meter = 3;
  else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
	  metro->meter_button_4)))
    meter = 4;
  else
    meter = gtk_spin_button_get_value_as_int(
	GTK_SPIN_BUTTON(metro->meter_spin_button));

  return meter;
}

/*
 * GUI volume change callback
 */
static void set_volume_cb(metro_t* metro)
{
  double* volume = (double*) g_malloc(sizeof(double));

  *volume = GTK_ADJUSTMENT(metro->volume_adjustment)->value / 100.0;
  comm_client_query(metro->inter_thread_comm, MESSAGE_TYPE_SET_VOLUME,
      volume);
}

/*
 * GUI rate change callback, called on speed_adjustment
 */
static void set_speed_cb(metro_t *metro)
{
  double* frequency = (double*) g_malloc(sizeof(double));
  gint old_index;
  gint new_index;

  *frequency = GTK_ADJUSTMENT(metro->speed_adjustment)->value / 60.0;
  if (debug)
    g_print ("set_speed_cb(): rate=%f bpm\n", *frequency * 60.0);

  old_index = (gint) gtk_combo_box_get_active(GTK_COMBO_BOX(metro->speed_name));
  new_index = get_name_index_from_speed((int)round(*frequency * 60.0));

  if (new_index != old_index)
    gtk_combo_box_set_active(GTK_COMBO_BOX(metro->speed_name), new_index);

  if (debug)
    g_print("set_speed_cb(): New speed name = \"%s\"\n",
            speed_names[new_index].name);

  comm_client_query(metro->inter_thread_comm,
                    MESSAGE_TYPE_SET_FREQUENCY,
		    frequency);
}

/*
 * Callback called on speed name change
 */
static void set_speed_name_cb(metro_t *metro)
{
  gint index = gtk_combo_box_get_active(GTK_COMBO_BOX(metro->speed_name));

  if (!is_in_range_of_index_name(
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->speed_adjustment)),
    index))
  {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->speed_adjustment),
      get_speed_from_name_index(index));
  }
}

/*
 * At least one accent check button has been toggled
 */
static void accents_changed_cb(metro_t* metro) {
  int* message = (int*) g_malloc (sizeof(int) * MAX_METER);
  int i;

  for (i = 0; i < MAX_METER; i++) {
    message[i] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
	                                          metro->accentbuttons[i]));
  }

  comm_client_query(metro->inter_thread_comm, MESSAGE_TYPE_SET_ACCENTS,
                    message);
}

/*
 * Change state
 */
static void set_state(metro_t* metro, int state) {

  if (state != metro->state) {

    if (metro->state == STATE_RUNNING && state == STATE_IDLE) {
      comm_client_query(metro->inter_thread_comm,
	                MESSAGE_TYPE_STOP_METRONOME, NULL);
      execute(metro->options->command_on_stop);
    } else if (metro->state == STATE_IDLE && state == STATE_RUNNING) {
      comm_client_query(metro->inter_thread_comm,
	                MESSAGE_TYPE_START_METRONOME, NULL);
      execute(metro->options->command_on_start);
      visualtick_sync(metro, 0);
    } else {
      fprintf(stderr, "Warning: Unhandled state change.\n");
    }

    gtk_statusbar_pop(GTK_STATUSBAR(metro->statusbar),
		      metro->status_context);
    gtk_statusbar_push(GTK_STATUSBAR(metro->statusbar),
		       metro->status_context,
		       _(state_data[state].state));
    gtk_label_set_text(GTK_LABEL(metro->togglebutton_label),
		       _(state_data[state].toggle_label));
    /* Set Start/Stop label */
    g_object_set(metro->start_action, "label",
                 _(state_data[state].toggle_label), NULL);

    metro->state = state;
  }
}

/*
 * handle messages from server (i.e., from metronome backend)
 */
static gint handle_comm(metro_t* metro) {
  message_type_t message_type;
  void* body = NULL;

  while ((message_type = comm_client_try_get_reply(metro->inter_thread_comm,
	                                           &body)) !=
         MESSAGE_TYPE_NO_MESSAGE)
  {
    switch (message_type) {
      case MESSAGE_TYPE_RESPONSE_SYNC:
	visualtick_sync(metro, *((unsigned int*) body));
	free(body);
	break;
      case MESSAGE_TYPE_RESPONSE_START_ERROR:
	gtk_widget_show(metro->start_error);

	set_state(metro, STATE_IDLE);
        break;
      default:
        printf("Warning: Unhandled message type: %d.\n", message_type);
    }
  }

  return TRUE;
}

/*
 * Callback for "Show Visual Tick" check item menu entry
 */
static void toggle_visualtick_cb(GtkToggleAction *action _U_, metro_t *metro) {
  if (GTK_WIDGET_VISIBLE(metro->visualtick)) {
    visualtick_disable(metro);
  } else {
    visualtick_enable(metro);
  }
}

/*
 * Callback for "Show Accent Table" check item menu entry
 */
static void toggle_accenttable_cb(GtkToggleAction *action _U_, metro_t *metro) {
  if (GTK_WIDGET_VISIBLE(metro->accentframe)) {
    gtk_widget_hide(metro->accentframe);
    gtk_window_resize(GTK_WINDOW(metro->window), 1, 1);
  } else {
    gtk_widget_show(metro->accentframe);
  }
}

/*
 * Callback for toggle button (metronome on/off ...)
 */
static void metro_toggle_cb(GtkToggleAction *action _U_, metro_t *metro) {
  int new_state;

  if (metro->state == STATE_IDLE) {
    new_state = STATE_RUNNING;
  } else {
    new_state = STATE_IDLE;
  }

  set_state(metro, new_state);
}

/*
 * Quit program and destroy callback
 */
static void quit_cb(GtkAction *action _U_, metro_t *metro)
{
  set_state(metro, STATE_IDLE);
  option_save_all(metro->options->option_list);
  comm_client_query(metro->inter_thread_comm, MESSAGE_TYPE_STOP_SERVER, NULL);
  gtk_main_quit();
}

/*
 * periodically (e.g. each 300 milliseconds) from gtk main loop called function
 */
static gint timeout_callback(metro_t* metro) {
  if (interrupted) {
    switch(interrupted) {
    case SIGINT:
      if (debug)
	fprintf(stderr, "SIGINT (Ctrl-C ?) caught.\n");
      break;
    case SIGTERM:
      if (debug)
	fprintf(stderr, "SIGTERM caught.\n");
      break;
    default:
      fprintf(stderr, "Warning: Unknown signal caught.\n");
    }
    quit_cb(NULL, metro);
  }

  return TRUE; /* call it again */
}

/*
 * indirecting callback showing gtk_options_dialog_new()
 */
static void preferences_cb(GtkAction *action _U_, metro_t *metro)
{
  GtkWidget* dialog = gtk_options_dialog_new(metro->window, metro->options);

  gtk_widget_show(dialog);
}

/*
 * option system callback for initializing sample name with default value
 * returns 0 on success, -1 otherwise
 */
static int new_sample(metro_t* metro) {
  metro->options->sample_name = strdup(DEFAULT_SAMPLE_FILENAME);
  comm_client_query(metro->inter_thread_comm,
                    MESSAGE_TYPE_SET_SOUND, strdup(DEFAULT_SAMPLE_FILENAME));
  if (metro->options->sample_name)
    return 0;
  else
    return -1;
}

/*
 * option system callback for destroying sample name
 */
static void delete_sample(metro_t* metro) {
  if (metro->options->sample_name)
    free(metro->options->sample_name);
}

/*
 * option system callback for choosing the sample
 *
 * returns 0 on success, -1 otherwise
 */
static int set_sample(metro_t* metro,
                      const char* option_name _U_, const char* sample_name)
{
  if (metro->options->sample_name)
    free(metro->options->sample_name);
  else
    fprintf(stderr,
	    "free() error: metro->options->sample_name not allocated.\n");
  metro->options->sample_name = strdup(sample_name);
  comm_client_query(metro->inter_thread_comm,
                    MESSAGE_TYPE_SET_SOUND, strdup(sample_name));
  if (metro->state == STATE_RUNNING) {
    comm_client_query(metro->inter_thread_comm,
	              MESSAGE_TYPE_STOP_METRONOME, NULL);
    comm_client_query(metro->inter_thread_comm,
	              MESSAGE_TYPE_START_METRONOME, NULL);
  }

  return 0;
}

/*
 * option system callback for spotting the sample
 */
static const char* get_sample(metro_t* metro,
                              int n _U_, char** option_name _U_)
{
  return metro->options->sample_name;
}

/*
 * option system callback for initializing sound device name with default value
 * returns 0 on success, -1 otherwise
 */
static int new_sound_device_name(metro_t* metro) {
  metro->options->sound_device_name = strdup(DEFAULT_SOUND_DEVICE_FILENAME);
  comm_client_query(metro->inter_thread_comm,
                    MESSAGE_TYPE_SET_DEVICE,
		    strdup(DEFAULT_SOUND_DEVICE_FILENAME));
  if (metro->options->sound_device_name)
    return 0;
  else
    return -1;
}

/*
 * option system callback for destroying sound device name
 */
static void delete_sound_device_name(metro_t* metro) {
  if (metro->options->sound_device_name)
    free(metro->options->sound_device_name);
}

/*
 * option system callback for choosing the sound device name
 *
 * returns 0 on success, -1 otherwise
 */
static int set_sound_device_name(metro_t* metro,
                                 const char* option_name _U_,
				 const char* sound_device_name)
{
  if (metro->options->sound_device_name)
    free(metro->options->sound_device_name);
  else
    fprintf(stderr,
	    "free() error: metro->options->sound_device_name not allocated.\n");
  metro->options->sound_device_name = strdup(sound_device_name);
  comm_client_query(metro->inter_thread_comm,
                    MESSAGE_TYPE_SET_DEVICE, strdup(sound_device_name));
  if (metro->state == STATE_RUNNING) {
    comm_client_query(metro->inter_thread_comm,
	              MESSAGE_TYPE_STOP_METRONOME, NULL);
    comm_client_query(metro->inter_thread_comm,
	              MESSAGE_TYPE_START_METRONOME, NULL);
  }

  return 0;
}


/*
 * option system callback for initializing sound system with default value
 * returns 0 on success, -1 otherwise
 */

static int new_sound_system(metro_t* metro) {
  metro->options->sound_device_name = strdup(DEFAULT_SOUND_SYSTEM);
  comm_client_query(metro->inter_thread_comm,
                    MESSAGE_TYPE_SET_SOUNDSYSTEM,
		    strdup(DEFAULT_SOUND_SYSTEM));
  if (metro->options->soundsystem)
    return 0;
  else
    return -1;
}

/*
 * option system callback for destroying sound system
 */
static void delete_sound_system(metro_t* metro) {
  if (metro->options->soundsystem)
    free(metro->options->soundsystem);
}

/*
 * option system callback for choosing the sound system
 *
 * returns 0 on success, -1 otherwise
 */
static int set_sound_system(metro_t* metro,
                                 const char* option_name _U_,
				 const char* sound_system)
{
  if (metro->options->soundsystem)
    free(metro->options->soundsystem);
  else
    fprintf(stderr,
	    "free() error: metro->options->sound_device_name not allocated.\n");
  metro->options->soundsystem = strdup(sound_system);
  comm_client_query(metro->inter_thread_comm,
                    MESSAGE_TYPE_SET_SOUNDSYSTEM, strdup(sound_system));
  if (metro->state == STATE_RUNNING) {
    comm_client_query(metro->inter_thread_comm,
	              MESSAGE_TYPE_STOP_METRONOME, NULL);
    comm_client_query(metro->inter_thread_comm,
	              MESSAGE_TYPE_START_METRONOME, NULL);
  }

  return 0;
}

/*
 * option system callback for spotting the sound device name
 */
static const char* get_sound_system(metro_t* metro,
                                         int n _U_, char** option_name _U_)
{
  return metro->options->soundsystem;
}

/*
 * option system callback for spotting the sound device name
 */
static const char* get_sound_device_name(metro_t* metro,
                                         int n _U_, char** option_name _U_)
{
  return metro->options->sound_device_name;
}

/*
 * option system callback for choosing the min_bpm
 *
 * returns 0 on success, -1 otherwise
 */
static int set_min_bpm(metro_t* metro,
                       const char* option_name _U_,
		       const char* min_bpm)
{
  gint new_min_bpm;

  if (!metro || !metro->speed_adjustment || !min_bpm)
    return -1;

  new_min_bpm = abs(strtol(min_bpm, NULL, 0));
  if (new_min_bpm >= MIN_BPM && new_min_bpm <= MAX_BPM) {
    if (new_min_bpm > GTK_ADJUSTMENT(metro->speed_adjustment)->upper)
      GTK_ADJUSTMENT(metro->speed_adjustment)->upper = (gdouble) new_min_bpm;
    GTK_ADJUSTMENT(metro->speed_adjustment)->lower = (gdouble) new_min_bpm;
    gtk_adjustment_changed(GTK_ADJUSTMENT(metro->speed_adjustment));
  }

  return 0;
}

/*
 * option system callback for spotting the min_bpm
 *
 * if called with metro == NULL, returns NULL
 */
static const char* get_min_bpm(metro_t* metro,
                               int n _U_, char** option_name _U_)
{
  if (metro == NULL) {
    return NULL;
  } else {
    return g_strdup_printf("%d", (int) round(
      GTK_ADJUSTMENT(metro->speed_adjustment)->lower));
  }
}

/*
 * option system callback for initializing min_bpm option
 */
static int new_min_bpm(metro_t* metro) {
  if (metro->speed_adjustment) {
    GTK_ADJUSTMENT(metro->speed_adjustment)->lower = DEFAULT_MIN_BPM;
    gtk_adjustment_changed(GTK_ADJUSTMENT(metro->speed_adjustment));
    return 0;
  } else {
    return -1;
  }
}

/*
 * option system callback for destroying min_bpm option
 */
static void delete_min_bpm(metro_t* metro _U_) {
}

/*
 * option system callback for choosing the max_bpm
 *
 * returns 0 on success, -1 otherwise
 */
static int set_max_bpm(metro_t* metro,
                       const char* option_name _U_,
		       const char* max_bpm)
{
  gint new_max_bpm;

  if (!metro || !metro->speed_adjustment || !max_bpm)
    return -1;

  new_max_bpm = abs(strtol(max_bpm, NULL, 0));
  if (new_max_bpm >= MIN_BPM && new_max_bpm <= MAX_BPM) {
    if (new_max_bpm < GTK_ADJUSTMENT(metro->speed_adjustment)->lower)
      GTK_ADJUSTMENT(metro->speed_adjustment)->lower = (gdouble) new_max_bpm;
    GTK_ADJUSTMENT(metro->speed_adjustment)->upper = (gdouble) new_max_bpm;
    gtk_adjustment_changed(GTK_ADJUSTMENT(metro->speed_adjustment));
  }

  return 0;
}

/*
 * option system callback for spotting the max_bpm
 *
 * if called with metro == NULL, returns NULL
 */
static const char* get_max_bpm(metro_t* metro,
                               int n _U_, char** option_name _U_)
{
  if (metro == NULL) {
    return NULL;
  } else {
    return g_strdup_printf("%d", (int) round(
      GTK_ADJUSTMENT(metro->speed_adjustment)->upper));
  }
}

/*
 * option system callback for initializing max_bpm option
 */
static int new_max_bpm(metro_t* metro) {
  if (metro->speed_adjustment) {
    GTK_ADJUSTMENT(metro->speed_adjustment)->upper = DEFAULT_MAX_BPM;
    gtk_adjustment_changed(GTK_ADJUSTMENT(metro->speed_adjustment));
    return 0;
  } else {
    return -1;
  }
}

/*
 * option system callback for destroying max_bpm option
 */
static void delete_max_bpm(metro_t* metro _U_) {
}

/*
 * option system callback for choosing the speed
 *
 * returns 0 on success, -1 otherwise
 */
int set_speed(metro_t* metro, const char* option_name _U_, const char* speed)
{
  if (!metro || !metro->speed_adjustment || !speed)
    return -1;

  gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->speed_adjustment),
                           (gdouble) strtol(speed, NULL, 0));

  return 0;
}

/*
 * option system callback for spotting the speed
 *
 * if called with metro == NULL, returns NULL
 */
const char* get_speed(metro_t* metro, int n _U_, char** option_name _U_)
{
  if (metro == NULL) {
    return NULL;
  } else {
    return g_strdup_printf("%d", (int) round(
        gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->speed_adjustment))));
  }
}

/*
 * option system callback for initializing speed option
 */
static int new_speed(metro_t* metro) {
  if (metro->speed_adjustment) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->speed_adjustment),
                             (gdouble) DEFAULT_SPEED);
    return 0;
  } else {
    return -1;
  }
}

/*
 * option system callback for destroying speed option
 */
static void delete_speed(metro_t* metro _U_) {
}

/*
 * option system callback for choosing the volume
 *
 * returns 0 on success, -1 otherwise
 */
int set_volume(metro_t* metro,
                      const char* option_name _U_, const char* volume)
{
  if (!metro || !metro->volume_adjustment || !volume)
    return -1;

  gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->volume_adjustment),
                           g_ascii_strtod(volume, NULL));

  return 0;
}

/*
 * option system callback for spotting the volume
 *
 * if called with metro == NULL, returns NULL
 */
const char* get_volume(metro_t* metro,
                              int n _U_, char** option_name _U_)
{
  if (metro == NULL) {
    return NULL;
  } else {
    gchar* result = g_malloc(G_ASCII_DTOSTR_BUF_SIZE);
    return g_ascii_dtostr(result, G_ASCII_DTOSTR_BUF_SIZE,
      gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->volume_adjustment)));
  }
}

/*
 * option system callback for initializing volume option
 */
static int new_volume(metro_t* metro) {
  if (metro->volume_adjustment) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->volume_adjustment),
                             DEFAULT_VOLUME);
    return 0;
  } else {
    return -1;
  }
}

/*
 * option system callback for destroying volume option
 */
static void delete_volume(metro_t* metro _U_) {
}

/*
 * Sets the specified meter (type int)
 */
static void set_meter_int(metro_t* metro, int meter)
{
  int* message;
  int i;

  switch (meter) {
    case 1:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(metro->meter_button_1),
	  TRUE);
      break;
    case 2:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(metro->meter_button_2),
	  TRUE);
      break;
    case 3:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(metro->meter_button_3),
	  TRUE);
      break;
    case 4:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(metro->meter_button_4),
	  TRUE);
      break;
    default:
      gtk_toggle_button_set_active(
	  GTK_TOGGLE_BUTTON(metro->meter_button_more), TRUE);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(metro->meter_spin_button),
				meter);
  }
  gtk_widget_set_sensitive(metro->meter_spin_button, meter > 4);

  message = (int*) g_malloc (sizeof(int));

  *message = meter;

  for (i = 0; i < MAX_METER; i++) {
    if (i < meter) {
      gtk_widget_show(metro->accentbuttons[i]);
    } else {
      gtk_widget_hide(metro->accentbuttons[i]);
    }
  }
  gtk_window_resize(GTK_WINDOW(metro->window), 1, 1);

  if (debug)
    g_print("new meter: %d\n", meter);

  visualtick_new_meter(metro, meter);

  comm_client_query(metro->inter_thread_comm, MESSAGE_TYPE_SET_METER, message);
}

/*
 * Callback for meter mode
 */
static void set_meter_cb(GtkWidget* widget, metro_t *metro) {
  if (GTK_IS_TOGGLE_BUTTON(widget) &&
      !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    return;

  set_meter_int(metro, gui_get_meter(metro));
}

/*
 * option system callback for choosing the meter
 *
 * returns 0 on success, -1 otherwise
 */
int set_meter(metro_t* metro, const char* option_name _U_, const char* meter)
{
  int n;

  if (!metro || !meter)
    return -1;

  n = (int) strtol(meter, NULL, 0);

  set_meter_int(metro, n);

  return 0;
}

/*
 * option system callback for spotting the meter
 *
 * if called with metro == NULL, deinitializes state and return NULL
 */
const char* get_meter(metro_t* metro, int n _U_, char** option_name _U_)
{
  static char* result = NULL;

  if (result)
    free(result);

  if (metro == NULL)
    return NULL;

  result = g_strdup_printf("%d", gui_get_meter(metro));

  return result;
}

/*
 * option system callback for initializing meter option
 */
static int new_meter(metro_t* metro) {
  char* s;
  int* meter;

  s = g_strdup_printf("%d", DEFAULT_METER);
  set_meter(metro, NULL, s);
  free(s);

  meter = (int*) g_malloc (sizeof(int));
  *meter = DEFAULT_METER;
  comm_client_query(metro->inter_thread_comm, MESSAGE_TYPE_SET_METER, meter);

  return 0;
}

/*
 * option system callback for destroying meter option
 */
static void delete_meter(metro_t* metro _U_) {
  get_meter(NULL, 0, NULL);
}

/* option system callback for initializing command for metronome start */
static int new_command_on_start(options_t* options) {
  if ((options->command_on_start = strdup(DEFAULT_COMMAND_ON_START)))
    return 0;
  else
    return -1;
}

/* option system callback for destroying command for metronome start */
static void delete_command_on_start(options_t* options) {
  if (options->command_on_start)
    free(options->command_on_start);
}

/* option system callback for setting command for metronome start */
static int set_command_on_start(options_t* options,
                                const char* option_name _U_,
				const char* command)
{
  if (options->command_on_start)
    free(options->command_on_start);
  if ((options->command_on_start = strdup(command)))
    return 0;
  else
    return -1;
}

/* option system callback for getting command for metronome start */
static const char* get_command_on_start(options_t* options,
                                        int n _U_, char** option_name _U_)
{
  return options->command_on_start;
}

/* option system callback for initializing command for metronome stop */
static int new_command_on_stop(options_t* options) {
  if ((options->command_on_stop = strdup(DEFAULT_COMMAND_ON_STOP)))
    return 0;
  else
    return -1;
}

/* option system callback for destroying command for metronome stop */
static void delete_command_on_stop(options_t* options) {
  if (options->command_on_stop)
    free(options->command_on_stop);
}

/* option system callback for setting command for metronome stop */
static int set_command_on_stop(options_t* options,
                               const char* option_name _U_,
			       const char* command)
{
  if (options->command_on_stop)
    free(options->command_on_stop);
  if ((options->command_on_stop = strdup(command)))
    return 0;
  else
    return -1;
}

/* option system callback for getting command for metronome stop */
static const char* get_command_on_stop(options_t* options,
                                       int n _U_, char** option_name _U_)
{
  return options->command_on_stop;
}

/*
 * option system callback for initializing visualtick
 * returns 0 on success, -1 on error (metro == NULL)
 */
static int new_visualtick(metro_t* metro) {
  if (metro) {
    gtk_toggle_action_set_active(metro->visualtick_action, FALSE);
    return 0;
  } else {
    return -1;
  }
}

/* option system callback for destroying visualtick option */
static void delete_visualtick(metro_t* metro _U_) {
}

/* option system callback for setting visualtick */
static int set_visualtick(metro_t* metro,
                          const char* option_name _U_,
                          const char* visualtick)
{
  if (metro) {
    gtk_toggle_action_set_active(metro->visualtick_action,
      !strcmp(visualtick, "yes") || !strcmp(visualtick, "1"));
    return 0;
  } else {
    return -1;
  }
}

/* option system callback for getting visualtick */
static const char* get_visualtick(metro_t* metro,
                                  int n _U_, char** option_name _U_)
{
  if (gtk_toggle_action_get_active(metro->visualtick_action))
  {
    return "1";
  } else {
    return "0";
  }
}

/* option system callback for initializing accenttable option */
static int new_show_accenttable(metro_t* metro) {
  if (metro) {
    gtk_toggle_action_set_active(metro->accenttable_action, FALSE);
    return 0;
  } else {
    return -1;
  }
}

/* option system callback for destroying accenttable option */
static void delete_show_accenttable(metro_t* metro _U_) {
}

/* option system callback for setting accenttable option */
static int set_show_accenttable(metro_t* metro,
                                const char* option_name _U_,
				const char* accenttable)
{
  if (metro) {
    gtk_toggle_action_set_active(metro->accenttable_action,
      !strcmp(accenttable, "yes") || !strcmp(accenttable, "1"));
    return 0;
  } else {
    return -1;
  }
}

/* option system callback for getting accenttable option */
static const char* get_show_accenttable(metro_t* metro,
                                        int n _U_, char** option_name _U_)
{
  if (gtk_toggle_action_get_active(metro->accenttable_action))
  {
    return "1";
  } else {
    return "0";
  }
}

/* option system callback for initializing accents */
static int new_accents(metro_t* metro) {
  if (metro) {
    int i;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(metro->accentbuttons[0]),
                                 TRUE);
    for (i = 1; i < MAX_METER; i++) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(metro->accentbuttons[i]),
	                           FALSE);
    }
    return 0;
  } else {
    return -1;
  }
}

/* option system callback for destroying accents option */
static void delete_accents(metro_t* metro _U_) {
}

/* option system callback for setting accents option */
int set_accents(metro_t* metro,
                const char* option_name _U_, const char* accents)
{
  if (metro) {
    int i = 0;

    while (*accents && i < MAX_METER) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(metro->accentbuttons[i]),
	                           *accents == '1');
      i++;
      accents++;
    }
    while (i < MAX_METER) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(metro->accentbuttons[i]),
	                           FALSE);
      i++;
    }
    return 0;
  } else {
    return -1;
  }
}

/* option system callback for getting accents option */
const char* get_accents(metro_t* metro,
                        int n _U_, char** option_name _U_)
{
  static char result[MAX_METER + 1];

  if (metro) {
    int i;

    for (i = 0; i < MAX_METER; i++) {
      result[i] =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
	                                 metro->accentbuttons[i])) ?
	'1' : '0';
    }

    result[MAX_METER] = '\0';

    i = MAX_METER - 1;
    while (i > 0 && result[i] == '0') {
      result[i] = '\0';
      i--;
    }

    return result;
  } else {
    return NULL;
  }
}

/*
 * returns the (main) menu bar for the specified window
 *
 * further initialization done in metro:
 * - menu-prepared accel_group added to window
 * - visualtick_menu_item
 * - accenttable_menu_item
 */
static GtkWidget* get_menubar(metro_t* metro) {
  GtkActionGroup *action_group;
  GtkAccelGroup *accel_group;
  GtkUIManager *ui_manager;
  GError *error;

  action_group = gtk_action_group_new("MenuActions");
  gtk_action_group_set_translation_domain(action_group, PACKAGE);
  gtk_action_group_add_actions(action_group, actions, G_N_ELEMENTS(actions),
                               metro);
  gtk_action_group_add_toggle_actions(action_group, toggle_actions,
                                      G_N_ELEMENTS(toggle_actions), metro);

  ui_manager = gtk_ui_manager_new();
  gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);

  accel_group = gtk_ui_manager_get_accel_group(ui_manager);
  gtk_window_add_accel_group(GTK_WINDOW(metro->window), accel_group);

  error = NULL;
  if (!gtk_ui_manager_add_ui_from_string(ui_manager, ui_description,
                                         -1, &error))
  {
    g_message("Building menus failed: %s", error->message);
    g_error_free(error);
    exit(EXIT_FAILURE);
  }

  metro->visualtick_action = GTK_TOGGLE_ACTION(
    gtk_ui_manager_get_action(ui_manager, "/ui/MainMenu/ViewMenu/VisualTick"));
  metro->accenttable_action = GTK_TOGGLE_ACTION(
    gtk_ui_manager_get_action(ui_manager, "/ui/MainMenu/ViewMenu/AccentTable"));
  metro->profiles_action = GTK_TOGGLE_ACTION(
    gtk_ui_manager_get_action(ui_manager, "/ui/MainMenu/ViewMenu/Profiles"));
  metro->start_action =
    gtk_ui_manager_get_action(ui_manager, "/ui/MainMenu/MetronomeMenu/Start");
  metro->accel_group = accel_group;

  return gtk_ui_manager_get_widget(ui_manager, "/MainMenu");
}

/*
 * The audio thread
 */
static gpointer audio_loop(metro_t* metro) {
  dsp_t* dsp;

  dsp = dsp_new(metro->inter_thread_comm);
  dsp_main_loop(dsp);
  dsp_delete(dsp);

  return NULL;
}

typedef struct closure_data_t {
  metro_t* metro;
  int value;
} closure_data_t;

/* meter shortcuts (e.g. '1'..'9') */
static void set_meter_shortcut_cb(closure_data_t* closure_data) {
  set_meter_int(closure_data->metro, closure_data->value);
}

/* callback for destruction of closure_data */
static void set_meter_shortcut_destroy_notify(closure_data_t* closure_data,
                                              GClosure *closure _U_)
{
  free(closure_data);
}


static void set_speed_shortcut_cb_plus(metro_t* metro) {
  gdouble speed =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->speed_adjustment));

  if (speed + 1 <= GTK_ADJUSTMENT(metro->speed_adjustment)->upper) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->speed_adjustment),
	                     speed + 1);
  }
}
static void set_speed_shortcut_cb_plusTwo(metro_t* metro) {
  gdouble speed =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->speed_adjustment));

  if (speed + 2 <= GTK_ADJUSTMENT(metro->speed_adjustment)->upper) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->speed_adjustment),
	                     speed + 2);
  }
}

static void set_speed_shortcut_cb_plusTen(metro_t* metro) {
  gdouble speed =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->speed_adjustment));

  if (speed + 10 <= GTK_ADJUSTMENT(metro->speed_adjustment)->upper) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->speed_adjustment),
	                     speed + 10);
  }
}

static void set_speed_shortcut_cb_minus(metro_t* metro) {
  gdouble speed =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->speed_adjustment));

  if (speed - 1 >= GTK_ADJUSTMENT(metro->speed_adjustment)->lower) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->speed_adjustment),
	                     speed - 1);
  }
}

static void set_speed_shortcut_cb_minusTwo(metro_t* metro) {
  gdouble speed =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->speed_adjustment));

  if (speed - 2 >= GTK_ADJUSTMENT(metro->speed_adjustment)->lower) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->speed_adjustment),
	                     speed - 2);
  }
}

static void set_speed_shortcut_cb_minusTen(metro_t* metro) {
  gdouble speed =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->speed_adjustment));

  if (speed - 10 >= GTK_ADJUSTMENT(metro->speed_adjustment)->lower) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->speed_adjustment),
	                     speed - 10);
  }
}

static void set_speed_shortcut_cb_double(metro_t* metro) {
  gdouble speed =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->speed_adjustment));

  if (speed * 2 <= GTK_ADJUSTMENT(metro->speed_adjustment)->upper) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->speed_adjustment),
	                     speed *= 2);
  }
}


static void set_speed_shortcut_cb_half(metro_t* metro) {
  gdouble speed =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->speed_adjustment));

  if (round(speed / 2) >= GTK_ADJUSTMENT(metro->speed_adjustment)->lower) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->speed_adjustment),
	                     speed = round(speed / 2));
  }
}



static void set_volume_shortcut_cb_plusTwo(metro_t* metro) {
  gdouble volume =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->volume_adjustment));

  if (volume + 2 <= 100) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->volume_adjustment),
	                     volume + 2);
  }
}

static void set_volume_shortcut_cb_minusTwo(metro_t* metro) {
  gdouble volume =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->volume_adjustment));

  if (volume - 2 >= 0) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->volume_adjustment),
	                     volume - 2);
  }
}

static void set_volume_shortcut_cb_plusTen(metro_t* metro) {
  gdouble volume =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->volume_adjustment));

  if (volume + 10 <= 100) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->volume_adjustment),
	                     volume + 10);
  }
}

static void set_volume_shortcut_cb_minusTen(metro_t* metro) {
  gdouble volume =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->volume_adjustment));

  if (volume - 10 >= 0) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->volume_adjustment),
	                     volume - 10);
  }
}


static void set_volume_shortcut_cb_double(metro_t* metro) {
  gdouble volume =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->volume_adjustment));

  if (volume * 2 <= 100) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->volume_adjustment),
	                     volume *= 2);
  }
  else {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->volume_adjustment),
                               volume = 100);
  }
}

static void set_volume_shortcut_cb_half(metro_t* metro) {
  gdouble volume =
    gtk_adjustment_get_value(GTK_ADJUSTMENT(metro->volume_adjustment));

  if (volume / 2 >= 0) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(metro->volume_adjustment),
	                     volume /= 2);
  }
}



/*
 * returns a new metro object
 */
metro_t* metro_new(void) {
  metro_t* metro;
  GtkObject* adjustment;
  GList* icon_list = NULL;
  GtkWidget* windowvbox;
  GtkWidget* vbox;
  GtkWidget* metroframe;
  GtkWidget* metrovbox;
  GtkWidget* label;
  GtkWidget* table;
  GtkWidget* beatframe;
  GtkWidget* beatvbox;
  GtkWidget* spinbutton;
  GtkWidget* hbox;
  GtkWidget* togglebutton;
  GtkWidget* scale;
  GtkWidget* accenttable;
  GtkWidget* button;
  GtkWidget* alignment;
  closure_data_t* closure_data;
  guint i, j;

  metro = g_malloc0(sizeof(metro_t));

  if (debug)
    printf("Creating metro object at %p.\n", metro);

  metro->options = options_new();
  metro->state = STATE_IDLE; /* set default */

  metro->inter_thread_comm = comm_new();
  metro->audio_thread =
    g_thread_new("metro", (GThreadFunc) audio_loop,
                         metro /* data passed to function */);

  option_register(&metro->options->option_list,
                  "SampleFilename",
		  (option_new_t) new_sample,
		  (option_delete_t) delete_sample,
		  (option_set_t) set_sample,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_sample,
		  (void*) metro);
  option_register(&metro->options->option_list,
                  "SoundSystem",
		  (option_new_t) new_sound_system,
		  (option_delete_t) delete_sound_system,
		  (option_set_t) set_sound_system,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_sound_system,
		  (void*) metro);

  option_register(&metro->options->option_list,
                  "SoundDevice",
		  (option_new_t) new_sound_device_name,
		  (option_delete_t) delete_sound_device_name,
		  (option_set_t) set_sound_device_name,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_sound_device_name,
		  (void*) metro);

  option_register(&metro->options->option_list,
                  "CommandOnStart",
		  (option_new_t) new_command_on_start,
		  (option_delete_t) delete_command_on_start,
		  (option_set_t) set_command_on_start,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_command_on_start,
		  (void*) metro->options);
  option_register(&metro->options->option_list,
                  "CommandOnStop",
		  (option_new_t) new_command_on_stop,
		  (option_delete_t) delete_command_on_stop,
		  (option_set_t) set_command_on_stop,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_command_on_stop,
		  (void*) metro->options);

  metro->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  g_signal_connect(G_OBJECT(metro->window), "destroy",
                   G_CALLBACK(quit_cb), metro);

  gtk_window_set_title (GTK_WINDOW (metro->window), TITLE " " VERSION);

  icon_list = g_list_append(icon_list,
          (gpointer) gdk_pixbuf_new_from_xpm_data((const char**) icon32x32));
  icon_list = g_list_append(icon_list,
          (gpointer) gdk_pixbuf_new_from_xpm_data((const char**) icon48x48));
  icon_list = g_list_append(icon_list,
          (gpointer) gdk_pixbuf_new_from_xpm_data((const char**) icon64x64));
  gtk_window_set_icon_list(GTK_WINDOW(metro->window), icon_list);

  windowvbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add(GTK_CONTAINER(metro->window), windowvbox);
  gtk_widget_show (windowvbox);

  metro->menu = get_menubar(metro);
  gtk_box_pack_start(GTK_BOX(windowvbox), metro->menu, FALSE, TRUE, 0);
  gtk_widget_show(metro->menu);

  vbox = gtk_vbox_new(FALSE, 18);
  gtk_box_pack_start_defaults(GTK_BOX(windowvbox), vbox);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
  gtk_widget_show(vbox);

  metro->statusbar = gtk_statusbar_new();
  metro->status_context =
    gtk_statusbar_get_context_id(GTK_STATUSBAR(metro->statusbar), "status");
  gtk_statusbar_push(GTK_STATUSBAR(metro->statusbar),
                     metro->status_context,
		     _(state_data[metro->state].state));
  gtk_box_pack_end(GTK_BOX(windowvbox), metro->statusbar, FALSE, TRUE, 0);
  gtk_widget_show(metro->statusbar);

  /* Metronome UI Code */
  metro->visualtick = visualtick_init(metro);
  gtk_box_pack_start_defaults(GTK_BOX(vbox), metro->visualtick);
  /* gtk_widget_show(metro->visualtick); */

  metroframe = gtk_frame_new(_("Settings"));
  label = gtk_frame_get_label_widget(GTK_FRAME(metroframe));
  gtk_label_attr_bold(GTK_LABEL(label));
  gtk_frame_set_shadow_type(GTK_FRAME(metroframe), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(vbox), metroframe, FALSE, TRUE, 0);
  gtk_widget_show (metroframe);

  alignment = gtk_alignment_new(1.0, 1.0, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 6, 0, 24, 0);
  gtk_container_add(GTK_CONTAINER(metroframe), alignment);
  gtk_widget_show(alignment);

  metrovbox = gtk_vbox_new(FALSE, 6);
  gtk_container_set_border_width(GTK_CONTAINER(metrovbox), 0);
  gtk_container_add(GTK_CONTAINER(alignment), metrovbox);
  gtk_widget_show(metrovbox);

  table = gtk_table_new(3, 3, FALSE);
  gtk_box_pack_start(GTK_BOX(metrovbox), table, FALSE, TRUE, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 6);
  gtk_table_set_col_spacings(GTK_TABLE(table), 12);
  gtk_widget_show(table);

  metro->speed_name = gtk_combo_box_new_text();
  for (i = 0; i < sizeof(speed_names) / sizeof(speed_name_t); i++) {
    gchar tmpbuf[1000];

    /* TRANSLATORS: This is a tempo name with its associated BPM
       (beats per minute) range */
    g_snprintf(tmpbuf, 1000, _("%s   (%d ... %d)"), _(speed_names[i].name),
               speed_names[i].min_bpm, speed_names[i].max_bpm - 1);
    gtk_combo_box_append_text(GTK_COMBO_BOX(metro->speed_name), tmpbuf);
  }
  gtk_table_attach_defaults(GTK_TABLE(table), metro->speed_name,
                            1, 2, 0, 1);
  gtk_widget_show(metro->speed_name);
  g_signal_connect_swapped(G_OBJECT(metro->speed_name),
                           "changed",
			   G_CALLBACK(set_speed_name_cb),
			   metro);

  /* TRANSLATORS: This button needs to be clicked ("tapped") at least twice for
     specifying a tempo */
  button = gtk_button_new_with_label(_("Tap"));
  gtk_table_attach_defaults(GTK_TABLE(table), button, 2, 3, 0, 1);
  gtk_widget_show(button);
  g_signal_connect_swapped(button, "clicked", G_CALLBACK(tap_cb), metro);

  label = gtk_label_new(_("Speed:"));
  gtk_table_attach(GTK_TABLE(table), label,
                   0, 1, 1, 2,
		   GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show (label);

  metro->speed_adjustment =
    gtk_adjustment_new(DEFAULT_BPM, DEFAULT_MIN_BPM, DEFAULT_MAX_BPM,
	               1.0, 1.0, 0.0);
  g_signal_connect_swapped(G_OBJECT(metro->speed_adjustment),
                           "value_changed",
		           G_CALLBACK(set_speed_cb),
		           metro);
  set_speed_cb(metro);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (metro->speed_adjustment));
  gtk_range_set_increments(GTK_RANGE (scale), 1.0, 1.0);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_table_attach(GTK_TABLE(table), scale,
                   1, 2, 1, 2,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (scale);

  spinbutton = gtk_spin_button_new(GTK_ADJUSTMENT(metro->speed_adjustment),
                                   1.0, /* Step */
				   0    /* number of decimals */);
  gtk_table_attach(GTK_TABLE(table), spinbutton,
                   2, 3, 1, 2,
		   GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(spinbutton);

  option_register(&metro->options->option_list,
                  "MinBPM",
		  (option_new_t) new_min_bpm,
		  (option_delete_t) delete_min_bpm,
		  (option_set_t) set_min_bpm,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_min_bpm,
		  (void*) metro);

  option_register(&metro->options->option_list,
                  "MaxBPM",
		  (option_new_t) new_max_bpm,
		  (option_delete_t) delete_max_bpm,
		  (option_set_t) set_max_bpm,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_max_bpm,
		  (void*) metro);

  option_register(&metro->options->option_list,
                  "Speed",
		  (option_new_t) new_speed,
		  (option_delete_t) delete_speed,
		  (option_set_t) set_speed,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_speed,
		  (void*) metro);

  option_register(&metro->options->option_list,
                  "Volume",
		  (option_new_t) new_volume,
		  (option_delete_t) delete_volume,
		  (option_set_t) set_volume,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_volume,
		  (void*) metro);

  label = gtk_label_new (_("Volume:"));
  gtk_table_attach(GTK_TABLE(table), label,
                   0, 1, 2, 3,
		   GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show (label);

  metro->volume_adjustment = gtk_adjustment_new(DEFAULT_VOLUME,
      VOLUME_MIN, VOLUME_MAX, 5.0, 25.0, 0.0);
  g_signal_connect_swapped(G_OBJECT(metro->volume_adjustment),
			   "value_changed",
			   G_CALLBACK(set_volume_cb),
			   metro);
  set_volume_cb(metro);

  scale = gtk_hscale_new (GTK_ADJUSTMENT(metro->volume_adjustment));
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_CONTINUOUS);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_table_attach(GTK_TABLE(table), scale,
                   1, 2, 2, 3,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (scale);

  spinbutton = gtk_spin_button_new(GTK_ADJUSTMENT(metro->volume_adjustment),
                                   1.0, /* Step */
				   1    /* number of decimals */);
  gtk_table_attach(GTK_TABLE(table), spinbutton,
                   2, 3, 2, 3,
		   GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(spinbutton);

  /* Timing UI Code */

  beatframe = gtk_frame_new (_("Meter"));
  gtk_frame_set_shadow_type(GTK_FRAME(beatframe), GTK_SHADOW_NONE);
  label = gtk_frame_get_label_widget(GTK_FRAME(beatframe));
  gtk_label_attr_bold(GTK_LABEL(label));
  gtk_box_pack_start(GTK_BOX(vbox), beatframe, FALSE, TRUE, 0);
  gtk_widget_show(beatframe);

  alignment = gtk_alignment_new(1.0, 1.0, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 6, 0, 24, 0);
  gtk_container_add(GTK_CONTAINER(beatframe), alignment);
  gtk_widget_show(alignment);

  beatvbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (beatvbox), 0);
  gtk_container_add (GTK_CONTAINER (alignment), beatvbox);
  gtk_widget_show(beatvbox);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start(GTK_BOX (beatvbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show (hbox);

  metro->meter_button_1 = gtk_radio_button_new_with_label (NULL,
      /* TRANSLATORS: all beats are equal */
      _("Even"));
  gtk_box_pack_start(GTK_BOX (hbox), metro->meter_button_1, FALSE, TRUE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (metro->meter_button_1), TRUE);
  gtk_widget_show (metro->meter_button_1);

  g_signal_connect(G_OBJECT(metro->meter_button_1),
                   "toggled",
		   G_CALLBACK(set_meter_cb),
		   metro);

  metro->meter_button_2 =
    gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(metro->meter_button_1),
	/* TRANSLATORS: duple meter */
	_("2/4"));
  gtk_box_pack_start(GTK_BOX(hbox), metro->meter_button_2, FALSE, TRUE, 0);
  gtk_widget_show (metro->meter_button_2);
  g_signal_connect(G_OBJECT(metro->meter_button_2),
                   "toggled",
		   G_CALLBACK(set_meter_cb),
		   metro);

  metro->meter_button_3 =
    gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(metro->meter_button_2),
	/* TRANSLATORS: triple meter */
	_("3/4"));
  gtk_box_pack_start(GTK_BOX(hbox), metro->meter_button_3, FALSE, TRUE, 0);
  gtk_widget_show (metro->meter_button_3);
  g_signal_connect(G_OBJECT(metro->meter_button_3),
                   "toggled",
		   G_CALLBACK(set_meter_cb),
		   metro);

  metro->meter_button_4 =
    gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(metro->meter_button_3),
	/* TRANSLATORS: quadruple meter / common meter */
	_("4/4"));
  gtk_box_pack_start(GTK_BOX(hbox), metro->meter_button_4, FALSE, TRUE, 0);
  gtk_widget_show (metro->meter_button_4);
  g_signal_connect(G_OBJECT(metro->meter_button_4),
                   "toggled",
		   G_CALLBACK(set_meter_cb),
		   metro);

  metro->meter_button_more =
    gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(metro->meter_button_4),
	/* TRANSLATORS: meter with more than 4 beats per measure */
	_("Other:"));
  gtk_box_pack_start(GTK_BOX(hbox), metro->meter_button_more,
      FALSE, TRUE, 0);
  gtk_widget_show(metro->meter_button_more);
  g_signal_connect(G_OBJECT(metro->meter_button_more),
                   "toggled",
                   G_CALLBACK(set_meter_cb),
	           metro);

  adjustment = gtk_adjustment_new(5, 5, MAX_METER, 1, 4, 0);
  metro->meter_spin_button =
    gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1, 0);
  gtk_widget_set_sensitive(metro->meter_spin_button, FALSE);
  gtk_widget_show(metro->meter_spin_button);
  gtk_box_pack_start(GTK_BOX(hbox), metro->meter_spin_button,
      FALSE, TRUE, 0);
  g_signal_connect(G_OBJECT(metro->meter_spin_button),
                   "value-changed",
                   G_CALLBACK(set_meter_cb),
		   metro);

  /* Accent table */
  metro->accentframe = gtk_frame_new(_("Beat Accent"));
  gtk_frame_set_shadow_type(GTK_FRAME(metro->accentframe), GTK_SHADOW_NONE);
  label = gtk_frame_get_label_widget(GTK_FRAME(metro->accentframe));
  gtk_label_attr_bold(GTK_LABEL(label));
  gtk_box_pack_start(GTK_BOX(vbox), metro->accentframe, FALSE, TRUE, 0);
  /* gtk_widget_show(metro->accentframe); */

  alignment = gtk_alignment_new(1.0, 1.0, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 6, 0, 24, 0);
  gtk_container_add(GTK_CONTAINER(metro->accentframe), alignment);
  gtk_widget_show(alignment);

  accenttable = gtk_table_new((MAX_METER - 1) / 10 + 1, 10, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(accenttable), 12);
  gtk_container_set_border_width(GTK_CONTAINER(accenttable), 0);
  gtk_container_add(GTK_CONTAINER(alignment), accenttable);
  gtk_widget_show(accenttable);

  metro->accentbuttons = (GtkWidget**) g_malloc(sizeof(GtkWidget*) * MAX_METER);

  for (i = 0; i < (MAX_METER - 1) / 10 + 1; i++) {
    for (j = 0; j < 10; j++) {
      char* temp;

      temp = g_strdup_printf("%d", i * 10 + j + 1);
      button = gtk_check_button_new_with_label(temp);
      free(temp);
      gtk_table_attach(GTK_TABLE(accenttable), button,
	                        j, j + 1, i, i + 1,
				GTK_FILL, GTK_FILL, 0, 0);

      g_signal_connect_swapped(button, "toggled",
			       G_CALLBACK(accents_changed_cb), metro);

      metro->accentbuttons[i * 10 + j] = button;
    }
  }

  option_register(&metro->options->option_list,
                  "Meter",
		  (option_new_t) new_meter,
		  (option_delete_t) delete_meter,
		  (option_set_t) set_meter,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_meter,
		  (void*) metro);

  option_register(&metro->options->option_list,
                  "ShowAccentTable",
		  (option_new_t) new_show_accenttable,
		  (option_delete_t) delete_show_accenttable,
		  (option_set_t) set_show_accenttable,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_show_accenttable,
		  (void*) metro);
  option_register(&metro->options->option_list,
                  "Accents",
		  (option_new_t) new_accents,
		  (option_delete_t) delete_accents,
		  (option_set_t) set_accents,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_accents,
		  (void*) metro);

  /* Profiles */
  metro->profileframe = profiles_new(metro);
  gtk_box_pack_start(GTK_BOX(vbox), metro->profileframe, FALSE, TRUE, 0);

  /* General UI Code */
  hbox = gtk_hbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX(windowvbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show (hbox);

  togglebutton = gtk_button_new();
  g_signal_connect(G_OBJECT(togglebutton), "clicked",
                   G_CALLBACK(metro_toggle_cb), metro);

  gtk_box_pack_start(GTK_BOX(hbox), togglebutton, TRUE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS (togglebutton, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (togglebutton);
  GTK_WIDGET_SET_FLAGS (togglebutton, GTK_CAN_FOCUS);
  gtk_widget_grab_focus (togglebutton);
  gtk_widget_show (togglebutton);

  metro->togglebutton_label =
    gtk_label_new(_(state_data[metro->state].toggle_label));
  gtk_misc_set_padding(GTK_MISC(metro->togglebutton_label), 64, 0);
  gtk_container_add(GTK_CONTAINER(togglebutton),
		    metro->togglebutton_label);
  gtk_widget_show(metro->togglebutton_label);

  /* set up additional short cuts */
  for (i = '1'; i <= '9'; i++) {
    closure_data = (closure_data_t*) g_malloc (sizeof(closure_data_t));
    closure_data->metro = metro;
    closure_data->value = i - '1' + 1;
    gtk_accel_group_connect(metro->accel_group, i, GDK_CONTROL_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_meter_shortcut_cb),
	                  closure_data,
			  (GClosureNotify) set_meter_shortcut_destroy_notify));
  }
  gtk_accel_group_connect(metro->accel_group, '+', GDK_CONTROL_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_speed_shortcut_cb_plus), metro, NULL));
  gtk_accel_group_connect(metro->accel_group, '-', GDK_CONTROL_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_speed_shortcut_cb_minus),metro, NULL));

  gtk_accel_group_connect(metro->accel_group, 'x', GDK_MOD1_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_speed_shortcut_cb_plusTwo), metro, NULL));
  gtk_accel_group_connect(metro->accel_group, 'z', GDK_MOD1_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_speed_shortcut_cb_minusTwo),metro, NULL));

  gtk_accel_group_connect(metro->accel_group, 'd', GDK_CONTROL_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_speed_shortcut_cb_double), metro, NULL));
  gtk_accel_group_connect(metro->accel_group, 'h', GDK_CONTROL_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_speed_shortcut_cb_half), metro, NULL));

  gtk_accel_group_connect(metro->accel_group, 'w', GDK_MOD1_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_speed_shortcut_cb_double), metro, NULL));
  gtk_accel_group_connect(metro->accel_group, 'q', GDK_MOD1_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_speed_shortcut_cb_half), metro, NULL));

  gtk_accel_group_connect(metro->accel_group, 's', GDK_MOD1_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_speed_shortcut_cb_plusTen), metro, NULL));
  gtk_accel_group_connect(metro->accel_group, 'a', GDK_MOD1_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_speed_shortcut_cb_minusTen), metro, NULL));

  gtk_accel_group_connect(metro->accel_group, 'x', GDK_SHIFT_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_volume_shortcut_cb_plusTwo), metro, NULL));
  gtk_accel_group_connect(metro->accel_group, 'z', GDK_SHIFT_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_volume_shortcut_cb_minusTwo),metro, NULL));

  gtk_accel_group_connect(metro->accel_group, 's', GDK_SHIFT_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_volume_shortcut_cb_plusTen), metro, NULL));
  gtk_accel_group_connect(metro->accel_group, 'a', GDK_SHIFT_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_volume_shortcut_cb_minusTen),metro, NULL));

  gtk_accel_group_connect(metro->accel_group, 'w', GDK_SHIFT_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_volume_shortcut_cb_double), metro, NULL));
  gtk_accel_group_connect(metro->accel_group, 'q', GDK_SHIFT_MASK, 0,
      g_cclosure_new_swap(G_CALLBACK(set_volume_shortcut_cb_half),metro, NULL));

  option_register(&metro->options->option_list,
                  "VisualTick",
		  (option_new_t) new_visualtick,
		  (option_delete_t) delete_visualtick,
		  (option_set_t) set_visualtick,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_visualtick,
		  (void*) metro);

  /* read rc file */
  option_restore_all(metro->options->option_list);

  /* set up additional handlers */
  gtk_timeout_add(TIMER_DELAY, (GtkFunction) timeout_callback, metro);
  gtk_timeout_add(VISUAL_DELAY * 1000, (GtkFunction) handle_comm, metro);

  signal(SIGINT, &terminate_signal_callback);
  signal(SIGTERM, &terminate_signal_callback);

  /* additional helper widgets: */

  /* Start error message */
  metro->start_error = gtk_message_dialog_new(GTK_WINDOW(metro->window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Couldn't start metronome.\n"
	      "Please check if specified sound device\n"
	      "and sample file are accessible."));
  /* Hide the dialog when the user responds to it */
  g_signal_connect_swapped(G_OBJECT(metro->start_error),
                           "response",
                           G_CALLBACK(gtk_widget_hide),
                           G_OBJECT(metro->start_error));

  gtk_widget_show (metro->window);

  return metro;
}

void metro_delete(metro_t* metro) {
  g_thread_join(metro->audio_thread);
  comm_delete(metro->inter_thread_comm);
  metro->inter_thread_comm = NULL;
  options_delete(metro->options);
  free(metro->accentbuttons);
  if (debug)
    printf("Destroying metro object at %p.\n", metro);
  g_free(metro);
}

