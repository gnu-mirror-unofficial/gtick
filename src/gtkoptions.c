/*
 * gtkoptions.c: Options dialog functions
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
#include <math.h>

/* own headers */
#include "globals.h"
#include "gtkoptions.h"
#include "options.h"
#include "option.h"
#include "util.h"
#include "gtkutil.h"

static void
sound_button_toggled(GtkToggleButton *togglebutton, options_t* options)
{
  GSList* group;
  GtkWidget* radio_button;
  const char* samplename;
#ifdef WITH_SNDFILE
  GtkWidget *entry;
#endif /* WITH_SNDFILE */

  /*
   * Sample filename setup
   */
  group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(togglebutton));
  while (group &&
    !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(group->data)))
  {
    group = group->next;
  }
  radio_button = GTK_WIDGET(group->data);
  if (!strcmp(g_object_get_data(G_OBJECT(radio_button), "choice"),
	      "<default>"))
  {
    samplename = "<default>";
  } else if (!strcmp(g_object_get_data(G_OBJECT(radio_button), "choice"),
		     "<sine>"))
  {
    samplename = "<sine>";
  } else { /* other */
#ifdef WITH_SNDFILE
    entry = g_object_get_data(G_OBJECT(radio_button), "sample_name_entry");
    samplename = gtk_entry_get_text(GTK_ENTRY(entry));
#else
    fprintf(stderr, "Warning: Unhandled samplename case: \"%s\".\n",
	    (char*) g_object_get_data(G_OBJECT(radio_button), "choice"));
    samplename = "";
#endif /* WITH_SNDFILE */
  }

  option_set(options->option_list, "SampleFilename", samplename);
}

static void
sound_device_entry_changed(GtkEntry *entry, options_t* options)
{
  const char* sounddevice;

  /*
   * Sound device setup
   */
  sounddevice = gtk_entry_get_text(entry);
  option_set(options->option_list, "SoundDevice", sounddevice);
}

static void
cmd_start_entry_changed(GtkEntry *entry, options_t* options)
{
  const char* command;

  /*
   * External commands setup
   */
  command = gtk_entry_get_text(entry);
  option_set(options->option_list, "CommandOnStart", command);
}

static void
cmd_stop_entry_changed(GtkEntry *entry, options_t* options)
{
  const char* command;

  /*
   * External commands setup
   */
  command = gtk_entry_get_text(entry);
  option_set(options->option_list, "CommandOnStop", command);
}

static void
min_bpm_value_changed(GtkSpinButton *entry, options_t* options)
{
  const char* value;

  /*
   * BPM bounds
   */
  value = g_strdup_printf("%d",
                          (int) round(gtk_spin_button_get_value(entry)));
  option_set(options->option_list, "MinBPM", value);
}

static void
max_bpm_value_changed(GtkSpinButton *entry, options_t* options)
{
  const char* value;

  /*
   * BPM bounds
   */
  value = g_strdup_printf("%d",
                          (int) round(gtk_spin_button_get_value(entry)));
  option_set(options->option_list, "MaxBPM", value);
}

/*
 * callback invoked by response signal in dialog
 */
static void response_cb(GtkDialog *dialog, gint response, options_t* options _U_) {
  switch (response) {
    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_NONE:
    case GTK_RESPONSE_CLOSE:
      gtk_widget_destroy(GTK_WIDGET(dialog));
      break;
    default:
      fprintf(stderr, "Warning: Unhandled response to dialog: %d\n", response);
  }
}

/*
 * callback invoked by ok button in file selection dialog
 */
#ifdef WITH_SNDFILE
static void choose_cb_store(GtkWidget *button) {
  const gchar* filename;
  GtkWidget* file_selection;
  GtkWidget* entry;

  file_selection = g_object_get_data(G_OBJECT(button), "file_selection");
  filename =
    gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selection));
  entry = g_object_get_data(G_OBJECT(button), "sample_name_entry");
  gtk_entry_set_text(GTK_ENTRY(entry), filename);
}
#endif /* WITH_SNDFILE */

