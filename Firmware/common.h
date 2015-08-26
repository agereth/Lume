/* 
 * File:   common.h
 * Author: Kreyl Laurelindo
 *
 * Created on 9 Июнь 2010 г., 11:07
 */

#ifndef _COMMON_H
#define	_COMMON_H

#define nop( )  asm volatile ("nop\n\t" ::) 
#define FORCE_INLINE inline __attribute__ ((__always_inline__))
#define io_uint8_t  volatile uint8_t    // for io register pointer

#endif	/* _COMMON_H */

