/*
 * kl_time.cpp
 *
 *  Created on: 13.06.2012
 *      Author: kreyl
 */

#include "kl_time.h"
#include "misc.h"
#include "stm32f10x_pwr.h"
#include "lcd1200.h"

TimeCounter_t Time;

//#define RTC_OUTPUT_ENABLE

void TimeCounter_t::Init() {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);    // Enable PWR and BKP clocks
    PWR_BackupAccessCmd(ENABLE);    // Allow access to BKP Domain
    // Check if time is set
    if (!IsSet()) {
        TimeIsSet = false;
        Uart.Printf("Nothing is set\r");
        // ==== Rtc config ====
        BKP_DeInit();                   // Reset Backup Domain
        RCC_LSEConfig(RCC_LSE_ON);      // Enable LSE
        uint32_t FTmr;
        Delay.Reset(&FTmr);
        while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {    // Wait till LSE is ready
            if (Delay.Elapsed(&FTmr, 2700)) break;
        }
        // Check if ok
        if (RCC_GetFlagStatus(RCC_FLAG_LSERDY)) {
            Uart.Printf("32768 clk started\r");
            RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);     // Select LSE as RTC Clock Source
            RCC_RTCCLKCmd(ENABLE);                      // Enable RTC Clock
            RTC_WaitForSynchro();                       // Wait for RTC registers synchronization
            RTC_WaitForLastTask();  // Wait until last write operation on RTC registers has finished
            // Set RTC prescaler: set RTC period to 1sec
            RTC_SetPrescaler(32767);// RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1)
            RTC_WaitForLastTask();  // Wait until last write operation on RTC registers has finished
            //Lcd.Printf(0,1, "32768 Ok");
        }
        else {
            Uart.Printf("32768 Fail\r");
            Lcd.Printf(0,0, "32768 Fail");
        }
        // Set default datetime values
        SetDateTime((DateTime_t){0,0,0, 2000,1,1});
        BKP_WriteBackupRegister(BKPREG_CHECK, 0xA5A5);  // Signal is set
    }
    else {
        Uart.Printf("Something is stored\r");
        RTC_WaitForSynchro();
        RTC_WaitForLastTask();
    }

#ifdef RTC_OUTPUT_ENABLE
    BKP_TamperPinCmd(DISABLE);      // To output RTCCLK/64 on Tamper pin, the tamper functionality must be disabled
    BKP_RTCOutputConfig(BKP_RTCOutputSource_CalibClock);    // Enable RTC Clock Output on Tamper Pin
#endif
}

void TimeCounter_t::GetDateTime(DateTime_t *PDateTime) {
    uint32_t time = RTC_GetCounter();
    uint32_t dayclock, DayCount;
    uint32_t year = YEAR_MIN;

    // Calculate time
    dayclock = time % SECS_DAY;
    PDateTime->S = dayclock % 60;
    PDateTime->M = (dayclock % 3600) / 60;
    PDateTime->H = dayclock / 3600;

    // Calculate year
    DayCount = time / SECS_DAY;
    while(DayCount >= YEARSIZE(year)) {
        DayCount -= YEARSIZE(year);
        year++;
    }
    PDateTime->Year = year;
    // Calculate month
    PDateTime->Month = 0;
    uint32_t Leap = LEAPYEAR(year)? 1 : 0;
    while (DayCount >= MonthDays[Leap][PDateTime->Month]) {
        DayCount -= MonthDays[Leap][PDateTime->Month];
        PDateTime->Month++;
    }
    PDateTime->Month++; // not in [0;11], but in [1;12]
    PDateTime->Day = DayCount + 1;
}

void TimeCounter_t::SetDateTime(DateTime_t ADateTime) {
    uint32_t DayCount=0, seconds=0;
    // Count days elapsed since YEAR_MIN
    for(int32_t y=YEAR_MIN; y<ADateTime.Year; y++)
        DayCount += YEARSIZE(y);
    // Count days in monthes elapsed
    uint32_t Leap = LEAPYEAR(ADateTime.Year)? 1 : 0;
    for(int32_t m=0; m < ADateTime.Month-1; m++)
        DayCount += MonthDays[Leap][m];

    DayCount += ADateTime.Day-1;
    seconds = ADateTime.H*3600 + ADateTime.M*60 + ADateTime.S;
    seconds += DayCount * SECS_DAY;

    RTC_WaitForLastTask();
    RTC_SetCounter(seconds);
    RTC_WaitForLastTask();
}
