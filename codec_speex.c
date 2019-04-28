
#include "codec.h"
#include "libspeex/speex.h"

typedef struct {
   	SpeexBits inbits;
    SpeexBits outbits;
    int speex_quality;
    int speex_complexity;
    int speex_sampling;
    void *pEncSpeex;
    void *pDecSpeex; 
} CODEC_SPEEX_DATA;

static CODEC_SPEEX_DATA codec_speex_data;

static int codec_speex_encode(int16_t *in, uint8_t *out);
static int codec_speex_decode(uint8_t *in, int16_t *out);

CODEC_COM codec_speex = {
    CODEC_ID_SPEEX,
    110,
    320,    // 160 word
    20,
    codec_speex_encode,
    codec_speex_decode,
    &codec_speex_data
};

int codec_speex_init(CODEC_OBJ *c)
{
    CODEC_SPEEX_DATA *p = &codec_speex_data;
	p->speex_quality = 3;
    p->speex_complexity = 1;
    p->speex_sampling = 8000;
       
    speex_bits_init(&(p->inbits));
	speex_bits_init(&(p->outbits));
 
    p->pEncSpeex = speex_encoder_init(&speex_nb_mode);
    
	speex_encoder_ctl(p->pEncSpeex, 
        SPEEX_SET_QUALITY, &(p->speex_quality));
    speex_encoder_ctl(p->pEncSpeex, 
        SPEEX_SET_COMPLEXITY, &(p->speex_complexity));
    speex_encoder_ctl(p->pEncSpeex, 
        SPEEX_SET_SAMPLING_RATE, &(p->speex_sampling));
    
	p->pDecSpeex = speex_decoder_init(&speex_nb_mode);

   	speex_decoder_ctl(p->pDecSpeex, 
       	SPEEX_SET_SAMPLING_RATE, &(p->speex_sampling));
 
    codec_register(c, &codec_speex, CODEC_ID_SPEEX);

    return 0;
}

static int codec_speex_encode(int16_t *in, uint8_t *out)
{
    CODEC_SPEEX_DATA *p = &codec_speex_data;

	speex_bits_reset(&(p->inbits));
    speex_encode_int(p->pEncSpeex, (short int *)in, &(p->inbits));
    speex_bits_write(&(p->inbits), (char*)out, 20);

    return 0;
}

static int codec_speex_decode(uint8_t *in, int16_t *out)
{
    CODEC_SPEEX_DATA *p = &codec_speex_data;
	
    speex_bits_read_from(&(p->outbits), (char *)in, 20);
	speex_decode_int(p->pDecSpeex, &(p->outbits), (short int *)out);

    return 0;
}

