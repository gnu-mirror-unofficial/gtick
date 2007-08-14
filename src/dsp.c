/*
 * dsp.c: sound device access code
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* libsndfile */
#ifdef WITH_SNDFILE
#include <sndfile.h>
#endif

/* OSS headers */
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include <sys/soundcard.h>
#else
#include <linux/soundcard.h>
#endif

/* GTK+ headers */
#include <glib.h>

/* own headers */
#include "g711.h"
#include "globals.h"
#include "metro.h"
#include "dsp.h"
#include "option.h"
#include "threadtalk.h"

/* default sampled sound effect */
#include "tickdata.c"

/* time to write ahead at least in milliseconds */
#define WRITE_AHEAD_INTERVAL 100

/* default DSP settings */
#define DEFAULT_RATE 44100
#define DEFAULT_FORMAT AFMT_S16_LE
#define DEFAULT_CHANNELS 1

/* sine generator settings */
#define SIN_FREQ 880.0
#define SIN_DUR 0.01
#define FADE_DUR 0.002

/* OSS sample format type */
typedef struct format_t {
  int format;
  char* name;
  int samplesize;
  char* description;
} format_t;

static format_t formats[] = {
  { AFMT_MU_LAW,    "AFMT_MU_LAW",     8, "8 bit mu-law"                     },
  { AFMT_A_LAW,     "AFMT_A_LAW",      8, "8 bit A-law"                      },
  { AFMT_IMA_ADPCM, "AFMT_IMA_ADPCM",  4, "4 bit IMA ADPCM"                  },
  { AFMT_U8,        "AFMT_U8",         8, "unsigned, 8 bit"                  },
  { AFMT_S16_LE,    "AFMT_S16_LE",    16, "signed, 16 bit (little endian)"   },
  { AFMT_S16_BE,    "AFMT_S16_BE",    16, "signed, 16 bit (big endian)"      },
  /* equals AFMT_S16_LE or AFMT_S16_BE, shouldn't be reported: */
  { AFMT_S16_NE,    "AFMT_S16_NE",    16, "signed, 16 bit (local CPU endian)"},
  /* not supported by default soundcard.h
  { AFMT_S32_LE,    "AFMT_S32_LE",    32, "signed, 32 bit (little endian)"   },
  { AFMT_S32_BE,    "AFMT_S32_BE",    32, "signed, 32 bit (big endian)"      },
  */
  { AFMT_U16_LE,    "AFMT_U16_LE",    16, "unsigned, 16 bit (little endian)" },
  { AFMT_U16_BE,    "AFMT_U16_BE",    16, "unsigned, 16 bit (big endian)"    },

  { 0,              "unknown",        0,  "???"                              }
};

/*
 * returns new dsp object
 */
dsp_t* dsp_new(comm_t* comm) {
  dsp_t* result;
  
  result = (dsp_t*) g_malloc0(sizeof(dsp_t));
  result->dspfd = -1;
  comm_server_register(comm);
  result->inter_thread_comm = comm;

  return result;
}

/*
 * destroys dsp object
 */
void dsp_delete(dsp_t* dsp) {
  comm_server_unregister(dsp->inter_thread_comm);
  if (dsp->devicename) free(dsp->devicename);
  if (dsp->soundname) free(dsp->soundname);
  free(dsp);
}

/*
 * Writes the specified <sample> (16 bit signed) to raw buffer <dest> with
 * specified <format> (see formats, above)
 */
