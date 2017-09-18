/*
 * main.h
 *
 *  Created on: 18 сент. 2017 г.
 *      Author: Kreyl
 */

#pragma once

enum State_t {stIdle, stHours, stMinutes, stYear, stMonth, stDay,
    stThreshold, stBrtHi, stBrtLo, stClrH, stClrM
};

extern State_t State;
