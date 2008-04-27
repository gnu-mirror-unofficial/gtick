/*
 * Visual Tick
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
#include <math.h>

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* own headers */
#include "globals.h"
#include "metro.h"
#include "visualtick.h"
#include "util.h"

/*
 * Draws a (filled) rectangle in a pixmap (convenience function)
 */
static void draw_rectangle(GdkPixmap* pixmap,
                           int x, int y, int width, int height,
                           const char* c)
{
  GdkGC* gc = gdk_gc_new(pixmap);
  GdkColor color;

  if (!gdk_color_parse(c, &color))
    printf("color parse error.\n");
  gdk_gc_set_rgb_fg_color(gc, &color);
  
  gdk_draw_rectangle(pixmap, gc, TRUE, x, y, width, height);

  g_object_unref(G_OBJECT(gc));
}

/*
 * Draws a rectangle outline in a pixmap (convenience function)
 */
static void draw_rectangle_outline(GdkPixmap* pixmap,
                           int x, int y, int width, int height,
                           const char* c)
{
  GdkGC* gc = gdk_gc_new(pixmap);
  GdkColor color;

  if (!gdk_color_parse(c, &color))
    printf("color parse error.\n");
  gdk_gc_set_rgb_fg_color(gc, &color);
  
  gdk_draw_rectangle(pixmap, gc, FALSE, x, y, width, height);

  g_object_unref(G_OBJECT(gc));
}

/*
 * updates the count display
 */
static void count_update(metro_t* metro, unsigned int old, unsigned int new) {
  int maxcount = gui_get_meter(metro);
  char color[] = "#000000";
  int redflag;
  int width;
  int height;
  GdkRectangle inval;
  
  gdk_drawable_get_size(metro->visualtick_count->window, &width, &height);
  draw_rectangle(metro->visualtick_count_pixmap,
      inval.x = (width - 1) * old / maxcount + 1,
      inval.y = 1,
      inval.width = (width - 1) * (old + 1) / maxcount
		  - (width - 1) * old / maxcount - 1,
      inval.height = height - 2,
      color);
  gdk_window_invalidate_rect(metro->visualtick_count->window, &inval, TRUE);

  redflag = gtk_toggle_button_get_active(
	      GTK_TOGGLE_BUTTON(metro->accentbuttons[new]));
  snprintf(color, 8, "#%02X%02X00", redflag * 255, (1 - redflag) * 255);
  draw_rectangle(metro->visualtick_count_pixmap,
      inval.x = (width - 1) * new / maxcount + 1,
      inval.y = 1,
      inval.width = (width - 1) * (new + 1) / maxcount
		  - (width - 1) * new / maxcount - 1,
      inval.height = height - 2,
      color);
  gdk_window_invalidate_rect(metro->visualtick_count->window, &inval, TRUE);
}

/*
 * Called when the count widget is instanciated
 */
static gboolean count_realize_cb(metro_t* metro _U_) {
  if (debug)
    fprintf(stderr, "Virtual Tick / count: realize signal\n");
  return TRUE;
}

/*
 * To be called when a new meter is set up
 */
void visualtick_new_meter(metro_t* metro, int meter) {
  if (GTK_WIDGET_VISIBLE(metro->visualtick)) {
    int width;
    int height;
    int i;
    GdkRectangle inval;

    gdk_drawable_get_size(metro->visualtick_count->window, &width, &height);
    draw_rectangle(metro->visualtick_count_pixmap,
	inval.x = 0, inval.y = 0, inval.width = width, inval.height = height,
	"#000000");
    for (i = 0; i < meter; i++) {
      draw_rectangle_outline(metro->visualtick_count_pixmap,
	  (width - 1) * i / meter, 0,
	  (width - 1) * (i + 1) / meter - (width - 1) * i / meter,
	  height - 1,
	  "#0000FF");
    }
    gdk_window_invalidate_rect(metro->visualtick_count->window, &inval, TRUE);
    
    count_update(metro, metro->visualtick_sync_pos, metro->visualtick_sync_pos);
  }
}

/*
 * Called upon creation and resize of count widget
 */
