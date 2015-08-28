/*
 * kl_time.cpp
 *
 *  Created on: 13.06.2012
 *      Author: kreyl
 */

#include "kl_time.h"
#include "interface.h"
#include "main.h"

TimeCounter_t Time;

//#define RTC_OUTPUT_ENABLE

void TimeCounter_t::Init() {
    if (!IsSet()) { // Not set
        Uart.Printf("\rNothing is set");
        // ==== Rtc config ====
        BackupSpc::Reset();     // Reset Backup Domain
        Clk.StartLSE();         // Enable LSE
        // Let it start
        for(int i=0; i < 99; i++) {
            chThdSleepMilliseconds(54);
            if(Clk.IsLseOn()) break;
        }
        // Check if ok
        if(Clk.IsLseOn()) {
            Uart.Printf("\r32768 clk started");
            Rtc::SetClkSrcLSE();    // Select LSE as RTC Clock Source
            Rtc::EnableClk();       // Enable RTC Clock
            Rtc::WaitForSync();     // Wait for RTC registers synchronization
            Rtc::WaitForLastTask();
            // Set RTC prescaler: set RTC period to 1sec
            Rtc::SetPrescaler(32767);// RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1)
            //Lcd.Printf(0,1, "32768 Ok");
            BKPREG_CHECK = 0xA5A5;  // Signal is set
        }
        else {
            Uart.Printf("\r32768 Failure");
            Interface.Error("32768 Fail");
        }
        // Set default datetime values
        dtNow = (DateTime_t){0,0,0, 2000,1,1};
        SetDateTime(&dtNow);
    }
    else { // Set
        Uart.Printf("\rSomething is stored");
        Rtc::WaitForSync();
        Rtc::WaitForLastTask();
    }
    // Enable every-second-IRQ
    Rtc::EnableSecondIRQ();
    nvicEnableVector(RTC_IRQn, CORTEX_PRIORITY_MASK(IRQ_PRIO_LOW));

#ifdef RTC_OUTPUT_ENABLE
    BKP_TamperPinCmd(DISABLE);      // To output RTCCLK/64 on Tamper pin, the tamper functionality must be disabled
    BKP_RTCOutputConfig(BKP_RTCOutputSource_CalibClock);    // Enable RTC Clock Output on Tamper Pin
#endif
}

void TimeCounter_t::GetDateTime() {
    uint32_t time = ((uint32_t)RTC->CNTH << 16) | ((uint32_t)RTC->CNTL);
    uint32_t dayclock, DayCount;
    uint32_t year = YEAR_MIN;

    // Calculate time
    dayclock = time % SECS_DAY;
    dtNow.S = dayclock % 60;
    dtNow.M = (dayclock % 3600) / 60;
    dtNow.H = dayclock / 3600;

    // Calculate year
    DayCount = time / SECS_DAY;
    while(DayCount >= YEARSIZE(year)) {
        DayCount -= YEARSIZE(year);
        year++;
    }
    dtNow.Year = year;
    // Calculate month
    dtNow.Month = 0;
    uint32_t Leap = LEAPYEAR(year)? 1 : 0;
    while (DayCount >= MonthDays[Leap][dtNow.Month]) {
        DayCount -= MonthDays[Leap][dtNow.Month];
        dtNow.Month++;
    }
    dtNow.Month++; // not in [0;11], but in [1;12]
    dtNow.Day = DayCount + 1;
}

void TimeCounter_t::SetDateTime(DateTime_t *PDateTime) {
    uint32_t DayCount=0, seconds=0;
    // Count days elapsed since YEAR_MIN
    for(int32_t y=YEAR_MIN; y<PDateTime->Year; y++) DayCount += YEARSIZE(y);
    // Count days in monthes elapsed
    uint32_t Leap = LEAPYEAR(PDateTime->Year)? 1 : 0;
    for(int32_t m=0; m < PDateTime->Month-1; m++) DayCount += MonthDays[Leap][m];

    DayCount += PDateTime->Day-1;
    seconds = PDateTime->H*3600 + PDateTime->M*60 + PDateTime->S;
    seconds += DayCount * SECS_DAY;

    Rtc::WaitForLastTask();
    Rtc::SetCounter(seconds);
    Rtc::WaitForLastTask();
}

extern "C" {
CH_IRQ_HANDLER(RTC_IRQHandler) {
    CH_IRQ_PROLOGUE();
    Rtc::ClearSecondIRQFlag();
//    Uart.PrintfI("\rRtcIrq");
    App.SignalEvtI(EVTMSK_SECOND);
    CH_IRQ_EPILOGUE();
}
} // extern c
