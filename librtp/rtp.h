#ifndef _RTP_H_
#define _RTP_H_

#include <stdint.h>

#define PORT     			40000 
#define MAXLINE 			1024 
#define MAXBUF				4096

#define PAYSIZE				320
//#define PAYSIZE					(320 * 2)
//#define PAYSIZE				32
//#define PAYSIZE				 12

typedef enum {
	PT_PCMU		= 0,
	PT_PCMA		= 8,
	PT_LPCM		= 96,
	PT_SPEEX	= 110,
	PT_CMD		= 120
} RTCP_TYPE;

#define RTP_PT_STOP				(1 << 0)	// stop pt increment

typedef struct {
	int fd;
	int port;
	int mode;
	int rate;					// sampling rate
	int payload;				// payload size (320)
	uint32_t tx_len;			// last transfered tx packet length		
	uint32_t rx_len;			// last transfered rx packet length		
	uint32_t tx_pay_len;		// last transfered tx payload length		
	uint32_t rx_pay_len;		// last transfered rx payload length		
	uint32_t peer_len;
	struct sockaddr_in svr;		// addr for server
	struct sockaddr_in addr;	// bind addr for server
	struct sockaddr_in peer;	// peer addr
	uint32_t ssrc;
	uint32_t ctrl;				// 1 : stop seq & pt
	char pt;
	char sbuf[MAXBUF];			// send buffer
	char rbuf[MAXBUF];			// recv buffer
	char cbuf[MAXBUF];
} RTP_OBJ;

/*
 * RPT data header (HOST : LE)
 */

typedef struct {
    uint8_t cc : 4;      /* CSRC count             */
    uint8_t x : 1;       /* header extension flag  */
    uint8_t p : 1;       /* padding flag           */
    uint8_t version : 2; /* protocol version       */
    uint8_t pt : 7;      /* payload type           */
    uint8_t m : 1;       /* marker bit             */
    uint16_t seq;        /* sequence number        */
    uint32_t ts;         /* timestamp              */
    uint32_t ssrc;       /* synchronization source */
} RTP_HDR;

/*
 * Custom command packet
 */

// Custom command

#define PT_CMD_REQ		(0 << 12)	// Request
#define PT_CMD_ACK		(1 << 12)	// Ack
#define PT_CMD_NOF		(2 << 12)	// Notifiy 

#define PT_CMD_STAT		(0)			// C->S->C (STAT)
#define PT_CMD_JOIN 	(1)			// C->S
#define PT_CMD_PING 	(2)			// C->S
#define PT_CMD_BYE 		(3)			// C->S
#define PT_CMD_PTT 		(4)			// C->S

#define PT_CMD(a, b)	(a | b)

#if 0
typedef enum {
	PT_CMD_REQ_REG		= 0,	//  in : serial number (64bit = 8 bytes)
	PT_CMD_ACK_REG		= 1,	// out : ID, Group ID
	PT_CMD_REQ_PING		= 2,	//  in : ID
	PT_CMD_ACK_PING		= 3,	// out : ID
	PT_CMD_REQ_BYE		= 4,	//  in : ID
	PT_CMD_ACK_BYE		= 5,	// out : ID
	PT_CMD_REQ_PTT		= 6,	//  in : ID, Group Mask, Departed Mask, Priority
	PT_CMD_ACK_PTT		= 7,	// out : ID, none (nack)
} PT_CMD_TYPE;
#else

#define PT_CMD_REQ_JOIN		(PT_CMD_REQ, PT_CMD_JOIN)
#define PT_CMD_ACK_JOIN		(PT_CMD_ACK, PT_CMD_JOIN)
#define PT_CMD_REQ_PTT		(PT_CMD_REQ, PT_CMD_PTT )
#define PT_CMD_ACK_PTT		(PT_CMD_ACK, PT_CMD_PTT )
#define PT_CMD_REQ_BYE		(PT_CMD_REQ, PT_CMD_BYE )
#define PT_CMD_ACK_BYE		(PT_CMD_ACK, PT_CMD_BYE )
#define PT_CMD_NOF_STAT		(PT_CMD_NOF, PT_CMD_STAT)

#endif

