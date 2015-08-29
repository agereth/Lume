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
#include "uart.h"

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
    void IncH() { H = (H+1 > 23)? 0: H+1; }
    void IncM() { M = (M+1 > 59)? 0: M+1; }
    void IncYear() { Year++; }
    void IncMonth() { Month = (Month+1 > 12)? 1 : Month+1; }
    void IncDay() {
        uint32_t Leap = LEAPYEAR(Year)? 1 : 0;
        uint8_t MaxDayCnt = MonthDays[Leap][Month-1];
        Day = (Day+1 > MaxDayCnt)? 1: Day+1;
    }

    void DecH() { H = (H <= 0)? 23 : H-1; }
    void DecM() { M = (M <= 0)? 59 : M-1; }
    void DecYear() { Year--; }
    void DecMonth() { Month = (Month <= 1)? 12 : Month-1; }
    void DecDay() {
        if(Day <= 1) {
            uint32_t Leap = LEAPYEAR(Year)? 1 : 0;
            Day = MonthDays[Leap][Month-1];
        }
        else Day--;
    }

    void Print() const { Uart.Printf("\r%04u/%02u/%02u %02u:%02u:%02u", Year, Month, Day, H, M, S); }
};

class TimeCounter_t {
public:
    void GetDateTime(DateTime_t *PDateTime);
    void SetDateTime(DateTime_t *PDateTime);
    void Init();
};

extern TimeCounter_t Time;

#endif /* KL_TIME_H_ */
