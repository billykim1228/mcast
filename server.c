
// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <signal.h>
#include <unistd.h> 
#include <string.h> 
#include <sys/time.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <netdb.h>
  
#include "librtp/rtp.h"
#include "helper.h"
#include "config.h"

#define MAX_CLIENT  256
#define MAX_GROUP   32

typedef struct {
    uint32_t id;                        // client id 
    uint32_t gid;                       // group id
    uint32_t use;
    uint32_t sn[2];                     // serial number, 0 : low, 1 : high;
    uint32_t g_mask;                    // group mask
    struct sockaddr_in peer;	        // client addr : port
    struct timeval stime;               // start time
} RTP_CLIENT;

typedef struct {
    uint32_t ptt_lock;
    uint32_t ptt_timer_tick;            // ptt start timer
    RTP_CLIENT *c;    
} RTP_GROUP;

static uint32_t ptt_lock;
static uint32_t ptt_s_index;
static uint32_t ptt_g_mask;

static uint32_t client_cnt = 0;
static RTP_CLIENT cli[MAX_CLIENT];
static RTP_GROUP grp[MAX_GROUP][2];     // 0 : low priority, 1 : high priority
static RTP_OBJ rtp;

char buf[MAXLINE];


// 
// Handler : PT_CMD_REQ_JOIN
//

int send_cmd_ack_join(uint32_t id, struct sockaddr_in *addr)
{
	RTP_CMD_DATA_ID data;
	data.id = htonl(id);

	send_cmd_packet_gen(&rtp, 
		PT_CMD_ACK_JOIN, 0, &(data), sizeof(RTP_CMD_DATA_ID ), addr);

	return 0;
}

int handler_cmd_packet_req_join(void)
{
    RTP_CMD_PKT_REQ_JOIN *p = (RTP_CMD_PKT_REQ_JOIN *)(rtp.rbuf);
    uint32_t id = ntohl(p->data.id);
    uint32_t gid = id >> 24;
    uint32_t sn_lo = ntohl(p->data.sn[0]); 
    uint32_t sn_hi = ntohl(p->data.sn[1]); 
    char str[128];

    inet_ntop(AF_INET, &(rtp.peer.sin_addr), str, 64);

    printf("REQ_JOIN : %3d, %8d, 0x%8.8x, [%3.3d] %s:%8.8d\n", 
        gid, id, 0, client_cnt, str, ntohs(rtp.peer.sin_port));
    
    for (int i = 0; i < MAX_CLIENT; i++) {
        //if (cli[i].use == 0 || cli[i].id == id) {
        if (cli[i].use == 0) {
            if (cli[i].use == 0) { 
                client_cnt++;
            }
            cli[i].use = 1;
            //cli[i].id  = (id & 0xff000000) | (sn_hi & 0x00ffffff);
            cli[i].id  = (id & 0xff000000) | (i & 0x00ffffff);
            cli[i].gid = (id & 0xff000000) >> 24;
            cli[i].gid %= 32;
            cli[i].peer = rtp.peer;
            printf("ACK_JOIN : %3d, %8d, 0x%8.8x, [%3.3d] %s:%8.8d\n", 
                cli[i].id > 24, cli[i].id, 0, client_cnt, str, ntohs(rtp.peer.sin_port));
            
            rtp.ctrl |= RTP_PT_STOP;
            send_cmd_ack_join(cli[i].id, &(cli[i].peer));
            rtp.ctrl &= ~RTP_PT_STOP;
            //rtp_reset(&rtp, 0x0001, 110);
            break;
        }
    }
 
    return 0;
}

// 
// Handler : PT_CMD_REQ_PING
//

int send_cmd_ack_ping(uint32_t id, struct sockaddr_in *addr)
{
	RTP_CMD_DATA_ID data;
	data.id = htonl(id);

	send_cmd_packet_gen(&rtp, 
		PT_CMD_ACK_PING, 0, &(data), sizeof(RTP_CMD_DATA_ID ), addr);

	return 0;
}