static void encode_sample(short sample, int format, unsigned char* dest)
{
  static int error = 0;
    
  switch (format) {
  case AFMT_MU_LAW:
    *dest = linear2ulaw(sample);
    break;
  case AFMT_A_LAW:
    *dest = linear2alaw(sample);
    break;
  case AFMT_IMA_ADPCM:
    if (!error) {
      fprintf(stderr, "NOTE: Can't generate samples due to still unsupported "
	              "format (AFMT_IMA_ADPCM).\n");
      error = 1;
    }
    *dest = 0;
    break;
  case AFMT_U8:
    *dest = (unsigned char) (sample / 256 + 128);
    break;
  case AFMT_S16_LE:
    *dest = (unsigned char) (sample & 0xff);
    *(dest + 1) = (unsigned char) (sample >> 8 & 0xff);
    break;
  case AFMT_S16_BE:
    *dest = (unsigned char) (sample >> 8 & 0xff);
    *(dest + 1) = (unsigned char) (sample & 0xff);
    break;
  case AFMT_U16_LE:
    *dest = (unsigned char) ((sample ^ 0x8000) & 0xff);
    *(dest + 1) = (unsigned char) ((sample ^ 0x8000) >> 8 & 0xff);
    break;
  case AFMT_U16_BE:
    *dest = (unsigned char) ((sample ^ 0x8000) >> 8 & 0xff);
    *(dest + 1) = (unsigned char) ((sample ^ 0x8000) & 0xff);
    break;
  default:
    if (!error) {
      fprintf(stderr,
	      "NOTE: Can't generate samples due to unsupported format.\n");
      error = 1;
    }
  }
}

/*
 * returns x if x < limit, else (limit - 1); thus it returns an exclusively
 * limited value
 */
static int limit_int(int x, int limit) {
  if (x < limit)
    return x;
  else
    return limit - 1;
}

/*
 * Generates Audio data fit for playback on initialized DSP, parameters of
 * which are stored in dsp
 *
 * input:
 *     from:          the sample data source in signed 16 bit format
 *     from_size:     number of frames in <from>
 *     from_rate:     the rate of <from>
 *     from_channels: number of channels in <from>
 *     dsp:           the dsp_t structure holding rate, channels, samplesize
 *                    and format of the initialized dsp
 *     
 * output:
 *     to:            the pointer to the allocated data, ready for playback
 *     return value:  number of bytes generated, -1 on error
 *
 * NOTES:
 *  - If fromchannels equals the number of channels of playback device,
 *    no mixing is performed.
 *    Else (fromchannels != channels), the input is mixed down to 1 channel
 *    which is equally directed to all output channels
 *  - <to> will be allocated by generate_data, but has to be free()d by caller
 */
static int generate_data(short* from,
                         int from_size, int from_rate, int from_channels,
                         dsp_t* dsp, unsigned char** to)
{
  double speed_factor = (double) from_rate / dsp->rate; /* output to input */
  int result_size = (long long)from_size * dsp->rate / from_rate; /* number of frames */
  unsigned char* result;
  int i, j;

  result = (unsigned char*)
	  g_malloc(result_size * dsp->samplesize / 8 * dsp->channels);

  for (i = 0; i < result_size; i++) { /* for each output frame */
    double mixdown = 0.0; /* only needed if dsp->channels != from_channels */

    if (dsp->channels != from_channels) { /* mixdown */
      double leftbound = i * speed_factor;
      double rightbound = (i + 1) * speed_factor;

      while (leftbound < rightbound) {
	int index = (int)leftbound;
	double dummy;
	double frac = modf(leftbound, &dummy);
        double weight;
	
	if (rightbound - leftbound < 1)
	  weight = rightbound - leftbound;
	else
	  weight = 1.0;
	
        for (j = 0; j < from_channels; j++) {
	  mixdown += weight * (
	      (1.0 - frac) * from[from_channels * index + j] +
	      frac * from[from_channels * limit_int(index + 1, from_size) + j]);
        }

	leftbound += 1.0;
      }
      mixdown /= (speed_factor * from_channels);
    }
    for (j = 0; j < dsp->channels; j++) { /* for each channel */
      double sample;
      
      if (dsp->channels != from_channels) {
	sample = mixdown;
      } else {
        double leftbound = i * speed_factor;
        double rightbound = (i + 1) * speed_factor;

	sample = 0.0;
        while (leftbound < rightbound) {
	  int index = (int)leftbound;
	  double dummy;
	  double frac = modf(leftbound, &dummy);
          double weight;
	
	  if (rightbound - leftbound < 1)
	    weight = rightbound - leftbound;
	  else
	    weight = 1.0;
	
	  sample += weight * (
	      (1.0 - frac) * from[from_channels * index + j] +
	      frac * from[from_channels * limit_int(index + 1, from_size) + j]);

	  leftbound += 1.0;
        }
        sample /= speed_factor;
      }

      encode_sample((short) (sample * dsp->volume), dsp->format,
          &result[(i * dsp->channels + j) * dsp->samplesize / 8]);

    }
  }

  *to = result;
  return result_size * dsp->channels * dsp->samplesize / 8;
}

