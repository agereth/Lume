/*
 * Mirilli.h
 *
 *  Created on: 30 сент. 2017 г.
 *      Author: Kreyl
 */

#pragma once

#include "color.h"

#define MIRILLI_H_CNT   12
#define MIRILLI_M_CNT   12

#define SMOOTH_VALUE    360

#define LED_CNT         (MIRILLI_H_CNT + MIRILLI_M_CNT)   // Number of WS2812 LEDs

void InitMirilli();
void SetTargetClrH(uint32_t H, ColorHSV_t Clr);
void SetTargetClrM(uint32_t M, ColorHSV_t Clr);
void WakeMirilli();
