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
#include "stones.h"
#include "buttons.h"
#include "BackupRegSettings.h"

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

    Stones.Init();

    PinSensors.Init();
    // LCD off and save settings timeout
    App.TmrBtnpressTimeout.Init(chThdSelf(), MS2ST(BTNPRESS_TIMEOUT_MS), EVTMSK_BTNPRESS_TIMEOUT, tvtOneShot);

    App.ITask();
}

__attribute__ ((__noreturn__))
void App_t::ITask() {
    // ==== Init ====
    // Read Brightness from Backup reg
    if(IsStored()) BrightnessPercent = BKPREG_BRIGHTNESS;
    else BrightnessPercent = BRIGHTNESS_TOP;
    if(BrightnessPercent > BRIGHTNESS_TOP or BrightnessPercent == 0) BrightnessPercent = BRIGHTNESS_TOP;
    BKPREG_BRIGHTNESS = BrightnessPercent; // Save possibly new value
    Uart.Printf("\rBrightness = %u", BrightnessPercent);
    Stones.Brightness = BrightnessPercent2Pwm(BrightnessPercent);

    // ==== Main cycle ====
    while(true) {
        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
        if(EvtMsk & EVTMSK_BUTTONS) {
            BtnEvtInfo_t EInfo;
            while(ButtonEvtBuf.Get(&EInfo) == OK) {
//                Uart.Printf("\rEinfo: %u, %u", EInfo.Type, EInfo.BtnID[0]);
                // Process keypress excluding repeat of select button
                if(!(EInfo.Type == 4 and EInfo.BtnID[0] == 0)) BtnHandler(EInfo.BtnID[0]);
            }
        }

        if(EvtMsk & EVTMSK_SECOND) {
//            Uart.Printf("\rNewSecond");
//            if(Selected == selNone) {   // Switch when time is set only
                Time.GetDateTime(&dtNow);
//            Time.dtNow.Print();
                Interface.DisplayTime();
                Interface.DisplayBrightness();
                Time2Stones();
//            }
        }

        if(EvtMsk & EVTMSK_BTNPRESS_TIMEOUT) {
            Lcd.Backlight(0);
            if(Selected != selNone) { // Timeout when something selected
                Selected = selNone;
//                Time.SetDateTime(&dtNow); // Save settings
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
            if(Selected == selBrightness) Selected = selNone;
            else Selected = (Selected_t)((uint8_t)Selected + 1);
            break;

        case btnPlus:
            switch(Selected) {
                case selNone: Stones.FadeIn({0,5}); break;  // FIXME
                case selH: dtNow.IncH(); break;
                case selM: dtNow.IncM(); break;
                case selS: dtNow.S = 0;  break; // Just reset seconds
                case selYear:  dtNow.IncYear(); break;
                case selMonth: dtNow.IncMonth(); break;
                case selDay:   dtNow.IncDay(); break;
                case selBrightness: IncBrightness(); break;
            }
            if(Selected != selNone and Selected != selBrightness) Time.SetDateTime(&dtNow); // Save settings
            break;

        case btnMinus:
            switch(Selected) {
                case selNone: Stones.FadeOut({0,5}); break; // FIXME
                case selH: dtNow.DecH(); break;
                case selM: dtNow.DecM(); break;
                case selS: dtNow.S = 0;  break; // Just reset seconds
                case selYear:  dtNow.DecYear(); break;
                case selMonth: dtNow.DecMonth(); break;
                case selDay:   dtNow.DecDay(); break;
                case selBrightness: DecBrightness(); break;
            }
            if(Selected != selNone and Selected != selBrightness) Time.SetDateTime(&dtNow); // Save settings
            break;
    } // switch btn
    Interface.DisplayTime();
    Interface.DisplayBrightness();
}

void App_t::IncBrightness() {
    if(BrightnessPercent == 1) BrightnessPercent = 5;
    else if(BrightnessPercent < 100) BrightnessPercent += 5;
    Interface.DisplayBrightness();
    Stones.Brightness = BrightnessPercent2Pwm(BrightnessPercent);
//    Uart.Printf("\rBrt = %u %u", BrightnessPercent, Stones.Brightness);
    BKPREG_BRIGHTNESS = BrightnessPercent; // Save new value
}
void App_t::DecBrightness() {
    if(BrightnessPercent == 5) BrightnessPercent = 1;
    else if(BrightnessPercent > 5) BrightnessPercent -= 5;
    Interface.DisplayBrightness();
    Stones.Brightness = BrightnessPercent2Pwm(BrightnessPercent);
//    Uart.Printf("\rBrt = %u %u", BrightnessPercent, Stones.Brightness);
    BKPREG_BRIGHTNESS = BrightnessPercent; // Save new value
}

uint8_t App_t::BrightnessPercent2Pwm(uint8_t Brt) {
    for(uint32_t i=0; i<BRT_CNT; i++) {
        if(BrtPercentPwmTable[i].Percent >= Brt) return BrtPercentPwmTable[i].Pwm;
    }
    return BrtPercentPwmTable[0].Pwm;
}

void App_t::Time2Stones() {
    static int32_t Hour, HMPrev;
    // Check if hour changed
    if(dtNow.H != Hour) {
        Stones.FadeOut(H2Indx[Hour]);
        Hour = dtNow.H;
        Stones.FadeIn(H2Indx[Hour]);
    }
    // Check if hyperminute changed
    int32_t S = dtNow.M * 60 + dtNow.S;
    int32_t HMNow = S / 150;   // 1 HyperMinute == 2.5 Minutes == 150 s
    if(HMNow != HMPrev) {
        // Check if odd
        if(HMNow & 0x01) { // 1,3,5,7...
            uint8_t HM = (HMNow / 2) + 1;
            Stones.FadeIn(M2Indx[HM]);
        }
        else {
            Stones.FadeOut(M2Indx[HMPrev]);
        }
        HMPrev = HMNow;
    }
}
