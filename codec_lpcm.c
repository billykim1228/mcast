

#include <arpa/inet.h>
#include "codec.h"

static int codec_lpcm_encode(int16_t *in, uint8_t *out);
static int codec_lpcm_decode(uint8_t *in, int16_t *out);

CODEC_COM codec_lpcm = {
    CODEC_ID_LPCM,
    96,
    320,    // 160 word
    320,
    codec_lpcm_encode,
    codec_lpcm_decode,
    0 
};

int codec_lpcm_init(CODEC_OBJ *c)
{
    codec_register(c, &codec_lpcm, CODEC_ID_LPCM);

    return 0;
}

static int codec_lpcm_encode(int16_t *in, uint8_t *out)
{
    // LE(host) -> BE
    int i;
    short int *s = (short int *)in;
    short int *d = (short int *)out;

    for (i = 0; i < codec_lpcm.frame_out_size / 2; i++) {
        d[i] = htons(s[i]);
    }
    return 0;
}

static int codec_lpcm_decode(uint8_t *in, int16_t *out)
{
    // BE -> LE(host)
    int i;
    short int *s = (short int *)in;
    short int *d = (short int *)out;

    for (i = 0; i < codec_lpcm.frame_out_size / 2; i++) {
        d[i] = ntohs(s[i]);
    }
    return 0;
}

