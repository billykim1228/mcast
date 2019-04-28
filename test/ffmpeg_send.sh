ffmpeg -ac 1 -f alsa -i hw:0,0 -acodec pcm_s16be -ar 8000 -ab 256k -ac 1 -f rtp rtp://192.168.1.164:40000