/*
 * callback invoked by filename choose button
 */
#ifdef WITH_SNDFILE
static void choose_cb(GtkWidget *button) {
  GtkWidget* file_selection =
    gtk_file_selection_new(_("Please choose a sound file."));
  gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);
  
  g_object_set_data(G_OBJECT(GTK_FILE_SELECTION(file_selection)->ok_button),
                    "sample_name_entry",
		    g_object_get_data(G_OBJECT(button), "sample_name_entry"));
  g_object_set_data(G_OBJECT(GTK_FILE_SELECTION(file_selection)->ok_button),
                    "file_selection",
		    file_selection);
  g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_selection)->ok_button),
                   "clicked",
		   G_CALLBACK(choose_cb_store),
		   NULL);
    
  /* destroy file selection dialog if ok or cancel button has been clicked */
  g_signal_connect_swapped(
      GTK_OBJECT(GTK_FILE_SELECTION(file_selection)->ok_button),
      "clicked",
      G_CALLBACK(gtk_widget_destroy), 
      (gpointer) file_selection); 
  g_signal_connect_swapped(
      GTK_OBJECT(GTK_FILE_SELECTION(file_selection)->cancel_button),
      "clicked",
      G_CALLBACK(gtk_widget_destroy),
      (gpointer) file_selection); 
   
  gtk_widget_show(file_selection);
}
#endif /* WITH_SNDFILE */

/*
 * callback invoked on toggled radiobutton for filename
 */
#ifdef WITH_SNDFILE
static void radio_toggled_cb(GtkWidget *button, GtkWidget* activewidget) {
  gtk_widget_set_sensitive(activewidget,
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
}
#endif /* WITH_SNDFILE */

/*
 * callback invoked by sample name entry change
 */
#ifdef WITH_SNDFILE
static void sample_name_entry_changed(GtkEntry *entry, options_t* options) {
  const char *samplename;

  samplename = gtk_entry_get_text(GTK_ENTRY(entry));
  option_set(options->option_list, "SampleFilename", samplename);
}
#endif /* WITH_SNDFILE */

/*
 * return options dialog
 */
GtkWidget* gtk_options_dialog_new(GtkWidget* parent, options_t* options) {
  GtkWidget* window = gtk_dialog_new_with_buttons(_("Preferences"),
      GTK_WINDOW(parent),
      GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
      NULL);
  GtkWidget* vbox;
  GtkWidget* frame;
  GtkWidget* soundvbox;
  GtkWidget* devicevbox;
  GtkWidget* radiobutton;
  GtkWidget* hbox;
  GtkWidget* entry;
  GtkWidget* spinbutton;
  GtkWidget* table;
#ifdef WITH_SNDFILE
  GtkWidget* button;
  GtkWidget* activehbox;
#endif /* WITH_SNDFILE */
  GtkWidget* label;
  GtkWidget* alignment;
  GtkSizeGroup* sizegroup;
  const char* samplename;

  if (!(samplename = option_get(options->option_list,
                                "SampleFilename", 0, NULL)))
  {
    fprintf(stderr, "gtk_options_dialog_new: "
	    "Couldn't get SampleFilename from options.\n");
    exit(1);
  }

  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(window)->vbox), 12);
  
  vbox = gtk_vbox_new(FALSE, 18);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window)->vbox), vbox);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
  gtk_widget_show(vbox);

  sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
  
  /*
   * Sound effect setting
   */
  frame = gtk_frame_new(_("Sound"));
  label = gtk_frame_get_label_widget(GTK_FRAME(frame));
  gtk_label_attr_bold(GTK_LABEL(label));
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show(frame);

  alignment = gtk_alignment_new(1.0, 1.0, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 6, 0, 24, 0);
  gtk_container_add(GTK_CONTAINER(frame), alignment);
  gtk_widget_show(alignment);

  soundvbox = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(alignment), soundvbox);
  gtk_widget_show(soundvbox);

  radiobutton = gtk_radio_button_new_with_label(NULL, _("Default"));
  g_object_set_data(G_OBJECT(radiobutton), "choice", "<default>");
  if (!strcmp(samplename, "<default>"))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton), TRUE);
  gtk_box_pack_start(GTK_BOX(soundvbox), radiobutton, FALSE, TRUE, 0);
  gtk_widget_show(radiobutton);

  /* Instant apply */
  g_signal_connect(G_OBJECT(radiobutton), "toggled",
                   G_CALLBACK(sound_button_toggled), options);

  radiobutton =
    gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radiobutton),
	                                        _("Sine"));
  g_object_set_data(G_OBJECT(radiobutton), "choice", "<sine>");
  if (!strcmp(samplename, "<sine>"))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton), TRUE);
  gtk_box_pack_start(GTK_BOX(soundvbox), radiobutton, FALSE, TRUE, 0);
  gtk_widget_show(radiobutton);

  /* Instant apply */
  g_signal_connect(G_OBJECT(radiobutton), "toggled",
                   G_CALLBACK(sound_button_toggled), options);

