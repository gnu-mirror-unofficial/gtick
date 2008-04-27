/*
 * Profiles implementation
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

#include <config.h>

/* GNU headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* own headers */
#include "globals.h"
#include "metro.h"
#include "option.h"
#include "profiles.h"
#include "util.h"
#include "gtkutil.h"

enum {
  COLUMN_NAME,
  COLUMN_PROFILE,
  N_COLUMNS
};

typedef struct profile_t {
  char* speed;
  char* meter;
  char* accents;
} profile_t;

static profile_t* profile_new(void) {
  return g_malloc0(sizeof(profile_t));
}

static void profile_delete(profile_t* profile) {
  if (profile) {
    if (profile->speed) {
      g_free(profile->speed);
    }
    if (profile->meter) {
      g_free(profile->meter);
    }
    if (profile->accents) {
      g_free(profile->accents);
    }
    g_free(profile);
  } else g_print("profile_delete(): profile==NULL\n");
}

static void profile_set_speed(profile_t* profile, const gchar* s) {
  if (profile) {
    if (profile->speed)
      g_free(profile->speed);
    profile->speed = g_strdup(s);
  } else g_print("profile_set_speed(): profile==NULL\n");
}

static gchar* profile_get_speed(profile_t* profile) {
  if (profile) {
    return profile->speed;
  } else {
    g_print("profile_get_speed(): profile==NULL\n");
    return NULL;
  }
}

static void profile_set_meter(profile_t* profile, const gchar* s) {
  if (profile) {
    if (profile->meter)
      g_free(profile->meter);
    profile->meter = g_strdup(s);
  } else g_print("profile_set_meter(): profile==NULL\n");
}

static gchar* profile_get_meter(profile_t* profile) {
  if (profile) {
    return profile->meter;
  } else {
    g_print("profile_get_meter(): profile==NULL\n");
    return NULL;
  }
}

static void profile_set_accents(profile_t* profile, const gchar* s) {
  if (profile) {
    if (profile->accents)
      g_free(profile->accents);
    profile->accents = g_strdup(s);
  } else g_print("profile_set_accents(): profile==NULL\n");
}

static gchar* profile_get_accents(profile_t* profile) {
  if (profile) {
    return profile->accents;
  } else {
    g_print("profile_get_accents(): profile==NULL\n");
    return NULL;
  }
}

/* Callback for changed selection -> setting to respective profile values */
static void selection_changed_cb(metro_t* metro)
{
  GtkTreeSelection* selection;
  GtkTreeIter iter;
  
  if (debug)
    g_print("Selection changed. Restoring settings from profile.\n");
  
  selection = gtk_tree_view_get_selection(metro->profiles_tree);
  if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
    GtkListStore* store =
      GTK_LIST_STORE(gtk_tree_view_get_model(metro->profiles_tree));
    profile_t* profile;
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                       COLUMN_PROFILE, &profile, -1);
    set_meter(metro, NULL, profile_get_meter(profile));
    set_speed(metro, NULL, profile_get_speed(profile));
    set_accents(metro, NULL, profile_get_accents(profile));
  } else if (debug)
    g_print("selection_changed_cb(): No Profile list selection available.\n");
}

/*
 * Callback for "Show Profiles" check item menu entry
 */
void toggle_profiles_cb(GtkToggleAction *action _U_, metro_t *metro) {
  if (GTK_WIDGET_VISIBLE(metro->profileframe)) {
    gtk_widget_hide(metro->profileframe);
    gtk_window_resize(GTK_WINDOW(metro->window), 1, 1);
  } else {
    gtk_widget_show(metro->profileframe);
  }
}

/* option system callback for initializing profiles show option */
static int new_show_profiles(metro_t* metro) {
  if (metro) {
    gtk_toggle_action_set_active(metro->profiles_action, FALSE);
    return 0;
  } else {
    return -1;
  }
}

/* option system callback for destroying profiles show option */
static void delete_show_profiles(metro_t* metro _U_) {
}

