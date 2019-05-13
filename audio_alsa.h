#ifndef _AUDIO_ALSA_H_
#define _AUDIO_ALSA_H_

//#define ENABLE_ALSA

#include "config.h"

#ifdef ENABLE_ALSA
#include <alsa/asoundlib.h>
//#else
//#include <jack/jack.h>
//#include "../libspeex/speex_resampler.h"
#endif

#ifdef ENABLE_ALSA

#define	PCM_PERIOD			(32)
#define PCM_BUFFER			(32*160)
#define PCM_THRESHOLD		(8*160)
//#define PCM_THRESHOLD		(30*160)


typedef struct {
	uint32_t mode;
	uint32_t rate;
	uint32_t format;
	// private
	snd_pcm_t *handle[2];	// 1 : playback , 0 : capture
} AUD_OBJ;


int aud_get_avail(AUD_OBJ *p, int dir);
int aud_open(AUD_OBJ *p, const char *name, int mode, int rate);
int aud_close(AUD_OBJ *p);
int aud_start(AUD_OBJ *p);
int aud_get_stream(AUD_OBJ *p, char *buf, int len, int *avail); 
int aud_put_stream(AUD_OBJ *p, char *buf, int len, int *avail);

#endif

#endif
