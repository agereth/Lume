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

#define APP_NAME        "Lume"
#define APP_VERSION     "v2.0"


class App_t {
private:
    Thread *PThread;
public:
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
    void ITask();
};

extern App_t App;


#endif /* MAIN_H_ */