#ifdef WITH_SNDFILE
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(soundvbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show(hbox);

  radiobutton =
    gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radiobutton),
	                                        _("Other:"));
  g_object_set_data(G_OBJECT(radiobutton), "choice", "<other>");
  gtk_box_pack_start(GTK_BOX(hbox), radiobutton, FALSE, FALSE, 0);
  gtk_widget_show(radiobutton);

  activehbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), activehbox);
  gtk_widget_show(activehbox);

  entry = gtk_entry_new();
  if (samplename[0] != '<')
    gtk_entry_set_text(GTK_ENTRY(entry), samplename);
  gtk_box_pack_start_defaults(GTK_BOX(activehbox), entry);
  gtk_widget_show(entry);

  /* save entry at togglebutton to retrieve value on apply */
  g_object_set_data(G_OBJECT(radiobutton), "sample_name_entry", entry);

  g_signal_connect(G_OBJECT(radiobutton), "toggled",
                   G_CALLBACK(radio_toggled_cb), activehbox);

  if (samplename[0] != '<') {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton), TRUE);
  } else {
    gtk_widget_set_sensitive(activehbox, FALSE);
  }

  button = gtk_button_new_with_label(_("Choose"));
  gtk_box_pack_start(GTK_BOX(activehbox), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  /* Instant apply */
  g_signal_connect(G_OBJECT(radiobutton), "toggled",
                   G_CALLBACK(sound_button_toggled), options);
  g_signal_connect(G_OBJECT(entry), "changed",
                   G_CALLBACK(sample_name_entry_changed), options);
  
  /* save entry at button to retrieve value in file selection dialog */
  g_object_set_data(G_OBJECT(button), "sample_name_entry", entry);

  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(choose_cb), options);
