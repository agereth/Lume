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
//    Printf("H=%u; Indx=%u\r", H, Indx);
    ITargetClr[Indx] = Clr.ToRGB();
    // Special case
#ifdef SECOND_0_LED_INDX
    if(H == 0) {
        ITargetClr[SECOND_0_LED_INDX] = Clr.ToRGB();
//        Printf("H=%u; Indx=%u\r\r", H, Indx);
    }
#endif
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

void ResetColors(ColorHSV_t ClrH, ColorHSV_t ClrM) {
    ClrH.V = OFF_LUMINOCITY;
    ClrM.V = OFF_LUMINOCITY;
    for(int32_t i=0; i<12; i++) {
      SetTargetClrH(i, ClrH);
      SetTargetClrM(i, ClrM);
    }
}
