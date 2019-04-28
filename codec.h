/**
 * @file    codec.h
 * @brief   codec core interface header
 * @version 1.0
 *
 */

#ifndef _CODEC_H_
#define _CODEC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define CODEC_ID_LPCM       0
#define CODEC_ID_SPEEX      1
#define MAX_CODEC		    2

typedef int (codec_encode)(int16_t *in, uint8_t *out);
typedef int (codec_decode)(uint8_t *in, int16_t *out);

typedef struct {
    uint32_t id;            /* id */
	uint32_t type;			/* type number */
	uint32_t frame_in_size;	/* input frame size */
	uint32_t frame_out_size;/* output frame size */ 
    codec_encode *encode;  	/* encode stream */
    codec_decode *decode;   /* decode stream */
    void *data;             /* private data */
} CODEC_COM;

typedef struct  {
	CODEC_COM *codec[MAX_CODEC];
    uint32_t index;
	uint32_t max_index;
} CODEC_OBJ;

// Common codec API

int codec_init(CODEC_OBJ *c);
int codec_register(CODEC_OBJ *c, CODEC_COM *codec, uint32_t id);
int codec_get_inteface(CODEC_OBJ *c, CODEC_COM **codec, uint32_t id); 

#ifdef __cplusplus 
} 
#endif

#endif