/* option system callback for setting profiles show option */
static int set_show_profiles(metro_t* metro,
                             const char* option_name _U_,
			     const char* show_profiles)
{
  if (metro) {
    gtk_toggle_action_set_active(metro->profiles_action,
      !strcmp(show_profiles, "yes") || !strcmp(show_profiles, "1"));
    return 0;
  } else {
    return -1;
  }
}

/* option system callback for getting profiles show option */
static const char* get_show_profiles(metro_t* metro,
                                     int n _U_, char** option_name _U_)
{
  if (gtk_toggle_action_get_active(metro->profiles_action))
  {
    return "1";
  } else {
    return "0";
  }
}

/* option system callback for initializing selected profile option */
static int new_profile_selected(metro_t* metro) {
  if (metro) {
    GtkListStore* store;
    if ((store =
          GTK_LIST_STORE(gtk_tree_view_get_model(metro->profiles_tree))))
    {
      GtkTreeSelection* selection =
        gtk_tree_view_get_selection(metro->profiles_tree);
      GtkTreeIter iter;
      if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter)) {
        gtk_tree_selection_select_iter(selection, &iter);
      } else if (debug) g_print("new_profile_selected(): No entries.\n");
    } else g_print("new_profile_selected(): No tree model available.\n");
    return 0;
  } else {
    return -1;
  }
}

/* option system callback for destroying selected profile option */
static void delete_profile_selected(metro_t* metro _U_) {
}

/* option system callback for setting selected profile option */
static int set_profile_selected(metro_t* metro,
                                const char* option_name _U_,
			        const char* profile_selected)
{
  if (metro) {
    GtkListStore* store;
    if ((store =
          GTK_LIST_STORE(gtk_tree_view_get_model(metro->profiles_tree))))
    {
      GtkTreeSelection* selection =
        gtk_tree_view_get_selection(metro->profiles_tree);
      GtkTreeIter iter;
      int n = strtol(profile_selected, NULL, 0) - 1;
      if (gtk_tree_model_iter_nth_child(
             GTK_TREE_MODEL(store), &iter, NULL, n))
      {
        g_signal_handlers_block_by_func(
	  selection, G_CALLBACK(selection_changed_cb), metro);
        gtk_tree_selection_select_iter(selection, &iter);
        g_signal_handlers_unblock_by_func(
	  selection, G_CALLBACK(selection_changed_cb), metro);
      } else if (debug)
          g_print("set_profile_selected(): No profile #%s available.\n",
	          profile_selected);
    } else g_print("set_profile_selected(): No tree model available.\n");
    return 0;
  } else {
    return -1;
  }
}

/* option system callback for getting selected profile option */
static const char* get_profile_selected(metro_t* metro,
                                        int n _U_, char** option_name _U_)
{
  static gchar selected[16];
  selected[0] = '1';
  selected[1] = 0;
  if (metro) {
    GtkListStore* store;
    if ((store =
          GTK_LIST_STORE(gtk_tree_view_get_model(metro->profiles_tree))))
    {
      GtkTreeSelection* selection =
        gtk_tree_view_get_selection(metro->profiles_tree);
      GtkTreeIter iter;
      int n = 1;
      if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter)) {
        gboolean valid = TRUE;

        while (valid && !gtk_tree_selection_iter_is_selected(selection, &iter)) {
	  valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	  n++;
	}
	if (valid) {
	  g_snprintf(selected, 16, "%d", n);
	}
      } else if (debug)
          g_print("get_profile_selected(): No items.\n");
    } else g_print("get_profile_selected(): No tree model available.\n");
  }
  return selected;
}

/* option system callback for initializing profile option */
static int new_profile(metro_t* metro _U_) {
  return 0;
}

/* option system callback for destroying profile option */
static void delete_profile(metro_t* metro _U_) {
}

