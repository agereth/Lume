/*
 * kl_time.h
 *
 *  Created on: 13.06.2012
 *      Author: kreyl
 */

#ifndef KL_TIME_H_
#define KL_TIME_H_

#include <stdint.h>
#include "kl_lib.h"
#include "stm32f10x_rtc.h"
#include "stm32f10x_bkp.h"

#define BKPREG_CHECK        BKP_DR1     // Register to store Time_is_set variable

#define SECS_DAY            (24L * 60L * 60L)
#define YEAR_MIN            2000
#define YEAR_MAX            2099
#define LEAPYEAR(year)      (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year)      (LEAPYEAR(year) ? 366 : 365)

const uint8_t MonthDays[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

struct DateTime_t {
    int32_t H, M, S;
    int32_t Year;
    int32_t Month;
    int32_t Day;
} PACKED;

class TimeCounter_t {
public:
    bool TimeIsSet;
    void GetDateTime(DateTime_t *PDateTime);
    void SetDateTime(DateTime_t ADateTime);
    void Init();
    bool IsSet() {return (BKP_ReadBackupRegister(BKPREG_CHECK) == 0xA5A5); }
    // Delay utils
    bool SecElapsed(uint32_t *PTimer, uint32_t ASeconds) {
        uint32_t t = RTC_GetCounter();
        if ((t - *PTimer) >= ASeconds) {
            *PTimer = t; // Reset delay
            return true;
        }
        else return false;
    }
    void Bypass(uint32_t *AVar, const uint32_t ADelay) { *AVar = RTC_GetCounter() - ADelay; }
};

extern TimeCounter_t Time;

#endif /* KL_TIME_H_ */
