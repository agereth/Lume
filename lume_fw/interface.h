/*
 * interface.h
 *
 *  Created on: 22 марта 2015 г.
 *      Author: Kreyl
 */

#pragma once

#include "lcd1200.h"
#include "kl_time.h"

class Interface_t {
public:
    void Reset() {
//        Lcd.Printf(11, 0, APP_VERSION);
        Lcd.Printf(0, 0, "Время  00:00:00");
        Lcd.Printf(0, 1, "Дата 0000/00/00");
//        Lcd.Printf(0, 7, "Яркость:");
    }
    void DisplayDateTime(DateTime_t *pDT) {
//        if(App.Selected == selH) Lcd.PrintfInverted(7, 2, "%02u", App.dtNow.H);
//        else
        Lcd.Printf(7, 0, "%02u", pDT->H);
//        if(App.Selected == selM) Lcd.PrintfInverted(10, 2, "%02u", App.dtNow.M);
//        else
        Lcd.Printf(10, 0, "%02u", pDT->M);
//        if(App.Selected == selS) Lcd.PrintfInverted(13, 2, "%02u", App.dtNow.S);
//        else
        Lcd.Printf(13, 0, "%02u", pDT->S);

//        if(App.Selected == selYear) Lcd.PrintfInverted(7, 3, "%04u", App.dtNow.Year);
//        else
        Lcd.Printf(5, 1, "%04u", pDT->Year);
//        if(App.Selected == selMonth) Lcd.PrintfInverted(7, 4, "%02u", App.dtNow.Month);
//        else
        Lcd.Printf(10, 1, "%02u", pDT->Month);
//        if(App.Selected == selDay) Lcd.PrintfInverted(7, 5, "%02u", App.dtNow.Day);
//        else
        Lcd.Printf(13, 1, "%02u", pDT->Day);
    }

    void DisplayBrightness() {
//        if(App.Selected == selBrightness) Lcd.PrintfInverted(9, 7, "%u%%  ", App.BrightnessPercent);
//        else Lcd.Printf(9, 7, "%u%%    ", App.BrightnessPercent);
    }

    void Error(const char* msg) { Lcd.PrintfInverted(0, 0, "%S", msg); }
};

extern Interface_t Interface;