/* option system callback for setting profile option */
static int set_profile(metro_t* metro,
                       const char* option_name,
		       const char* option_value)
{
  gchar* name_copy = g_strdup(option_name);
  gchar* tmp;
  int n = 0;

  //g_print("\n");

  tmp = strtok(name_copy, "_"); /* Initial "Profile_" */
  if (tmp) {
    if (!strcmp(tmp, "Profile")) {
      tmp = strtok(NULL, "_"); /* Profile number */
      if (tmp) {
        n = strtol(tmp, NULL, 0) - 1;
	if (n >= 0) {
	  tmp = strtok(NULL, "_"); /* Option suffix */
	  if (tmp) {
            GtkListStore* store;
            if ((store = GTK_LIST_STORE(
	             gtk_tree_view_get_model(metro->profiles_tree))))
            {
              GtkTreeIter iter;
	      profile_t* profile = NULL;
              while (!gtk_tree_model_iter_nth_child(
                GTK_TREE_MODEL(store), &iter, NULL, n))
              { /* Create new entry */
	        gtk_list_store_append(store, &iter);
	        gtk_list_store_set(store, &iter,
	          COLUMN_NAME, _("Unnamed Profile"),
	          COLUMN_PROFILE, profile_new(),
	          -1);
	      }
	      gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
	                         COLUMN_PROFILE, &profile,
				 -1);
	      if (!strcmp(tmp, "Name")) {
	        gtk_list_store_set(store, &iter, COLUMN_NAME, option_value, -1);
	      } else if (!strcmp(tmp, "Speed")) {
	        profile_set_speed(profile, option_value);
	      } else if (!strcmp(tmp, "Meter")) {
	        profile_set_meter(profile, option_value);
	      } else if (!strcmp(tmp, "Accents")) {
	        profile_set_accents(profile, option_value);
	      } else {
	        g_print("set_profile(): Bad option suffix: %s\n", tmp);
                g_free(name_copy);
		return -1;
	      }
              g_free(name_copy);
	      return 0;
	    } else g_print("set_profile(): No Tree Model available.\n");
	  } else g_print("set_profile(): No option suffix.\n");
	} else g_print("set_profile(): Bad profile number in option: %s.\n", tmp);
      } else g_print("set_profile(): No profile number in option.\n");
    } else g_print("set_profile(): Bad option name prefix: %s.\n", tmp);
  } else g_print("set_profile(): No option name prefix.\n");
  g_free(name_copy);
  return -1;
}

/* option system callback for getting profile option */
static int get_n_profile(metro_t* metro)
{
  GtkListStore* store;
  if ((store =
    GTK_LIST_STORE(gtk_tree_view_get_model(metro->profiles_tree))))
  {
    return gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL)
           * 4 /* name + profile data entries */;
  } else {
    g_print("get_n_profile(): No tree model available.\n");
    return 0;
  }

}

/* option system callback for getting profile option */
static const char* get_profile(metro_t* metro,
                               int n, char** option_name)
{
  static gchar* name = NULL;
  gchar* value = NULL;

  int profile_n = n / 4;
  int suboption_n = n % 4;
  
  GtkListStore* store;

  gchar* name_prefix = option_name ? *option_name : "Profile_";
  gchar* name_suffix = "";

  if (name) {
    g_free(name);
    name = NULL;
  }

  if ((store =
        GTK_LIST_STORE(gtk_tree_view_get_model(metro->profiles_tree))))
  {
    GtkTreeIter iter;

    if (gtk_tree_model_iter_nth_child(
          GTK_TREE_MODEL(store), &iter, NULL, profile_n))
    {
      profile_t* profile;
      gchar* s;
      gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                         COLUMN_NAME, &s,
                         COLUMN_PROFILE, &profile,
			 -1);
      switch (suboption_n) {
        case 0:
          name_suffix = "Name";
	  value = s;
          break;
        case 1:
          name_suffix = "Speed";
	  value = profile_get_speed(profile);
          break;
        case 2:
          name_suffix = "Meter";
	  value = profile_get_meter(profile);
          break;
        case 3:
          name_suffix = "Accents";
	  value = profile_get_accents(profile);
          break;
        default:
          g_print("get_profile(): unhandled suboption: %d.\n", suboption_n);
      }

      name = g_strdup_printf("%s%d_%s", name_prefix, profile_n + 1, name_suffix);

      if (option_name)
        *option_name = name;

    } else g_print("get_profile(): No profile #%d available.\n", profile_n);
  } else g_print("get_profile(): No tree model available.\n");

  return value;
}

