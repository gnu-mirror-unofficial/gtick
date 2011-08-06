/*
 * Global definitions
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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <config.h>

#define TITLE "GTick"

/* GNU gettext introduction */
#include <locale.h>
#include "gettext.h"
#define _(String) gettext (String)
#define N_(String) gettext_noop (String)

#define MIN_BPM 10
#define MAX_BPM 1000
#define DEFAULT_BPM 100
#define DEFAULT_MIN_BPM 30
#define DEFAULT_MAX_BPM 250
#define VOLUME_MIN 0
#define VOLUME_MAX 100
#define MAX_METER 100
#define DEFAULT_SOUND_SYSTEM "<pulseaudio>"
#define DEFAULT_SOUND_DEVICE_FILENAME "/dev/dsp"
#define DEFAULT_SAMPLE_FILENAME "<default>"
#define DEFAULT_SPEED 75
#define DEFAULT_VOLUME VOLUME_MAX
#define DEFAULT_METER 1
#define DEFAULT_COMMAND_ON_START ""
#define DEFAULT_COMMAND_ON_STOP ""

/* How often to update the "Visual Tick" */
#define VISUAL_DELAY 0.03

extern int debug;

#endif /* GLOBALS_H */