int handler_cmd_packet_req_ping(void)
{
    RTP_CMD_PKT_REQ_PING *p = (RTP_CMD_PKT_REQ_PING *)(rtp.rbuf);
    uint32_t id = ntohl(p->data.id);
    uint32_t gid = id >> 24;
    char str[128];

    inet_ntop(AF_INET, &(rtp.peer.sin_addr), str, 64);

    printf("REQ_PING : %3d, %8d, 0x%8.8x, [%3.3d] %s:%8.8d\n", 
        gid, id, 0, client_cnt, str, ntohs(rtp.peer.sin_port));
    
    for (int i = 0; i < MAX_CLIENT; i++) {
        //if (cli[i].use == 0 || cli[i].id == id) {
        if (cli[i].id == id) {
            cli[i].peer = rtp.peer;
            printf("ACK_PING : %3d, %8d, 0x%8.8x, [%3.3d] %s:%8.8d\n", 
                cli[i].id > 24, cli[i].id, 0, client_cnt, str, ntohs(rtp.peer.sin_port));
            
            rtp.ctrl |= RTP_PT_STOP;
            send_cmd_ack_ping(cli[i].id, &(cli[i].peer));
            rtp.ctrl &= ~RTP_PT_STOP;
            //rtp_reset(&rtp, 0x0001, 110);
            break;
        }
    }
 
    return 0;
}


// 
// Handler : PT_CMD_REQ_BYE
//

int handler_cmd_packet_req_bye(void)
{
    RTP_CMD_PKT_REQ_BYE *p = (RTP_CMD_PKT_REQ_BYE *)(rtp.rbuf);
    uint32_t id = ntohl(p->data.id);
    uint32_t gid = id >> 24;
    char str[128];

    inet_ntop(AF_INET, &(rtp.peer.sin_addr), str, 64);
    
    printf("REQ_BYE  : %3d, %8d, 0x%8.8x, [%3.3d] %s:%8.8d\n", gid, id, 0, client_cnt, str, ntohs(rtp.peer.sin_port));
    
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (cli[i].use && cli[i].id == ntohl(p->data.id)) {
            cli[i].use = 0;
            client_cnt--;
        }
    }


    return 0;
}

// 
// Handler : PT_CMD_REQ_PTT
//

int send_cmd_ack_ptt(uint32_t id, struct sockaddr_in *addr)
{
	RTP_CMD_DATA_ID data;
	data.id = htonl(id);

    rtp.ctrl |= RTP_PT_STOP;
	send_cmd_packet_gen(&rtp, 
		PT_CMD_ACK_PTT, 0, &(data), sizeof(RTP_CMD_DATA_ID ), addr);
    rtp.ctrl &= ~RTP_PT_STOP;
	
    return 0;
}

int handler_cmd_packet_req_ptt(void)
{
    RTP_CMD_PKT_REQ_PTT *p = (RTP_CMD_PKT_REQ_PTT *)(rtp.rbuf);
    uint32_t id = ntohl(p->data.id);
    uint32_t gid = id >> 24;
    uint32_t g_mask = ntohl(p->data.g_mask);

    printf("REQ_PTT  : %3d, %8d, 0x%8.8x\n", gid, id, g_mask);
 
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (cli[i].use == 1 && cli[i].id == id) {
            if (grp[cli[i].gid][0].ptt_lock == 0) {
                grp[cli[i].gid][0].ptt_timer_tick = start_timer();
                grp[cli[i].gid][0].ptt_lock = 1;
                grp[cli[i].gid][0].c = &cli[i];
                //cli[i].id = id;
                //cli[i].id = i;
                cli[i].g_mask = g_mask;
                printf("ACK_PTT  : %3d, %8d\n", cli[i].gid, id);
                rtp.ctrl |= RTP_PT_STOP;
                send_cmd_ack_ptt(cli[i].id, &(cli[i].peer));
                rtp.ctrl &= ~RTP_PT_STOP;
                break;
            }
        }
    }

    return 0;
}

int handler_cmd_packet_nof_stat(void)
{
    RTP_CMD_PKT_NOF_STAT *p = (RTP_CMD_PKT_NOF_STAT *)(rtp.rbuf);
    RTP_CLIENT *c_src;
    uint32_t id = ntohl(p->data.id);
    uint32_t gid = id >> 24;
    uint32_t g_mask = ntohl(p->data.g_mask);
    uint32_t s_mask = ntohl(p->data.s_mask);
    RTP_CMD_DATA_STAT data;

    rtp.pt = p->hdr.pt;

	data.id     = p->data.id;
	data.g_mask = p->data.g_mask;
	data.d_mask = p->data.d_mask;
	data.s_mask = p->data.s_mask;

    if ((gid = ntohl(p->hdr.ssrc) >> 24) > 32) {
        return -1;
    }

    if ((c_src = grp[gid][0].c) == NULL) {
        return -1;
    }

    if (s_mask > 0) {
        printf("NOF_SOF  : %3d, %8d, 0x%8.8x\n", gid, id, g_mask);
    } else {
        printf("NOF_EOF  : %3d, %8d, 0x%8.8x\n", gid, id, g_mask);
    }

    rtp_set_ssrc(&rtp, RTP_SERVER_SSRC, p->hdr.pt);

    for (int i = 0; i < MAX_CLIENT; i++) {
        if (cli[i].use/* && ntohs(cli[i].peer.sin_port) != 40000*/) {
            if (c_src->g_mask & (1 << cli[i].gid)) {
                //printf("Broadcast %4d, %d\n", ntohs(cli[i].peer.sin_port), rtp.pt);
                rtp.ctrl |= RTP_PT_STOP;
            	send_cmd_packet_gen(&rtp,  
		            PT_CMD_NOF_STAT, 0, &(data), sizeof(RTP_CMD_DATA_STAT), &(cli[i].peer));
                rtp.ctrl &= ~RTP_PT_STOP;
            }
        }
    }

    if (s_mask == 0) {  // PTT Off
        grp[gid][0].ptt_lock = 0;  
        c_src = grp[gid][0].c = NULL;
    }

    return 0;
}

