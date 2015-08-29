/*
 * stones.cpp
 *
 *  Created on: 29 рту. 2015 у.
 *      Author: Kreyl
 */

#include "stones.h"
#include "kl_time.h"    // Backupreg constants
#include "BackupRegSettings.h"
#include "uart.h"

Stones_t Stones;

// ==== Stones Thread ====
static WORKING_AREA(waStonesThread, 128);
__attribute__((noreturn))
static void StonesThread(void *arg) {
    chRegSetThreadName("Stones");
    Stones.ITask();
}

void Stones_t::Init() {
    i2c.Init();
//    i2c.BusScan();
    // Stones init
    for(uint32_t i=0; i<STONE_CNT; i++) {
        Stone[i].BrtCurrent = 0;
        Stone[i].BrtDesired = 0;
    }
    // Read Brightness from Backup reg
    if(IsStored()) Brightness = BKPREG_BRIGHTNESS;
    else Brightness = BRIGHTNESS_TOP;
    if(Brightness > BRIGHTNESS_TOP or Brightness == 0) Brightness = BRIGHTNESS_TOP;
    BKPREG_BRIGHTNESS = Brightness; // Save possibly new value
    Uart.Printf("\rBrightness = %u", Brightness);

    // Enable LEDs
    PinSetupOut(GPIOB, 8, omPushPull);
    PinClear(GPIOB, 8); // Always enabled
    // Init PCAs
    uint8_t rslt;
    rslt = i2c.Write(PcaAddr[0], (uint8_t*)PcaInitPkt, PCAINITPKT_SZ);
    if(rslt != OK) Uart.Printf("\rPCA0 Init Failure %u", rslt);
    rslt = i2c.Write(PcaAddr[1], (uint8_t*)PcaInitPkt, PCAINITPKT_SZ);
    if(rslt != OK) Uart.Printf("\rPCA1 Init Failure %u", rslt);
    // Create and start thread
    chThdCreateStatic(waStonesThread, sizeof(waStonesThread), NORMALPRIO, (tfunc_t)StonesThread, NULL);
}

__attribute__ ((__noreturn__))
void Stones_t::ITask() {
    while(true) {
        chThdSleepMilliseconds(540);
        Stone[5].BrtCurrent++;
        SetLedBrt(5);
        Stone[6].BrtCurrent++;
        SetLedBrt(6);
        Stone[7].BrtCurrent++;
        SetLedBrt(7);
    }
}

void Stones_t::SetLedBrt(uint8_t Indx) {
    uint8_t Pkt[2];
    uint8_t Addr = PcaAddr[INDX2PCA_ID(Indx)];
    Indx -= INDX2PCA_ID(Indx) * 16;     // Inner indx of LED
    Pkt[0] = PCA_PWM0_ADDR + Indx;
    Pkt[1] = Stone[Indx].BrtCurrent;
    uint8_t rslt = i2c.Write(Addr, Pkt, 2);
//    Uart.Printf("\r%A", Pkt, 2, ' ');
    if(rslt != OK) Uart.Printf("\rPca %u Brt Fail %u", Addr, rslt);
}

void Stones_t::SetGroupBrt() {
    uint8_t Pkt[2];
    Pkt[0] = PCA_GRPPWM_ADDR;
    Pkt[1] = Brightness;
    uint8_t rslt;
    rslt = i2c.Write(PcaAddr[0], Pkt, 2);
    if(rslt != OK) Uart.Printf("\rPca0 Brt Fail %u", rslt);
    rslt = i2c.Write(PcaAddr[1], Pkt, 2);
    if(rslt != OK) Uart.Printf("\rPca1 Brt Fail %u", rslt);
}
