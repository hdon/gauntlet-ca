#include <SDL.h>
#include <math.h>
#include "noise.h"

static TNoise noises[128];
static TNoise *next_noise;

static TNoise * noise_new() {
  if (next_noise >= noises + 128) return noises + 127;
  return next_noise++;
}

void noise_play(TNoise * n) {
  TNoise *nn = noise_new();
  nn->freq = n->freq / (22050.0 * M_PI);
  nn->time = n->time;
}

void noise_callback(void *data, Uint8 *buf, int len) {
  static Uint16 immortal_counter = 0;
  TNoise *noise;

  /* initialize buffer */
  memset(buf, 0, len); /* TODO use a real silence value */

  /* any noises happening? */
  if (next_noise == noises) return;

  /* for each noise.. */
  for (noise = next_noise-1; noise >= noises; noise--) {
    float counter = (float)immortal_counter;
    float freq;
    int time, fill, i;

    /* how many samples to fill? */
    time = noise->time;
    if (len < time) fill = time;
    else fill = len;

    /* grab the rest of the noise's attributes */
    freq = noise->freq;

    /* deduct fill time from noise's total time */
    time -= fill;
    /* erase noise if it's finishing */
    if (time == 0) {
      next_noise--;
      if ((noise < noises + 127) && (noise != next_noise))
        memcpy(noise, next_noise, sizeof(TNoise));
    }

    /* fill sample buffer */
    for (i=0; i<fill; i++) {
      buf[i] = sin(counter * (float)freq);
    }
  }

  immortal_counter += len;
}

