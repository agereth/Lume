/*
 * Mirilli.cpp
 *
 *  Created on: 30 сент. 2017 г.
 *      Author: Kreyl
 */

#include "kl_lib.h"
#include "Mirilli.h"
#include "ws2812b.h"

IntelLeds_t Leds;
static thread_reference_t PThd = nullptr;

// Tables of accordance between hours/minutes and LED indxs
static const uint32_t H2LedN[MIRILLI_H_CNT] = { 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
static const uint32_t M2LedN[MIRILLI_H_CNT] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

// Target colors
static Color_t ITargetClr[LED_CNT];
static bool ColorsDone = true;

static uint32_t ICalcDelayN(uint32_t n, uint32_t SmoothValue) {
    return Leds.ICurrentClr[n].DelayToNextAdj(ITargetClr[n], SmoothValue);
}

static THD_WORKING_AREA(waMirilli, 64);
__noreturn
static void MirilliThread(void *arg) {
    chRegSetThreadName("Mirilli");
    while(true) {
        if(ColorsDone) {
            chSysLock();
            chThdSuspendS(&PThd);
            chSysUnlock();
        }
        else {
            uint32_t Delay = 0;
            for(int32_t i=0; i<LED_CNT; i++) {
                uint32_t tmp = ICalcDelayN(i, SMOOTH_VALUE);  // }
                if(tmp > Delay) Delay = tmp;                  // } Calculate Delay
                Leds.ICurrentClr[i].Adjust(ITargetClr[i]);    // Adjust current color
            } // for
            Leds.ISetCurrentColors();
            if (Delay == 0) ColorsDone = true;  // Setup completed
            else chThdSleepMilliseconds(Delay);
        }
    } // while true
}

void InitMirilli() {
    Leds.Init();
    chThdCreateStatic(waMirilli, sizeof(waMirilli), HIGHPRIO, (tfunc_t)MirilliThread, NULL);
}

void SetTargetClrH(uint32_t H, ColorHSV_t Clr) {
    if(H >= MIRILLI_H_CNT) return;
    uint32_t Indx = H2LedN[H];
    ITargetClr[Indx] = Clr.ToRGB();
}
void SetTargetClrM(uint32_t M, ColorHSV_t Clr) {
    if(M >= MIRILLI_M_CNT) return;
    uint32_t Indx = M2LedN[M];
    ITargetClr[Indx] = Clr.ToRGB();
}

void WakeMirilli() {
    ColorsDone = false;
    if(PThd == nullptr) return;
    chSysLock();
    chThdResumeS(&PThd, MSG_OK);
    chSysUnlock();
}