/*
 * callback invoked by response signal in dialog
 */
static void edit_response_cb(GtkDialog *dialog, gint response, metro_t* metro) {
  GtkTreeSelection* selection = NULL;
  GtkTreeIter iter;
  GtkListStore* store;

  switch (response) {
    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_CLOSE:
      gtk_widget_destroy(GTK_WIDGET(dialog));
      break;
    case GTK_RESPONSE_OK:
      store = GTK_LIST_STORE(gtk_tree_view_get_model(metro->profiles_tree));
      selection = gtk_tree_view_get_selection(metro->profiles_tree);
      if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
        GtkEntry* entry =
	  (GtkEntry*) g_object_get_data(G_OBJECT(dialog), "profile_entry");
        gtk_list_store_set(GTK_LIST_STORE(store), &iter, COLUMN_NAME,
	  gtk_entry_get_text(entry), -1);
      } else g_print("edit_response_cb(): no selection available.\n");

      gtk_widget_destroy(GTK_WIDGET(dialog));
      break;
    default:
      fprintf(stderr, "Warning: Unhandled response to dialog: %d\n", response);
  }
}

/*
 * Actual save function: saves current settings to selected profile
 */
static void save_profile(metro_t* metro, GtkTreeIter* iter)
{
  GtkListStore* store =
    GTK_LIST_STORE(gtk_tree_view_get_model(metro->profiles_tree));
  profile_t* profile;
  gtk_tree_model_get(GTK_TREE_MODEL(store), iter,
                     COLUMN_PROFILE, &profile, -1);
  profile_set_meter(profile, get_meter(metro, 1, NULL));
  profile_set_speed(profile, get_speed(metro, 1, NULL));
  profile_set_accents(profile, get_accents(metro, 1, NULL));
}

/*
 * Actual editing dialog
 */
static void edit_profile(metro_t* metro)
{
  GtkTreeSelection* selection;
  GtkTreeIter iter;
  
  selection = gtk_tree_view_get_selection(metro->profiles_tree);
  if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
    GtkWidget* window;
    GtkWidget* table;
    GtkWidget* label;
    GtkWidget* entry;
    gchar* s;
    GtkListStore* store =
      GTK_LIST_STORE(gtk_tree_view_get_model(metro->profiles_tree));
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COLUMN_NAME, &s, -1);

    window = gtk_dialog_new_with_buttons(_("Edit Profile"), GTK_WINDOW(metro->window),
        GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL |
          GTK_DIALOG_NO_SEPARATOR,
        GTK_STOCK_OK, GTK_RESPONSE_OK,
        NULL);

    g_signal_connect(G_OBJECT(window),
                     "destroy",
		     G_CALLBACK(gtk_widget_destroy),
	             window);

    table = gtk_table_new(1, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), table, TRUE, TRUE, 0);
    gtk_widget_show(table);

    label = gtk_label_new(_("Please enter new Profile name: "));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
    gtk_widget_show(label);

    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), s);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 0, 1);
    gtk_widget_show(entry);

    g_object_set_data(G_OBJECT(window), "profile_entry", entry);

    g_signal_connect(G_OBJECT(window), "response",
                     G_CALLBACK(edit_response_cb), metro);

    gtk_widget_show(window);

  } else g_print("edit_profile(): No Profile list selection available.\n");
}

/* Callback for Up button */
static void up_cb(metro_t* metro)
{
  GtkTreeSelection* selection;
  GtkTreeIter iter;

  if (debug) g_print("Up button clicked.\n");

  selection = gtk_tree_view_get_selection(metro->profiles_tree);

  if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
    /* Get previous entry for swapping */
    GtkListStore* store =
      GTK_LIST_STORE(gtk_tree_view_get_model(metro->profiles_tree));
    if (store) {
      GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);

      if (gtk_tree_path_prev(path)) {
        GtkTreeIter prev_iter;
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &prev_iter, path)) {
          gtk_list_store_swap(store, &iter, &prev_iter);
	  if (debug)
	    g_print("Entries swapped.\n");
        } else if (debug)
                 g_print("Couldn't get iterator for previous node path.\n");
      } else if (debug) g_print("No previous entry available.\n");
      gtk_tree_path_free(path);
    } else g_print("up_cb(): No TreeView model available!\n");
  } else if (debug) g_print("No selection available.\n");
}

