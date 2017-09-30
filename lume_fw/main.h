/*
 * main.h
 *
 *  Created on: 18 сент. 2017 г.
 *      Author: Kreyl
 */

#pragma once

#include "kl_lib.h"

enum State_t {stIdle, stHours, stMinutes, stYear, stMonth, stDay,
    stThreshold, stBrtHi, stBrtLo, stClrH, stClrM
};

extern State_t State;

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

class Hypertime_t {
public:
    void ConvertFromTime();
    int32_t H, M;
    bool NewH = true, NewM = true;
};
