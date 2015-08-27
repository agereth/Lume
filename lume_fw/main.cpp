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
#include "uart.h"

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

//    Keys.Init(chThdSelf());

    // ==== Main cycle ====
    while(true) {
        eventmask_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
        if(EvtMsk & EVTMSK_KEY) {
//            if(LedIsOn) Led.SetSmoothly(0);
//            else Led.SetSmoothly(LED_TOP_VALUE);
//            LedIsOn = !LedIsOn;
        }

    } // while
}

