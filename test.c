#include <stdio.h>
#include <SDL/SDL.h>

#include "ymf262.h"

SDL_AudioSpec sdl_audio_spec = {0};
int sdl_sample_rate = 48000;
int sdl_samples = 4096;
int sample_frame_size = 0;
int ymf262_device_id = 0;

#define OPL3_SAMPLE_BITS   16
#define OPL3_INTERNAL_FREQ 14400000

char* ymf262_sample_buffer;
char* ymf262_sample_buffer_channel[18] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

void oml3_out(short reg, short data) {
  int op = 0;
  if (reg > 255) {
    op = 2;
    reg &= 0xff;
  }

  YMF262Write(ymf262_device_id, op,   reg);
  YMF262Write(ymf262_device_id, op+1, data);
}

void oml3_exp(short data) {
  YMF262Write(ymf262_device_id, 2, data & 0xff);
  YMF262Write(ymf262_device_id, 3, data >> 8);
}

void callback(
    void*  userdata,
    Uint8* stream,
    int    length) {
  char* buffer_table[18];

  // For each sample:
  for (int counter = 0; counter < (length / 4); counter++) {
    // Update partial channel samples
    for (int i = 0; i < 18; i++) {
      buffer_table[i] = ymf262_sample_buffer_channel[i] + (counter * 4);
    }

    // Update one step
    YMF262UpdateOne(ymf262_device_id, (short*)(ymf262_sample_buffer + (counter * 4)), (short**)&buffer_table, 1);
  }

  // Copy buffer
  memcpy(ymf262_sample_buffer, stream, length);
}

int uninit() {
  // Uninit YMF Module
  YMF262Shutdown();

  // Uninit SDL
  SDL_PauseAudio(1);
  SDL_CloseAudio();

  // Deallocate
  if (ymf262_sample_buffer != NULL) {
    free(ymf262_sample_buffer);
    ymf262_sample_buffer = NULL;
  }

  for (int i = 0; i < 18; i++) {
    if (ymf262_sample_buffer_channel[i] != NULL) {
      free(ymf262_sample_buffer_channel[i]);
      ymf262_sample_buffer_channel[i] = NULL;
    }
  }
}

int init() {
  // Create buffers
  sample_frame_size = sdl_sample_rate / 50;
  ymf262_sample_buffer = malloc(sdl_samples * 4);

  if (ymf262_sample_buffer == NULL) {
    // Out of memory
    uninit();
    return -1;
  }

  for (int i = 0; i < 18; i++) {
    ymf262_sample_buffer_channel[i] = malloc(sdl_samples * 4);
    if (ymf262_sample_buffer_channel[i] == NULL) {
      // Out of memory
      uninit();
      return -1;
    }
  }

  // Initialize the YMF Module
  ymf262_device_id = YMF262Init(1, OPL3_INTERNAL_FREQ, sdl_sample_rate);
  YMF262ResetChip(ymf262_device_id);

  // Initialize SDL Audio
  sdl_audio_spec.freq = sdl_sample_rate;
  sdl_audio_spec.format = AUDIO_S16;
  sdl_audio_spec.channels = 2;
  sdl_audio_spec.samples = sdl_samples;
  sdl_audio_spec.callback = &callback;
  sdl_audio_spec.userdata = NULL;

  SDL_AudioSpec have = {0};

  int ret = SDL_OpenAudio(&sdl_audio_spec, &have);

  if (ret < 0) {
    // Error initializing audio
    return -1;
  }

  SDL_PauseAudio(0);

  return 0;
}

int main(int argc, char** argv) {
  int initialized = init();
  if (initialized != 0) {
    printf("error\n");
    return -1;
  }

  SDL_Delay(5000);

  uninit();

  return 0;
}
