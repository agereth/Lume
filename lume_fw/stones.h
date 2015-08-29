/*
 * stones.h
 *
 *  Created on: 29 рту. 2015 у.
 *      Author: Kreyl
 */

#ifndef STONES_H_
#define STONES_H_

#include "kl_lib.h"

// Correspondence between hours and LED indxs
const uint8_t H2Indx[12] = {
        0,1,2,3,4,5,6,7,8,9,10,11,
};

// Correspondence between hyperminutes and LED indxs
const uint8_t M2Indx[12] = {
        12,13,14,15,16, 17,18,19,20, 21,22,23
};

enum StoneState_t {sstDark, sstFadeIn, sstShine, sstFadeOut};

#define INDX2PCA_ID(Indx)   ((Indx) < 16? 0 : 1)

struct Stone_t {
    uint8_t BrtCurrent, BrtDesired;
} PACKED;
#define STONE_CNT   32  // Total LEDs available

#if 1 // ==== PCA9635 ====
const uint8_t PcaAddr[2] = {0x1A, 0x2B};
#define PCA_PWM0_ADDR   0x02
#define PCA_GRPPWM_ADDR 0x12

const uint8_t PcaInitPkt[] = {
        0b10000000, // ControlReg: Autoincrement enable, first register is 0x00
        0b10000000, // Mode1: Non-sleep mode, do not respond to extra adresses
        0b00000010, // Mode2: Group control = dimming, output not inverted, change on STOP, OpenDrain, high-Z when OE=1
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // PWM 0...15
        0xFF,       // GrpPWM: Max common brightness
        0,          // GrpFreq: Don't care as DBLINK bit is 0
        0xFF,0xFF,0xFF,0xFF // LEDOut0...3: LED driver has individual brightness and group dimming/blinking and can be controlled through its PWMx register and the GRPPWM registers
};
#define PCAINITPKT_SZ       sizeof(PcaInitPkt)

#endif

// ==== Stones ====
#define BRIGHTNESS_TOP  255
#define MAX_LED_BRT     255
class Stones_t {
private:
    i2c_t i2c{I2C1, GPIOB, 6, 7, 100000, STM32_DMA1_STREAM6, STM32_DMA1_STREAM7};
    Stone_t Stone[STONE_CNT];
    uint16_t OldBrightness = 0;
    void SetGroupBrt();
    void SetLedBrt(uint8_t Indx);
public:
    void Init();
    uint16_t Brightness;
    void FadeIn(uint8_t Indx)  { if(Indx < STONE_CNT) Stone[Indx].BrtDesired = MAX_LED_BRT; }
    void FadeOut(uint8_t Indx) { if(Indx < STONE_CNT) Stone[Indx].BrtDesired = 0; }
    // Inner use
    void ITask();
};

extern Stones_t Stones;

#endif /* STONES_H_ */
