/*
 * help.c: Help menu contents code
 *
 * This file is part of GTick
 *
 * Copyright (c) 1999, Alex Roberts
 * Copyright (c) 2003, 2004, 2005, 2006, 2007 Roland Stigge <stigge@antcom.de>
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
#include <sys/ioctl.h>

/* GTK headers*/
#include <gtk/gtk.h>
#include <glib.h>

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* own headers */
#include "globals.h"
#include "metro.h"
#include "help.h"
#include "gtkutil.h"

/* logo */
#include "aboutlogo.xpm"

#define RESPONSE_LICENSE 1

/*
 * callback invoked by response signal in dialog
 */
static void response_cb(GtkDialog *dialog, gint response, options_t* options _U_) {
  switch (response) {
    case RESPONSE_LICENSE:
      license_box();
      break;
    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_OK:
    case GTK_RESPONSE_CLOSE:
      gtk_widget_destroy(GTK_WIDGET(dialog));
      break;
    default:
      fprintf(stderr, "Warning: Unhandled response to dialog: %d\n", response);
  }
}

void about_box(GtkAction *action _U_, metro_t* metro)
{
  GtkWidget* window;
  GtkWidget* vbox;
  GtkWidget* label;

  GdkPixmap* pixmap;
  GdkBitmap* mask;
  GtkWidget* pixmapwidget;

  window = gtk_dialog_new_with_buttons(_("About GTick"), GTK_WINDOW(metro->window),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      _("License"), RESPONSE_LICENSE,
      GTK_STOCK_OK, GTK_RESPONSE_OK,
      NULL);

  g_signal_connect(G_OBJECT(window),
                   "destroy",
		   G_CALLBACK(gtk_widget_destroy),
	           window);

  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show(vbox);

  gtk_widget_realize(window);
  pixmap = gdk_pixmap_create_from_xpm_d(window->window,
  				        &mask,
				        NULL,
				        (gchar**) aboutlogo_xpm);
  pixmapwidget = gtk_pixmap_new(pixmap, mask);
  gtk_box_pack_start(GTK_BOX(vbox), pixmapwidget, TRUE, TRUE, 0);
  gtk_widget_show(pixmapwidget);
 
  label = gtk_label_new (_(
    "Copyright (c) 1999, Alex Roberts <bse@dial.pipex.com>\n"
    "Copyright (c) 2003, 2004, 2005, 2006, 2007 Roland Stigge <stigge@antcom.de>\n"
    /* TRANSLATORS: actually, "Andrés" with acute accent (/) */
    "Logo by Mario Andres Pagella <mapagella@fitzroy-is.com.ar>\n"
    "\n"
    "Homepage:\n"
    "http://www.antcom.de/gtick/\n"
    "\n"
    "Email bug reports, comments, etc. to\n"
    "Developer's mailing list:\n"
    "gtick-devel@gnu.org"
  ));
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
  gtk_widget_show(label);

  g_signal_connect(G_OBJECT(window), "response",
                   G_CALLBACK(response_cb), G_OBJECT(window));

  gtk_widget_show(window);
}

/* the help menu entry callback */
void license_box(void) {
  GtkWidget* window = get_ok_dialog(_("GTick License"),
    _("GTick - The Metronome\n"
      "Copyright (c) 1999, Alex Roberts <bse@dial.pipex.com>\n"
      "Copyright (c) 2003, 2004, 2005, 2006 Roland Stigge <stigge@antcom.de>\n"
      "\n"
      "This program is free software; you can redistribute it and/or\n"
      "modify it under the terms of the GNU General Public License\n"
      "as published by the Free Software Foundation; either version 3\n"
      "of the License, or (at your option) any later version.\n"
      "\n"
      "This program is distributed in the hope that it will be useful,\n"
      "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
      "GNU General Public License for more details.\n"
      "\n"
      "You should have received a copy of the GNU General Public License\n"
      "along with this program; if not, see <http://www.gnu.org/licenses/>."),
      GTK_JUSTIFY_CENTER);

  gtk_widget_show(window);
}

void help_shortcuts(GtkAction *action _U_, metro_t *metro _U_) {
  GtkWidget* window = gtk_dialog_new();
  GtkWidget* button;
  GtkWidget* table;
  GtkWidget* label;
  
  gtk_window_set_title(GTK_WINDOW(window), _("Shortcuts"));

  /* vbox area */
  gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(window)->vbox), 0);
  table = gtk_table_new(15, 2, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 15);
  
  label = gtk_label_new(_("Ctrl-S"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
  
  label = gtk_label_new(_("Ctrl-+"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
  
  label = gtk_label_new(_("Ctrl--"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
  
  label = gtk_label_new(_("Ctrl-1 ... Ctrl-9"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Alt-w"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Alt-q"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
  
  label = gtk_label_new(_("Alt-a"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 6, 7,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Alt-s"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 7, 8,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
  
  label = gtk_label_new(_("Alt-z"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 8, 9,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Alt-x"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 9, 10,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
  
  label = gtk_label_new(_("Shift-w"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 10, 11,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Shift-q"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 11, 12,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
  
  label = gtk_label_new(_("Shift-a"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 12, 13,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Shift-s"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 13, 14,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
  
  label = gtk_label_new(_("Shift-z"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 14, 15,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Shift-x"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 15, 16,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
    
  label = gtk_label_new(_("Start/Stop"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
  
  label = gtk_label_new(_("Speed: Faster"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 1, 2,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
  
  label = gtk_label_new(_("Speed: Slower"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 2, 3,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
  
  label = gtk_label_new(_("Meter"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 3, 4,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
  
  label = gtk_label_new(_("Double Speed"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 4, 5,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Half Speed"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 5, 6,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
	
  label = gtk_label_new(_("Speed: -10"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 6, 7,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Speed: +10"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 7, 8,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
	
  label = gtk_label_new(_("Speed: -2"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 8, 9,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Speed: +2"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 9, 10,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Double Volume"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 10, 11,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Half Volume"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 11, 12,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
	
  label = gtk_label_new(_("Volume: -10"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 12, 13,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Volume: +10"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 13, 14,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
	
  label = gtk_label_new(_("Volume: -2"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 14, 15,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);

  label = gtk_label_new(_("Volume: +2"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, 15, 16,
                   GTK_EXPAND | GTK_FILL, 0, 10, 5);
  gtk_widget_show(label);
  
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), table, TRUE, FALSE, 0);
  gtk_widget_show(table);

  /* OK button */
  button = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_box_pack_start_defaults(
      GTK_BOX(GTK_CONTAINER(GTK_DIALOG(window)->action_area)), button);
  g_signal_connect_swapped(G_OBJECT(button),
                           "clicked",
			    G_CALLBACK(gtk_widget_destroy),
			    window);
  gtk_widget_show(button);

  gtk_widget_show(window);
}