/*
 * generates sine signal
 *
 * input:   samplefreq: sampling frequency, (e.g. 44100) in Hz
 *          sinfreq:    frequency of sine (e.g. 440.0) in Hz
 *          sigdur:     duration of signal in seconds
 *          fadedur:    duration of fadein / fadeout in seconds
 * output:  samples:    buffer containing the (signed short) samples
 *
 * returns: number of samples returned in <samples>,
 *          -1 on error
 *
 * NOTE: caller has to free *samples himself
 */
int generate_sine(int samplefreq, double sinfreq, double sigdur, double fadedur,
                  short** samples)
{
  int size = samplefreq * sigdur;
  short* s = (short*) g_malloc(size * sizeof(short));
  int scale = -SHRT_MIN < SHRT_MAX ? -SHRT_MIN : SHRT_MAX;
  int i;
  double sample;

  for (i = 0; i < size; i++) {
    sample = sin((double)i / samplefreq * sinfreq * 2 * M_PI);
    if (i < samplefreq * fadedur)
      sample *= (-cos((double)i / (samplefreq * sigdur) * 2 * M_PI) + 1) * 0.5;
    if (i > samplefreq * (sigdur - fadedur))
      sample *=
	(-cos((double)(size - i) / (samplefreq * sigdur) * 2 * M_PI) + 1) * 0.5;
    s[i] = sample * scale;
  }
 
  *samples = s;
  return size;
}

/*
 * returns (unsigned short) samples from specified file
 *
 * input: filename: file to read samples from, must be in a format
 *                  recognized by libsndfile
 * output: samples: the allocated and initialized buffer
 *         rate:    sampling rate (e.g. 44100) in Hz
 *         channels: number of channels in file (usually 1 or 2)
 *
 * returns: number of frames returned in samples,
 *          -1 on error
 *
 * NOTE: caller has to free *samples himself
 */
#ifdef WITH_SNDFILE
int sndfile_get_samples(const char* filename,
                        short** samples, int* rate, int* channels)
{
  short* s;
  SF_INFO sfinfo;
  SNDFILE* sf;
  int frames;
  int bufsize = 1024 * 16; /* number of frames in one read */
  int i = 0; /* counter */
  int result = 1; /* temporary result in read loop */

  sfinfo.format = 0;
  if (!(sf = sf_open(filename, SFM_READ, &sfinfo))) {
    if (debug)
      fprintf(stderr, "Error opening file \"%s\".\n", filename);
    return -1;
  }
  if (!sfinfo.seekable)
    return -1;
 
  frames = sfinfo.frames;
  *rate = sfinfo.samplerate;
  *channels = sfinfo.channels ;

  s = (short*) g_malloc(frames * sizeof(short) * sfinfo.channels);
  
  while (i < frames && result != 0) {
    result = sf_readf_short(sf, &s[i * sfinfo.channels], bufsize);
    i += result;
  }
  
  sf_close(sf);
  *samples = s;
  return frames;
}
#endif /* WITH_SNDFILE */

/*
 * allocates and initializes dsp->frames
 * and initializes dsp->number_of_frames
 * according to dsp->soundname
 *
 * returns 0 on success, -1 otherwise
 */
