/*
 * kl_time.cpp
 *
 *  Created on: 13.06.2012
 *      Author: kreyl
 */

#include "kl_time.h"
#include "interface.h"
#include "MsgQ.h"

TimeCounter_t Time;

#define DATE2REG(Ytens, Yunits, WeekDay, Mtens, Munits, Dtens, Dunits)  \
    ((Ytens<<20)|(Yunits<<16)|(WeekDay<<13)|(Mtens<<12)|(Munits<<8)|(Dtens<<4)|Dunits)

void TimeCounter_t::Init() {
    if(!BackupSpc::IsSetup()) {
        Printf("Nothing is set\r");
        // ==== Rtc config ====
        BackupSpc::Reset();     // Reset Backup Domain
        Clk.EnableLSE();
        // Let it start for a second
        systime_t StartTime = chVTGetSystemTimeX();
        while(true) {
            if(Clk.LseIsOn()) {
                Printf("32768 clk started\r");
                Clk.SetLSELevel(lselvlLow); // Set Low power of crystal
                Rtc::SetClkSrcLSE();    // Select LSE as RTC Clock Source
                Rtc::EnableClk();       // Enable RTC Clock
                // Init RTC
                chSysLock();
                Rtc::DisableWriteProtection();
                chSysUnlock();
                Rtc::EnterInitMode();
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

                // Setup default time & date
                RTC->TR = 0;    // Time = 0
                RTC->DR = DATE2REG(1, 7, 7, 0, 9, 1, 8); // 17 09 18
                Rtc::ExitInitMode();
                Rtc::WaitSync();
                Rtc::EnableWriteProtection();
                BackupSpc::SetSetup();
                break;
            }
            else if(chVTTimeElapsedSinceX(StartTime) > MS2ST(999)) {
                // Timeout
                Printf("32768 Failure\r");
                Interface.Error("32768 Fail");
                break;
            }
        } // while
    }
    else Printf("Time is setup\r");
    // Enable every-second-IRQ
    // Allow IRQ on EXTI line 20 (connected to Wakeup timer) and select rising edge
    EXTI->IMR |= EXTI_IMR_MR20;
    EXTI->RTSR |= EXTI_RTSR_TR20;
    Rtc::ClearWakeupFlag(); // Otherwise already set flag will not trigger interrupt
    nvicEnableVector(RTC_IRQn, IRQ_PRIO_LOW);
}

void TimeCounter_t::GetDateTime() {
    Curr.Year  = ((RTC->DR >> 20) & 0b1111)* 10 + ((RTC->DR >> 16) & 0b1111) + 2000;
    Curr.Month = ((RTC->DR >> 12) & 0b1  ) * 10 + ((RTC->DR >> 8)  & 0b1111);
    Curr.Day   = ((RTC->DR >>  4) & 0b11 ) * 10 + ((RTC->DR >> 0)  & 0b1111);
    Curr.H     = ((RTC->TR >> 20) & 0b11 ) * 10 + ((RTC->TR >> 16) & 0b1111);
    Curr.M     = ((RTC->TR >> 12) & 0b111) * 10 + ((RTC->TR >> 8)  & 0b1111);
    Curr.S     = ((RTC->TR >>  4) & 0b111) * 10 + ((RTC->TR >> 0)  & 0b1111);
}

void TimeCounter_t::SetDateTime() {
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
CH_IRQ_HANDLER(Vector48) {
    CH_IRQ_PROLOGUE();
    Rtc::ClearWakeupFlag();
    EXTI->PR |= EXTI_PR_PR20;   // Clear exti flag
//    PrintfI("RtcIrq\r");
    chSysLockFromISR();
    EvtMsg_t Msg(evtIdEverySecond);
    EvtQMain.SendNowOrExitI(Msg);
    chSysUnlockFromISR();
    CH_IRQ_EPILOGUE();
}
} // extern c
