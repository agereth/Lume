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
        Lcd.Printf(7, 2, "%02u:%02u:%02u", Time.DateTime.H, Time.DateTime.M, Time.DateTime.S);
        Lcd.Printf(7, 4, "%04u", Time.DateTime.Year);
        Lcd.Printf(7, 5, "%02u", Time.DateTime.Month);
        Lcd.Printf(7, 6, "%02u", Time.DateTime.Day);
    }

    void Error(const char* msg) { Lcd.PrintfInverted(0, 2, "%S", msg); }
};

extern Interface_t Interface;

#endif /* INTERFACE_H_ */