static int init_sample(dsp_t* dsp) {
  if (!strcmp(dsp->soundname, "<default>")) {
    dsp->frames = (short*) g_malloc(sizeof(tickdata));
    memcpy(dsp->frames, tickdata, sizeof(tickdata));
    dsp->number_of_frames = (signed int) sizeof(tickdata) / sizeof(short);
    dsp->rate_in = 44100;
    dsp->channels_in = 1;
  } else if (!strcmp(dsp->soundname, "<sine>")) {
    dsp->number_of_frames =
      generate_sine(44100, SIN_FREQ, SIN_DUR, FADE_DUR, &dsp->frames);
    dsp->rate_in = 44100;
    dsp->channels_in = 1;
  } else {
#ifdef WITH_SNDFILE
    dsp->number_of_frames = sndfile_get_samples(dsp->soundname, &dsp->frames,
	&dsp->rate_in, &dsp->channels_in);
#else
    fprintf(stderr, "Warning: Unhandled sample name case: \"%s\".\n",
	    dsp->soundname);
#endif /* WITH_SNDFILE */
  }

  if (dsp->number_of_frames == -1)
    return -1;

  return 0;
}
  
/*
 * allocates and initializes dsp->tickdata{0,1,2}
 * and initializes dsp->td{0,1,2}_size
 * according to dsp->frames and dsp->number_of_frames
 *
 * returns 0 on success, -1 otherwise
 */
static int prepare_buffers(dsp_t* dsp) {
  short *tmp_buf; /* temporary buffer for generation of different sounds */
  int i;

  int attack = 0;
  
  /* generate single ticks */
  if ((dsp->td0_size = generate_data(dsp->frames, dsp->number_of_frames,
	  dsp->rate_in, dsp->channels_in, dsp, &dsp->tickdata0)) == -1)
  {
    return -1;
  }

  /* generate first tick */
  if ((dsp->td1_size = generate_data(dsp->frames, dsp->number_of_frames,
	  dsp->rate_in * 2, dsp->channels_in, dsp, &dsp->tickdata1)) == -1)
  {
    return -1;
  }

  /* attack padding for accents */
  while (attack <
      dsp->number_of_frames && abs(dsp->frames[attack]) < SHRT_MAX / 20)
  {
    attack++;
  }

  if (attack < dsp->number_of_frames / 3) {
    unsigned char* newdata;
    int offset;
    
    offset = attack / 2 * dsp->channels * dsp->samplesize / 8;
    if ((newdata = realloc(dsp->tickdata1, dsp->td1_size + offset))) {
      int modul = dsp->channels * dsp->samplesize / 8;
      int i;
      
      dsp->tickdata1 = newdata;
      
      for (i = dsp->td1_size - 1; i >= 0; i--)
	dsp->tickdata1[i + offset] = dsp->tickdata1[i];
      for (i = 0; i < offset; i++)
	dsp->tickdata1[i] = dsp->silence[i % modul];
      
      dsp->td1_size += offset;

      if (debug)
        fprintf(stderr, "Attack padding for accents: %d frames.\n", attack / 2);
    }
  }

  /* generate secondary ticks */
  tmp_buf = (short*) g_malloc(dsp->number_of_frames * sizeof(short));
  for (i = 0; i < dsp->number_of_frames; i++) {
    tmp_buf[i] = dsp->frames[i] / 2;
  }
  if ((dsp->td2_size = generate_data(tmp_buf, dsp->number_of_frames,
	  dsp->rate_in, dsp->channels_in, dsp, &dsp->tickdata2)) == -1)
  {
    return -1;
  }
  free(tmp_buf);

  return 0;
}

/*
 * Opens sound device specified in dsp
 *
 * returns 0 on success, -1 otherwise
 */
