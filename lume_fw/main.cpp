/*
 * File:   main.cpp
 * Author: Kreyl
 * Project: Armlet2South
 *
 * Created on Feb 05, 2013, 20:27
 */

#include "kl_lib.h"
#include "clocking.h"
#include "ch.h"
#include "hal.h"
#include "main.h"
#include "interface.h"
#include "kl_time.h"

App_t App;

int main(void) {
    // ==== Init clock system ====
//    Clk.SetupBusDividers(ahbDiv2, apbDiv1, apbDiv1);
    Clk.UpdateFreqValues();

    // ==== Init OS ====
    halInit();
    chSysInit();

    // ==== Init Hard & Soft ====
    JtagDisable();
    Uart.Init(115200);
    Uart.Printf("\r%S %S", APP_NAME, APP_VERSION);
    Clk.PrintFreqs();

    App.InitThread();
    Lcd.Init();
    // LCD brightness control: remap Timer15 to PB14 & PB15
    AFIO->MAPR2 |= 0x00000001;

    // Time and backup space
    BackupSpc::EnableAccess();
    Interface.Reset();
    Time.Init();
    // ==== Main cycle ====
    App.ITask();
}

__attribute__ ((__noreturn__))
void App_t::ITask() {
    while(true) {
        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
        if(EvtMsk & EVTMSK_KEY) {

        }

        if(EvtMsk & EVTMSK_SECOND) {
//            Uart.Printf("\rNewSecond");
            Time.GetDateTime();
            Time.dtNow.Print();
            Interface.DisplayTime();
        }
    } // while true
}
