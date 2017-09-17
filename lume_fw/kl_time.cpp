/*
 * kl_time.cpp
 *
 *  Created on: 13.06.2012
 *      Author: kreyl
 */

#include "kl_time.h"
//#include "interface.h"
//#include "main.h"
//#include "BackupRegSettings.h"

TimeCounter_t Time;

//#define RTC_OUTPUT_ENABLE

#define DATE2REG(Ytens, Yunits, WeekDay, Mtens, Munits, Dtens, Dunits)  \
    ((Ytens<<20)|(Yunits<<16)|(WeekDay<<13)|(Mtens<<12)|(Munits<<8)|(Dtens<<4)|Dunits)

void TimeCounter_t::Init() {
    if (!BackupSpc::IsSetup()) {
        Printf("Nothing is set\r");
        // ==== Rtc config ====
        BackupSpc::Reset();     // Reset Backup Domain
        Clk.EnableLSE();
        // Let it start for a second
        systime_t StartTime = chVTGetSystemTimeX();
        while(true) {
            if(Clk.LseIsOn()) {
                Printf("32768 clk started\r");
//                Clk.SetLSELevel(lselvlLow); // XXX
                Rtc::SetClkSrcLSE();    // Select LSE as RTC Clock Source
                Rtc::EnableClk();       // Enable RTC Clock
#if defined STM32F10X_LD_VL
                Rtc::WaitForSync();     // Wait for RTC registers synchronization
                Rtc::WaitForLastTask();
                // Set RTC prescaler: set RTC period to 1sec
                Rtc::SetPrescaler(32767);// RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1)
                //Lcd.Printf(0,1, "32768 Ok");
#elif defined STM32F072xB
                // Init RTC
                chSysLock();
                Rtc::DisableWriteProtection();
                chSysUnlock();
                // Enter initialization mode
                RTC->ISR |= RTC_ISR_INIT;
                while(!BitIsSet(RTC->ISR, RTC_ISR_INITF));
                // Program both the prescaler factors
                RTC->PRER = (0x7FUL << 16) | (0xFFUL);  // async pre = 128, sync = 256 => 32768->1
                // Clear RTC_CR FMT (24h format), OSEL (no output), disable WUTE
                RTC->CR &= ~(RTC_CR_FMT | RTC_CR_OSEL | RTC_CR_WUTE | RTC_CR_WUCKSEL);

                // ==== Setup wake-up timer ====
                // Wait WakeUp timer to allow changes
                while(!BitIsSet(RTC->ISR, RTC_ISR_WUTWF));
                RTC->WUTR = 0;      // Flag is set every WUTR+1 cycles => every second with 1Hz input freq
                RTC->CR |= 0b100UL; // ck_spre (usually 1 Hz) clock is selected
                RTC->CR |= RTC_CR_WUTE | RTC_CR_WUTIE;  // Enable Wake-up timer and its irq

                // Setup initial time & date
                RTC->TR = 0;    // Time = 0
                RTC->DR = DATE2REG(1, 7, 7, 0, 9, 1, 8); // 17 09 18
                RTC->ISR &= ~RTC_ISR_INIT; // Exit Initialization mode
                // Wait for sync
                Rtc::WaitSync();
                Rtc::EnableWriteProtection();
#endif
//                BackupSpc::SetSetup();
                break;
            }
            else if(chVTTimeElapsedSinceX(StartTime) > MS2ST(999)) {
                // Timeout
                Printf("32768 Failure\r");
//            Interface.Error("32768 Fail");
                break;
            }
        } // while
        // Set default datetime values
        DateTime_t dt{0,0,0, 2017,9,17};
        SetDateTime(&dt);
    }
    else { // Set
        Printf("Time is setup\r");
//        Rtc::WaitForSync();
//        Rtc::WaitForLastTask();
    }
    // Enable every-second-IRQ
#if defined STM32F10X_LD_VL
    Rtc::EnableSecondIRQ();
#elif defined STM32F072xB
    // Allow IRQ on EXTI line 20 (connected to Wakeup timer) and select rising edge
    EXTI->IMR |= EXTI_IMR_MR20;
    EXTI->RTSR |= EXTI_RTSR_TR20;
#endif
    nvicEnableVector(RTC_IRQn, IRQ_PRIO_LOW);

#ifdef RTC_OUTPUT_ENABLE
    BKP_TamperPinCmd(DISABLE);      // To output RTCCLK/64 on Tamper pin, the tamper functionality must be disabled
    BKP_RTCOutputConfig(BKP_RTCOutputSource_CalibClock);    // Enable RTC Clock Output on Tamper Pin
#endif
}

void TimeCounter_t::GetDateTime(DateTime_t *PDateTime) {
//    uint32_t time = ((uint32_t)RTC->CNTH << 16) | ((uint32_t)RTC->CNTL);
//    uint32_t dayclock, DayCount;
//    uint32_t year = YEAR_MIN;
//
//    // Calculate time
//    dayclock = time % SECS_DAY;
//    PDateTime->S = dayclock % 60;
//    PDateTime->M = (dayclock % 3600) / 60;
//    PDateTime->H = dayclock / 3600;
//
//    // Calculate year
//    DayCount = time / SECS_DAY;
//    while(DayCount >= YEARSIZE(year)) {
//        DayCount -= YEARSIZE(year);
//        year++;
//    }
//    PDateTime->Year = year;
//    // Calculate month
//    PDateTime->Month = 0;
//    uint32_t Leap = LEAPYEAR(year)? 1 : 0;
//    while (DayCount >= MonthDays[Leap][PDateTime->Month]) {
//        DayCount -= MonthDays[Leap][PDateTime->Month];
//        PDateTime->Month++;
//    }
//    PDateTime->Month++; // not in [0;11], but in [1;12]
//    PDateTime->Day = DayCount + 1;
}

void TimeCounter_t::SetDateTime(DateTime_t *PDateTime) {
//    uint32_t DayCount=0, seconds=0;
//    // Count days elapsed since YEAR_MIN
//    for(int32_t y=YEAR_MIN; y<PDateTime->Year; y++) DayCount += YEARSIZE(y);
//    // Count days in monthes elapsed
//    uint32_t Leap = LEAPYEAR(PDateTime->Year)? 1 : 0;
//    for(int32_t m=0; m < PDateTime->Month-1; m++) DayCount += MonthDays[Leap][m];
//
//    DayCount += PDateTime->Day-1;
//    seconds = PDateTime->H*3600 + PDateTime->M*60 + PDateTime->S;
//    seconds += DayCount * SECS_DAY;
//
//    Rtc::WaitForLastTask();
//    Rtc::SetCounter(seconds);
//    Rtc::WaitForLastTask();
}

extern "C" {
#if defined STM32F10X_LD_VL
CH_IRQ_HANDLER(RTC_IRQHandler) {
    CH_IRQ_PROLOGUE();
    Rtc::ClearSecondIRQFlag();
//    PrintfI("\rRtcIrq");
    App.SignalEvtI(EVTMSK_SECOND);
    CH_IRQ_EPILOGUE();
}
#elif defined STM32F072xB
CH_IRQ_HANDLER(Vector48) {
    CH_IRQ_PROLOGUE();
    Rtc::ClearWakeupFlag();
    EXTI->PR |= EXTI_PR_PR20;   // Clear exti flag
    PrintfI("RtcIrq\r");
    CH_IRQ_EPILOGUE();
}
#endif
} // extern c