/* Callback for Down button */
static void down_cb(metro_t* metro)
{
  GtkTreeSelection* selection;
  GtkTreeIter iter;
  
  if (debug) g_print("Down button clicked.\n");
  
  selection = gtk_tree_view_get_selection(metro->profiles_tree);

  if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
    /* Get previous entry for swapping */
    GtkListStore* store =
      GTK_LIST_STORE(gtk_tree_view_get_model(metro->profiles_tree));
    if (store) {
      GtkTreeIter next_iter = iter;
      if (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &next_iter)) {
        gtk_list_store_swap(store, &iter, &next_iter);
        if (debug)
	  g_print("Entries swapped.\n");
      } else if (debug) g_print("No next entry available.\n");
    } else g_print("down_cb(): No TreeView model available!\n");
  } else if (debug) g_print("No selection available.\n");
}

/* Callback for Add button */
static void add_cb(metro_t* metro)
{
  GtkListStore* store;

  if (debug) g_print("Add button clicked.\n");

  if ((store =
        GTK_LIST_STORE(gtk_tree_view_get_model(metro->profiles_tree))))
  {
    GtkTreeSelection* selection =
      gtk_tree_view_get_selection(metro->profiles_tree);
    GtkTreeIter iter;

    gtk_list_store_append(GTK_LIST_STORE(store), &iter);
    gtk_list_store_set(
      GTK_LIST_STORE(store), &iter,
      COLUMN_NAME, _("Unnamed Profile"),
      COLUMN_PROFILE, profile_new(),
      -1);
    save_profile(metro, &iter);
    gtk_tree_selection_select_iter(selection, &iter);
    edit_profile(metro);
  } else g_print("add_cb(): No TreeView model available!\n");
}

/* Callback for Delete button */
static void delete_cb(metro_t* metro)
{
  GtkTreeSelection* selection;
  GtkTreeIter iter;
  
  if (debug) g_print("Delete button clicked.\n");
  
  selection = gtk_tree_view_get_selection(metro->profiles_tree);

  if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
    GtkListStore* store =
      GTK_LIST_STORE(gtk_tree_view_get_model(metro->profiles_tree));
    profile_t* profile;
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                       COLUMN_PROFILE, &profile, -1);
    profile_delete(profile);
    gtk_list_store_remove(store, &iter);
  } else if (debug) g_print("delete_cb(): No selection available.\n");
}

/* Callback for Save button */
static void save_cb(metro_t* metro)
{
  GtkTreeSelection* selection;
  GtkTreeIter iter;
  
  if (debug) g_print("Save button clicked.\n");

  selection = gtk_tree_view_get_selection(metro->profiles_tree);
  if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
    save_profile(metro, &iter);
  } else
    if (debug)
      g_print("save_cb(): No Profile selection available.\n");
}

/* Callback for Edit button */
static void edit_cb(metro_t* metro)
{
  GtkTreeSelection* selection;
  
  if (debug) g_print("Edit button clicked.\n");

  selection = gtk_tree_view_get_selection(metro->profiles_tree);
  if (gtk_tree_selection_get_selected(selection, NULL, NULL)) {
    edit_profile(metro);
  } else
    if (debug)
      g_print("edit_cb(): No Profile selection available.\n");
}

/*
 * Creates new profile frame and connects it with metro (options)
 */
