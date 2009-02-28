#ifndef NOISE_HEADER_AJLSDFJAO8GO38HGOHIHEIOBEB
#define NOISE_HEADER_AJLSDFJAO8GO38HGOHIHEIOBEB
#include <SDL.h>

typedef struct {
  float freq;
  Uint16 time;
} TNoise;

void noise_play(TNoise * n);

#endif

