/*
 * interface.h
 *
 *  Created on: 22 марта 2015 г.
 *      Author: Kreyl
 */

#ifndef INTERFACE_H_
#define INTERFACE_H_

#include "lcd1200.h"
#include "main.h"
#include "kl_time.h"

class Interface_t {
public:
    void Reset() {
        Lcd.Printf(11, 0, APP_VERSION);
        Lcd.Printf(0, 2, "Время:");
        Lcd.Printf(0, 3, "Год:");
        Lcd.Printf(0, 4, "Месяц:");
        Lcd.Printf(0, 5, "День:");
        Lcd.Printf(0, 7, "Яркость:");
    }
    void DisplayTime() {
        Lcd.Printf(7, 2, "%02u:%02u:%02u", App.dtNow.H, App.dtNow.M, App.dtNow.S);
        Lcd.Printf(7, 3, "%04u", App.dtNow.Year);
        Lcd.Printf(7, 4, "%02u", App.dtNow.Month);
        Lcd.Printf(7, 5, "%02u", App.dtNow.Day);
    }

    void DisplaySelected() {
        switch(App.Selected) {
            case selH:
                Lcd.PrintfInverted(7, 2, "%02u", App.dtNow.H);
                break;
            case selM:
                Lcd.Printf(7, 2, "%02u", App.dtNow.H);
                Lcd.PrintfInverted(10, 2, "%02u", App.dtNow.M);
                break;
            case selS:
                Lcd.Printf(10, 2, "%02u", App.dtNow.M);
                Lcd.PrintfInverted(13, 2, "%02u", App.dtNow.S);
                break;
            case selYear:
                Lcd.Printf(13, 2, "%02u", App.dtNow.S);
                Lcd.PrintfInverted(7, 3, "%04u", App.dtNow.Year);
                break;
            case selMonth:
                Lcd.Printf(7, 3, "%04u", App.dtNow.Year);
                Lcd.PrintfInverted(7, 4, "%02u", App.dtNow.Month);
                break;
            case selDay:
                Lcd.Printf(7, 4, "%02u", App.dtNow.Month);
                Lcd.PrintfInverted(7, 5, "%02u", App.dtNow.Day);
                break;
            case selNone:
                Lcd.Printf(7, 5, "%02u", App.dtNow.Day);
                break;
        } // switch
    }

    void DisplayBrightness() { Lcd.Printf(9, 7, "%u%%  ", App.Brightness); }

    void Error(const char* msg) { Lcd.PrintfInverted(0, 2, "%S", msg); }
};

extern Interface_t Interface;

#endif /* INTERFACE_H_ */
