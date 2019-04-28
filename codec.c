
#include "codec.h"

extern int codec_speex_init(CODEC_OBJ *c);
extern int codec_lpcm_init(CODEC_OBJ *c);

int codec_init(CODEC_OBJ *c)
{
    c->index = 0;
    c->max_index = 0;

    codec_lpcm_init(c);
    codec_speex_init(c);

    return 0;
}

int codec_register(CODEC_OBJ *c, CODEC_COM *codec,  uint32_t id)
{
    c->codec[id] = codec;
    c->max_index++;
    return 0;
}

int codec_get_inteface(CODEC_OBJ *c, CODEC_COM **codec, uint32_t id)
{
    if (id < MAX_CODEC) {
        *codec = c->codec[id];
        return 0;
    } 

    return -1;
}