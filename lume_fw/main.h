/*
 * main.h
 *
 *  Created on: 26 рту. 2015 у.
 *      Author: Kreyl
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "uart.h"
#include "evt_mask.h"
#include "kl_time.h"

#define APP_NAME        "Lume"
#define APP_VERSION     "v2.0"

#define BTNPRESS_TIMEOUT_MS     3600

// Brightness in percent to brightness in PWM
struct BrtPercentPwm_t {
    uint8_t Percent, Pwm;
};
const BrtPercentPwm_t BrtPercentPwmTable[] = {
        {1,1},
        {5,2},
        {10,4},
        {15,7},
        {20,11},
        {25,17},
        {30,24},
        {35,32},
        {40,41},
        {45,52},
        {50,64},
        {55,77},
        {60,91},
        {65,107},
        {70,124},
        {75,142},
        {80,161},
        {85,182},
        {90,204},
        {95,227},
        {100,255},
};
#define BRT_CNT         countof(BrtPercentPwmTable)
#define BRIGHTNESS_TOP  100

enum Selected_t {selNone, selH, selM, selS, selYear, selMonth, selDay, selBrightness};

class App_t {
private:
    Thread *PThread;
    void BtnHandler(uint8_t Btn);
    void Time2Stones();
    uint16_t BrightnessPercent;
    uint8_t BrightnessPercent2Pwm(uint8_t Brt);
    void IncBrightness();
    void DecBrightness();
public:
    TmrVirtual_t TmrBtnpressTimeout;
    DateTime_t dtNow;
    // Eternal methods
    void InitThread() { PThread = chThdSelf(); }
    void SignalEvt(eventmask_t Evt) {
        chSysLock();
        chEvtSignalI(PThread, Evt);
        chSysUnlock();
    }
    void SignalEvtI(eventmask_t Evt) {
        chEvtSignalI(PThread, Evt);
        }
    void OnUartCmd(Uart_t *PUart);
    // Inner use
    Selected_t Selected = selNone;
    void ITask();
    friend class Interface_t;
};

extern App_t App;


#endif /* MAIN_H_ */