static gboolean count_configure_cb(metro_t* metro, GdkEventConfigure *event) {
  int width;
  int height;
  
  if (debug)
    fprintf(stderr,
	"Virtual Tick / count: configure event: (%d, %d), %d x %d\n",
	event->x, event->y, event->width, event->height);

  if (metro->visualtick_count_pixmap)
    g_object_unref(G_OBJECT(metro->visualtick_count_pixmap));
  gdk_drawable_get_size(metro->visualtick_count->window, &width, &height);
  metro->visualtick_count_pixmap =
    gdk_pixmap_new(metro->visualtick_count->window, width, height, -1);
 
  visualtick_new_meter(metro, gui_get_meter(metro));
  
  return TRUE;
}

/*
 * Called when something in the count window has been changed
 */
static gboolean count_expose_cb(metro_t* metro, GdkEventExpose *event) {

  if (debug)
    fprintf(stderr, "Virtual Tick / count: expose event: (%d, %d), %d x %d\n",
        event->area.x, event->area.y, event->area.width, event->area.height);
  
  gdk_draw_drawable(metro->visualtick_count->window,
    metro->visualtick_count->style->fg_gc[GTK_STATE_NORMAL],
    metro->visualtick_count_pixmap,
    event->area.x, event->area.y,
    event->area.x, event->area.y, event->area.width, event->area.height);
  
  return TRUE;
}

/*
 * Called when the slider widget is instanciated
 */
static gboolean slider_realize_cb(metro_t* metro _U_) {
  if (debug >= 2)
    fprintf(stderr, "Virtual Tick / slider: realize signal\n");
  return TRUE;
}

/*
 * Called upon creation and resize of slider widget
 */
static gboolean slider_configure_cb(metro_t* metro, GdkEventConfigure *event) {
  int width;
  int height;
  
  if (debug >= 2)
    fprintf(stderr,
	"Virtual Tick / slider: configure event: (%d, %d), %d x %d\n",
	event->x, event->y, event->width, event->height);

  if (metro->visualtick_slider_pixmap)
    g_object_unref(G_OBJECT(metro->visualtick_slider_pixmap));
  gdk_drawable_get_size(metro->visualtick_slider->window, &width, &height);
  metro->visualtick_slider_pixmap =
    gdk_pixmap_new(metro->visualtick_slider->window, width, height, -1);
 
  /* black background for sliding tick */
  draw_rectangle(metro->visualtick_slider_pixmap,
      0, 0, width, height, "#000000");
  
  return TRUE;
}

/*
 * Called when something in the slider window has been changed
 */
static gboolean slider_expose_cb(metro_t* metro, GdkEventExpose *event) {

  if (debug >= 2)
    fprintf(stderr, "Virtual Tick / slider: expose event: (%d, %d), %d x %d\n",
        event->area.x, event->area.y, event->area.width, event->area.height);
  
  gdk_draw_drawable(metro->visualtick_slider->window,
    metro->visualtick_slider->style->fg_gc[GTK_STATE_NORMAL],
    metro->visualtick_slider_pixmap,
    event->area.x, event->area.y,
    event->area.x, event->area.y, event->area.width, event->area.height);
  
  return TRUE;
}

/*
 * Returns the Visual Tick widget and initializes the corresponding
 * parts in metro
 */
GtkWidget* visualtick_init(metro_t* metro) {
  GtkWidget* result;

  result = gtk_vbox_new(FALSE, 16);

  metro->visualtick_count = gtk_drawing_area_new();
  gtk_widget_set_size_request(metro->visualtick_count, -1, 18);
  gtk_box_pack_start_defaults(GTK_BOX(result), metro->visualtick_count);
  gtk_widget_show(metro->visualtick_count);

  g_signal_connect_swapped(G_OBJECT(metro->visualtick_count),
                           "realize",
                           G_CALLBACK(count_realize_cb), metro);
  g_signal_connect_swapped(G_OBJECT(metro->visualtick_count),
                           "configure_event",
                           G_CALLBACK(count_configure_cb), metro);
  g_signal_connect_swapped(G_OBJECT(metro->visualtick_count),
                           "expose_event",
                           G_CALLBACK(count_expose_cb), metro);
  
  metro->visualtick_slider = gtk_drawing_area_new();
  gtk_widget_set_size_request(metro->visualtick_slider, -1, 16);
  gtk_box_pack_start_defaults(GTK_BOX(result), metro->visualtick_slider);
  gtk_widget_show(metro->visualtick_slider);

  g_signal_connect_swapped(G_OBJECT(metro->visualtick_slider),
                           "realize",
                           G_CALLBACK(slider_realize_cb), metro);
  g_signal_connect_swapped(G_OBJECT(metro->visualtick_slider),
                           "configure_event",
                           G_CALLBACK(slider_configure_cb), metro);
  g_signal_connect_swapped(G_OBJECT(metro->visualtick_slider),
                           "expose_event",
                           G_CALLBACK(slider_expose_cb), metro);

  return result;
}

