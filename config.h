/**
 * @file    config.h
 * @brief   definition
 * @version 1.0
 *
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

//#define ENABLE_ALSA             1
#define	DEFAULT_CODEC_ID		1	
#define DEFAULT_SAMPLE_RATE		6000
#define DEFAULT_RTP_PORT		40001	
#define DEFAULT_GROUP			0
#define DEFAULT_GROUP_MASK		0xffffffff

#define RTP_SERVER_SSRC         0x10000
#define TIMEOUT_PTT		        500

#ifdef __cplusplus 
} 
#endif

#endif

