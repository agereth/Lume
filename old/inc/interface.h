/*
 * interface.h of Armlet1 project
 *
 *  Created on: 15.09.2012
 *      Author: kreyl
 */

#ifndef MENU_H_
#define MENU_H_

#include "lcd1200.h"
#include "kl_lib.h"
#include "kl_time.h"

#define BLINK_DELAY     306  // ms
#define STATE_DELAY     4005 // ms
#define BCKLT_DELAY     4005 // ms // Delay before backlight to fade

enum MenuState_t {
    msIdle, msHour, msMinute, msYear, msMonth, msDay
};

class Interface_t {
private:
    DateTime_t IDateTime;
    void DisplayHour(bool ShowNotClear)   { Lcd.Printf(7,  2, (ShowNotClear? "%02u:" : "  :"), IDateTime.H); }
    void DisplayMinute(bool ShowNotClear) { Lcd.Printf(10, 2, (ShowNotClear? "%02u:" : "  :"), IDateTime.M); }
    void DisplaySecond(bool ShowNotClear) { Lcd.Printf(13, 2, (ShowNotClear? "%02u" : "  "),   IDateTime.S); }
    void DisplayYear(bool ShowNotClear)   { Lcd.Printf(7,  4, (ShowNotClear? "%04u" : "    "), IDateTime.Year); }
    void DisplayMonth(bool ShowNotClear)  { Lcd.Printf(7,  5, (ShowNotClear? "%02u" : "  "),   IDateTime.Month); }
    void DisplayDay(bool ShowNotClear)    { Lcd.Printf(7,  6, (ShowNotClear? "%02u" : "  "),   IDateTime.Day);  }
    uint32_t TimeChangeTmr, BckLtTmr;
    void EnterIdle();
    // Inc / Dec
    void IInc(bool AFast, int32_t *PValue, int32_t AMaxValue, int32_t AMinValue);
    void IDec(bool AFast, int32_t *PValue, int32_t AMaxValue, int32_t AMinValue);
public:
    MenuState_t State;
    void Init(void);
    void Task(void);
    // Events
    void SettingChanged(void);
    void SettingsLoad(void);
    void SettingsSave(void);
};
extern Interface_t Interface;

#endif /* MENU_H_ */
