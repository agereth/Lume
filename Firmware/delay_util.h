/* 
 * File:   time_utils.h
 * Author: Laurelindo
 *
 * Created on 5 Октябрь 2009 г., 0:40
 */

#ifndef _TIME_UTILS_H
#define	_TIME_UTILS_H

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <util/atomic.h>

// ========================== Function Prototypes ==============================
bool DelayElapsed(uint16_t *AVar, const uint16_t ADelay);
void DelayReset(uint16_t *AVar);

//void PrintTime (struct Time_t *ATime);

//void IncSecond (struct Time_t *ATime);

#endif	/* _TIME_UTILS_H */