typedef struct {
	uint16_t ver;		// command version
	uint16_t cmd;		// command
	uint16_t code;		// return code (0 : ack, others : nack for ACK)
	uint16_t len;		// default data is 12 bytes (4 x 3)
} RTP_CMD_HDR;

typedef struct {
	RTP_HDR hdr;
	RTP_CMD_HDR c_hdr;		
} RTP_CMD_PKT;

//
// Command
//

typedef struct {
	uint32_t sn[2];
	uint32_t id;			// request id (> 1), 8 bit group id (0 ~ 31) + 24 bit 0
	char name[64];
} RTP_CMD_DATA_JOIN;

typedef struct {
	uint32_t id;			// request id (> 1), 8 bit group id (0 ~ 31) + 24 bit 0
} RTP_CMD_DATA_ID;
	
typedef struct {
	uint32_t id;			// source id
	uint32_t g_mask;		// destination group mask (bit : 0 ~ 31)
	uint32_t d_mask;		// destination depart mask (bit : 0/1/2/3/4/5)
	uint32_t prio;			// PTT priority level
} RTP_CMD_DATA_PTT;

typedef struct {
	uint32_t id;			// source id
	uint32_t g_mask;		// destination group mask (bit : 0 ~ 31)
	uint32_t d_mask;		// destination depart mask (bit : 0/1/2/3/4/5)
	uint32_t s_mask;		// state mask (1 : PTT)
} RTP_CMD_DATA_STAT;

//
// Command JOIN 
//

typedef struct {
	RTP_HDR hdr;
	RTP_CMD_HDR c_hdr;	
	RTP_CMD_DATA_JOIN data;
} RTP_CMD_PKT_REQ_JOIN;

typedef struct {
	RTP_HDR hdr;
	RTP_CMD_HDR c_hdr;	
	RTP_CMD_DATA_ID data;	
} RTP_CMD_PKT_ACK_JOIN;

//
// Command BYE
//

typedef struct {
	RTP_HDR hdr;
	RTP_CMD_HDR c_hdr;	
	RTP_CMD_DATA_ID data;	
} RTP_CMD_PKT_REQ_BYE;

typedef struct {
	RTP_HDR hdr;
	RTP_CMD_HDR c_hdr;	
	RTP_CMD_DATA_ID data;	
} RTP_CMD_PKT_ACK_BYE;

//
// Command PING 
//

typedef struct {
	RTP_HDR hdr;
	RTP_CMD_HDR c_hdr;	
	RTP_CMD_DATA_ID data;	
} RTP_CMD_PKT_REQ_PING;

typedef struct {
	RTP_HDR hdr;
	RTP_CMD_HDR c_hdr;	
	RTP_CMD_DATA_ID data;	
} RTP_CMD_PKT_ACK_PING;

//
// Command PTT 
//

typedef struct {
	RTP_HDR hdr;
	RTP_CMD_HDR c_hdr;	
	RTP_CMD_DATA_PTT data;
} RTP_CMD_PKT_REQ_PTT;		// server PTT req forward to client

typedef struct {
	RTP_HDR hdr;
	RTP_CMD_HDR c_hdr;	
	RTP_CMD_DATA_ID data;
} RTP_CMD_PKT_ACK_PTT;		// for PTT grant

//
// Notify RPT (per 20 ms x 4)
//

#define NOF_PTT_ON			(1 << 0)

typedef struct {
	RTP_HDR hdr;
	RTP_CMD_HDR c_hdr;	
	RTP_CMD_DATA_STAT data;
} RTP_CMD_PKT_NOF_STAT;		 



int rtp_reset(RTP_OBJ *c, uint32_t ssrc, char pt);
int rtp_set_ssrc(RTP_OBJ *c, uint32_t ssrc, char pt);
int rtp_send(RTP_OBJ *c, char *buf, int len, struct sockaddr_in *addr);
int rtp_recv(RTP_OBJ *c, char *buf, int len);
int rtp_open(RTP_OBJ *c, int port, int mode, char *svr);
int rtp_close(RTP_OBJ *c);

int send_cmd_packet(RTP_OBJ *c, uint16_t cmd, uint16_t code, struct sockaddr_in *addr);
int send_cmd_packet_gen(RTP_OBJ *c, uint16_t cmd,  uint16_t code, void *data, uint16_t len, struct sockaddr_in *addr); 

#endif
