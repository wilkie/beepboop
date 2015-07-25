#include <stdio.h>
#include <SDL/SDL.h>

#define USE_DOSBOX_OPL

#ifdef USE_DOSBOX_OPL
#include "dbopl.hpp"
DBOPL::Handler dbopl_device;
#elif defined(USE_YMF262_OPL)
#define OPL3_INTERNAL_FREQ 14318180
#include "ymf262.h"
int ymf262_device_id = 0;
#else
#include "fmopl.h"
FM_OPL* fmopl_device;
#endif

void playPair(FILE* f);

unsigned int del = 0;
unsigned int speed = 0;

typedef struct {
  FILE* f;
} PlayDetails;

PlayDetails current_song;

SDL_AudioSpec sdl_audio_spec = {0};
int sdl_sample_rate = 49716;
int sdl_samples = 4096;
int sample_frame_size = 0;

#define OPL3_SAMPLE_BITS   16

short* ymf262_sample_buffer;
short* ymf262_sample_buffer_channel[18] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

void opl3_out(short reg, short data) {
#ifdef USE_DOSBOX_OPL
  dbopl_device.WriteReg(reg, data);
#else
  int op = 0;
  if (reg > 255) {
    op = 2;
    reg &= 0xff;
  }
#ifdef USE_YMF262_OPL
  YMF262Write(ymf262_device_id, op,   reg);
  YMF262Write(ymf262_device_id, op+1, data);
#else
  OPLWrite(fmopl_device, op,   reg);
  OPLWrite(fmopl_device, op+1, data);
#endif
#endif
}

float current_time = 0.0f;

void callback(
    void*  userdata,
    Uint8* stream,
    int    length) {

#ifdef USE_DOSBOX_OPL
  DBOPL::OPLMixer oplmixer((int16_t*)stream);
#endif

  short* buffer_table[18];

  static long minicnt = 0;

  long towrite = length / 4;

  short* stream_pos = (short*)stream;

  while(towrite > 0) {
    while(minicnt < 0) {
      minicnt += sdl_sample_rate;
      if (current_song.f != NULL) {
        playPair(current_song.f);
      }
    }

    float refresh = 1193180.0 / (speed ? speed : 0xffff);

    long i = (long)(minicnt / refresh + 4) & ~3;
    if (towrite < i) {
      i = towrite;
    }

#ifdef USE_DOSBOX_OPL
    dbopl_device.Generate(&oplmixer, i);
#elif defined(USE_YMF262_OPL)
    YMF262UpdateOne(ymf262_device_id, stream_pos, NULL, i);
#else
    YM3812UpdateOne(fmopl_device, stream_pos, i);
    // Duplicate channels
    for (long p = i; p>0; p--) {
      stream_pos[p*2-1] = stream_pos[p-1];
      stream_pos[p*2-2] = stream_pos[p-1];
    }
#endif
    stream_pos += i * 2;
    towrite -= i;

    minicnt -= (long)(refresh * i);
  }
}

int uninit() {
  // Uninit SDL
  SDL_PauseAudio(1);
  SDL_CloseAudio();

  // Uninit YMF Module
#ifdef USE_DOSBOX_OPL
#elif defined(USE_YMF262_OPL)
  YMF262Shutdown();
#else
  OPLDestroy(fmopl_device);
#endif

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

  if (current_song.f != NULL) {
    fclose(current_song.f);
  }
}

int init() {
  // Create current playing song structure
  current_song.f = NULL;

  // Create buffers
  ymf262_sample_buffer = (short*)malloc(sdl_samples * 4);

  if (ymf262_sample_buffer == NULL) {
    // Out of memory
    uninit();
    return -1;
  }

  for (int i = 0; i < 18; i++) {
    ymf262_sample_buffer_channel[i] = (short*)malloc(sdl_samples * 4);
    if (ymf262_sample_buffer_channel[i] == NULL) {
      // Out of memory
      uninit();
      return -1;
    }
  }

  // Initialize the YMF Module
#ifdef USE_DOSBOX_OPL
  dbopl_device.Init(sdl_sample_rate);
#elif defined(USE_YMF262_OPL)
  ymf262_device_id = YMF262Init(1, OPL3_INTERNAL_FREQ, sdl_sample_rate);
  YMF262ResetChip(ymf262_device_id);
#else
  fmopl_device = OPLCreate(OPL_TYPE_YM3812, 3579545, sdl_sample_rate);
  OPLResetChip(fmopl_device);
#endif

  // 4-waveform Select (paranoia)
  opl3_out(1, 32);

  // Enable OPL3
  //opl3_out(105, 1);

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

  printf("opened device, sample rate: %d, samples: %d\n", have.freq, have.samples);
  sdl_sample_rate = have.freq;
  sdl_samples = have.samples;

  return 0;
}

void setchip(const char num) {
  printf("chip??\n");
}

void playPair(FILE* f) {
  unsigned int setspeed = 0;

  unsigned char param = 0;
  unsigned char command = 0;

  if(del) {
    del--;
    return;
  }

  do {
    setspeed = 0;

    fread(&param, 1, 1, f);
    fread(&command, 1, 1, f);

    switch(command) {
    case 0:
      del = param - 1;
      if (param == 0) {
        del = 255;
      }
      break;
    case 2:
      if(!param) {
        speed = param + (command << 8);
        setspeed = 1;
      } else
        setchip(param - 1);
      break;
    case 0xff:
      if(param == 0xff) {
        printf("END OF SONG\n");
        return; // End of song
      }
      break;
    default:
      opl3_out(command, param);
      break;
    }
  } while((command || setspeed) && !feof(f));
}

void playRAW(const char* filename) {
  FILE* f = fopen(filename, "r");

  if (f == NULL) {
    return;
  }

  char signature[9] = {0};

  fread(&signature, 8, 1, f);
  if(strncmp(signature,"RAWADATA",8)) {
    fclose(f);
    printf("Error loading RAW.\n");
    return;
  }

  // load section
  unsigned short clock;
  fread(&clock, 2, 1, f);
  speed = clock;

  current_song.f = f;
}

int main(int argc, char** argv) {
  int initialized = init();
  if (initialized != 0) {
    printf("Error.\n");
    return -1;
  }

  if (argc > 1) {
    playRAW(argv[1]);
  }

  // Play
  SDL_PauseAudio(0);

  // Wait until it finishes
  SDL_Delay(1000000);

  uninit();

  return 0;
}
