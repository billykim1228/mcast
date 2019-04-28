#ifndef _AUDIO_JACK_H_
#define _AUDIO_JACK_H_

#include "config.h"
//#define ENABLE_ALSA

#ifdef ENABLE_ALSA
#include <alsa/asoundlib.h>
#else
#include <jack/jack.h>
#include "../libspeex/speex_resampler.h"
#endif

#ifndef ENABLE_ALSA

typedef struct {
	int mode;
	int rate;
	int format;
	// private
	int base_rate;
	SpeexResamplerState *rso;		// resampler for capture
	SpeexResamplerState *rsi;		// resampler for playback
	int16_t obuf[160];
	int16_t ibuf[160];
	int16_t tbuf[4096];
} AUD_OBJ;


int aud_get_avail(AUD_OBJ *p, int dir);
int aud_open(AUD_OBJ *p, const char *name, int mode, int rate);
int aud_close(AUD_OBJ *p);
int aud_start(AUD_OBJ *p);
int aud_get_stream(AUD_OBJ *p, char *buf, int len, int *avail); 
int aud_put_stream(AUD_OBJ *p, char *buf, int len, int *avail);

#endif

#endif
