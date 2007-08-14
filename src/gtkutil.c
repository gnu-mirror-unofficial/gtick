/* 
 * GTK utility functions
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

/* GNU headers */
#include <stdio.h>
#include <string.h>

/* GTK headers*/
#include <gtk/gtk.h>
#include <glib.h>

/* own headers */
#include "globals.h"
#include "metro.h"

#include "gtkutil.h"

/*
 * return a dialog window with a (big) label and an ok button to close
 * (good for displaying a note)
 *
 * justification: justification of label (e.g. GTK_JUSTIFY_LEFT)
 *
 * NOTE: caller has to show the window itself with gtk_widget_show()
 *       and maybe want to make it modal with
 *         gtk_window_set_modal(GTK_WINDOW(window), TRUE);
 */
GtkWidget *get_ok_dialog(const char *title, const char *contents,
			 GtkJustification justification) {
  GtkWidget *window;
  GtkWidget *button;
  GtkWidget *label;

  window = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(window), title);

  /* vbox area */
  gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(window)->vbox), 0);
  label = gtk_label_new(contents);
  gtk_label_set_justify(GTK_LABEL(label), justification);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), label,
		     TRUE, FALSE, 0);
  gtk_misc_set_padding(GTK_MISC(label), 10, 10);
  gtk_widget_show(label);

  /* OK button */
  button = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_box_pack_start_defaults(
      GTK_BOX(GTK_CONTAINER(GTK_DIALOG(window)->action_area)), button);
  g_signal_connect_swapped(G_OBJECT(button),
                           "clicked",
			    G_CALLBACK(gtk_widget_destroy),
			    window);
  gtk_widget_show(button);

  /* caller has to show the window itself */

  return window;
}

/*
 * Make the specified label bold
 */
void gtk_label_attr_bold(GtkLabel* label) {
  PangoAttribute *bold_attr;
  PangoAttrList *bold_attr_list;

  bold_attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
  bold_attr->start_index = 0;
  bold_attr->end_index = strlen(gtk_label_get_text(label));
  bold_attr_list = pango_attr_list_new();
  pango_attr_list_insert(bold_attr_list, bold_attr);

  gtk_label_set_attributes(label, bold_attr_list);
}

