/*
 * stones.cpp
 *
 *  Created on: 29 рту. 2015 у.
 *      Author: Kreyl
 */

#include "stones.h"
#include "kl_time.h"    // Backupreg constants
#include "uart.h"

Stones_t Stones;

// ==== PCA9635 ====
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

// TimeToWaitBeforeNextAdjustment = SmoothVar / (N+4) + 1, where N - current LED brightness.
static inline uint32_t ICalcDelay(uint32_t CurrentBrightness) { return (uint32_t)((SMOOTH_VAR / (CurrentBrightness+4)) + 1); }

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
    for(uint32_t i=0; i<16; i++) {
        Stone[0][i].BrtCurrent = 0;
        Stone[0][i].BrtDesired = 0;
        Stone[1][i].BrtCurrent = 0;
        Stone[1][i].BrtDesired = 0;
    }

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
        chThdSleepMilliseconds(2);
        // Setup group brightness
        if(Brightness != OldBrightness) {
            OldBrightness = Brightness;
            SetGroupBrt();
        }
        // Setup individual LEDs
        for(uint8_t i=0; i<2; i++) {
            for(uint8_t j=0; j<16; j++) {
                if(Stone[i][j].BrtCurrent != Stone[i][j].BrtDesired) {    // Check if need to change
                    if((chTimeNow() - Stone[i][j].Time) > Stone[i][j].Delay) {    // Check if time has come
                        if(Stone[i][j].BrtCurrent < Stone[i][j].BrtDesired) Stone[i][j].BrtCurrent++;
                        else Stone[i][j].BrtCurrent--;
                        SetLedBrt((LedIndx_t){i, j});
                        // Calculate next delay
                        Stone[i][j].Time = chTimeNow();
                        Stone[i][j].Delay = MS2ST(ICalcDelay(Stone[i][j].BrtCurrent));
                    } // if time
                } // if not equal
            } // for i
        } // for j
    } // while true
}

void Stones_t::SetLedBrt(LedIndx_t Indx) {
    uint8_t Pkt[2];
    uint8_t Addr = PcaAddr[Indx.Pca];
    Pkt[0] = PCA_PWM0_ADDR + Indx.LED;
    Pkt[1] = Stone[Indx.Pca][Indx.LED].BrtCurrent;
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
