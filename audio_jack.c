#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/time.h>
  
#include "audio_jack.h"

#ifndef ENABLE_ALSA

#include "../libspeex/speex_resampler.h"

// Ring buffer

typedef struct {
    int16_t * const buffer;
    int head;
    int tail;
    const int maxlen;
} circ_bbuf_t;

#define CIRC_BBUF_DEF(x,y)                \
    int16_t x##_data_space[y];            \
    circ_bbuf_t x = {                     \
        .buffer = x##_data_space,         \
        .head = 0,                        \
        .tail = 0,                        \
        .maxlen = y                       \
    }

CIRC_BBUF_DEF(tx_circ_buf, 160 * 8);
CIRC_BBUF_DEF(rx_circ_buf, 160 * 8);

static inline int circ_bbuf_len(circ_bbuf_t *c)
{
	return (c->head + c->maxlen - c->tail) % c->maxlen;
}

static inline int circ_bbuf_push(circ_bbuf_t *c, int16_t data)
{
    int next;

    next = c->head + 1;  // next is where head will point to after this write.
    if (next >= c->maxlen)
        next = 0;

    if (next == c->tail)  // if the head + 1 == tail, circular buffer is full
        return -1;

    c->buffer[c->head] = data;  // Load data and then move
    c->head = next;             // head to next data offset.
    return 0;  // return success to indicate successful push.
}

static inline int circ_bbuf_pop(circ_bbuf_t *c, int16_t *data)
{
    int next;

    if (c->head == c->tail)  // if the head == tail, we don't have any data
        return -1;

    next = c->tail + 1;  // next is where tail will point to after this read.
    if(next >= c->maxlen)
        next = 0;

    *data = c->buffer[c->tail];  // Read data and then move
    c->tail = next;              // tail to next offset.
    return 0;  // return success to indicate successful push.
}


// Jack implementation

static jack_port_t *input_port;
static jack_port_t *output_port;
static jack_client_t *client;

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client does nothing more than copy data from its input
 * port to its output port. It will exit when stopped by 
 * the user (e.g. using Ctrl-C on a unix-ish operating system)
 */

#define JACK_TRIGGER_LEVEL	(160 * 4)

int
process (jack_nframes_t nframes, void *arg)
{
	static int trigger = 0;
	jack_default_audio_sample_t *in, *out;
	int16_t data;
	AUD_OBJ *p = (AUD_OBJ *)arg;

	//printf("nframes : %d, %d\n", nframes, p->base_rate);
	
	in = jack_port_get_buffer (input_port, nframes);
	out = jack_port_get_buffer (output_port, nframes);

	{
		int err;
		spx_uint32_t in_len = nframes;
		spx_uint32_t out_len = 160;

		for (int i = 0; i < nframes; i++) {
			p->tbuf[i] = (int16_t)(in[i] * 32767.0);
		}
		err = speex_resampler_process_int(p->rso, 0, p->tbuf, &in_len, p->obuf, &out_len);
	}

	for (int i = 0; i < 160; i++) {
		data = p->obuf[i];
		circ_bbuf_push(&tx_circ_buf, data);
		if (circ_bbuf_len(&rx_circ_buf) > JACK_TRIGGER_LEVEL) {
			trigger = 1;
		} else if (circ_bbuf_len(&rx_circ_buf) == 0) {
			trigger = 0;
		}
		if (trigger) {
			circ_bbuf_pop(&rx_circ_buf, &data);
			p->ibuf[i] = data;
		} else {
			p->ibuf[i] = 0;
		}
	}

	{
		int err;
		spx_uint32_t in_len = 160;
		spx_uint32_t out_len = nframes;
		err = speex_resampler_process_int(p->rsi, 0, p->ibuf, &in_len, p->tbuf, &out_len);
		for (int i = 0; i < nframes; i++) {
			out[i] = (jack_default_audio_sample_t)(p->tbuf[i]) / 32767.0;
		}
	}

	return 0;      
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown (void *arg)
{
	exit (1);
}

int aud_open(AUD_OBJ *p, const char *name, int mode, int rate)
{
	const char **ports;
	const char *client_name = "simple";
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;
	
	/* open a client connection to the JACK server */

	client = jack_client_open (client_name, options, &status, server_name);
	if (client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		return -1;
	}
	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}

	p->base_rate = jack_get_sample_rate (client);
	p->rate = rate;

	{
		int err;
		p->rso = speex_resampler_init(1, p->base_rate, p->rate, 3, &err);	// for capture
		p->rsi = speex_resampler_init(1, p->rate, p->base_rate, 3, &err);	// for playback
	}

	jack_set_buffer_size(client, 160 * p->base_rate / p->rate);

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	//jack_set_process_callback (client, process, 0);
	jack_set_process_callback (client, process, p);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	//jack_on_shutdown (client, jack_shutdown, 0);

	/* display the current sample rate. 
	 */

	printf ("engine sample rate: %" PRIu32 ", %d\n",
		jack_get_sample_rate(client), jack_get_buffer_size(client));



	/* create two ports */

	input_port = jack_port_register (client, "input",
					 JACK_DEFAULT_AUDIO_TYPE,
					 JackPortIsInput, 0);
	output_port = jack_port_register (client, "output",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput, 0);

	if ((input_port == NULL) || (output_port == NULL)) {
		fprintf(stderr, "no more JACK ports available\n");
		return -1;
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	//sleep(2);

	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		return -1;
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsOutput);
	if (ports == NULL) {
		fprintf(stderr, "no physical capture ports\n");
		return -1;
	}

	if (jack_connect (client, ports[0], jack_port_name (input_port))) {
		fprintf (stderr, "cannot connect input ports\n");
	}

	free (ports);
	
	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		return -1;
	}

	if (jack_connect (client, jack_port_name (output_port), ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}


	free (ports);



	return 0;
}

int aud_close(AUD_OBJ *p)
{
	jack_client_close (client);
	
	return 0;
}

int aud_start(AUD_OBJ *p)
{

	return 0;
}

int aud_get_stream(AUD_OBJ *p, char *buf, int len, int *avail) 
{
	int i;
	int16_t data;
	int16_t *in = (int16_t *)buf;
	int size = len / 2;

	if (circ_bbuf_len(&tx_circ_buf) > len) {
		for (i = 0; i < size; i++) {
			circ_bbuf_pop(&tx_circ_buf, &data);
			in[i] = data;
		}

		*avail =circ_bbuf_len(&tx_circ_buf);
		return size;
	}

	return 0;
}

int aud_put_stream(AUD_OBJ *p, char *buf, int len, int *avail) 
{
	int i;
	int16_t data;
	int16_t *out = (int16_t *)buf;
	int size = len / 2;

	if (circ_bbuf_len(&rx_circ_buf) + len < rx_circ_buf.maxlen) {
		for (i = 0; i < size; i++) {
			data = out[i];
			circ_bbuf_push(&rx_circ_buf, data);
		}

		*avail =circ_bbuf_len(&rx_circ_buf);
		return size;
	}

	return 0;
}

int aud_get_avail(AUD_OBJ *p, int dir)
{
	return rx_circ_buf.maxlen - circ_bbuf_len(&rx_circ_buf);
}


#endif
