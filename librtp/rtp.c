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
  
#include "rtp.h"

//
// RTP module
//

int rtp_reset(RTP_OBJ *c, uint32_t ssrc, char pt)
{
    RTP_HDR *h = (RTP_HDR *)(c->sbuf);

    c->rate = 8000;
    c->payload = PAYSIZE;
    c->pt = pt;
    c->ssrc = ssrc;

    h->ssrc = htonl(c->ssrc);
    h->ts = 0;
    h->seq = 0;
    h->m = 0;
    h->pt = c->pt;
    h->version = 2;
    h->x = 0;
    h->cc = 0;

    memcpy(c->rbuf, c->sbuf, MAXBUF);

    return 0;
}

int rtp_set_ssrc(RTP_OBJ *c, uint32_t ssrc, char pt)
{
    RTP_HDR *h = (RTP_HDR *)(c->sbuf);
    
    c->pt = pt;
    c->ssrc = ssrc;

    h->ssrc = htonl(c->ssrc);
    h->pt = c->pt;

    return 0;
}

int rtp_send(RTP_OBJ *c, char *buf, int len, struct sockaddr_in *addr)
{
    int n; 
    int ret;
    RTP_HDR *h = (RTP_HDR *)(c->sbuf);

    if ((c->ctrl & RTP_PT_STOP) == 0) {
        h->seq = ntohs(h->seq) + 1;             // increase seq number
        h->seq = htons(h->seq);
        h->ts  = ntohl(h->ts) + c->payload/2;   // increase time-stamp (sampling rate / frame rate)
        h->ts  = htonl(h->ts);
    }

    memcpy(c->sbuf+sizeof(RTP_HDR), buf, len);

#ifdef __APPLE__
    n = sendto(c->fd, (const char *)(c->sbuf), len+sizeof(RTP_HDR), 
        0, (const struct sockaddr *)addr,  
            sizeof(struct sockaddr_in )); 
#else
    n = sendto(c->fd, (const char *)(c->sbuf), len+sizeof(RTP_HDR), 
        MSG_CONFIRM, (const struct sockaddr *)addr,  
            sizeof(struct sockaddr_in )); 
#endif

    if (n < 0) {
        ret = n;
    } else {
        if (n > sizeof(RTP_HDR)) {
            if (n != len+sizeof(RTP_HDR)) {
                perror("RTP");
            }
            ret = n - sizeof(RTP_HDR);
        } else {
            ret = 0;   
        }
        c->tx_len = n;
        c->tx_pay_len = ret;
    }

    //printf("RTP send : %d\n", ret);

	return ret;
}

int rtp_recv(RTP_OBJ *c, char *buf, int len)
{
	int n;
    int ret;
    socklen_t size;

    RTP_HDR *h = (RTP_HDR *)(c->rbuf);
    struct sockaddr_in addr;

    size = sizeof(struct sockaddr_in);
    n = recvfrom(c->fd, (char *)(c->rbuf), len,  
                MSG_WAITALL, (struct sockaddr *) &addr/*&(c->peer)*/, 
                &size); 

    c->peer_len = size;
    c->peer = addr;

    if (n < 0) {
        ret = n;
    } else {
        //printf("Recv : %d\n", n);
        //char str[64];
        //inet_ntop(AF_INET, &(c->peer.sin_addr), str, 64);
        //printf("%d, %d, %8u, %8u, %8u from %s : %d\n", 
        //    n, h->version, ntohl(h->ssrc), c->ssrc, ntohl(h->ts), str, ntohs(c->peer.sin_port));
        if (n > sizeof(RTP_HDR)) {
            if (h->version != 2) {
                printf("Invalid RTP version\n");
                ret = 0;
            }else {
                ret = n - sizeof(RTP_HDR);
                memcpy(buf, c->rbuf+sizeof(RTP_HDR), ret);
            }
        } else {
            ret = 0;
        } 
        c->rx_len = n;
        c->rx_pay_len = ret;
    }

    return ret;
}

int rtp_open(RTP_OBJ *c, int port, int mode, char *svr)    // 1 : server, 0 : client
{ 
    struct timeval timeout;
  
    if ( (c->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
		return -1;
    } 

    timeout.tv_sec = 0;
    timeout.tv_usec = 10;
    setsockopt(c->fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)); 
    //setsockopt(c->fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)); 
  
    int optval = 1;
    setsockopt(c->fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(c->fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    
    memset(&(c->addr), 0, sizeof(c->addr)); 
    memset(&(c->peer), 0, sizeof(c->peer)); 
      
    c->addr.sin_family = AF_INET; 
    c->addr.sin_addr.s_addr = INADDR_ANY; 
    c->addr.sin_port = htons(port); 

    c->svr.sin_family = AF_INET; 
    c->svr.sin_port = htons(port); 

    if (svr) {
        inet_pton(AF_INET, svr, &(c->svr.sin_addr));
    } else {
        c->svr.sin_addr.s_addr = INADDR_ANY; 
    }
 
    //printf("Peer port : %d, %d\n", ntohs(c->addr.sin_port), ntohs(c->peer.sin_port));

    if (mode) {
        if ( bind(c->fd, (const struct sockaddr *)&(c->addr),  
            sizeof(c->addr)) < 0 )  {
       	    perror("bind failed"); 
            return -1;
	    } else {
            printf("bind ok\n");
        }
    } else {
        ;
    }

    return 0; 
} 

int rtp_close(RTP_OBJ *c) 
{
    close(c->fd); 
	return 0;
}

int send_cmd_packet_gen(RTP_OBJ *c, uint16_t cmd,  uint16_t code, void *data, uint16_t len, struct sockaddr_in *addr) 
{
    int ret;
	RTP_CMD_HDR *p = (RTP_CMD_HDR *)(c->cbuf);

	rtp_reset(c, 0x0001, PT_CMD);

	memset(c->cbuf, 0, 20);

    p->ver  = htons(1);
	p->cmd  = htons(cmd);	// command
	p->code = htons(code);	// return code
	p->len  = htons(len);	// parameter len

    memcpy(c->cbuf + sizeof(RTP_CMD_HDR), data, len); 

    ret = rtp_send(c, (char *)(c->cbuf), len+sizeof(RTP_CMD_HDR), addr);

    //printf("cmd : %d, %d\n", cmd, len);

	return ret;
}


int handler_cmd_packet(RTP_OBJ *c, uint32_t id, uint32_t cmd, void *data, int len) 
{


	return 0;
}

