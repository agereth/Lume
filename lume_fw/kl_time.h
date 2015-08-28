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

#define BKPREG_CHECK        BKP->DR1     // Register to store Time_is_set variable

#define SECS_DAY            (24UL * 60UL * 60UL)
#define YEAR_MIN            2000
#define YEAR_MAX            2099
#define LEAPYEAR(year)      (!((year) % 4) and (((year) % 100) or !((year) % 400)))
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
};

class TimeCounter_t {
public:
    DateTime_t DateTime;
    void GetDateTime();
    void SetDateTime(DateTime_t *PDateTime);
    void Init();
    bool IsSet() { return (BKPREG_CHECK == 0xA5A5); }
    void PrintDatetime();
};

extern TimeCounter_t Time;

#endif /* KL_TIME_H_ */
