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
        Lcd.Printf(0, 4, "Год:");
        Lcd.Printf(0, 5, "Месяц:");
        Lcd.Printf(0, 6, "День:");
//        Lcd.Symbols(0, 5, LineHorizDouble, 6, LineHorizDoubleUp, 1, LineHorizDouble, 9, 0);
//        for(int i=0; i<5; i++) Lcd.Symbols(6, i, ((i == 2)? LineVertDoubleLeft : LineVertDouble), 1,0);
//        Lcd.Symbols(0, 2, LineHorizDouble, 6, 0);
    }
    void DisplayTime() {
        Lcd.Printf(7, 2, "%02u:%02u:%02u", App.dtNow.H, App.dtNow.M, App.dtNow.S);
        Lcd.Printf(7, 4, "%04u", App.dtNow.Year);
        Lcd.Printf(7, 5, "%02u", App.dtNow.Month);
        Lcd.Printf(7, 6, "%02u", App.dtNow.Day);
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
                Lcd.PrintfInverted(7, 4, "%04u", App.dtNow.Year);
                break;
            case selMonth:
                Lcd.Printf(7, 4, "%04u", App.dtNow.Year);
                Lcd.PrintfInverted(7, 5, "%02u", App.dtNow.Month);
                break;
            case selDay:
                Lcd.Printf(7, 5, "%02u", App.dtNow.Month);
                Lcd.PrintfInverted(7, 6, "%02u", App.dtNow.Day);
                break;
            case selNone:
                Lcd.Printf(7, 6, "%02u", App.dtNow.Day);
                break;
        } // switch
    }

    void Error(const char* msg) { Lcd.PrintfInverted(0, 2, "%S", msg); }
};

extern Interface_t Interface;

#endif /* INTERFACE_H_ */
