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
//#include "SimpleSensors.h"
#include "buttons.h"

App_t App;
Interface_t Interface;

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
    AFIO->MAPR2 |= AFIO_MAPR2_TIM15_REMAP;

    // Time and backup space
    BackupSpc::EnableAccess();
    Interface.Reset();
    Time.Init();
    Time.GetDateTime(&App.dtNow);

    PinSensors.Init();
    // LCD off and save settings timeout
    App.TmrBtnpressTimeout.Init(chThdSelf(), MS2ST(BTNPRESS_TIMEOUT_MS), EVTMSK_BTNPRESS_TIMEOUT, tvtOneShot);

    // ==== Main cycle ====
    App.ITask();
}

__attribute__ ((__noreturn__))
void App_t::ITask() {
    while(true) {
        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
        if(EvtMsk & EVTMSK_BUTTONS) {
            BtnEvtInfo_t EInfo;
            while(ButtonEvtBuf.Get(&EInfo) == OK) {
                Uart.Printf("\rEinfo: %u, %u", EInfo.Type, EInfo.BtnID[0]);
                // Process keypress excluding repeat of select button
                if(!(EInfo.Type == 4 and EInfo.BtnID[0] == 0)) BtnHandler(EInfo.BtnID[0]);
            }
        }

        if(EvtMsk & EVTMSK_SECOND) {
//            Uart.Printf("\rNewSecond");
            if(Selected == selNone) {   // Switch when time is set only
                Time.GetDateTime(&dtNow);
//            Time.dtNow.Print();
                Interface.DisplayTime();
            }
        }

        if(EvtMsk & EVTMSK_BTNPRESS_TIMEOUT) {
            Lcd.Backlight(0);
            if(Selected != selNone) { // Timeout when something selected
                Selected = selNone;
                Time.SetDateTime(&dtNow); // Save settings
                Interface.DisplayTime();
            }
        }
    } // while true
} // Main thread

void App_t::BtnHandler(uint8_t Btn) {
    Lcd.Backlight(81);
    TmrBtnpressTimeout.Restart();
    switch((BtnName_t)Btn) {
        case btnSelect:
            if(Selected == selDay) {
                Selected = selNone;
                Time.SetDateTime(&dtNow); // Save settings
            }
            else Selected = (Selected_t)((uint8_t)Selected + 1);
            Interface.DisplaySelected();
            break;

        case btnPlus:
            switch(Selected) {
                case selH: dtNow.IncH(); break;
                case selM: dtNow.IncM(); break;
                case selS: dtNow.S = 0;  break; // Just reset seconds
                case selYear:  dtNow.IncYear(); break;
                case selMonth: dtNow.IncMonth(); break;
                case selDay:   dtNow.IncDay(); break;
                case selNone: break;
            }
            Interface.DisplaySelected();
            break;

        case btnMinus:
            switch(Selected) {
                case selH: dtNow.DecH(); break;
                case selM: dtNow.DecM(); break;
                case selS: dtNow.S = 0;  break; // Just reset seconds
                case selYear:  dtNow.DecYear(); break;
                case selMonth: dtNow.DecMonth(); break;
                case selDay:   dtNow.DecDay(); break;
                case selNone: break;
            }
            Interface.DisplaySelected();
            break;
    } // switch btn
}