/*
 * Shows the Visual Tick widget and starts the regular update
 */
void visualtick_enable(metro_t* metro) {
  gtk_widget_show(metro->visualtick);

  metro->visualtick_timeout_handler_id =
    gtk_timeout_add(1000.0 * VISUAL_DELAY,
                    (GtkFunction) visualtick_update, metro);
}

/*
 * Stops the regular update of the widget and hides it
 */
void visualtick_disable(metro_t* metro) {
  gtk_timeout_remove(metro->visualtick_timeout_handler_id);
  
  gtk_widget_hide(metro->visualtick);
  gtk_window_resize(GTK_WINDOW(metro->window), 1, 1);
}

/*
 * Called with the tick number when the server signals a new tick
 */
void visualtick_sync(metro_t* metro, unsigned int pos) {
  if (GTK_WIDGET_VISIBLE(metro->visualtick)) {
    count_update(metro, metro->visualtick_sync_pos, pos);
    
    gettimeofday(&metro->visualtick_sync_time, NULL);
    metro->visualtick_sync_pos = pos;
    metro->visualtick_ticks ++;

    visualtick_update(metro);
  }
}

/*
 * Called to update the widget
 */
gboolean visualtick_update(metro_t* metro) {
  double pos;
  struct timeval current_time;
  struct timeval timediff;
  int meter;
  double integer;
  int width;
  int height;
  double red;
  double green;
  double red0;
  double red1;
  char color[] = "#000000";
  GdkRectangle inval;
  int x0;
  int x1;

  if (metro->state == STATE_RUNNING) {
    gettimeofday(&current_time, NULL);
    timeval_subtract(&timediff, &current_time, &metro->visualtick_sync_time);
    pos = (double) metro->visualtick_sync_pos +
	      (timediff.tv_usec * 0.000001 + timediff.tv_sec) *
	      GTK_ADJUSTMENT(metro->speed_adjustment)->value / 60.0;
    meter = gui_get_meter(metro);
    pos = modf(pos / meter, &integer) * meter;

    pos = modf(pos, &integer);
    red0 = gtk_toggle_button_get_active(
	GTK_TOGGLE_BUTTON(metro->accentbuttons[(int) integer])) ?
	  1 : 0;
    red1 = gtk_toggle_button_get_active(
	GTK_TOGGLE_BUTTON(metro->accentbuttons[((int) integer + 1) % meter])) ?
	  1 : 0;
    red = red0 * (1.0 - pos) + red1 * pos;
    green = 1.0 - red;
    red *= 2;
    green *= 2;
    if (red > 1) red = 1;
    if (green > 1) green = 1;
    snprintf(color, 8, "#%02X%02X00",
	     (int)(red * 255), (int)(green * 255));
    
    if (metro->visualtick_ticks % 2)
      pos = 1 - pos;
    gdk_drawable_get_size(metro->visualtick_slider->window, &width, &height);
    draw_rectangle(metro->visualtick_slider_pixmap,
	           x0 = 0.9 * width * metro->visualtick_old_pos, 0,
		   width / 10, height,
		   "#000000");
    draw_rectangle(metro->visualtick_slider_pixmap,
	           x1 = 0.9 * width * pos, 0,
		   width / 10, height,
		   color);
    
    inval.x = x0 < x1 ? x0 : x1;
    inval.y = 0;
    inval.width = width / 10 + abs(x0 - x1);
    inval.height = height;
    
    gdk_window_invalidate_rect(metro->visualtick_slider->window, &inval, TRUE);

    metro->visualtick_old_pos = pos;
  }
  return 1; /* call it again */
}

