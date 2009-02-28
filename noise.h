#ifndef NOISE_HEADER_AJLSDFJAO8GO38HGOHIHEIOBEB
#define NOISE_HEADER_AJLSDFJAO8GO38HGOHIHEIOBEB
#include <SDL.h>

typedef struct {
  float freq;
  Uint16 time;
} TNoise;

void make_noise(float freq, int duration);
void noise_play(TNoise * n);
void noise_callback(void *data, Uint8 *buf, int len);

#endif

