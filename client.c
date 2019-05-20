/*
 * Copyright (c) 2012 Daniel Mack
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

/*
 * See README
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h>     
#include <sys/time.h>
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include <sched.h>      // for linux
#include <errno.h>

#include <time.h>
#include <signal.h>

#include "librtp/rtp.h"
#include "libspeex/speex.h"

#include "audio_alsa.h"
#include "audio_jack.h"
#include "codec.h"
#include "helper.h"
#include "config.h"

//
// App module
//

#define ENABLE_SPEEX        1

short int buf[320];
char net_buf[320];
static RTP_OBJ rtp;
static AUD_OBJ aud;
static CODEC_OBJ codec_obj;

static uint64_t ptt_time;
static uint32_t codec_id = DEFAULT_CODEC_ID;
static uint32_t sample_rate = DEFAULT_SAMPLE_RATE;
static uint32_t port_num = DEFAULT_RTP_PORT;
static uint32_t group = DEFAULT_GROUP;
static uint32_t g_mask = DEFAULT_GROUP_MASK;
static uint32_t user_id;
static uint32_t state;          // 0 = idle, 1 = run
static uint32_t ptt_enable;     // 0 = ptt off, 1 = ptt on

static int payload_map[] = {
    PT_LPCM,    // LPCM
    PT_SPEEX,   // Speex
    PT_PCMU,    // PCMU (u-law)
    PT_PCMA     // PCMA (a-law)
};

#define STATE_IDLE              0
#define STATE_READY             1
#define STATE_TX                2
#define STATE_TX_WAIT           3

//
// Protocol & Handler
//

int send_cmd_notify_stat(uint32_t id, uint32_t g_mask, uint32_t d_mask, uint32_t s_mask)
{
    RTP_CMD_DATA_STAT data;

    data.id     = htonl(id);
    data.g_mask = htonl(g_mask);
    data.d_mask = htonl(d_mask);
    data.s_mask = htonl(s_mask);

    send_cmd_packet_gen(&rtp,  
        PT_CMD_NOF_STAT, 0, &(data), sizeof(RTP_CMD_DATA_STAT), &(rtp.svr));

    if (s_mask & 1) {
        printf("SEND NOF_SOF  : %4d, 0x%8.8x\n", user_id > 24, g_mask);
    } else {
        printf("SEND NOF_EOF  : %4d, 0x%8.8x\n", user_id > 24, g_mask);
    }

    return 0;
}

int send_cmd_req_join(uint32_t sn[2], uint32_t id)
{
    RTP_CMD_DATA_JOIN data;

    data.sn[0] = htonl(sn[0]);
    data.sn[1] = htonl(sn[1]);
    data.id = htonl(id);

    send_cmd_packet_gen(&rtp,  
        PT_CMD_REQ_JOIN, 0, &(data), sizeof(RTP_CMD_DATA_JOIN), &(rtp.svr));

    printf("SEND REQ_JOIN : %4d, 0x%8.8x\n", (id & 0xff00000) >> 24, sn[1]);
    
    return 0;
}

int handler_cmd_ack_join(void *buf)
{
    uint32_t id;
    RTP_CMD_PKT_ACK_JOIN *p = (RTP_CMD_PKT_ACK_JOIN *)buf;

    user_id = ntohl(p->data.id);

    if (user_id == ntohl(p->data.id)) {
        printf("RECV ACK_JOIN : %4d, 0x%8.8x\n", (user_id & 0xff000000) > 24, user_id);
        state = STATE_READY;
    }

    return 0;
}

int send_cmd_req_ping(uint32_t id)
{
    RTP_CMD_DATA_ID data;
  
    data.id = htonl(id);

    send_cmd_packet_gen(&rtp,  
        PT_CMD_REQ_PING, 0, &(data), sizeof(RTP_CMD_DATA_ID), &(rtp.svr));

    printf("SEND REQ_PING : %4d\n", (id & 0xff00000) >> 24);
    
    return 0;
}

int handler_cmd_ack_ping(void *buf)
{
    uint32_t id;
    RTP_CMD_PKT_ACK_PING *p = (RTP_CMD_PKT_ACK_PING *)buf;

    user_id = ntohl(p->data.id);

    if (user_id == ntohl(p->data.id)) {
        printf("RECV ACK_PING : %4d, 0x%8.8x\n", (user_id & 0xff000000) > 24, user_id);
        state = STATE_READY;
    }

    return 0;
}

int send_cmd_req_bye(uint32_t id)
{
    RTP_CMD_DATA_ID data;

    data.id = htonl(id);

    send_cmd_packet_gen(&rtp,  
        PT_CMD_REQ_BYE, 0, &(data), sizeof(RTP_CMD_DATA_ID), &(rtp.svr));

    printf("SEND REQ_BYE : %d\n", user_id > 24);
    
    return 0;
}

int handler_cmd_ack_bye(void *buf)
{
    uint32_t id;
    RTP_CMD_PKT_ACK_BYE *p = (RTP_CMD_PKT_ACK_BYE *)buf;

    if (user_id == ntohl(p->data.id)) {
        printf("RECV ACK_BYE\n");
    }

    return 0;
}

int send_cmd_req_ptt(uint32_t id, uint32_t g_mask, uint32_t d_mask, uint32_t prio)
{
    RTP_CMD_DATA_PTT data;

    data.id = htonl(id);
    data.g_mask = htonl(g_mask);
    data.d_mask = htonl(d_mask);
    data.prio = htonl(prio);

    send_cmd_packet_gen(&rtp,  
        PT_CMD_REQ_PTT, 0, &(data), sizeof(RTP_CMD_DATA_PTT), &(rtp.svr));

    printf("SEND REQ_PTT  : %4d, 0x%8.8x\n", user_id > 24, g_mask);
    
    return 0;
}

int handler_cmd_ack_ptt(void *buf)
{
    uint32_t id;
    RTP_CMD_PKT_ACK_PTT *p = (RTP_CMD_PKT_ACK_PTT *)buf;

    if (user_id == ntohl(p->data.id)) {
        printf("RECV ACK_PTT\n");
        if (state == STATE_TX_WAIT) {
            state = STATE_TX;
            send_cmd_notify_stat(user_id, g_mask, 0xffffffff, NOF_PTT_ON);      // PTT On
        }
    }

    return 0;
}


//
// Printf packet
//

int print_recv_packet(void *buf)
{
    uint64_t ms;
    struct timeval tv;
    RTP_HDR *h = (RTP_HDR *)(buf);
    static int first = 1;
    char *p = (char *)buf;
    int avail;

    avail = aud_get_avail(&aud, 1);

    if (first) {
        first = 0;
        printf("HEAD : (%2.2x,%2.2x,%2.2x)\n", p[0], p[1], p[2]);
    }
    gettimeofday(&tv, NULL);
    ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    printf("RECV [%8u ms] : %4d, %8u, %8u: [%5d] (%d,%d,%d,%d,%3d,%d, %d)\n", 
        (uint32_t)ms,
        rtp.rx_pay_len,
        ntohs(h->seq),
        ntohl(h->ts),
        avail,
        h->version,
        h->cc,
        h->m,
        h->p,
        h->pt,
        h->x,
        ntohl(h->ssrc)
    );

    return 0;
}

int print_send_packet(void *buf, CODEC_COM *c, int avail)
{
    uint64_t ms;
    struct timeval tv;
    RTP_HDR *h = (RTP_HDR *)(buf);

    gettimeofday(&tv, NULL);
    ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    printf("SEND [%8u ms] : %4d, %8u, %8u: [%5d], %d\n", 
        (uint32_t)ms,
        rtp.tx_pay_len, 
        ntohs(h->seq),
        ntohl(h->ts),
        avail,
        payload_map[c->id]);

        return 0;
}

//
// PTT controller
//

#include <termios.h>

struct termios initial_settings, new_settings;

int ptt_ctrl_init(void)
{
  tcgetattr(0,&initial_settings);
 
  new_settings = initial_settings;
  new_settings.c_lflag &= ~ICANON;
  new_settings.c_lflag &= ~ECHO;
  new_settings.c_lflag &= ~ISIG;
  new_settings.c_cc[VMIN] = 0;
  new_settings.c_cc[VTIME] = 0;
 
  tcsetattr(0, TCSANOW, &new_settings);

  return 0;
}

int ptt_ctrl_exit(void)
{
  tcsetattr(0, TCSANOW, &initial_settings);
  return 0;
}

int ptt_ctrl_update(void)
{
    //
    // Scan key
    //

    int key;    
    char buf = 0;
    if (read(0, &buf, 1) <= 0) {
        key = EOF;  
    } else {
        key = buf;
    }

    if (key != EOF) {
        if (key == 27) {    // ESC : exit
            return -1;
        } else if (key == 'p') {
            ptt_enable = (ptt_enable == 0) ? 1 : 0;
        } else {
            printf("key = 0x%8.8x\n", key);
        }
    }

    //
    // Update state
    //

    if (state == STATE_TX) {
        if (!ptt_enable) {
            // send stop notify
            state = STATE_READY;
            send_cmd_notify_stat(user_id, g_mask, 0xffffffff, 0);       // PTT Off
        }
    } else if (state == STATE_TX_WAIT) {
        if (ptt_enable) {
            if (check_timer(ptt_time) > TIMEOUT_PTT) {
                printf("PTT timeout\n");
                state = STATE_READY;
            }
        } else {
            state = STATE_READY;
        }
    } else if (state == STATE_READY) {
        if (ptt_enable) {
            ptt_time = start_timer();       
            send_cmd_req_ptt(user_id, g_mask, 0xffffffff, 0);
            state = STATE_TX_WAIT;
        } else {
            static uint8_t ping_run;
            static uint64_t ping_time = 0;
            if (!ping_run) {
                ping_time = start_timer();
                send_cmd_req_ping(user_id);
                ping_run = 1;
            } else {
                if (check_timer(ping_time) > 1000) {
                    ping_run = 0;
                }
            }

        }
    } else if (state == STATE_IDLE) {

    }

    return 0;
} 

//
// Packet send handler
//

int handler_send_packet(CODEC_COM *c)
{
    int ret;
    int avail;

    //printf("state : %d\n", state);

    if (state != STATE_TX) {
        return -1;
    }

    ret = aud_get_stream(&aud, (char *)buf, c->frame_in_size, &avail);
    if (ret > 0) {

    //
    // Encode audio
    //

    c->encode(buf, (uint8_t *)net_buf);

    //
    // Send RTP
    //
                
    rtp_set_ssrc(&rtp, user_id, payload_map[c->id]);
    ret = rtp_send(&rtp, (char *)net_buf, c->frame_out_size, &(rtp.svr));

        print_send_packet(rtp.rbuf, c, avail);

        if (ret <= 0) {
            perror("send error : ");
        }

    }

    return ret;
}

//
// Packet recv handler
//

int handler_recv_packet(CODEC_COM *c, int mode)
{
    int ret;

    ret = rtp_recv(&rtp, (char *)net_buf, MAXLINE); 

    if (ret > 0) {

        RTP_CMD_PKT *p = (RTP_CMD_PKT *)(rtp.rbuf);
    
        if (p->hdr.pt == PT_CMD) {
            
            // Handler : CMD_ACK_JOIN
            if (ntohs(p->c_hdr.cmd) == PT_CMD_ACK_JOIN) {
                handler_cmd_ack_join(rtp.rbuf);
                rtp_set_ssrc(&rtp, user_id, payload_map[c->id]);
            }
            // Handler : CMD_ACK_PTT
            else if (ntohs(p->c_hdr.cmd) == PT_CMD_ACK_PTT) {
                handler_cmd_ack_ptt(rtp.rbuf);
                rtp_set_ssrc(&rtp, user_id, payload_map[c->id]);
            }
            // Handler : CMD_ACK_BYE
            else if (ntohs(p->c_hdr.cmd) == PT_CMD_ACK_BYE) {
                handler_cmd_ack_bye(rtp.rbuf);
                state = STATE_IDLE;
            } 
            // Handler : CMD_NOF_STAT
            else if (ntohs(p->c_hdr.cmd) == PT_CMD_NOF_STAT) {
                RTP_CMD_PKT_NOF_STAT *p = (RTP_CMD_PKT_NOF_STAT *)(rtp.rbuf);
                if (p->data.s_mask & NOF_PTT_ON) {
                    printf("RECV NOF_SOF\n");
                } else {
                    printf("RECV NOF_EOF\n");
                }
            } 
            // Handler : CMD_ACK_PING
            else if (ntohs(p->c_hdr.cmd) == PT_CMD_ACK_PING) {
                handler_cmd_ack_ping(rtp.rbuf);
            } 

            else {
                printf("receive Invalid CMD : %d, %d\n", ntohs(p->c_hdr.cmd), ntohs(p->c_hdr.len));
            }   
        } 
        
        else if (state == STATE_READY || state == STATE_TX_WAIT) {

            //
            // Decode speex
            //

            RTP_HDR *h = (RTP_HDR *)(rtp.rbuf);
            int avail;

            if (RTP_SERVER_SSRC == ntohl(h->ssrc)) {

                print_recv_packet(rtp.rbuf);    

                if (c->type == h->pt) {
                    c->decode((uint8_t *)net_buf, buf);
                    //if ((avail = aud_get_avail(&aud, 1)) < 4000) {
                    //    if (avail > 0) {
                    //        return 0;
                    //    }
                    //}
                    aud_put_stream(&aud, (char *)buf, c->frame_in_size, &avail);
                }

            }
        }
    }

    return ret;
}

volatile sig_atomic_t flag = 0;
void my_function(int sig){ // can be called asynchronously
  flag = 1; // set flag
}

int main(int argc, char *argv[])
{
    CODEC_COM *c;
    int avail;
    int ret;
    int opt;
    int mode;

    //char ip[128];
    char *svr_ip = NULL;
    struct hostent *he;

    uint32_t id;
    struct timeval rtime;
    gettimeofday(&rtime, NULL);
    id = rtime.tv_usec;
    
    signal(SIGINT, my_function); 

    while ((opt = getopt(argc, argv, "srf:c:g:m:d:p:")) != -1) {
        switch (opt) {
        case 'f':
            sample_rate = atoi(optarg);
            break;
        case 'c':
            codec_id = atoi(optarg);
            break;
        case 's':
            mode = 0;
            break;
        case 'r':
            mode = 1;
            break;
        case 'g':
            group = atoi(optarg);
            break;
        case 'm':
            g_mask = strtol(optarg, NULL, 16);
            break;
        case 'd':
            svr_ip = optarg;
            break;
        case 'p':
            port_num = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s [-sr] [-f rate] [-c codec_id] [-p rtp port] -d server_ip\n", argv[0]);
            exit(-1);
        }
    }

    if (argc == 1) {
        fprintf(stderr, "Usage: %s [-sr] [-f rate] [-c codec_id] -d server_ip\n", argv[0]);
        exit(-1);
    }

    //
    // Init codec module
    //

    codec_init(&codec_obj);

    if (codec_get_inteface(&codec_obj, &c, codec_id) < 0) {
        fprintf(stderr, "Invalid codec_id\n");
        exit(-1);
    }

    //
    // Init RTP module
    //

    rtp_open(&rtp, port_num, 0, svr_ip);
    rtp_reset(&rtp, 0x0000, c->id);

    //
    // Init AUDIO module
    //

    aud_open(&aud, "default", mode, sample_rate);
    aud_start(&aud);

    memset(buf, 0, sizeof(buf));
    
    printf("group      : %d\n", group);
    printf("group_mask : 0x%8.8x\n", g_mask);
    printf("codec      : %d, %d, %d, %d\n", 
        c->id, c->type, c->frame_in_size, c->frame_out_size);


    uint32_t sn[] = {rtime.tv_sec,rtime.tv_usec};
    send_cmd_req_join(sn, group << 24);

    ptt_ctrl_init();

    while (1) {

        if (flag || (ptt_ctrl_update() < 0)) { 
               
            // my action when signal set it 1 or detect exit key

            printf("\n Signal caught!\n");
            printf("\n default action it not termination!\n");
   
            flag = 0;
            send_cmd_req_bye(user_id);  
            ptt_ctrl_exit();
            usleep(100000);
            exit(0);

        } 

        if ((ret = handler_recv_packet(c, mode)) <= 0) {

        }

        if (mode == 0) {        // Send mode

            if (handler_send_packet(c) <= 0) {
            }

        }

    }

    aud_close(&aud);

    return 0;
}
