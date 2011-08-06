/*
 * DSP interface
 *
 * This file is part of GTick
 *
 *
 * Copyright (c) 1999, Alex Roberts
 * Copyright (c) 2003, 2004, 2005, 2006  Roland Stigge <stigge@antcom.de>
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

#ifndef DSP_H
#define DSP_H

/* GTK headers */
#include <gtk/gtk.h>

#include <pulse/simple.h>

/* own headers */
#include "threadtalk.h"

typedef struct dsp_t {
  char* devicename;
  char* soundname;
  char* soundsystem;

  pa_simple *pas;   /* pa simple playback stream */

  int dspfd;        /* file descriptor */
  int fragmentsize; /* fragment size */
  int fragstotal;   /* number of fragments in DSP buffer */
  int channels;     /* number of channels */
  int rate;         /* number of frames per second in Hz */
  int samplesize;   /* number of bits per item (usually 8 or 16) */
  int format;

  int rate_in;      /* rate of input data in Hz */
  int channels_in;  /* number of channels of input data in Hz */

  unsigned char* fragment;

  unsigned char* tickdata0; /* raw dsp sample bytes for single tick */
  int td0_size;  /* length in bytes */
  unsigned char* tickdata1; /* raw dsp sample bytes for first tick */
  int td1_size;
  unsigned char* tickdata2; /* dsp sample bytes for other ticks */
  int td2_size;

  unsigned char* silence; /* size = channels * samplesize / 8 */

  short* frames;         /* the original frames yet to be scaled by volume */
  int number_of_frames;

  int meter;        /* meter mode */
  double frequency; /* ticking frequency in Hz */
  int cyclepos;     /* current number of tick (0, 1, 2 for 3/4) */
  int tickpos;      /* number of frame in tick */
  int* accents;

  int running;      /* on/off flag */

  double volume;    /* 0.0 ... 1.0 */

  int sync_flag;

  comm_t* inter_thread_comm;
} dsp_t;

dsp_t* dsp_new(comm_t* comm);
void dsp_delete(dsp_t* dsp);

int dsp_open(dsp_t* dsp);
void dsp_close(dsp_t* dsp);
int dsp_init(dsp_t* dsp);
void dsp_deinit(dsp_t* dsp);
gboolean dsp_feed(dsp_t* dsp);

double dsp_get_volume(dsp_t* dsp);
void dsp_set_volume(dsp_t* dsp, double volume);

void dsp_main_loop(dsp_t* dsp);

#endif /* DSP_H */
