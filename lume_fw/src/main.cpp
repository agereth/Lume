/*
 * File:   main.cpp
 * Author: Kreyl
 *
 * Created on May 27, 2011, 6:37 PM
 */

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_pwr.h"
#include "kl_lib.h"
#include "lcd1200.h"
#include "kl_time.h"
#include "interface.h"
#include "keys.h"
#include "i2c_mgr.h"
#include "leds_pca.h"
#include "showtime.h"

// Prototypes
static void Init();

// ============================== Implementation ===============================
int main() {
    Init();

    while (1) {
        Uart.Task();
        Lcd.Task();
        Interface.Task();
        Keys.Task();
        i2cMgr.Task();
        Leds.Task();
    } // while 1
}

static inline void Init() {
    InitClock(clk8MHzInternal);
    klJtagDisable();

    Delay.Init();
    Uart.Init(115200);
    Uart.Printf("\rLume2\r");
    Lcd.Init();
    Lcd.Backlight(54);

    Time.Init();
    Keys.Init();
    Interface.Init();

    i2cMgr.Init();
    Leds.Init();
}