// 
// Handler : normal voice packet
//

int handler_voice_packet(void *buf, size_t n)
{
    RTP_CMD_PKT *p = (RTP_CMD_PKT *)(rtp.rbuf);
    RTP_HDR *h = (RTP_HDR *)(rtp.rbuf); 
    RTP_CLIENT *c_src;
    uint32_t gid;

    rtp.pt = p->hdr.pt;
    h->pt = rtp.pt;

    if ((gid = ntohl(h->ssrc) >> 24) > 32) {
        return -1;
    }

    if ((c_src = grp[gid][0].c) == NULL) {
        return -1;
    }

    //printf("SRC ID, GID : %8d, %3d, %3d\n", ntohl(h->ssrc), gid, ntohl(h->ssrc) >> 24);

    rtp_set_ssrc(&rtp, RTP_SERVER_SSRC, h->pt);
    grp[gid][0].ptt_timer_tick = start_timer();  // update ptt timer

    rtp.ctrl &= ~RTP_PT_STOP;
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (cli[i].use/* && ntohs(cli[i].peer.sin_port) != 40000*/) {
            if (c_src->g_mask & (1 << cli[i].gid)) {
                //printf("Broadcast %4d, %d\n", ntohs(cli[i].peer.sin_port), rtp.pt);
                rtp_send(&rtp, (char *)buf, n, &(cli[i].peer));
                rtp.ctrl |= RTP_PT_STOP;
            }
        }
    }

    return 0;
}


int main(void) 
{ 
    rtp_open(&rtp, 40001, 1, NULL);
    rtp_reset(&rtp, 0x0001, 110);

    memset(&(rtp.peer), 0, sizeof(struct sockaddr));
    
    while (1) {

        int len, n; 
        static int cnt = 0;

        //
        // Check tiemr
        //

        for (int i = 0; i < MAX_GROUP; i++) {
            if (grp[i][0].ptt_lock) {
                uint32_t diff;
                if ((diff = check_timer(grp[i][0].ptt_timer_tick)) > 300) {      // > 300ms
                    grp[i][0].ptt_lock = 0;
                    grp[i][0].c = NULL;
                    printf("PTT Timer expire : %u\n", diff);
                }
            }
        }

        n = rtp_recv(&rtp, (char *)buf, MAXLINE); 

        if (n > 0) { 
            
            RTP_CMD_PKT *p = (RTP_CMD_PKT *)(rtp.rbuf);
            if (p->hdr.pt == PT_CMD) {
                if (ntohs(p->c_hdr.cmd) == PT_CMD_REQ_JOIN) {
                    handler_cmd_packet_req_join();
                } 
                else if (ntohs(p->c_hdr.cmd) == PT_CMD_REQ_BYE) {
                    handler_cmd_packet_req_bye();
                }
                else if (ntohs(p->c_hdr.cmd) == PT_CMD_REQ_PTT) {
                    handler_cmd_packet_req_ptt();
                }
                else if (ntohs(p->c_hdr.cmd) == PT_CMD_NOF_STAT) {
                    handler_cmd_packet_nof_stat();
                } 
                else if (ntohs(p->c_hdr.cmd) == PT_CMD_REQ_PING) {
                    handler_cmd_packet_req_ping();
                }
                else {
                    printf("RTP invalid cmd : %d\n", ntohs(p->c_hdr.cmd));
                }
            } 
            else if (p->hdr.pt != PT_CMD) {
                handler_voice_packet(buf, n);
            } 
            else {
                printf("Recv PT : %d\n", p->hdr.pt);
            }
        }

    }

    return 0; 
} 

