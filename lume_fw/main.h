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
#define APP_VERSION     __DATE__ " " __TIME__  //"v2.0"

#define BTNPRESS_TIMEOUT_MS     3600

enum Selected_t {selNone, selH, selM, selS, selYear, selMonth, selDay};

class App_t {
private:
    Thread *PThread;
    void BtnHandler(uint8_t Btn);
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
