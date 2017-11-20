/*
 * hypertime.h
 *
 *  Created on: 12 ????. 2017 ?.
 *      Author: juice
 */

#ifndef TIMESETTINGS_H_
#define TIMESETTINGS_H_

#include "mirilli.h"
#include "shell.h"
#include "kl_time.h"

#define TOP_BRIGHTNESS      99  // to fit in two-digit place


void IndicateNewSecond();
void InitSettings();
void OnCmd(Shell_t *PShell);


class Hypertime_t {
public:
    void ConvertFromTime();
    int32_t H, M;
    bool NewH = true, NewM = true;
};

// Settings
union Settings_t {
    struct {
        uint32_t R1,R2,R3;
    } __packed;
    struct {
        uint8_t Threshold;
        uint8_t BrtHi;
        uint8_t BrtLo;
        uint16_t ClrIdH, ClrIdM;
    } __packed;
} __packed;

extern Settings_t Settings;
extern ColorHSV_t ClrH;
extern ColorHSV_t ClrM;
extern uint32_t CurrentLum;
extern TmrKL_t TmrMenu;

#endif /* TIMESETTINGS_H_ */