#endif /* WITH_SNDFILE */

  
  /*
   * Sound device setting
   */
  frame = gtk_frame_new(_("Sound Device"));
  label = gtk_frame_get_label_widget(GTK_FRAME(frame));
  gtk_label_attr_bold(GTK_LABEL(label));
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show(frame);

  alignment = gtk_alignment_new(1.0, 1.0, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 6, 0, 24, 0);
  gtk_container_add(GTK_CONTAINER(frame), alignment);
  gtk_widget_show(alignment);

  devicevbox = gtk_vbox_new(FALSE, 12);
  gtk_container_add(GTK_CONTAINER(alignment), devicevbox);
  gtk_widget_show(devicevbox);

  hbox = gtk_hbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(devicevbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show(hbox);

  label = gtk_label_new(_("Device filename:"));
  gtk_size_group_add_widget(sizegroup, label);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
  gtk_widget_show(label);

  entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_entry_set_text(GTK_ENTRY(entry),
                     option_get(options->option_list,
		                "SoundDevice", 0, NULL));
  gtk_widget_show(entry);

  /* Instant apply */
  g_signal_connect(G_OBJECT(entry), "changed",
                   G_CALLBACK(sound_device_entry_changed), options);

  /*
   * External commands
   */
  frame = gtk_frame_new(_("External Commands"));
  label = gtk_frame_get_label_widget(GTK_FRAME(frame));
  gtk_label_attr_bold(GTK_LABEL(label));
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show(frame);

  alignment = gtk_alignment_new(1.0, 1.0, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 6, 0, 24, 0);
  gtk_container_add(GTK_CONTAINER(frame), alignment);
  gtk_widget_show(alignment);

  table = gtk_table_new(2, 2, FALSE);
  gtk_container_add(GTK_CONTAINER(alignment), table);
  gtk_table_set_row_spacings(GTK_TABLE(table), 6);
  gtk_table_set_col_spacings(GTK_TABLE(table), 12);
  gtk_widget_show(table);

  label = gtk_label_new(_("Execute on start:"));
  gtk_size_group_add_widget(sizegroup, label);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
      0, GTK_EXPAND, 0, 0);
  gtk_widget_show(label);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry),
                     option_get(options->option_list,
		                "CommandOnStart", 0, NULL));
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1,
      GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show(entry);

  /* Instant apply */
  g_signal_connect(G_OBJECT(entry), "changed",
                   G_CALLBACK(cmd_start_entry_changed), options);
  
  label = gtk_label_new(_("Execute on stop:"));
  gtk_size_group_add_widget(sizegroup, label);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
      0, GTK_EXPAND, 0, 0);
  gtk_widget_show(label);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry),
                     option_get(options->option_list,
		                "CommandOnStop", 0, NULL));
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 1, 2,
      GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show(entry);

  /* Instant apply */
  g_signal_connect(G_OBJECT(entry), "changed",
                   G_CALLBACK(cmd_stop_entry_changed), options);

  /*
   * BPM bounds
   */
  frame = gtk_frame_new(_("Speed Range"));
  label = gtk_frame_get_label_widget(GTK_FRAME(frame));
  gtk_label_attr_bold(GTK_LABEL(label));
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show(frame);

  alignment = gtk_alignment_new(1.0, 1.0, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 6, 0, 24, 0);
  gtk_container_add(GTK_CONTAINER(frame), alignment);
  gtk_widget_show(alignment);

  table = gtk_table_new(2, 2, FALSE);
  gtk_container_add(GTK_CONTAINER(alignment), table);
  gtk_table_set_row_spacings(GTK_TABLE(table), 6);
  gtk_table_set_col_spacings(GTK_TABLE(table), 12);
  gtk_widget_show(table);

  label = gtk_label_new(_("Minimum BPM:"));
  gtk_size_group_add_widget(sizegroup, label);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
      0, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(label);

  spinbutton = gtk_spin_button_new_with_range(MIN_BPM, MAX_BPM, 10.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spinbutton), 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton),
      (gdouble) strtol(option_get(options->option_list, "MinBPM", 0, NULL),
                       NULL, 0));
  gtk_table_attach(GTK_TABLE(table), spinbutton, 1, 2, 0, 1,
      0, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(spinbutton);
  g_signal_connect(G_OBJECT(spinbutton), "value-changed",
                   G_CALLBACK(min_bpm_value_changed), options);
  
  label = gtk_label_new(_("Maximum BPM:"));
  gtk_size_group_add_widget(sizegroup, label);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
      0, GTK_EXPAND, 0, 0);
  gtk_widget_show(label);

  spinbutton = gtk_spin_button_new_with_range(MIN_BPM, MAX_BPM, 10.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spinbutton), 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton),
      (gdouble) strtol(option_get(options->option_list, "MaxBPM", 0, NULL),
                       NULL, 0));
  gtk_table_attach(GTK_TABLE(table), spinbutton, 1, 2, 1, 2,
      0, GTK_EXPAND, 0, 0);
  gtk_widget_show(spinbutton);
  g_signal_connect(G_OBJECT(spinbutton), "value-changed",
                   G_CALLBACK(max_bpm_value_changed), options);
  
  g_signal_connect(G_OBJECT(window), "response",
                   G_CALLBACK(response_cb), options);
  return window;
}