int dsp_open(dsp_t* dsp) {
  static int debug_todo = 1;
  unsigned int format_index;
  int requested_format = DEFAULT_FORMAT;
  audio_buf_info info;
  
  dsp->fragmentsize = 0x7fff0008; /* at least request fragment size 2^8=256 */
                                    /* = minimum recommended size */
  if (debug && debug_todo)
    g_print ("dsp_open: Initialising %s ...\n", dsp->devicename);

  /* Initialise sound device */
  if ((dsp->dspfd = open(dsp->devicename, O_WRONLY)) == -1)
    {
      perror(dsp->devicename);
      return -1;
    }

  if (ioctl(dsp->dspfd, SNDCTL_DSP_SETFRAGMENT, &dsp->fragmentsize) == -1) {
    perror("SNDCTL_DSP_SETFRAGMENT");
    return -1;
  }

  /* Query driver for supported formats for debugging convenience */
  if (debug && debug_todo) {
    unsigned int i;
    int mask;
    
    if (ioctl(dsp->dspfd, SNDCTL_DSP_GETFMTS, &mask) == -1) {
      perror("SNDCTL_DSP_GETFMTS");
    }
    g_print("Supported formats:\n");
    for (i = 0; i < sizeof(formats) / sizeof(format_t); i++) {
      if (mask & formats[i].format)
	g_print("  %s (%s)\n", formats[i].name, formats[i].description);
    }
  }

  /* set up output format */
  dsp->format = requested_format;
  if (ioctl (dsp->dspfd, SNDCTL_DSP_SETFMT, &dsp->format) == -1)
    { /* Fatal error */
      perror ("SNDCTL_DSP_SETFMT");
      return -1;
    }
  
  for (format_index = 0;
    formats[format_index].format != 0 &&
      formats[format_index].format != dsp->format;
    format_index++);
  
  if (debug && debug_todo) {
    g_print("dsp_open: Used sample format: %s (%s)\n",
	    formats[format_index].name, formats[format_index].description);
  }

  dsp->samplesize = formats[format_index].samplesize;
		  
  /* Set dsp to default: mono */
  dsp->channels = DEFAULT_CHANNELS;
  if (ioctl (dsp->dspfd, SNDCTL_DSP_CHANNELS, &dsp->channels) == -1) {
    perror("SNDCTL_DSP_CHANNELS");
    return -1;
  }
  if (debug && debug_todo) {
    g_print("dsp_open: Number of channels = %d\n", dsp->channels);
  }
  
  /* Set the DSP rate (in Hz) */
  dsp->rate = DEFAULT_RATE; /* requested default */
  if (ioctl (dsp->dspfd, SNDCTL_DSP_SPEED, &dsp->rate) == -1) {
    perror("SNDCTL_DSP_SPEED");
    return -1;
  }
  if (debug && debug_todo) {
    g_print("dsp_open: Sampling rate = %d\n", dsp->rate);
  }

  /* "verify" fragment size:
  *  let the driver actually calculate the fragment size
  */
  if (ioctl(dsp->dspfd, SNDCTL_DSP_GETBLKSIZE, &dsp->fragmentsize)) {
    perror("SNDCTL_DSP_GETBLKSIZE");
  }

  if (debug && debug_todo)
    g_print ("dsp_open: fragment size = %d\n", dsp->fragmentsize);

  dsp->fragment = g_malloc(dsp->fragmentsize);

  /* get total number of fragments in dsp buffer */
  if (ioctl(dsp->dspfd, SNDCTL_DSP_GETOSPACE, &info) == -1) {
    perror("SNDCTL_DSP_GETOSPACE");
  }
  dsp->fragstotal = info.fragstotal;
  if (debug && debug_todo)
    g_print("dsp_open: Total number of fragments in DSP buffer = %d.\n",
	    dsp->fragstotal);
  
  debug_todo = 0;
  return 0;
}

/*
 * close sound device
 */
void dsp_close(dsp_t* dsp) {
  static int debug_todo = 1;
  
  if (debug && debug_todo)
    g_print ("dsp_close: Closing sound device ...\n");
  if (dsp->dspfd != -1) {
    if (ioctl(dsp->dspfd, SNDCTL_DSP_RESET, 0) == -1) {
      perror("SNDCTL_DSP_RESET");
    }
    close (dsp->dspfd);
    dsp->dspfd = -1;
    if (dsp->fragment) {
      g_free(dsp->fragment);
      dsp->fragment = NULL;
    }
  }

  debug_todo = 0;
}

/*
 * Opens DSP and prepare metronome <dsp> to play
 *
 * returns 0 on success, -1 otherwise
 */