GtkWidget* profiles_new(metro_t* metro)
{
  GtkWidget* profileframe = gtk_frame_new(_("Profiles"));
  GtkWidget* label;
  GtkWidget* alignment;
  GtkWidget* table;
  GtkWidget* button;
  GtkListStore *store;
  GtkWidget *tree;      /* the list in a GtkTreeView */
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkWidget* scrolled_window;

  gtk_frame_set_shadow_type(GTK_FRAME(profileframe), GTK_SHADOW_NONE);
  label = gtk_frame_get_label_widget(GTK_FRAME(profileframe));
  gtk_label_attr_bold(GTK_LABEL(label));

  alignment = gtk_alignment_new(1.0, 1.0, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 6, 0, 24, 0);
  gtk_container_add(GTK_CONTAINER(profileframe), alignment);
  gtk_widget_show(alignment);
  
  table = gtk_table_new(3, 3, FALSE);
  gtk_container_add(GTK_CONTAINER(alignment), table);
  gtk_widget_show(table);

  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_table_attach(GTK_TABLE(table), scrolled_window, 0, 1, 0, 3,
                   GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
  gtk_widget_show(scrolled_window);

  store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes(
              "Preset Name", renderer, "text", COLUMN_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect_swapped(G_OBJECT(selection), "changed",
                           G_CALLBACK(selection_changed_cb), metro);
  g_signal_connect_swapped(G_OBJECT(tree), "button-press-event",
                           G_CALLBACK(selection_changed_cb), metro);
  metro->profiles_tree = GTK_TREE_VIEW(tree);
  gtk_container_add(GTK_CONTAINER(scrolled_window), tree);
  gtk_widget_show(tree);

  button = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
  gtk_table_attach(GTK_TABLE(table), button, 1, 2, 0, 1,
                   GTK_FILL, GTK_FILL, 5, 5);
  g_signal_connect_swapped(button, "clicked", G_CALLBACK(up_cb), metro);
  gtk_widget_show(button);
  button = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
  gtk_table_attach(GTK_TABLE(table), button, 1, 2, 1, 2,
                   GTK_FILL, GTK_FILL, 5, 5);
  g_signal_connect_swapped(button, "clicked", G_CALLBACK(down_cb), metro);
  gtk_widget_show(button);
  button = gtk_button_new_from_stock(GTK_STOCK_SAVE);
  gtk_table_attach(GTK_TABLE(table), button, 1, 2, 2, 3,
                   GTK_FILL, GTK_FILL, 5, 5);
  g_signal_connect_swapped(button, "clicked", G_CALLBACK(save_cb), metro);
  gtk_widget_show(button);
  button = gtk_button_new_from_stock(GTK_STOCK_ADD);
  gtk_table_attach(GTK_TABLE(table), button, 2, 3, 0, 1,
                   GTK_FILL, GTK_FILL, 5, 5);
  g_signal_connect_swapped(button, "clicked", G_CALLBACK(add_cb), metro);
  gtk_widget_show(button);
  button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
  gtk_table_attach(GTK_TABLE(table), button, 2, 3, 1, 2,
                   GTK_FILL, GTK_FILL, 5, 5);
  g_signal_connect_swapped(button, "clicked", G_CALLBACK(delete_cb), metro);
  gtk_widget_show(button);
  button = gtk_button_new_from_stock(GTK_STOCK_EDIT);
  gtk_table_attach(GTK_TABLE(table), button, 2, 3, 2, 3,
                   GTK_FILL, GTK_FILL, 5, 5);
  g_signal_connect_swapped(button, "clicked", G_CALLBACK(edit_cb), metro);
  gtk_widget_show(button);
  
  option_register(&metro->options->option_list,
                  "ShowProfiles",
		  (option_new_t) new_show_profiles,
		  (option_delete_t) delete_show_profiles,
		  (option_set_t) set_show_profiles,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_show_profiles,
		  (void*) metro);
  option_register(&metro->options->option_list,
                  "Profile_",
		  (option_new_t) new_profile,
		  (option_delete_t) delete_profile,
		  (option_set_t) set_profile,
		  (option_get_n_t) get_n_profile,
		  (option_get_t) get_profile,
		  (void*) metro);
  option_register(&metro->options->option_list,
                  "ProfileSelected",
		  (option_new_t) new_profile_selected,
		  (option_delete_t) delete_profile_selected,
		  (option_set_t) set_profile_selected,
		  (option_get_n_t) option_return_one,
		  (option_get_t) get_profile_selected,
		  (void*) metro);

  return profileframe;
}

