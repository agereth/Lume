/*
 * stones.h
 *
 *  Created on: 29 рту. 2015 у.
 *      Author: Kreyl
 */

#ifndef STONES_H_
#define STONES_H_

#include "kl_lib.h"

struct LedIndx_t {
    uint8_t Pca, LED;
} PACKED;

// Correspondence between hours and LED indxs
const LedIndx_t H2Indx[12] = {
        {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6},
        {0, 7}, {0, 8}, {0, 9}, {0,10}, {0,11}, {0,12},
};

// Correspondence between hyperminutes and LED indxs
const LedIndx_t M2Indx[12] = {
        {1, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, {1, 6},
        {1, 7}, {1, 8}, {1, 9}, {1,10}, {1,11}, {1,12},
};

struct Stone_t {
    uint32_t Time, Delay;
    uint8_t BrtCurrent, BrtDesired;
} PACKED;

const uint8_t PcaAddr[2] = {0x1A, 0x2B};

// ==== Stones ====
#define MAX_LED_BRT     255
#define SMOOTH_VAR      1107 // Speed of brightness change.
class Stones_t {
private:
    i2c_t i2c{I2C1, GPIOB, 6, 7, 100000, STM32_DMA1_STREAM6, STM32_DMA1_STREAM7};
    Stone_t Stone[2][16];   // 2 PCA's with 16 LEDs each
    uint16_t OldBrightness = 0;
    void SetGroupBrt();
    void SetLedBrt(LedIndx_t Indx);
public:
    void Init();
    uint16_t Brightness;
    void FadeIn(LedIndx_t Indx)  {
        if(Indx.Pca > 1 or Indx.LED > 15) return;
        Stone[Indx.Pca][Indx.LED].BrtDesired = MAX_LED_BRT;
        Stone[Indx.Pca][Indx.LED].Time = 0;
    }
    void FadeOut(LedIndx_t Indx) {
        if(Indx.Pca > 1 or Indx.LED > 15) return;
        Stone[Indx.Pca][Indx.LED].BrtDesired = 0;
        Stone[Indx.Pca][Indx.LED].Time = 0;
    }
    // Inner use
    void ITask();
};

extern Stones_t Stones;

#endif /* STONES_H_ */