int dsp_init(dsp_t* dsp)
{
  short silencelevel = 0;
 
  if (dsp_open(dsp) == -1)
    return -1;
  
  dsp->frames = NULL;
  dsp->silence = NULL;
  dsp->tickdata0 = NULL;
  dsp->tickdata1 = NULL;
  dsp->tickdata2 = NULL;
  
  /* silence */
  generate_data(&silencelevel, 1, dsp->rate, 1, dsp, &dsp->silence);

  /* allocate and initialize dsp->frames */
  if (init_sample(dsp) == -1) {
    return -1;
  }
  /* allocate and initialize dsp->tickdata{0,1,2} */
  if (prepare_buffers(dsp) == -1) {
    return -1;
  }

  dsp->cyclepos = 0; /* init */
  dsp->tickpos = 0;

  dsp->running = 1;

  return 0;
}

/*
 * close device and clean up
 */
void dsp_deinit(dsp_t* dsp)
{
  dsp->running = 0;
  dsp_close(dsp);
  
  if (dsp->tickdata0) {
    g_free(dsp->tickdata0);
    dsp->tickdata0 = NULL;
  }
  if (dsp->tickdata1) {
    g_free(dsp->tickdata1);
    dsp->tickdata1 = NULL;
  }
  if (dsp->tickdata2) {
    g_free(dsp->tickdata2);
    dsp->tickdata2 = NULL;
  }
  if (dsp->silence) {
    g_free(dsp->silence);
    dsp->silence = NULL;
  }
  if (dsp->frames) {
    g_free(dsp->frames);
    dsp->frames = NULL;
  }
}

/*
 * starts counting tickpos / cyclepos limits are exceeded
 */
static void wrap_position(dsp_t* dsp, int ticklen) {
  unsigned int* reply;
  
  if (dsp->tickpos >= ticklen) {
    dsp->tickpos = 0;
    dsp->cyclepos++;
    if (dsp->cyclepos >= dsp->meter)
      dsp->cyclepos = 0;

    reply = (unsigned int*) g_malloc(sizeof(unsigned int));
    *reply = dsp->cyclepos;
    comm_server_send_response(dsp->inter_thread_comm,
			      MESSAGE_TYPE_RESPONSE_SYNC, reply);
  }
}

/*
 * Feed dsp device with next samples
 *
 * used as output start and callback
 */
gboolean dsp_feed(dsp_t* dsp)
{
  audio_buf_info info; /* OSS structure to obtain buffering parameters */
  int ticklen = rint(dsp->rate / dsp->frequency) *
                dsp->channels * dsp->samplesize / 8;
  unsigned char *the_data; /* pointer to actual buffer */
  int data_size;           /* size of actual buffer */

  int fragments; /* number of fragments yet to write */
  int limit; /* number of fragments we want to have filled */
 
  /* get number of fragments to write to dsp */
  if (ioctl(dsp->dspfd, SNDCTL_DSP_GETOSPACE, &info) == -1) {
    perror("SNDCTL_DSP_GETOSPACE");
  }

  limit =
    dsp->rate * dsp->channels * dsp->samplesize / 8 *
    WRITE_AHEAD_INTERVAL / (1000 * dsp->fragmentsize);
  if (limit < 2) /* we want to have filled at least 2 fragments */
    limit = 2;
  fragments = limit - (info.fragstotal - info.fragments);

  wrap_position(dsp, ticklen);

  /* write as many fragments as possible */
  while (fragments > 0) {
    int i;
    
    /* generate fragment */
    for (i = 0; i < dsp->fragmentsize; i++) {
      int index;
	
      if (dsp->meter == 1) {                 /* single ticks */
	the_data = dsp->tickdata0;
	data_size = dsp->td0_size;
      } else if (dsp->accents[dsp->cyclepos]) {  /* accentuate 1st tick */
	the_data = dsp->tickdata1;
	data_size = dsp->td1_size;
      } else {                                 /* sound of 2nd tick */
	the_data = dsp->tickdata2;
	data_size = dsp->td2_size;
      }

      if ((index = dsp->tickpos) < data_size) { /* tick! */
	dsp->fragment[i] = the_data[index];
      } else { /* silence (between ticks)  */
	dsp->fragment[i] =
	  dsp->silence[i % (dsp->samplesize / 8 * dsp->channels)];
      }

      dsp->tickpos++;
      wrap_position(dsp, ticklen);
    }

    write(dsp->dspfd, dsp->fragment, dsp->fragmentsize);
    fragments--;
  }

  return 1;
}

