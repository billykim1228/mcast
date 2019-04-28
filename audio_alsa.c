#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/time.h>
  
#include "audio_alsa.h"

#ifdef ENABLE_ALSA

//
// Audio module
//

int aud_open_device(AUD_OBJ *p, const char *name, int mode, int rate)
{
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	snd_pcm_uframes_t buffer_size = PCM_BUFFER;
	const char *dirname = (mode == 1) ? "PLAYBACK" : "CAPTURE";
	const int dir =  (mode == 1) ? SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE;
	int err;
	
	p->mode = mode;
	p->rate = rate;
	p->format = SND_PCM_FORMAT_S16_LE;//format;

	//if ((err = snd_pcm_open(&(p->handle), name, dir, 0)) < 0) {
	if ((err = snd_pcm_open(&(p->handle[mode]), name, dir, SND_PCM_NONBLOCK)) < 0) {
		fprintf(stderr, "%s (%s): cannot open audio device (%s)\n", 
			name, dirname, snd_strerror(err));
		return err;
	}
	   
	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		fprintf(stderr, "%s (%s): cannot allocate hardware parameter structure(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}
			 
	if ((err = snd_pcm_hw_params_any(p->handle[mode], hw_params)) < 0) {
		fprintf(stderr, "%s (%s): cannot initialize hardware parameter structure(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_access(p->handle[mode], hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf(stderr, "%s (%s): cannot set access type(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_format(p->handle[mode], hw_params, p->format)) < 0) {
		fprintf(stderr, "%s (%s): cannot set sample format(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_rate_near(p->handle[mode], hw_params, &rate, NULL)) < 0) {
		fprintf(stderr, "%s (%s): cannot set sample rate(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}


	if ((err = snd_pcm_hw_params_set_periods(p->handle[mode], hw_params, PCM_PERIOD, 0)) < 0) {
		fprintf(stderr, "%s (%s): cannot set period(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
    }

	if ((err = snd_pcm_hw_params_set_buffer_size_near(p->handle[mode], hw_params, &buffer_size)) < 0) {
		fprintf(stderr, "%s (%s): cannot set buffer size(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
    }

	if ((err = snd_pcm_hw_params_set_channels(p->handle[mode], hw_params, 1)) < 0) {
		fprintf(stderr, "%s (%s): cannot set channel count(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params(p->handle[mode], hw_params)) < 0) {
		fprintf(stderr, "%s (%s): cannot set parameters(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

#if 0
	{
		snd_pcm_uframes_t period;

		if (err = snd_pcm_hw_params_get_period_size_min(hw_params, &period, NULL)) {
			fprintf(stderr, "%s (%s): cannot set parameters(%s)\n",
				name, dirname, snd_strerror(err));
		} else {
			printf("period size : %d\n", period);
		}
	}
#endif


	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
		fprintf(stderr, "%s (%s): cannot allocate software parameters structure(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}
	if ((err = snd_pcm_sw_params_current(p->handle[mode], sw_params)) < 0) {
		fprintf(stderr, "%s (%s): cannot initialize software parameters structure(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

#if 0
	if ((err = snd_pcm_sw_params_set_avail_min(*handle, sw_params, BUFSIZE/128)) < 0) {
		fprintf(stderr, "%s (%s): cannot set minimum available count(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

#endif

    if (dir == SND_PCM_STREAM_PLAYBACK) {
		if ((err = snd_pcm_sw_params_set_start_threshold(p->handle[mode], sw_params, PCM_THRESHOLD)) < 0) {
			fprintf(stderr, "%s (%s): cannot set threshold(%s)\n",
				name, dirname, snd_strerror(err));
			return err;
		}
		/*
		if ((err = snd_pcm_sw_params_set_avail_min(p->handle[mode], sw_params, PCM_THRESHOLD)) < 0) {
			fprintf(stderr, "%s (%s): cannot set avail min(%s)\n",
				name, dirname, snd_strerror(err));
			return err;
		}
		*/
    }

	if ((err = snd_pcm_sw_params(p->handle[mode], sw_params)) < 0) {
		fprintf(stderr, "%s (%s): cannot set software parameters(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

	return 0;

}

int aud_open(AUD_OBJ *p, const char *name, int mode, int rate)
{
	int ret;
	ret = aud_open_device(p, name, 0, rate);
	ret = aud_open_device(p, name, 1, rate);
	return ret;
}

int aud_close(AUD_OBJ *p)
{
	snd_pcm_close(p->handle[0]);
	snd_pcm_close(p->handle[1]);
}

int aud_start(AUD_OBJ *p)
{
	int err = -1;

    /* drop any output we might got and stop */

    //snd_pcm_drop(p->handle[0]);
    //snd_pcm_drop(p->handle[1]);
    
	    /* prepare for use */
	if ((err = snd_pcm_start(p->handle[0])) < 0) {
	//if ((err = snd_pcm_prepare(p->handle[0])) < 0) {
		fprintf(stderr, "cannot start audio interface for use(%s)\n",
		 	snd_strerror(err));
		return err;
	}

	//if (p->mode == 1) {

		if ((err = snd_pcm_prepare(p->handle[1])) < 0) {
			fprintf(stderr, "cannot prepare audio interface for use(%s)\n",
			 	snd_strerror(err));
			return err;
		}

	//} 

	return err;
}

int aud_get_stream(AUD_OBJ *p, char *buf, int len, int *avail) 
{
	int ret;
	int frames = len/2;

	ret = snd_pcm_avail(p->handle[0]);
	if (avail) {
		*avail = ret;
	}
	if (ret >= frames) {
		ret = snd_pcm_readi(p->handle[0], buf, frames);
		if (ret != frames) {
			printf("Alsa read error\n");
			return -1;
		} else {
			return ret;
		}
	} else {
		return 0;
	}          
}


int aud_put_stream(AUD_OBJ *p, char *buf, int len, int *avail) 
{
	int ret;
	int frames = len/2;

    ret = snd_pcm_avail(p->handle[1]);
	if (avail) {
		*avail = ret;
		if (ret < (PCM_BUFFER-PCM_THRESHOLD) && ret > 0) {
			return ret;
		}
	}
    if (ret > frames) {
        ret = snd_pcm_writei(p->handle[1], buf, frames);
        if (ret != frames) {
			printf("write errro...%d\n", ret);    
        }
    } else if (ret < 0) {
        printf("underrun  %d\n", ret);
        snd_pcm_prepare(p->handle[1]);
        ret = snd_pcm_writei(p->handle[1], buf, frames);
        printf("write : %d\n", ret);
    }
}

int aud_get_avail(AUD_OBJ *p, int dir)
{
	return snd_pcm_avail(p->handle[1]);
}

#endif

