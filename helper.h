/**
 * @file    helper.h
 * @brief   helper function interface header
 * @version 1.0
 *
 */

#ifndef _HELPER_H_
#define _HELPER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t start_timer(void);
uint64_t check_timer(uint64_t start);

#ifdef __cplusplus 
} 
#endif

#endif