/*
 * Gets mixer setting
 *
 * returns value from 0.0 to 1.0 on success
 */
double dsp_get_volume(dsp_t* dsp)
{
  return dsp->volume;
}

/*
 * Sets mixer setting
 *
 * accepts a value from 0.0 to 1.0
 */
void dsp_set_volume(dsp_t* dsp, double volume)
{
  dsp->volume = volume;
  if (dsp->running) {
    free(dsp->tickdata0);
    free(dsp->tickdata1);
    free(dsp->tickdata2);
    
    prepare_buffers(dsp);
  }
}

/*
 * the main loop of the metronome
 */
void dsp_main_loop(dsp_t* dsp) {
  struct timespec requested = { (WRITE_AHEAD_INTERVAL / 2) / 1000,
                                ((WRITE_AHEAD_INTERVAL / 2) % 1000) * 1000};
  struct timespec remaining; /* dummy */
  int repeat_flag = 1;

  while (repeat_flag) {
    message_type_t message_type;
    void* message;
    int get_volume = 0;         /* flag */
    void* reply = NULL;
    
    while ((message_type = comm_server_try_get_query(dsp->inter_thread_comm,
	                                             &message))
	   != MESSAGE_TYPE_NO_MESSAGE)
    {
      
      switch (message_type) {
        case MESSAGE_TYPE_STOP_SERVER:
	  repeat_flag = 0;
	  break;
	case MESSAGE_TYPE_SET_DEVICE:
	  if (dsp->devicename) free(dsp->devicename);
	  dsp->devicename = (char*) message;
	  break;
	case MESSAGE_TYPE_SET_SOUND:
	  if (dsp->soundname) free(dsp->soundname);
	  dsp->soundname = (char*) message;
	  break;
	case MESSAGE_TYPE_SET_METER:
	  dsp->meter = *((int*) message);
	  free(message);
	  break;
	case MESSAGE_TYPE_SET_ACCENTS:
	  if (dsp->accents) free(dsp->accents);
	  dsp->accents = (int*) message;
	  break;
	case MESSAGE_TYPE_SET_FREQUENCY:
	  dsp->frequency = *((double*) message);
	  free(message);
	  break;
        case MESSAGE_TYPE_START_METRONOME:
	  if (dsp_init(dsp) == -1) {
            comm_server_send_response(dsp->inter_thread_comm,
		                      MESSAGE_TYPE_RESPONSE_START_ERROR, NULL);
	    dsp_deinit(dsp);
	  }
	  break;
        case MESSAGE_TYPE_STOP_METRONOME:
	  dsp_deinit(dsp);
	  break;
	case MESSAGE_TYPE_START_SYNC:
	  dsp->sync_flag = 1;
	  break;
	case MESSAGE_TYPE_STOP_SYNC:
	  dsp->sync_flag = 0;
	  break;
	case MESSAGE_TYPE_SET_VOLUME:
	  dsp_set_volume(dsp, *((double*) message));
	  free(message);
	  break;
	case MESSAGE_TYPE_GET_VOLUME:
	  get_volume = 1;
	  break;
	default:
	  fprintf(stderr, "Warning: Unhandled message type in audio thread.\n");
      }
    }

    if (get_volume) {
      double volume = dsp_get_volume(dsp);
      
      reply = (double*) g_malloc0 (sizeof(double));
      *((double*)reply) = volume;
      if (*((double*)reply) != -1) {
	comm_server_send_response(dsp->inter_thread_comm,
				  MESSAGE_TYPE_RESPONSE_VOLUME, reply);
      } else {
	free(reply);
      }
    }

    if (dsp->running) { /* metronome running */
      dsp_feed(dsp);
    }
    nanosleep(&requested, &remaining);
  }
}

